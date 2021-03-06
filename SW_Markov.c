/********************************************************/
/********************************************************/
/*  Source file: Markov.c
 *  Type: module; used by Weather.c
 *
 *  Application: SOILWAT - soilwater dynamics simulator
 *  Purpose: Read / write and otherwise manage the markov
 *           weather generation information.
 *  History:
 *     (8/28/01) -- INITIAL CODING - cwb
 *    12/02 - IMPORTANT CHANGE - cwb
 *          refer to comments in Times.h regarding base0
 06/27/2013	(drs)	closed open files if LogError() with LOGFATAL is called in SW_MKV_read_prob(), SW_MKV_read_cov()
 */
/********************************************************/
/********************************************************/

/* =================================================== */
/* =================================================== */
/*                INCLUDES / DEFINES                   */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "generic.h"
#include "filefuncs.h"
#include "rands.h"
#include "Times.h"
#include "myMemory.h"
#include "SW_Defines.h"
#include "SW_Files.h"
#include "SW_Weather.h"
#include "SW_Model.h"
#include "SW_Markov.h"
#include "pcg/pcg_basic.h"

/* =================================================== */
/*                  Global Variables                   */
/* --------------------------------------------------- */
extern SW_MODEL SW_Model;
pcg32_random_t markov_rng;
SW_MARKOV SW_Markov; /* declared here, externed elsewhere */

/* =================================================== */
/*                Module-Level Variables               */
/* --------------------------------------------------- */

static char *MyFileName;

/* =================================================== */
/* =================================================== */
/*             Private Function Definitions            */
/* --------------------------------------------------- */

/**
  @brief Adjust average maximum/minimum daily temperature for whether day is
         wet or dry

  This function accounts for the situation such as daily maximum temperature
  being reduced on wet days compared to average (e.g., clouds prevent warming
  during daylight hours) and such as daily minimum temperature being increased
  on wet days compared to average (e.g., clouds prevent radiative losses
  during the night).

  All temperature values are in units of degree Celsius.

  @param tmax The average daily maximum temperature to be corrected
    (as produced by `mvnorm`).
  @param tmin The average daily minimum temperature to be corrected
    (as produced by `mvnorm`).
  @param rain Today's rain amount.
  @param cfmax_wet The additive correction factor for maximum temperature on a
    wet day.
  @param cfmax_dry The additive correction factor for maximum temperature on a
    dry day.
  @param cfmin_wet The additive correction factor for minimum temperature on a
    wet day.
  @param cfmin_dry The additive correction factor for minimum temperature on a
    dry day.

  @return tmax The corrected maximum temperature, i.e., tmax + cfmax.
  @return tmin The corrected minimum temperature, i.e., tmin + cfmin.
*/
static void temp_correct_wetdry(RealD *tmax, RealD *tmin, RealD rain,
  RealD cfmax_wet, RealD cfmax_dry, RealD cfmin_wet, RealD cfmin_dry) {

  if (GT(rain, 0.)) {
    // today is wet: use wet-day correction factors
    *tmax = *tmax + cfmax_wet;
    *tmin = fmin(*tmax, *tmin + cfmin_wet);

  } else {
    // today is dry: use dry-day correction factors
    *tmax = *tmax + cfmax_dry;
    *tmin = fmin(*tmax, *tmin + cfmin_dry);
  }
}


#ifdef SWDEBUG
  // since `temp_correct_wetdry` is static we cannot do unit tests unless we set it up
  // as an externed function pointer
  void (*test_temp_correct_wetdry)(RealD *, RealD *, RealD, RealD, RealD, RealD, RealD) = &temp_correct_wetdry;
#endif



