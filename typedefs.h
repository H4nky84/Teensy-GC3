#ifndef __TYPEDEFS_H
#define __TYPEDEFS_H


// headers for this project
#include "project.h"


struct rectangle{
  int X;
  int Y;
  int W;
  int H;
  boolean active;
};

enum screenDisplay{
  Splash,
  Main,
  Overload,
  Settings,
  ActiveTimeoutSplash,
  InactiveTimeoutSplash,
  DispatchSplash,
};



#endif	// __TYPEDEFS_H
