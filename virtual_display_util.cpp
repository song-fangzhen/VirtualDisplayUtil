#include "virtual_display_util.h"

#include <stdio.h>

#include <cstring>

VirtualDisplayUtil::VirtualDisplayUtil() {
  printf("VirtualDisplayUtil Construct.\n");

  OpenDisplay();
}

VirtualDisplayUtil::~VirtualDisplayUtil() {    
  CloseDisplay();

  printf("VirtualDisplayUtil Destruct.\n");
}

void VirtualDisplayUtil::LogDisplayInfo() {
  printf("VirtualDisplayUtil: num_crtcs = %d\tnum_outputs = %d\t\
    num_modes = %d\n", 
    ncrtc_, screen_resources_->noutput, screen_resources_->nmode);

  printf("m_screen_resources: modes[0].name = %s\n", 
    screen_resources_->modes[0].name);

  // CTRC informations.
  for (int c = 0; c < ncrtc_; c++) {
      RRCrtc crtc = crtcs_[c];
      XRRCrtcInfo* crtc_info = origin_crtcs_info_[crtc];
      printf("\tCrtc[%d]:%ld crtc_info->mode = %ld crtc_info->noutput = %d \
          crtc_info->x: %d crtc_info->y: %d crtc_info->width: %d \
          crtc_info->height: %d\n", 
          c,  crtc, crtc_info->mode, crtc_info->noutput, 
          crtc_info->x, crtc_info->y, crtc_info->width, crtc_info->height);
  }

  // Output informations.
  for (int o = 0; o < screen_resources_->noutput; o++) {
      XRROutputInfo* output_info = 
          XRRGetOutputInfo(display_, screen_resources_, outputs_[o]);
      printf("\tOutput[%d](%ld): %s connection = %d mm_width = %ld \
          mm_height = %ld ncrtc = %d nmode = %d npreferred = %d\n", 
          o, outputs_[o], output_info->name, output_info->connection, 
          output_info->mm_width, output_info->mm_height, 
          output_info->ncrtc, output_info->nmode, output_info->npreferred);
  }

  double dpi = CaculateDPI();
  printf("DPI: %f\n", dpi);

  RROutput primary_output = XRRGetOutputPrimary(display_, root_window_);
  printf("Primary Output: %ld\n", primary_output);

  RRMode primary_output_mode = GetPrimaryOutputMode();
  printf("Primary Output Mode: %ld\n", primary_output_mode);

  printf("Find available crtc %ld.\n", FindAvailableCtrc());
  printf("Find available output %ld.\n", FindAvailableOutput());

  XRRProviderResources *provider_resource = 
      XRRGetProviderResources(display_, root_window_);
  printf("Providers Num: %d\n", provider_resource->nproviders);

  int n;
  XRRMonitorInfo* m = XRRGetMonitors(display_, root_window_, true, &n);
  printf("Monitor num: %d\n", n);
  for (int i = 0; i < n; i++) {
      XRRMonitorInfo monitor = m[i];
      printf("monitor[%d] width: %d height: %d noutput: %d\n", 
          i, monitor.width, monitor.height, monitor.noutput);
  }

  // Xrandr related.
  int event_base, error_base;
  int major, minor;
  XRRQueryExtension(display_, &event_base, &error_base);
  XRRQueryVersion(display_, &major, &minor);
  
  static int  minWidth, maxWidth, minHeight, maxHeight;
  XRRGetScreenSizeRange(display_, root_window_, &minWidth, &minHeight,
                      &maxWidth, &maxHeight);

  // Some infomations.
  printf("screenNumber:%d XDisplayString:(%s) \
      ProtocolVersion.ProtocolRevision:%d.%d \
      ScreenCount:%d ServerVendor:%s\n \
      (DisplayWidth, DisplayHeight, DisplayWidthMM, DisplayHeightMM) = \
      (%d, %d, %d, %d)\n \
      ScreenCount = %d\n \
      (event_base, error_base, major, minor) = (%d, %d, %d, %d)\n \
      (minWidth, maxWidth, minHeight, maxHeight) = (%d, %d, %d, %d)\n", 
      default_screen_, XDisplayString(display_), 
      ProtocolVersion(display_), ProtocolRevision(display_), 
      ScreenCount(display_),
      ServerVendor(display_),
      DisplayWidth(display_, default_screen_), 
      DisplayHeight(display_, default_screen_), 
      DisplayWidthMM(display_, default_screen_), 
      DisplayHeightMM(display_, default_screen_), 
      ScreenCount(display_),
      event_base, error_base, major, minor,
      minWidth, maxWidth, minHeight, maxHeight);
}