/** Calculate multivariate normal variates for a set of
    minimum and maximum temperature means, variances, and their covariance
    for a specific day

    @param wTmax Mean weekly maximum daily temperature (degree Celsius);
           previously `_ucov[0]`
    @param wTmin Mean weekly minimum daily temperature (degree Celsius);
           previously `_ucov[1]`
    @param wTmax_var Mean weekly variance of maximum daily temperature;
           previously `vc00 = _vcov[0][0]`
    @param wTmin_var Mean weekly variance of minimum daily temperature;
           previously `vc11 = _vcov[1][1]`
    @param wT_covar Mean weekly covariance between maximum and minimum
           daily temperature; previously `vc10 = _vcov[1][0]`

    @return Daily minimum (*tmin) and maximum (*tmax) temperature.
*/
static void mvnorm(RealD *tmax, RealD *tmin, RealD wTmax, RealD wTmin,
	RealD wTmax_var, RealD wTmin_var, RealD wT_covar) {
	/* --------------------------------------------------- */
	/* This proc is distilled from a much more general function
	 * in the original fortran version which was prepared to
	 * handle any number of variates.  In our case, there are
	 * only two, tmax and tmin, so there can be many fewer
	 * lines.  The purpose is to compute a random normal tmin
	 * value that covaries with tmax based on the covariance
	 * file read in at startup.
	 *
	 * cwb - 09-Dec-2002 -- This used to be two functions but
	 *       after some extensive debugging in this and the
	 *       RandNorm() function, it seems silly to maintain
	 *       the extra function call.
	 * cwb - 24-Oct-03 -- Note the switch to double (RealD).
	 *       C converts the floats transparently.
	 */
	RealD s, z1, z2, wTmax_sd, vc10, vc11;

	// Gentle, J. E. 2009. Computational statistics. Springer, Dordrecht; New York.
	// ==> mvnorm = mean + A * z
	// where
	//   z = vector of standard normal variates
	//   A = Cholesky factor or the square root of the variance-covariance matrix

	// 2-dimensional case:
	//   mvnorm1 = mean1 + sd1 * z1
	//   mvnorm2 = mean2 + sd2 * (rho * z1 + sqrt(1 - rho^2) * z2)
	//      where rho(Pearson) = covariance / (sd1 * sd2)

	// mvnorm2 = sd2 * rho * z1 + sd2 * sqrt(1 - rho^2) * z2
	// mvnorm2 = covar / sd1 * z1 + sd2 * sqrt(1 - covar ^ 2 / (var1 * var2)) * z2
	// mvnorm2 = covar / sd1 * z1 + sqrt(var2 - covar ^ 2 / var1) * z2
	// mvnorm2 = vc10 * z1 + vc11 * z2
	// with
	//   vc10 = covar / sd1
	//   s = covar ^ 2 / var1
	//   vc11 = sqrt(var2 - covar ^ 2 / var1)

	// Generate two independent standard normal random numbers
	z1 = RandNorm(0., 1., &markov_rng);
	z2 = RandNorm(0., 1., &markov_rng);

	wTmax_sd = sqrt(wTmax_var);
	vc10 = (GT(wTmax_sd, 0.)) ? wT_covar / wTmax_sd : 0;
	s = vc10 * vc10;

	if (GT(s, wTmin_var)) {
		LogError(logfp, LOGFATAL, "\nBad covariance matrix in mvnorm()");
	}

	vc11 = (EQ(wTmin_var, s)) ? 0. : sqrt(wTmin_var - s);

	// mvnorm = mean + A * z
	*tmax = wTmax_sd * z1 + wTmax;
	*tmin = fmin(*tmax, (vc10 * z1) + (vc11 * z2) + wTmin);
}

#ifdef SWDEBUG
  // since `mvnorm` is static we cannot do unit tests unless we set it up
  // as an externed function pointer
  void (*test_mvnorm)(RealD *, RealD *, RealD, RealD, RealD, RealD, RealD) = &mvnorm;
#endif



/* =================================================== */
/* =================================================== */
/*             Public Function Definitions             */
/* --------------------------------------------------- */
/**
@brief Markov constructor for global variables.
*/
void SW_MKV_construct(void) {
	/* =================================================== */
	SW_MARKOV *m = &SW_Markov;
	size_t s = sizeof(RealD);

	/* STEPWAT2: The markov_rng seed will be reset with `Globals.randseed` by
		 its `main` at the beginning of each iteration */
	RandSeed(0, &markov_rng);

	m->ppt_events = 0;

	m->wetprob = (RealD *) Mem_Calloc(MAX_DAYS, s, "SW_MKV_construct");
	m->dryprob = (RealD *) Mem_Calloc(MAX_DAYS, s, "SW_MKV_construct");
	m->avg_ppt = (RealD *) Mem_Calloc(MAX_DAYS, s, "SW_MKV_construct");
	m->std_ppt = (RealD *) Mem_Calloc(MAX_DAYS, s, "SW_MKV_construct");
	m->cfxw = (RealD *) Mem_Calloc(MAX_DAYS, s, "SW_MKV_construct");
	m->cfxd = (RealD *) Mem_Calloc(MAX_DAYS, s, "SW_MKV_construct");
	m->cfnw = (RealD *) Mem_Calloc(MAX_DAYS, s, "SW_MKV_construct");
	m->cfnd = (RealD *) Mem_Calloc(MAX_DAYS, s, "SW_MKV_construct");
}

