#pragma once
const double AXIS0_REV = 2048 * 4;
const double AXIS1_REV = 1024 * 4;
const double GEAR_RATIO_44 = 44;

const int MAX_NUM_AXES = 6;
const int NUM_AXES = 2;

// convert from revs to encoder count
extern double scld[NUM_AXES];
// axis 0: convert from REVSm/sec to velocity count REVSm(1/10000)/min
// axis 1: convert from REVSg/sec to velocity count REVSm(1/10000)/min
extern double sclv[NUM_AXES];
// convert from revs/sec^2 to acceleration count (REVSm/min)/sec
extern double scla[NUM_AXES];

void scalePosition(long input[], double output[]);