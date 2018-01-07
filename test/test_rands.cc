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


namespace {
  // This tests the beta random number generator
  TEST(BetaGeneratorTest, ZeroToOneOutput) {
    EXPECT_LT(RandBeta(0.5, 2), 1);
    EXPECT_LT(RandBeta(1, 3), 1);
    EXPECT_GT(RandBeta(1, 4), 0);
    EXPECT_GT(RandBeta(0.25, 1), 0);
  }

  TEST(BetaGeneratorDeathTest, Errors) {
    EXPECT_DEATH(RandBeta(-0.5, 2), "AA <= 0.0");
    EXPECT_DEATH(RandBeta(1, -3), "BB <= 0.0");
    EXPECT_DEATH(RandBeta(-1, -3), "AA <= 0.0");
  }

} // namespace