/**
@brief Markov deconstructor; frees up memory space.
*/
void SW_MKV_deconstruct(void)
{
	if (!isnull(SW_Markov.wetprob)) {
		Mem_Free(SW_Markov.wetprob);
		SW_Markov.wetprob = NULL;
	}

	if (!isnull(SW_Markov.dryprob)) {
		Mem_Free(SW_Markov.dryprob);
		SW_Markov.dryprob = NULL;
	}

	if (!isnull(SW_Markov.avg_ppt)) {
		Mem_Free(SW_Markov.avg_ppt);
		SW_Markov.avg_ppt = NULL;
	}

	if (!isnull(SW_Markov.std_ppt)) {
		Mem_Free(SW_Markov.std_ppt);
		SW_Markov.std_ppt = NULL;
	}

	if (!isnull(SW_Markov.cfxw)) {
		Mem_Free(SW_Markov.cfxw);
		SW_Markov.cfxw = NULL;
	}

	if (!isnull(SW_Markov.cfxd)) {
		Mem_Free(SW_Markov.cfxd);
		SW_Markov.cfxd = NULL;
	}

	if (!isnull(SW_Markov.cfnw)) {
		Mem_Free(SW_Markov.cfnw);
		SW_Markov.cfnw = NULL;
	}

	if (!isnull(SW_Markov.cfnd)) {
		Mem_Free(SW_Markov.cfnd);
		SW_Markov.cfnd = NULL;
	}
}

