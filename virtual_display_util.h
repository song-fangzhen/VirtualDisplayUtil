#include <X11/Xlib.h> 
#include <X11/extensions/Xrandr.h>

#include <map>
#include <string>

class VirtualDisplayUtil {
 public:
  VirtualDisplayUtil();
  ~VirtualDisplayUtil();

  // For Test.
  void LogDisplayInfo();
  void DoScreenModify();
  void WaitForEnding();
  unsigned int GetOriginDisplayWidth();
  unsigned int GetOriginDisplayHeight();

  // Judge if VirtualDisplayUtil can be used.
  bool IsValid();

  void AddDisplay(unsigned int width, unsigned int height);

 private:
  class ErrorHandler {
   public:
    ErrorHandler();
    ~ErrorHandler();

    static bool Errorhappended();

    static int ErrorHandlerCallback(Display* d, XErrorEvent* e);

   private:
    static bool error_happended_;
    
    XErrorHandler original_error_handler_;
  };

  void OpenDisplay();
  void CloseDisplay();

  bool DealStatus(const RRCrtc& crtc, Status s);

  void RecordOriginDisplayInfo();
  void ApplyOriginDisplayInfo();

  void PopulateCrtcsInfo();
  
  bool DisableCrtc(const RRCrtc& crtc);

  Status CrtcApply(RRCrtc crtc, RRMode mode);
  Status RevertCrtc(const RRCrtc& crtc);

  RRMode FindMode(const char* mode_name);
  RRCrtc FindAvailableCtrc();
  RROutput FindAvailableOutput();

  double CaculateDPI();

  RRMode GetPrimaryOutputMode();

  bool IsModeContainedInOutput(const RRMode mode, const RROutput output);

  ErrorHandler error_handler_;

  std::string display_name_;

  Display* display_ = NULL;
  int default_screen_;
  Window root_window_;
  XRRScreenResources* screen_resources_ = NULL;
  int ncrtc_;
  RRCrtc* crtcs_ = NULL;
  RROutput* outputs_ = NULL;

  RRMode added_output_mode_ = BadRRMode;
  RROutput available_output_ = BadRROutput;

  std::map<RRCrtc, XRRCrtcInfo*> origin_crtcs_info_;
  unsigned int m_origin_display_width;
  unsigned int m_origin_display_Height;
  unsigned long m_origin_display_width_mm;
  unsigned long m_origin_display_height_mm;
};
