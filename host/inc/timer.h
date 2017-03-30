/* (C) 1992-2014 Altera Corporation. All rights reserved.                          */
/* Your use of Altera Corporation's design tools, logic functions and other        */
/* software and tools, and its AMPP partner logic functions, and any output        */
/* files any of the foregoing (including device programming or simulation          */
/* files), and any associated documentation or information are expressly subject   */
/* to the terms and conditions of the Altera Program License Subscription          */
/* Agreement, Altera MegaCore Function License Agreement, or other applicable      */
/* license agreement, including, without limitation, that your use is for the      */
/* sole purpose of programming logic devices manufactured by Altera and sold by    */
/* Altera or its authorized distributors.  Please refer to the applicable          */
/* agreement for further details.                                                  */
    

// timer.h

#ifndef TIMER_H
#define TIMER_H


#ifdef WINDOWS
#include <windows.h>

class Timer {
  public:
    Timer();
    void start();
    void stop();
    float get_time_s();
  private:
    LARGE_INTEGER m_start_time;
    LARGE_INTEGER m_stop_time;
    LARGE_INTEGER m_ticks_per_second;
};

#else // LINUX

#include <time.h>

class Timer {
  public:
    Timer();
    void start();
    void stop(); 
    float get_time_s();
    
  private:
    double m_start_time;
    double m_stop_time;
    double get_cur_time_s(void);
};
#endif

#endif // TIMER_H