/**
@brief Calculates precipitation and temperature.

@param doy0 Day of the year (base0).
@param *tmax Maximum temperature (&deg;C).
@param *tmin Mininum temperature (&deg;C).
@param *rain Rainfall (cm).

@sideeffect *tmax Updated maximum temperature (&deg;C).
@sideeffect *tmin Updated minimum temperature (&deg;C).
@sideeffect *rain Updated rainfall (cm).
*/
void SW_MKV_today(TimeInt doy0, RealD *tmax, RealD *tmin, RealD *rain) {
	/* =================================================== */
	/* enter with rain == yesterday's ppt, doy0 as array index: [0, 365] = doy - 1
	 * leave with rain == today's ppt
	 */
	TimeInt week;
	RealF prob, p, x;

	#ifdef SWDEBUG
	short debug = 0;
	#endif

	/* Calculate Precipitation:
		prop = probability that it precipitates today depending on whether it
			was wet (precipitated) yesterday `wetprob` or
			whether it was dry yesterday `dryprob` */
	prob = (GT(*rain, 0.0)) ? SW_Markov.wetprob[doy0] : SW_Markov.dryprob[doy0];

	p = RandUni(&markov_rng);
	if (LE(p, prob)) {
		x = RandNorm(SW_Markov.avg_ppt[doy0], SW_Markov.std_ppt[doy0], &markov_rng);
		*rain = fmax(0., x);
	} else {
		*rain = 0.;
	}

	if (GT(*rain, 0.)) {
		SW_Markov.ppt_events++;
	}

	/* Calculate temperature */
	week = doy2week(doy0 + 1);

	mvnorm(tmax, tmin,
		SW_Markov.u_cov[week][0],    // mean weekly maximum daily temp
		SW_Markov.u_cov[week][1],    // mean weekly minimum daily temp
		SW_Markov.v_cov[week][0][0], // mean weekly variance of maximum daily temp
		SW_Markov.v_cov[week][1][1], // mean weekly variance of minimum daily temp
		SW_Markov.v_cov[week][1][0]  // mean weekly covariance of min/max daily temp
	);

	temp_correct_wetdry(tmax, tmin, *rain,
		SW_Markov.cfxw[week],  // correction factor for tmax for wet days
		SW_Markov.cfxd[week],  // correction factor for tmax for dry days
		SW_Markov.cfnw[week],  // correction factor for tmin for wet days
		SW_Markov.cfnd[week]   // correction factor for tmin for dry days
	);

	#ifdef SWDEBUG
	if (debug) {
		swprintf("mkv: yr=%d/doy0=%d/week=%d: ppt=%.3f, tmax=%.3f, tmin=%.3f\n",
			SW_Model.year, doy0, week, *rain, *tmax, *tmin);
	}
	#endif

}
/**
@brief Reads prob file in and checks input variables for errors, then stores files in SW_Markov.

@return swTRUE Returns true if prob file is correctly opened and closed.
*/
Bool SW_MKV_read_prob(void) {
	/* =================================================== */
	SW_MARKOV *v = &SW_Markov;
	const int nitems = 5;
	FILE *f;
	int lineno = 0, day, x, msg_type = 0;
	char msg[200]; // error message
	RealF wet, dry, avg, std;

	/* note that Files.read() must be called prior to this. */
	MyFileName = SW_F_name(eMarkovProb);

	if (NULL == (f = fopen(MyFileName, "r")))
		return swFALSE;

	while (GetALine(f, inbuf)) {
		if (lineno++ == MAX_DAYS)
			break; /* skip extra lines */

		x = sscanf(inbuf, "%d %f %f %f %f",
			&day, &wet, &dry, &avg, &std);

		// Check that text file is ok:
		if (x < nitems) {
			msg_type = LOGFATAL;
			sprintf(msg, "Too few values in line %d of file %s\n",
				lineno, MyFileName);
		}

		// Check that input values meet requirements:

		// day is a real calendar day
		if (!isfinite((float) day) || day < 1 || day > MAX_DAYS)
		{
			msg_type = LOGFATAL;
			sprintf(msg, "'day' = %d is out of range in line %d of file %s\n",
				day, lineno, MyFileName);
		}

		// Probabilities are in [0, 1]
		if (!isfinite(wet) || LT(wet, 0.) || GT(wet, 1.) ||
				!isfinite(dry) || LT(dry, 0.) || GT(dry, 1.))
		{
			msg_type = LOGFATAL;
			sprintf(msg, "Probabilities of being wet = %f and/or of being dry = %f "\
				"are out of range in line %d of file %s\n",
				wet, dry, lineno, MyFileName);
		}

		// Mean and SD of daily precipitation are >= 0
		if (!isfinite(avg) || LT(avg, 0.) || !isfinite(std) || LT(std, 0.))
		{
			msg_type = LOGFATAL;
			sprintf(msg, "Mean daily precipitation = %f and/or SD = %f "\
				"are out of range in line %d of file %s\n",
				avg, std, lineno, MyFileName);
		}

		// If any input is bad, then close file and fail with message:
		if (msg_type != 0)
		{
			CloseFile(&f);
			LogError(logfp, LOGFATAL, "%s", msg);
		}

		// Store values in `SW_Markov`
		day--; // base1 -> base0

		v->wetprob[day] = wet; // probability of being wet today given a wet yesterday
		v->dryprob[day] = dry; // probability of being wet today given a dry yesterday
		v->avg_ppt[day] = avg; // mean precip (cm) of wet days
		v->std_ppt[day] = std; // std dev. for precip of wet days
	}

	CloseFile(&f);

	return swTRUE;
}