void VirtualDisplayUtil::DoScreenModify() {
  double dpi = CaculateDPI();
  int fb_width = 1920;
  int fb_height = 1080;
  int fb_width_mm = 25.4 * fb_width / dpi;
  int fb_height_mm = 25.4 * fb_height / dpi;
  XRRSetScreenSize(display_, root_window_,
            fb_width,
            fb_height,
            fb_width_mm,
            fb_height_mm);

  printf("DoScreenModify done.\n");
}

void VirtualDisplayUtil::WaitForEnding() {
  unsigned long white = WhitePixel(display_, default_screen_);
  unsigned long black = BlackPixel(display_, default_screen_);
  Window win = XCreateSimpleWindow(display_,
          DefaultRootWindow(display_),
          0, 0,   // origin
          200, 200, // size
          10, black, // border
          white );  // backgd
  XMapWindow(display_, win);
  
  long eventMask = ButtonPressMask | ButtonReleaseMask;
  XSelectInput(display_, win, eventMask);

  printf("Waiting for click ...\n");

  XEvent evt;
  do {
      XNextEvent(display_, &evt);
  } while (evt.type != ButtonRelease);

  XDestroyWindow(display_, win);
}

unsigned int VirtualDisplayUtil::GetOriginDisplayWidth() {
  return m_origin_display_width;
}

unsigned int VirtualDisplayUtil::GetOriginDisplayHeight() {
  return m_origin_display_Height;
}

bool VirtualDisplayUtil::IsValid() {
  return display_ != NULL;
}

void VirtualDisplayUtil::AddDisplay(unsigned int width, 
                                         unsigned int height) {
  RRCrtc available_crtc = FindAvailableCtrc();
  if (available_crtc == BadRRCrtc) {
    printf("Cannot find available CRTC.\n");
    return;
  }

  RROutput available_output = FindAvailableOutput();
  if (available_output == BadRROutput) {
    printf("Cannot find available Output.\n");
    return;
  }

  available_output_ = available_output;

  char mode_name[20];
  sprintf(mode_name, "%dx%d", width, height);
  RRMode output_mode = FindMode(mode_name);
  if (output_mode == BadRRMode) {
    printf("Cannot find available Mode(%s).\n", mode_name);
    return;
  }

  if (!IsModeContainedInOutput(output_mode, available_output)) {
    printf("Add output mode %ld for Output %ld.\n", 
      output_mode, available_output);

    XRRAddOutputMode(display_, available_output, output_mode);

    // Flushes the output buffer and then waits until all requests have been 
    // received and processed by the X server. `discard`	specifies a Boolean 
    // value that indicates whether XSync() discards all events on the event 
    // queue.
    XSync(display_, /*discard=*/False);

    if (error_handler_.Errorhappended()) {
      return;
    }

    added_output_mode_ = output_mode;
  }

  Status s = CrtcApply(available_crtc, output_mode);
  if (!DealStatus(available_crtc, s)) {
      RevertCrtc(available_crtc);

      printf("AddDisplay fails.\n");
      return;
  }

  double dpi = CaculateDPI();
  unsigned int fb_width = m_origin_display_width + width;;
  unsigned int fb_height = std::max(m_origin_display_Height, height);
  unsigned long fb_width_mm = 25.4 * fb_width / dpi;
  unsigned long fb_height_mm = 25.4 * fb_height / dpi;
  XRRSetScreenSize(display_, root_window_,
              fb_width,
              fb_height,
              fb_width_mm,
              fb_height_mm);

  printf("DoScreenModify (%d, %d, %ld, %ld) done.\n", 
    fb_width, fb_height, fb_width_mm, fb_height_mm);
}

VirtualDisplayUtil::ErrorHandler::ErrorHandler() {
  printf("ErrorHandler Construct.\n");

  original_error_handler_ = XSetErrorHandler(ErrorHandlerCallback);
}

