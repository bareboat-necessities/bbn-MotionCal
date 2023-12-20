#ifndef IMUread_h_
#define IMUread_h_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(LINUX)
  #include <termios.h>
  #include <unistd.h>
  #include <GL/gl.h>  // sudo apt install mesa-common-dev
  #include <GL/glu.h> // sudo apt install libglu1-mesa-dev freeglut3-dev
#elif defined(WINDOWS)
  #include <windows.h>
  #include <GL/gl.h>
  #include <GL/glu.h>
#elif defined(MACOSX)
  #include <termios.h>
  #include <unistd.h>
  #include <OpenGL/gl.h>
  #include <OpenGL/glu.h>
#endif


#include "libcalib.h"

extern libcalib::Calibrator calib;

#if defined(LINUX)
  #define PORT "/dev/ttyACM0"
#elif defined(WINDOWS)
  #define PORT "COM3"
#elif defined(MACOSX)
  #define PORT "/dev/cu.usbmodemfd132"
#endif

#define TIMEOUT_MSEC 14

extern int port_is_open(void);
extern int open_port(const char *name);
extern int read_serial_data(void);
extern int write_serial_data(const void *ptr, int len);
extern void close_port(void);
void cal1_data(const float *data);
void cal2_data(const float *data);
void calibration_confirmed(void);
int send_calibration(void);
void visualize_init(void);
void display_callback(void);
void resize_callback(int width, int height);
#endif
