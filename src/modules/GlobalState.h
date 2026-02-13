#pragma once

// Global State for Calibration Mode
// These are defined in main.cpp and extern'd here for Network.cpp

extern bool g_isCalibrationMode;
extern int
    g_calOverrideValues[3]; // 0=Hour, 1=Minute, 2=Second. -1 = No Override