/**
@brief Reads cov file in and checks input variables for errors, then stores files in SW_Markov.

@return Returns true if cov file is correctly opened and closed.
*/
Bool SW_MKV_read_cov(void) {
	/* =================================================== */
	SW_MARKOV *v = &SW_Markov;
	const int nitems = 11;
	FILE *f;
	int lineno = 0, week, x, msg_type = 0;
	char msg[200]; // error message
	RealF t1, t2, t3, t4, t5, t6, cfxw, cfxd, cfnw, cfnd;

	MyFileName = SW_F_name(eMarkovCov);

	if (NULL == (f = fopen(MyFileName, "r")))
		return swFALSE;

	while (GetALine(f, inbuf)) {
		if (lineno++ == MAX_WEEKS)
			break; /* skip extra lines */

		x = sscanf(inbuf, "%d %f %f %f %f %f %f %f %f %f %f",
			&week, &t1, &t2, &t3, &t4, &t5, &t6, &cfxw, &cfxd, &cfnw, &cfnd);

		// Check that text file is ok:
		if (x < nitems) {
			msg_type = LOGFATAL;
			sprintf(msg, "Too few values in line %d of file %s\n",
				lineno, MyFileName);
		}

		// week is a real calendar week
		if (!isfinite((float) week) || week < 1 || week > MAX_WEEKS)
		{
			msg_type = LOGFATAL;
			sprintf(msg, "'week' = %d is out of range in line %d of file %s\n",
				week, lineno, MyFileName);
		}

		// Mean weekly temperature values are real numbers
		if (!isfinite(t1) || !isfinite(t2))
		{
			msg_type = LOGFATAL;
			sprintf(msg, "Mean weekly temperature (max = %f and/or min = %f) "\
				"are not real numbers in line %d of file %s\n",
				t1, t2, lineno, MyFileName);
		}

		// Covariance values are finite
		if (!isfinite(t3) || !isfinite(t4) || !isfinite(t5) || !isfinite(t6))
		{
			msg_type = LOGFATAL;
			sprintf(msg, "One of the covariance values is not a real number "\
				"(t3 = %f; t4 = %f; t5 = %f; t6 = %f) in line %d of file %s\n",
				t3, t4, t5, t6, lineno, MyFileName);
		}

		// Correction factors are real numbers
		if (!isfinite(cfxw) || !isfinite(cfxd) ||
				!isfinite(cfnw) || !isfinite(cfnd))
		{
			msg_type = LOGFATAL;
			sprintf(msg, "One of the correction factor is not a real number "\
				"(cfxw = %f; cfxd = %f; cfnw = %f; cfnd = %f) in line %d of file %s\n",
				cfxw, cfxd, cfnw, cfnd, lineno, MyFileName);
		}

		// If any input is bad, then close file and fail with message:
		if (msg_type != 0)
		{
			CloseFile(&f);
			LogError(logfp, LOGFATAL, "%s", msg);
		}

		// Store values in `SW_Markov`
		week--; // base1 -> base0

		v->u_cov[week][0] = t1;    // mean weekly maximum daily temp
		v->u_cov[week][1] = t2;    // mean weekly minimum daily temp
		v->v_cov[week][0][0] = t3; // mean weekly variance of maximum daily temp
		v->v_cov[week][0][1] = t4; // mean weekly covariance of min/max daily temp
		v->v_cov[week][1][0] = t5; // mean weekly covariance of min/max daily temp
		v->v_cov[week][1][1] = t6; // mean weekly variance of minimum daily temp
		v->cfxw[week] = cfxw;      // correction factor for tmax for wet days
		v->cfxd[week] = cfxd;      // correction factor for tmax for dry days
		v->cfnw[week] = cfnw;      // correction factor for tmin for wet days
		v->cfnd[week] = cfnd;      // correction factor for tmin for dry days
	}

	CloseFile(&f);

	return swTRUE;
}


void SW_MKV_setup(void) {
  SW_MKV_construct();

  if (!SW_MKV_read_prob()) {
    LogError(logfp, LOGFATAL, "Markov weather requested but could not open %s",
      SW_F_name(eMarkovProb));
  }

  if (!SW_MKV_read_cov()) {
    LogError(logfp, LOGFATAL, "Markov weather requested but could not open %s",
      SW_F_name(eMarkovCov));
  }
}


#ifdef DEBUG_MEM
#include "myMemory.h"
/*======================================================*/
void SW_MKV_SetMemoryRefs( void) {
	/* when debugging memory problems, use the bookkeeping
	 code in myMemory.c
	 This routine sets the known memory refs in this module
	 so they can be  checked for leaks, etc.  All refs will
	 have been cleared by a call to ClearMemoryRefs() before
	 this, and will be checked via CheckMemoryRefs() after
	 this, most likely in the main() function.
	 */

	NoteMemoryRef(SW_Markov.wetprob);
	NoteMemoryRef(SW_Markov.dryprob);
	NoteMemoryRef(SW_Markov.avg_ppt);
	NoteMemoryRef(SW_Markov.std_ppt);
	NoteMemoryRef(SW_Markov.cfxw);
	NoteMemoryRef(SW_Markov.cfxd);
	NoteMemoryRef(SW_Markov.cfnw);
	NoteMemoryRef(SW_Markov.cfnd);

}

#endif