VirtualDisplayUtil::ErrorHandler::~ErrorHandler() {
  printf("ErrorHandler Destruct.\n");

  XSetErrorHandler(original_error_handler_);
}

// static
bool VirtualDisplayUtil::ErrorHandler::Errorhappended() {
  return error_happended_;
}

// static
int VirtualDisplayUtil::ErrorHandler::ErrorHandlerCallback(
                                            Display* d, XErrorEvent* e) {
  /*
    Eror log example:
    
    X Error of failed request:  BadMatch (invalid parameter attributes)
      Major opcode of failed request:  140 (RANDR)
      Minor opcode of failed request:  21 (RRSetCrtcConfig)
      Serial number of failed request:  20
      Current serial number in output stream:  20
  */

  error_happended_ = true;

  char buffer[BUFSIZ];
  XGetErrorText(d, e->error_code, buffer, BUFSIZ);

  printf("X Error of failed request : %s\n\
    Major opcode of failed request: %d\n\
    Minor opcode of failed request: %d\n\
    Serial number of failed request: %ld\n\
    Current serial number in output stream:\n", 
    buffer, e->request_code, e->minor_code, e->serial);
  
  return 0;
}

// static
bool VirtualDisplayUtil::ErrorHandler::error_happended_ = false;

void VirtualDisplayUtil::OpenDisplay() {
  Display* dsp = XOpenDisplay(NULL);
  if(!dsp) {
      printf("Open Display fails.\n");
      return;
  }

  display_ = dsp;
  display_name_ = XDisplayString(display_);

  printf("Open Display %s success.\n", display_name_.c_str());

  default_screen_ = DefaultScreen(display_);
  root_window_ = RootWindow(display_, default_screen_);
  screen_resources_ = XRRGetScreenResourcesCurrent(display_, root_window_);
  ncrtc_ = screen_resources_->ncrtc;
  crtcs_ = screen_resources_->crtcs;
  outputs_ = screen_resources_->outputs;

  RecordOriginDisplayInfo();
}

void VirtualDisplayUtil::CloseDisplay() {
  if (!display_) {
      return;
  }

  ApplyOriginDisplayInfo();

  if (added_output_mode_ != BadRRMode && available_output_ != BadRROutput) {
    printf("Delete output mode %ld for Output %ld.\n", 
      added_output_mode_, available_output_);

    XRRDeleteOutputMode(display_, available_output_, added_output_mode_);
  }

  XRRFreeScreenResources(screen_resources_);

  XCloseDisplay(display_);

  printf("Close Display %s done.\n", display_name_.c_str());

  added_output_mode_ = BadRRMode;
  available_output_ = BadRROutput;

  display_ = NULL;
  screen_resources_ = NULL;
  crtcs_ = NULL;
  outputs_ = NULL;
}

bool VirtualDisplayUtil::DealStatus(const RRCrtc& crtc, Status s) {
  bool result = false;
  std::string status;
  switch (s)
  {
  case RRSetConfigFailed:
    status = "failed";
    break;

  case BadAlloc:
    status = "bad alloc";
    break;

  case RRSetConfigSuccess:
    status = "success";
    result = true;
    break;
  
  default:
    break;
  }

  printf("XRRSetCrtcConfig Crtc %ld %s!\n", crtc, status.c_str());

  return result;
}

void VirtualDisplayUtil::RecordOriginDisplayInfo() {
  PopulateCrtcsInfo();

  m_origin_display_width = DisplayWidth(display_, default_screen_);
  m_origin_display_Height = DisplayHeight(display_, default_screen_);
  m_origin_display_width_mm = DisplayWidthMM(display_, default_screen_);
  m_origin_display_height_mm = DisplayHeightMM(display_, default_screen_);
}

void VirtualDisplayUtil::ApplyOriginDisplayInfo() {
  /* first disable all crtcs */
  for (int i = 0; i < ncrtc_; ++i) {
      DisableCrtc(crtcs_[i]);
  }
  // Screen revert.
  XRRSetScreenSize (display_, root_window_,
            m_origin_display_width,
            m_origin_display_Height,
            m_origin_display_width_mm,
            m_origin_display_height_mm);
  /* now restore all crtcs */
  for (int i = 0; i < ncrtc_; ++i) {
      RevertCrtc(crtcs_[i]);
  }
}

