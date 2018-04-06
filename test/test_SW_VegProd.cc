#include "gtest/gtest.h"
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "../generic.h"
#include "../myMemory.h"
#include "../filefuncs.h"
#include "../rands.h"
#include "../Times.h"
#include "../SW_Defines.h"
#include "../SW_Times.h"
#include "../SW_Files.h"
#include "../SW_Carbon.h"
#include "../SW_Site.h"
#include "../SW_VegProd.h"
#include "../SW_VegEstab.h"
#include "../SW_Model.h"
#include "../SW_SoilWater.h"
#include "../SW_Weather.h"
#include "../SW_Markov.h"
#include "../SW_Sky.h"

#include "sw_testhelpers.h"


extern SW_MODEL SW_Model;
extern SW_VEGPROD SW_VegProd;


namespace {
  SW_VEGPROD *v = &SW_VegProd;
  int k;

  // Test the SW_VEGPROD constructor 'SW_VPD_construct'
  TEST(VegTest, Constructor) {
    SW_VPD_construct();

    ForEachVegType(k) {
      EXPECT_DOUBLE_EQ(1., v->veg[k].co2_multipliers[BIO_INDEX][0]);
      EXPECT_DOUBLE_EQ(1., v->veg[k].co2_multipliers[BIO_INDEX][MAX_NYEAR - 1]);

      EXPECT_DOUBLE_EQ(1., v->veg[k].co2_multipliers[WUE_INDEX][0]);
      EXPECT_DOUBLE_EQ(1., v->veg[k].co2_multipliers[WUE_INDEX][MAX_NYEAR - 1]);
    }

    // Reset to previous global state
    Reset_SOILWAT2_after_UnitTest();
  }


  // Test the application of the biomass CO2-effect
  TEST(VegTest, BiomassCO2effect) {
    int i;
    double x;
    double biom1[12], biom2[12];

    for (i = 0; i < 12; i++) {
      biom1[i] = i + 1.;
    }

    // One example
    x = v->veg[SW_GRASS].co2_multipliers[BIO_INDEX][SW_Model.startyr + SW_Model.addtl_yr];
    apply_biomassCO2effect(biom2, biom1, x);

    for (i = 0; i < 12; i++) {
      EXPECT_DOUBLE_EQ(biom2[i], biom1[i] * x);
    }
    Reset_SOILWAT2_after_UnitTest();
  }

  // check the rank function to ensure properly ordering critical values
  TEST(VegTest, rank){
    int k;
    get_critical_rank(); // function to put critical values in order

    // check to make sure ranked in proper order, largest to smallest. (ex: -2.0, -2.0, -3.5, -3.9)
    ForEachVegType(k) {
        if(k != NVEGTYPES-1){ // dont want to check the last round since the below test does k+1 if we checked the last loop it would fail.
          ASSERT_GE(SW_VegProd.critSoilWater[SW_VegProd.rank_SWPcrits[k]], SW_VegProd.critSoilWater[SW_VegProd.rank_SWPcrits[k+1]]);
       }
    }


    ForEachVegType(k){ // test when all input vals are the same
  		SW_VegProd.critSoilWater[k] = SW_VegProd.critSoilWater[0]; // set all crit values to be the same
  	}

    get_critical_rank(); // re-run function with same critical value inputs to ensure it works when all values the same.

    ForEachVegType(k) {
        if(k != NVEGTYPES-1){ // dont want to check the last round since the below test does k+1 if we checked the last loop it would fail.
          ASSERT_GE(SW_VegProd.critSoilWater[SW_VegProd.rank_SWPcrits[k]], SW_VegProd.critSoilWater[SW_VegProd.rank_SWPcrits[k+1]]);
       }
    }

    Reset_SOILWAT2_after_UnitTest();
  }

} // namespace
