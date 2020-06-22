#include "gtest/gtest.h"


#define length(array) (sizeof(array) / sizeof(*(array))) //get length of an array

static const double
  tol0 = 1e-0, tol1 = 1e-1, tol2 = 1e-2, tol3 = 1e-3, tol6 = 1e-6, tol9 = 1e-9;


void Reset_SOILWAT2_after_UnitTest(void);

void _set_layers(LyrIndex nlyrs, RealF dmax[], RealF matricd[], RealF f_gravel[],
  RealF evco[], RealF trco_grass[], RealF trco_shrub[], RealF trco_tree[],
  RealF trco_forb[], RealF psand[], RealF pclay[], RealF imperm[], RealF soiltemp[]);

void create_test_soillayers(unsigned int nlayers);