void VirtualDisplayUtil::PopulateCrtcsInfo() {
  for (int i = 0; i < screen_resources_->ncrtc; ++i) {
      RRCrtc crtc = crtcs_[i];
      origin_crtcs_info_[crtc] = 
          XRRGetCrtcInfo(display_, screen_resources_, crtc);
  }
}

bool VirtualDisplayUtil::DisableCrtc(const RRCrtc& crtc) {
  Status s = 
      XRRSetCrtcConfig (display_, screen_resources_, crtc, 
          CurrentTime, 0, 0, None, RR_Rotate_0, NULL, 0);
  if (!DealStatus(crtc, s)) {
      RevertCrtc(crtc);
      return false;
  }
  return true;
}

Status VirtualDisplayUtil::CrtcApply(RRCrtc crtc, RRMode mode) {
  int n_outputs = 1;
  RROutput* outputs = (RROutput*)calloc(n_outputs, sizeof(RROutput));
  if (!outputs)
    return BadAlloc;

  outputs[n_outputs - 1] = available_output_;

  Status s = XRRSetCrtcConfig(display_, screen_resources_, crtc, 
                  CurrentTime,
                  0 + m_origin_display_width, 0,
                  mode, RR_Rotate_0,
                  outputs, n_outputs);

  free(outputs);

  return s;
}

Status VirtualDisplayUtil::RevertCrtc(const RRCrtc& crtc) {
  printf ("Revert crtc %ld\n", crtc);

  XRRCrtcInfo* crtc_info = origin_crtcs_info_[crtc];
  return XRRSetCrtcConfig(display_, screen_resources_, crtc, 
              CurrentTime,
              crtc_info->x, crtc_info->y,
              crtc_info->mode, crtc_info->rotation,
              crtc_info->outputs, crtc_info->noutput);
}

RRMode VirtualDisplayUtil::FindMode(const char* mode_name) {
  for (int m = 0; m < screen_resources_->nmode; ++m) {
    XRRModeInfo mode_info = screen_resources_->modes[m];
    if (!strcmp(mode_info.name, mode_name)) {
      return mode_info.id;
    }
  }

  return BadRRMode;
}

RRCrtc VirtualDisplayUtil::FindAvailableCtrc() {
  bool for_test = false;
  if (for_test) {
      return crtcs_[1];
  }

  for (int c = 0; c < ncrtc_; c++) {
      RRCrtc crtc = crtcs_[c];
      XRRCrtcInfo* crtc_info = origin_crtcs_info_[crtc];
      if (!crtc_info->noutput) {
        return crtc;
      }
  }

  return BadRRCrtc;
}

RROutput VirtualDisplayUtil::FindAvailableOutput() {
  bool for_test = false;
  if (for_test) {
    return outputs_[1];
  }

  // Output informations.
  for (int o = 0; o < screen_resources_->noutput; o++) {
    RROutput output = outputs_[o];
    XRROutputInfo* output_info = 
        XRRGetOutputInfo(display_, screen_resources_, output);
    if (output_info->connection == RR_Disconnected && strstr(output_info->name, "HDMI")) {
      return output;
    }
  }

  return BadRROutput;
}

double VirtualDisplayUtil::CaculateDPI() {
  return 25.4 * DisplayHeight(display_, default_screen_) 
      / DisplayHeightMM(display_, default_screen_);
}

RRMode VirtualDisplayUtil::GetPrimaryOutputMode() {
  RROutput primary_output = XRRGetOutputPrimary(display_, root_window_);
  XRROutputInfo* output_info = 
          XRRGetOutputInfo(display_, screen_resources_, primary_output);
  return output_info->modes[output_info->npreferred];
}

bool VirtualDisplayUtil::IsModeContainedInOutput(const RRMode mode, 
                                                      const RROutput output) {
  XRROutputInfo* output_info = 
    XRRGetOutputInfo(display_, screen_resources_, output);
  for (int i = 0; i < output_info->nmode; ++i) {
    if (mode == output_info->modes[i]) {
      return true;
    }
  }
  return false;
}
