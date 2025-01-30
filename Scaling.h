#pragma once
const double AXIS0_REV = 2048 * 4;
const double AXIS1_REV = 1024 * 4;
const double GEAR_RATIO_44 = 44;

const int NUM_AXES = 2;

// convert from revs to encoder count
double scld[NUM_AXES] = { AXIS0_REV, AXIS1_REV * GEAR_RATIO_44 };
// axis 0: convert from REVSm/sec to velocity count REVSm(1/10000)/min
// axis 1: convert from REVSg/sec to velocity count REVSm(1/10000)/min
double sclv[NUM_AXES] = { 60.0 * 10000.0, GEAR_RATIO_44 * 60 * 10000.0 };
// convert from revs/sec^2 to acceleration count (REVSm/min)/sec
double scla[NUM_AXES] = { 60.0, GEAR_RATIO_44 * 60.0 };