#include "virtual_display_util.h"

int main() {
  VirtualDisplayUtil virtual_display_util;
  if(!virtual_display_util.IsValid()) {
      return 1;
  }

  int width = 1920;
  int height = 1080;
  virtual_display_util.AddDisplay(width, height);

  virtual_display_util.LogDisplayInfo();

  virtual_display_util.WaitForEnding();

  return 0;
}
