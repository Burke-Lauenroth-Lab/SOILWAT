/********************************************************/
/********************************************************/
/*	Source file: SW_Flow_lib.c
 Type: module
 Application: SOILWAT - soilwater dynamics simulator
 Purpose: Water flow subroutines that can be used
 as a more or less independent library of
 soil water flow routines.  These routines
 are designed to operate independently of
 the soilwater model's data structures.

 See Also: SoWat_flow.c  SoWat_flow_subs.c

 History:
 (4/10/2000) -- INITIAL CODING - cwb
 10/19/2010	(drs) added function hydraulic_redistribution()
 11/16/2010	(drs) added function forest_intercepted_water()
 renamed evap_litter_stdcrop() -> evap_litter_veg()
 01/06/2011	(drs) drainout is calculated in infiltrate_water_high() and infiltrate_water_low(), but was not added up -> fixed it by changing (*drainout) = drainlw to (*drainout) += drainlw in function infiltrate_water_low()
 01/06/2011	(drs) drainlw was not set to 0. in the for-loop, i.e. a layer with swc below pwp inherited incorrectly drainlw from layer above instead of drainlw=0
 02/22/2011	(drs) evap_litter_veg() set aet implicitely to 0. and then added litter evap; changed such that litter evap is now added to whatever value aet has previously
 07/21/2011	(drs) infiltrate_water_high() & infiltrate_water_low();
 - added variables: saturated water content - swcsat, impermeability, water standing above soil - standingWater
 - percolation is adjusted by (1-impermeability)
 - if a lower soil layer becomes saturated then water is pushed back up even above the soil surface into standingWater
 07/22/2011	(drs) included evaporation from standingWater into evap_litter_veg_surfaceWater() previously called evap_litter_veg()
 07/22/2011	(drs) included evaporation from standingWater into reduce_rates_by_surfaceEvaporation() previously called reduce_rates_by_intercepted()
 08/22/2011	(drs) renamed bs_ev_tr_loss() to EsT_partitioning()
 08/22/2011	(drs) added EsT_partitioning_forest() to partition bare-soil evaporation (Es) and transpiration (T) in forests
 09/08/2011	(drs) renamed stcrp_intercepted_water() to grass_intercepted_water();
 added shrub_intercepted_water() as current copy from grass_intercepted_water();
 added double scale to each xx_intercepted_water() function to scale for snowdepth effects and vegetation type fraction
 added double a,b,c,d to each xx_intercepted_water() function for for parameters in intercepted rain = (a + b*veg) + (c+d*veg) * ppt
 09/09/2011	(drs) added xx_EsT_partitioning() for each vegetation type (grass, shrub, tree); added double lai_param as parameter in exp-equation
 09/09/2011	(drs) replaced evap_litter_veg_surfaceWater() with evap_fromSurface() to be called for each surface water pool seperately
 09/09/2011	(drs) replaced SHD_xx constanst with input parameters in pot_transp()
 09/09/2011	(drs) added double Es_param_limit to pot_soil_evap() to scale and limit bare-soil evaporation rate (previously hidden constant in code)
 09/09/2011	(drs) renamed reduce_rates_by_surfaceEvaporation() to reduce_rates_by_unmetEvapDemand()
 09/11/2011	(drs) added double scale to hydraulic_redistribution() function to scale for vegetation type fraction
 09/13/2011	(drs)	added double scale, a,b,c,d to each litter_intercepted_water() function for parameters in litter intercepted rain = ((a + b*blitter) + (c+d*blitter) * ppt) * scale
 09/21/2011	(drs)	reduce_rates_by_unmetEvapDemand() is obsolete, complete E and T scaling in SW_Flow.c
 01/28/2012	(drs)	unsaturated flow drains now to swcmin instead of swcwp: changed infiltrate_water_low() accordingly
 02/04/2012	(drs)	in 'remove_from_soil': control that swc_avail >= 0
 03/23/2012	(drs)	excluded hydraulic redistribution from top soil layer (assuming that this layer is <= 5 cm deep)
 03/26/2012	(drs)	infiltrate_water_low(): allow unsaturated percolation in first layer
 05/24/2012  (DLM) started coding soil_temperature function
 05/25/2012  (DLM)  added all this fun crazy linear regression stuff to soil_temperature function
 05/29/2012  (DLM) still working on soil_temperature function, linear regression stuff should work now
 05/30/2012  (DLM) got rid of nasty segmentation fault error in soil_temperature function, also tested math seems correct after checking by hand.  added the ability to change the value of deltaX
 05/30/2012  (DLM) added soil_temp_error variable... it simply keeps track of whether or not an error has been reported in the soil_temperature function.  0 for no, 1 for yes.
 05/30/2012  (DLM) added # of lyrs check & maxdepth check at the beginning of soil_temperature function to make sure code doesn't blow up... if there isn't enough lyrs (ie < 2) or the maxdepth is too little (ie < deltaX * 2), the function quits out and reports an error to the user
 05/31/2012  (DLM) added theMaxDepth variable to soil_temperature function to allow the changing of the maxdepth of the equation, also soil_temperature() function now stores most regression data in a structure to reduce redundant regression calculations & speeds things up
 05/31/2012  (DLM) added soil_temperature_init() function for use in the soil_temperature function to initialize the values for use in the regressions... it is not apart of the header file, because it's not meant to be an external function
 06/01/2012  (DLM) edited soil_temperature function(), changed deltaT variable from hours to seconds, also changed some of the regression calculations so that swc, fc, & wp regressions are scaled properly... results are actually starting to look usable!
 06/13/2012  (DLM) soil_temperature function no longer extrapolates values for regression layers that are out of the bounds of the soil layers... instead they are now set to the last soil layers values.  extrapolating code is still in the function and can be commented out and used if wishing to go back to extrapolating the values...
 10/11/2012	(drs) petfunc(): annotated all equations, wind is now [m/s] at 2-m above ground (instead of miles/h);
 11/06/2012	(clk) petfunc(): added slope and aspect into the calculation for solar radiation
 01/31/2013	(clk)	Added new function, pot_soil_evap_bs() to the code. This function is similar to pot_soil_evap() but since it deals with bare ground as the type of vegetation, doesn't need several parameters, i.e. totagb, which also simplifies the function.
 03/07/2013	(clk)	In the functions soil_temperature and soil_temperature_init, added code to determine whether or not a soil layer was frozen
 Used the Eitzinger 2000 article, equation 3 to calculate the fusion pool that needed to be accounted for when freezing/thawing since, when freezing/thawing something, the temperature will remain the same until all of that material has froze/thawed. The amount of energy needed to do this is the fusion pool. The the rest of the temperature change can be adjusted from there.
 Also, now that we are incorporating freezing soil layers into the model, needed to account for these frozen layers with the soil water content movement.
 Needed to adjust the functions infiltrate_water_high(), evap_from_soil(), infiltrate_water_low(), and hydraulic_redistribution()
 Adjusted these function so that they only made changes to the output as long as the current soil layer was not frozen.
 In the case of drainage though, needed to make sure that the layer below the current layer was not frozen, to make sure that the drainage had somewhere to go.
 03/28/2013	(clk) 	Changed the soil_temperature function to use the actual soil temp values when determining for freezing soils, instead of using the regression values. Since the regression values were no longer being updated when the temperature would change this way, made the function call soil_temperature_init everytime that the soil temperature would change due to freezing/thawing of the soil.
 Also moved the initialization of oldsFusionPool to its own function becuase you are now possibly calling soil_temperature_init multiple times, to adjust the temperatures, and don't want to also initialize the fusion pools again.
 04/04/2013	(clk)	In infiltrate_water_high(), needed to also change the lines
 swc[0] += pptleft;
 (*standingWater) = 0.;
 to only happen when the top soil layer is not frozen.
 06/24/2013	(rjm)	made 'soil_temp_error', 'soil_temp_init' and 'fusion_pool_init' into global variable (instead of module-level) and moved them to SW_Flow.c: otherwise they will not get reset to 0 (in call to construct) between consecutive calls as a dynamic library
 07/09/2013	(clk)	initialized the two new functions: forb_intercepted_water and forb_EsT_partitioning
 01/05/2016 (drs)	added new function set_frozen_unfrozen() to determine frozen/unfrozen status of soil layers based on criteria by Parton et al. 1998 GCB
 					re-wrote code for consequences of a frozen soil layer: now based on Parton et al. 1998 GCB
 						- infiltrate_water_high(): if frozen, saturated hydraulic conductivity is reduced to 1%; infiltration is not impeded by a frozen soil
 						- infiltrate_water_low(): if frozen, unsaturated hydraulic conductivity is reduced to 1%
 						- hydraulic_redistribution(): no hd between two layers if at least one is frozen
 						- remove_from_soil(): no evaporation and transpiration from a frozen soil layer
02/08/2016 (CMA & CTD) In the function surface_temperature_under_snow(), used Parton's Eq. 5 & 6 from 1998 paper instead of koren paper
								 Adjusted function calls to surface_temperature_under_snow to account for the new parameters
03/01/2016 (CTD) Added error check for Rsoilwat called tempError()
*/
/********************************************************/
/********************************************************/

/* =================================================== */
/*                INCLUDES / DEFINES                   */
/* --------------------------------------------------- */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "generic.h"
#include "SW_Defines.h"
#include "SW_Flow_lib.h"
#include "SW_Flow_subs.h"
#include "Times.h"


/* =================================================== */
/*                  Global Variables                   */
/* --------------------------------------------------- */
extern SW_SITE SW_Site;
unsigned int soil_temp_error;  // simply keeps track of whether or not an error has been reported in the soil_temperature function.  0 for no, 1 for yes.
unsigned int soil_temp_init;   // simply keeps track of whether or not the values for the soil_temperature function have been initialized.  0 for no, 1 for yes.
unsigned int fusion_pool_init;   // simply keeps track of whether or not the values for the soil fusion (thawing/freezing) section of the soil_temperature function have been initialized.  0 for no, 1 for yes.

/* *************************************************** */
/*                Module-Level Variables               */
/* --------------------------------------------------- */

static ST_RGR_VALUES stValues; // keeps track of the soil_temperature values

#ifdef RSOILWAT
SEXP tempError()
{
	SEXP swR_temp_error;
	PROTECT(swR_temp_error = NEW_LOGICAL(1));
	if (soil_temp_error == 1)
	{
		LOGICAL_POINTER(swR_temp_error)[0] = TRUE;
	}
	else
	{
		LOGICAL_POINTER(swR_temp_error)[0] = FALSE;
	}
	UNPROTECT(1);
	return swR_temp_error;
}
#endif
/* *************************************************** */
/* *************************************************** */
/*              Local Function Definitions             */
/* --------------------------------------------------- */

/**********************************************************************
 PURPOSE: Calculate the water intercepted by grasses.

 HISTORY:
 4/30/92  (SLC)
 7/1/92   (SLC) Reset pptleft to 0 if less than 0 (due to round off)
 1/19/93  (SLC) Check if vegcov is zero (in case there was no biomass),
 then no standing crop interception is possible.
 15-Oct-03 (cwb) replaced Parton's original equations with new ones
 developed by John Bradford based on Corbet and Crouse 1968.
 Replaced the following code:
 par1 = LE(vegcov, 8.5) ?  0.9 + 0.04 * vegcov
 : 1.24 + (vegcov-8.5) *.35;
 par2 = LE(vegcov, 3.0) ? vegcov * .33333333
 : 1. + (vegcov-3.)*0.182;
 *wintstcr = par1 * .026 * ppt + 0.094 * par2;

 21-Oct-03 (cwb) added MAX_WINTLIT line

 INPUTS:
 ppt     - precip. for the day
 vegcov  - vegetation cover for the day (based on monthly biomass
 values, see the routine "initprod")

 OUTPUT:
 pptleft -  precip. left after interception by standing crop.
 wintstcr - amount of water intercepted by standing crop.
 **********************************************************************/
void grass_intercepted_water(double *pptleft, double *wintgrass, double ppt, double vegcov, double scale, double a, double b, double c, double d) {
	double intcpt, slope;

	if (GT(vegcov, 0.) && GT(ppt, 0.)) {
		intcpt = b * vegcov + a;
		slope = d * vegcov + c;

		*wintgrass = (intcpt + slope * ppt) * scale;

		*wintgrass = fmin(*wintgrass, ppt);
		*wintgrass = fmin(*wintgrass, MAX_WINTSTCR);
		*pptleft = fmax( ppt - *wintgrass, 0.0);
	} else { /*  no precip., so obviously nothing is intercepted by standing crop. */
		*pptleft = ppt;
		*wintgrass = 0.0;
	}
}

void shrub_intercepted_water(double *pptleft, double *wintshrub, double ppt, double vegcov, double scale, double a, double b, double c, double d) {
	/**********************************************************************
	 PURPOSE: Calculate the water intercepted by shrubs
	 **********************************************************************/
	double intcpt, slope;

	if (GT(vegcov, 0.) && GT(ppt, 0.)) {
		intcpt = b * vegcov + a;
		slope = d * vegcov + c;

		*wintshrub = (intcpt + slope * ppt) * scale;

		*wintshrub = fmin(*wintshrub, ppt);
		*wintshrub = fmin(*wintshrub, MAX_WINTSTCR);
		*pptleft = fmax( ppt - *wintshrub, 0.0);
	} else { /*  no precip., so obviously nothing is intercepted by standing crop. */
		*pptleft = ppt;
		*wintshrub = 0.0;
	}
}

void tree_intercepted_water(double *pptleft, double *wintfor, double ppt, double LAI, double scale, double a, double b, double c, double d) {
	/**********************************************************************
	 PURPOSE: Calculate water intercepted by forests

	 HISTORY:
	 11/16/2010	(drs)

	 INPUTS:
	 ppt     - precip. for the day in cm
	 LAI	- forest LAI in cm/cm
	 scale - scale interception with fraction of tree vegetation component or with snowdepth-scaler

	 OUTPUT:
	 pptleft -  precip. left after interception by forest in cm.
	 wintfor - amount of water intercepted by forest in cm.
	 **********************************************************************/
	double intcpt, slope;

	if (GT(LAI, 0.) && GT(ppt, 0.)) {
		intcpt = b * LAI + a;
		slope = d * LAI + c;

		*wintfor = (intcpt + slope * ppt) * scale;

		*wintfor = fmin(*wintfor, ppt);
		*wintfor = fmin(*wintfor, MAX_WINTFOR);
		*pptleft = fmax( ppt - *wintfor, 0.0);
	} else { /*  no precip., so obviously nothing is intercepted by forest. */
		*pptleft = ppt;
		*wintfor = 0.0;
	}
}

void forb_intercepted_water(double *pptleft, double *wintforb, double ppt, double vegcov, double scale, double a, double b, double c, double d) {
	/**********************************************************************
	 PURPOSE: Calculate water intercepted by forbs


	 HISTORY:
	 07/09/2013	(clk)

	 INPUTS:

	 ppt     - precip. for the day in cm
	 vegcov	- vegetation cover for the day (based on monthly biomass
	 values, see the routine "initprod")
	 scale - scale interception with fraction of forb vegetation component or with snowdepth-scaler

	 OUTPUT:

	 pptleft -  precip. left after interception by forb in cm.
	 wintforb - amount of water intercepted by forb in cm.
	 **********************************************************************/
	double intcpt, slope;

	if (GT(vegcov, 0.) && GT(ppt, 0.)) {
		intcpt = b * vegcov + a;
		slope = d * vegcov + c;

		*wintforb = (intcpt + slope * ppt) * scale;

		*wintforb = fmin(*wintforb, ppt);
		*wintforb = fmin(*wintforb, MAX_WINTSTCR);
		*pptleft = fmax( ppt - *wintforb, 0.0);
	} else { /*  no precip., so obviously nothing is intercepted by forest. */
		*pptleft = ppt;
		*wintforb = 0.0;
	}
}

void litter_intercepted_water(double *pptleft, double *wintlit, double blitter, double scale, double a, double b, double c, double d) {
	/**********************************************************************
	 PURPOSE: Calculate water intercepted by litter

	 HISTORY:
	 4/30/92  (SLC)
	 7/1/92   (SLC) Reset pptleft to 0 if less than 0 (due to round off)
	 6-Oct-03 (cwb) wintlit = 0 if no litter.
	 15-Oct-03 (cwb) replaced Parton's original equations with new ones
	 developed by John Bradford based on Corbet and Crouse, 1968.
	 Replaced the following code:
	 par1 = exp((-1. + .45 * log10(blitter+1.)) * log(10.));
	 *wintlit = (.015 * (*pptleft) + .0635) * exp(par1);

	 21-Oct-03 (cwb) added MAX_WINTLIT line

	 INPUTS:
	 blitter - biomass of litter for the day

	 OUTPUTS:
	 pptleft -  precip. left after interception by litter.
	 wintlit  - amount of water intercepted by litter .
	 **********************************************************************/
	double intcpt, slope;

	if (ZRO(blitter)) {
		*wintlit = 0.0;
	} else if (GT(*pptleft, 0.0)) {
		intcpt = b * blitter + a;
		slope = d * blitter + c;

		*wintlit = (intcpt + slope * (*pptleft)) * scale;

		*wintlit = fmin(*pptleft,*wintlit);
		*wintlit = fmin(*wintlit, MAX_WINTLIT);
		*pptleft -= *wintlit;
		*pptleft = fmax(*pptleft, 0.0);

	} else {
		*pptleft = 0.0;
		*wintlit = 0.0;
	}
}

void infiltrate_water_high(double swc[], double drain[], double *drainout, double pptleft, unsigned int nlyrs, double swcfc[], double swcsat[], double impermeability[],
		double *standingWater) {
			/**********************************************************************
			 PURPOSE: Infilitrate water into soil layers under high water
			 conditions.

			 HISTORY:
			 4/30/92  (SLC)
			 1/14/02 - (cwb) fixed off by one error in loop.
			 10/20/03 - (cwb) added drainout variable to return drainage
			 out of lowest layer

			 INPUTS:
			 swc - soilwater content before drainage.
			 swcfc    - soilwater content at field capacity.
			 pptleft  - precip. available to the soil.
			 nlyrs - number of layers to drain from

			 OUTPUTS:
			 drain  - drainage from layers
			 swc_local - soilwater content after water has been drained
			 **********************************************************************/
	unsigned int i;
	int j;
	double d[nlyrs];
	double push, ksat_rel;

	ST_RGR_VALUES *st = &stValues;

	// Infiltration
	swc[0] += pptleft;
	(*standingWater) = 0.;

	// Saturated percolation
	for (i = 0; i < nlyrs; i++) {
		if (st->lyrFrozen[i] == 1) {
			ksat_rel = 0.01; // roughly estimated from Parton et al. 1998 GCB
		} else {
			ksat_rel = 1.;
		}

		/* calculate potential saturated percolation */
		d[i] = fmax(0., ksat_rel * (1. - impermeability[i]) * (swc[i] - swcfc[i]) );
		drain[i] = d[i];

		if (i < nlyrs - 1) { /* percolate up to next-to-last layer */
			swc[i + 1] += d[i];
			swc[i] -= d[i];
		} else { /* percolate last layer */
			(*drainout) = d[i];
			swc[i] -= (*drainout);
		}
	}

	/* adjust (i.e., push water upwards) if water content of a layer is now above saturated water content */
	for (j = nlyrs; j >= 0; j--) {
		if (GT(swc[j], swcsat[j])) {
			push = swc[j] - swcsat[j];
			swc[j] -= push;
			if (j > 0) {
				drain[j - 1] -= push;
				swc[j - 1] += push;
			} else {
				(*standingWater) = push;
			}
		}
	}
}

double petfunc(unsigned int doy, double avgtemp, double rlat, double elev, double slope, double aspect, double reflec, double humid, double windsp, double cloudcov,
		double transcoeff) {
	/***********************************************************************
	 PURPOSE: Calculate the potential evapotranspiration [mm/day] rate using pennmans equation (1948)

	 HISTORY:
	 4/30/92  (SLC)
	 10/11/2012	(drs)	annotated all equations;
	 replaced unknown equation for solar declination with one by Spencer (1971);
	 replaced unknown equation for 'Slope of the Saturation Vapor Pressure-Temperature Curve' = arads with one provided by Allen et al. (1998) and (2005);
	 replaced constant psychrometric constant (0.27 [mmHg/F]) as function of pressure, and pressure as function of elevation of site (Allen et al. (1998) and (2005));
	 windspeed data is in [m/s] and not in code-required [miles/h] -> fixed conversation factor so that code now requires [m/s];
	 replaced conversion addend from C to K (previously, 273) with 273.15;
	 updated solar constant from S = 2.0 [ly/min] to S = 1.952 [ly/min] (Kopp et al. 2011) in the equation to calculate solrad;
	 replaced unknown numerical factor of 0.201 in black-body long wave radiation to sigma x conversion-factor = 0.196728 [mm/day/K4];
	 --> further update suggestions:
	 - add Seller's factor '(mean(d)/d)^2' with (mean(d) = mean distance and d = instantaneous distance of the earth from the sun) to shortwave calculations
	 11/06/2012	(clk)	allowed slope and aspect to be used in the calculation for solar radiation;
	 if slope is 0, will still use old equation,
	 else will sum up Seller (1965), page 35, eqn. 3.15 from sunrise to sunset.

	 SOURCES:
	 Allen RG, Pereira LS, Raes D, Smith M (1998) In Crop evapotranspiration - Guidelines for computing crop water requirements. FAO - Food and Agriculture Organizations of the United Nations, Rome.
	 Allen RG, Walter IA, Elliott R, Howell T, Itenfisu D, Jensen M (2005) In The ASCE standardized reference evapotranspiration equation, pp. 59. ASCE-EWRI Task Committee Report.
	 Bowen IS (1926) The Ratio of Heat Losses by Conduction and by Evaporation from any Water Surface. Physical Review, 27, 779.
	 Brunt D (1939) Physical and dynamical meteorology. Cambridge University Press.
	 Kopp G, Lean JL (2011) A new, lower value of total solar irradiance: Evidence and climate significance. Geophysical Research Letters, 38, L01706.
	 Merva GE (1975) Physioengineering principles. Avi Pub. Co., Westport, Conn., ix, 353 p. pp.
	 Penman HL (1948) Natural evaporation form open water, bare soil and grass. Proceedings of the Royal Society of London. Series A, Mathematical and Physical Sciences, 193, 120-145.
	 Sellers WD (1965) Physical climatology. University of Chicago Press, Chicago, USA.
	 Spencer JW (1971) Fourier Series Representation of the Position of the Sun. Search, 2, 172-172.


	 INPUTS:
	 Time.Model:
	 doy            		- current day number
	 sky_parms:
	 humid(month)   		- average relative humidity for the month. (%)
	 windsp(month)   	- average wind speed for the month at 2-m above ground. (m/s)
	 cloudcov(month)   	- average cloud cover for the month. (%)
	 transcoeff(month) 	- transmission coefficient for the month (not used in result)
	 weather:
	 avgtemp         	- average temperature for the day [C]
	 site_parm:
	 reflec          	- albedo [-]
	 rlat	       		- latitude of the site (in radians)
	 elev				- elevation of site (m above sea level)
	 slope			- slope of the site (in degrees)
	 aspect			- aspect of the site (in degrees)

	 LOCAL VARIABLES:
	 solrad - solar radiation (ly/day)
	 declin - solar declination (radians)
	 ahou   - sunset hour angle
	 azmth  - azimuth angle of the sun
	 azmthSlope - azimuth angle of the slope
	 rslope - slope of the site (radians)
	 hou    - hour angle
	 shwave - short wave solar radiation (mm/day)
	 kelvin - average air temperature [K]
	 arads  - 'Slope of the Saturation Vapor Pressure-Temperature Curve' [mmHg/F]
	 clrsky - relative amount of clear sky
	 fhumid - saturation vapor pressure at dewpoint [mmHg]
	 ftemp  - theoretical black-body radiation [mm/day]
	 par1,par2 - parameters in computation of pet.
	 cosZ,sinZ - parameters in computation of pet.
	 cosA,sinA - parameters in computation of pet.
	 stepSize - the step size to use in integration

	 ***********************************************************************/

	double declin, par1, par2, ahou, hou, azmth, solrad, shwave, kelvin, arads, clrsky, ftemp, vapor, result, dayAngle, P, gamma, cosZ, sinZ, cosA, sinA, stepSize, azmthSlope,
			rslope;

	/* Unit conversion factors:
	 1 langley = 1 ly = 41840 J/m2 = 0.0168 evaporative-mm (1 [ly] / 2490 [kJ/kg heat of vaporization at about T = 10-15 C], see also Allen et al. (1998, ch. 1))
	 1 mmHg = 101.325/760 kPa = 0.1333 kPa
	 1 mile = 1609.344 m
	 0 C = 273.15 K */

	/* calculate solar declination */
	/* pre-Oct/11/2012 equation (unknown source): declin = .401426 *sin(6.283185 *(doy -77.) /365.); */
	dayAngle = 6.283185 * (doy - 1.) / 365.; /* Spencer (1971): dayAngle = day angle [radians] */
	declin = 0.006918 - 0.399912 * cos(dayAngle) + 0.070257 * sin(dayAngle) - 0.006758 * cos(2. * dayAngle) + 0.000907 * sin(2. * dayAngle) - 0.002697 * cos(3. * dayAngle)
			+ 0.00148 * sin(3. * dayAngle); /* Spencer (1971): declin = solar declination [radians] */

	/* calculate the short wave solar radiation on a clear day using an equation presented by Sellers (1965)*/
	par2 = -tan(rlat) * tan(declin); /* Sellers (1965), page 15, eqn. 3.3: par2 = cos(H) with H = half-day length = ahou = sunset hour angle */
	par1 = sqrt(1. - (par2*par2)); /* trigonometric identities: par1 = sin(H) */
	ahou = fmax(atan2(par1,par2), 0.0); /* calculate ahou = H from trigonometric function: tan(H) = sin(H)/cos(H) */

	if (slope != 0) {
		stepSize = (ahou / 24); /* step size is calculated by the the difference in our limits of integrations, for hou, using 0 to ahou, divided by some resolution. The best resolution size seems to be around 24*/
		azmthSlope = 6.283185 * (aspect - 180) / 360; /* convert the aspect of the slope from degrees into radians */
		rslope = 6.283185 * slope / 360; /* convert the slope from degrees into radians */
		solrad = 0; /* start with an initial solrad of zero, then begin the summation */
		for (hou = -ahou; hou <= ahou; hou += stepSize) /* sum Sellers (1965), page 35, eqn. 3.15 over the period of sunrise to sunset, h=-ahou to h=ahou */
		{
			cosZ = sin(rlat) * sin(declin) + cos(rlat) * cos(declin) * cos(hou); /* calculate the current value for cos(Z), Z = zenith angle of the sun, for current hour angle */
			sinZ = sqrt(1. - (cosZ*cosZ)); /* calculate the current value for sin(Z), Z = zenith angle of the sun, for current hour angle */
			cosA = (sin(rlat) * cosZ - sin(declin)) / (cos(rlat) * sinZ); /* cos(A) = cosine of the azimuth angle of the sun */
			sinA = (cos(declin) * sin(hou)) / sinZ; /* sin(A) = sine of the azimuth angle of the sun */
			azmth = atan2(sinA, cosA); /* determines the current azimuth angle of the sun based on the current hour angle */
			solrad += stepSize * (cosZ * cos(rslope) + sinZ * sin(rslope) * cos(azmth - azmthSlope)); /* Sellers (1965), page 35, eqn. 3.15: Qs [langlay] = solrad = instantaneous solar radiation on a sloped surface. */
		}
	} else /* if no slope, use old equation that doesn't account for slope to save some time */
	{
		solrad = ahou * sin(rlat) * sin(declin) + cos(rlat) * cos(declin) * sin(ahou); /* Sellers (1965), page 16, eqn. 3.7: Qs [langlay/day] = solrad = daily total solar radiation incident on a horizontal surface at the top of the atmosphere; factor '(mean(d)/d)^2' with (mean(d) = mean distance and d = instantaneous distance of the earth from the sun) of Seller's equation is missing here */
		solrad = solrad * 2; /* multiply solrad by two to account for both halves of the day, as eqn. 3.7 only integrates half a day */
	}
	solrad = (1440 / 6.283185) * 1.952 * solrad * transcoeff; /* 917. = S * 1440/pi with S = solar constant = 2.0 [langlay/min] (Sellers (1965), page 11) and with 1440 = min/day; however, solar constant S (Kopp et al. 2011) = 1.952 [langley/min] = 1361 [W/m2] <> Seller's value of S = 2.0 [langlay/min] = 1440 [W/m2] => instead of factor 917 (pre-Oct 11, 2012), it should be 895;factor 'transcoeff' is not in Seller's equation and drops out of the result with next line of code; */

	shwave = solrad * .0168 / transcoeff; /* shwave used in Penman (1948), eqn. 13: shwave [evaporation equivalent-mm/day] = RA = total radiation if the atmosphere were perfectly clear; Rc = Short-wave radiation from sun and sky [usually in evaporation equivalent of mm/day] ? [radiation/cm2/day,] = RA*(0.18+0.55n/N) is approximation for Rothamsted based on monthly values over the period 1931-40; with x = 0.0168 = conversion factor from [ly] to [mm] */

	/* calculate long wave radiation */
	kelvin = avgtemp + 273.15; /* kelvin = Ta = average air temperature of today [C] converted to [K] */
	ftemp = kelvin * .01;
	ftemp = ftemp * ftemp * ftemp * ftemp * 11.71 * 0.0168; /* Sellers (1965), eqn. 3.8: ftemp [mm/day] = theoretical black-body radiation at Ta [K] = Stefan-Boltzmann law = sigma*Ta^4 [W/m2] with sigma = 5.670373*1e-8 [W/m2/K4] = 11.71*1e-8 [ly/day/K4] (http://physics.nist.gov/cgi-bin/cuu/Value?sigma);
	 ftemp is used in Penman (1948), eqn. 13 with units = [evaporation equivalent-mm/day];
	 with unknown x = 0.201*1e-8 (value pre-Oct 11, 2012), though assuming x = sigma * conversion factor([ly] to [mm]) = 11.71*10\88-8 [ly/day/K4] * 0.0168 [mm/ly] = 0.196728 [mm/day/K4] ca.= 0.201 ? */

	/* calculate the PET using Penman (1948) */
	vapor = svapor(avgtemp); /* Penman (1948), ea = vapor = saturation vapor pressure at air-Tave [mmHg] */
	/* pre-Oct/11/2012 equation (unknown source): arads = vapor *3010.21 / (kelvin*kelvin); with unknown: x = 3010.12 =? 5336 [mmHg*K] (Merva (1975)) * 9/5 [F/K] = 2964 [mmHg*F]; however, result virtually identical with FAO and ASCE formulations (Allen et al. 1998, 2005) --> replaced  */
	arads = 4098. * vapor / ((avgtemp + 237.3) * (avgtemp + 237.3)) * 5. / 9.; /* Allen et al. (1998, ch.3 eqn. 13) and (2005, eqn. 5): arads used in Penman (1948), eqn. 16: arads [mmHg/F] = Delta [mmHg/C] * [C/F] = slope of e:T at T=Ta = 'Slope of the Saturation Vapor Pressure-Temperature Curve' */
	clrsky = 1. - cloudcov / 100.; /* Penman (1948): n/N = clrsky = Ratio of actual/possible hours of sunshine = 1 - m/10 = 1 - fraction of sky covered by cloud */
	humid *= vapor / 100.; /* Penman (1948): ed = humid = saturation vapor pressure at dewpoint [mmHg] = relative humidity * ea */
	windsp *= 53.70; /* u2 [miles/day at 2-m above ground] = windsp [miles/h at 2-m above ground] * 24 [h/day] = windsp [m/s at 2-m above ground] * 86400 [s/day] * 1/1609.344 [miles/m] with 86400/1609 = 53.70 */
	par1 = .35 * (vapor - humid) * (1. + .0098 * windsp); /* Penman (1948), eqn. 19: par1 = Ea [mm/day] = evaporation rate from open water with ea instead of es as required in eqn. 16 */
	par2 = (1. - reflec) * shwave * (.18 + .55 * clrsky) /* Penman (1948), eqn. 13 [mm/day]: par2 = H = net radiant energy available at surface [mm/day] */
	- ftemp * (.56 - .092 * sqrt(humid)) * (.10 + .90 * clrsky);
	P = 101.3 * powe((293. - 0.0065 * elev) / 293., 5.26); /* Allen et al. (1998, ch.3 eqn. 7) and (2005, eqn. 3): P [kPa] = atmospheric pressure with elev [m] */
	gamma = 0.000665 * P * 760. / 101.325 * 5. / 9.; /* Allen et al. (1998, ch.3 eqn. 8) and (2005, eqn. 4): gamma [mmHg/F] = psychrometric constant [kPa/C] * [mmHG/kPa] * [C/F] */
	result = ((arads * par2 + gamma * par1) / (arads + gamma)) / 10.;/* Penman (1948), eqn. 16: result*10 = E [mm/day] = evaporation from open water */
	/* originally and pre-Oct/11/2012, Penman (1948) gamma [mmHg/F] == 0.27*/

	return fmax(result, 0.01);
}

/**
  * calculates the saturation vapor pressure of water
	*
	* @author SLC
	*
	* @param  temp the average temperature for the day
	* @return svapor the saturation vapor pressure (mm of hg)
	*/
double svapor(double temp) {
	/*********************************************************************
	 PURPOSE: calculate the saturation vapor pressure of water
	 the clausius-clapeyron equation (hess, 1959) is used
	 HISTORY:
	 4/30/92  (SLC)

	 Hess SL (1959) Introduction to theoretical meteorology. Holt, New York.

	 INPUTS:
	 atemp - average temperature for the day

	 OUTPUT:
	 svapor - saturation vapor pressure (mm of hg)

	 *********************************************************************/
	double par1, par2;

	par1 = 1. / (temp + 273.);
	par2 = log(6.11) + 5418.38 * (.00366 - par1); /*drs: par2 = ln(es [mbar]) = ln(es(at T = 273.15K) = 6.11 [mbar]) + (mean molecular mass of water vapor) * (latent heat of vaporization) / (specific gas constant) * (1/(273.15 [K]) - 1/(air temperature [K])) */

	return (exp(par2) * .75);
}

void transp_weighted_avg(double *swp_avg, unsigned int n_tr_rgns, unsigned int n_layers, unsigned int tr_regions[], double tr_coeff[], double swc[]) {
	/**********************************************************************

	 PURPOSE: Compute weighted average of soilwater potential to be
	 used for transpiration calculations.

	 HISTORY:
	 Original:
	 4/30/92  SLC
	 6/9/93   (SLC) check that (sum_tr_co <> 0) before dividing swp by this
	 number
	 4/10/2000 CWB -- began recoding in C
	 9/21/01   cwb -- adjusted method for determining transpiration
	 regions to reflect the new design.  removed
	 tr_reg_min and max, added n_layers and tr_regions[].
	 4-Mar-02  cwb -- moved this function after ppt enters soil.  Originally,
	 it was the first function called, so evapotransp was
	 based on swp_avg prior to wetting.  Also, set return
	 value as a pointer argument to be more consistent with
	 the rest of the code.
	 1-Oct-03  cwb -- Removed sum_tr_coeff[] requirement as it might as well
	 be calculated here and save the confusion of
	 having to keep up with it in the rest of the code.

	 INPUTS:
	 n_tr_lyrs  - number of layer regions used in weighted average
	 (typically 3, to represent shallow, mid, & deep depths)
	 to compute transpiration rate.
	 n_layers - number of soil layers
	 tr_regions - list of n_tr_lyrs elements of transp regions each
	 soil layer belongs to.
	 tr_coeff  - transpiration coefficients per layer.
	 sum_tr_coeff - sum of transpiration coefficients per layer structure.
	 swc -- current swc per layer

	 1-Oct-03 - local sumco replaces previous sum_tr_coeff[]

	 OUTPUT:
	 swp_avg - weighted average of soilwater potential and transpiration
	 coefficients
	 **********************************************************************/
	unsigned int r, i;
	double swp, sumco;

	*swp_avg = 0;
	for (r = 1; r <= n_tr_rgns; r++) {
		swp = sumco = 0.0;

		for (i = 0; i < n_layers; i++) {
			if (tr_regions[i] == r) {
				swp += tr_coeff[i] * SWCbulk2SWPmatric(SW_Site.lyr[i]->fractionVolBulk_gravel, swc[i], i);
				sumco += tr_coeff[i];
			}
		}

		swp /= GT(sumco, 0.) ? sumco : 1.;

		/* use smallest weighted average of regions */
		(*swp_avg) = (r == 1) ? swp : fmin( swp, (*swp_avg));

	}
}

/**
	* calculates the fraction of water lost from bare soil
	*
	* @author SLC
	*
	* @param fbse the fraction of water loss from bare soil evaporation
	* @param fbst the fraction of water loss from bare soil transpiration
	* @param blivelai the live biomass leaf area index
	* @param lai_param
	*/
void grass_EsT_partitioning(double *fbse, double *fbst, double blivelai, double lai_param) {
	/**********************************************************************
	 PURPOSE: Calculate fraction of water loss from bare soil
	 evaporation and transpiration

	 HISTORY:
	 4/30/92  (SLC)
	 24-Oct-03 (cwb) changed exp(-blivelai*bsepar1) + bsepar2;
	 to exp(-blivelai);

	 INPUTS:
	 blivelai - live biomass leaf area index

	 OUTPUTS:
	 fbse - fraction of water loss from bare soil evaporation.
	 fbst - "                           " transpiration.

	 **********************************************************************/
	/* CWB- 4/00 Not sure what's the purpose of bsepar2, unless it's a
	 * fudge-factor to be played with.
	 */

	double bsemax = 0.995;

	*fbse = exp(-lai_param * blivelai);

	*fbse = fmin(*fbse, bsemax);
	*fbst = 1. - (*fbse);
}

void shrub_EsT_partitioning(double *fbse, double *fbst, double blivelai, double lai_param) {
	/**********************************************************************
	 PURPOSE: Calculate fraction of water loss from bare soil
	 evaporation and transpiration
	 **********************************************************************/

	double bsemax = 0.995;

	*fbse = exp(-lai_param * blivelai);

	*fbse = fmin(*fbse, bsemax);
	*fbst = 1. - (*fbse);
}

void tree_EsT_partitioning(double *fbse, double *fbst, double blivelai, double lai_param) {
	/**********************************************************************
	 PURPOSE: Calculate fraction of water loss from bare soil
	 evaporation and transpiration

	 08/22/2011	(drs)	According to a regression based on a review by Daikoku, K., S. Hattori, A. Deguchi, Y. Aoki, M. Miyashita, K. Matsumoto, J. Akiyama, S. Iida, T. Toba, Y. Fujita, and T. Ohta. 2008. Influence of evaporation from the forest floor on evapotranspiration from the dry canopy. Hydrological Processes 22:4083-4096.
	 **********************************************************************/

	double bsemax = 0.995;

	*fbse = exp(-lai_param * blivelai);

	*fbse = fmin(*fbse, bsemax);
	*fbst = 1. - (*fbse);
}

void forb_EsT_partitioning(double *fbse, double *fbst, double blivelai, double lai_param) {
	/**********************************************************************
	 PURPOSE: Calculate fraction of water loss from bare soil
	 evaporation and transpiration

	 **********************************************************************/

	double bsemax = 0.995;

	*fbse = exp(-lai_param * blivelai);

	*fbse = fmin(*fbse, bsemax);
	*fbst = 1. - (*fbse);
}

void pot_soil_evap(double *bserate, unsigned int nelyrs, double ecoeff[], double totagb, double fbse, double petday, double shift, double shape, double inflec, double range,
		double width[], double swc[], double Es_param_limit) {
	/**********************************************************************
	 PURPOSE: Calculate potential bare soil evaporation rate.
	 See 2.11 in ELM doc.

	 HISTORY:
	 4/30/92  (SLC)
	 8/27/92  (SLC) Put in a check so that bserate cannot become
	 negative.  If total aboveground biomass (i.e.
	 litter+bimoass) is > 999., bserate=0.
	 6 Mar 02 (cwb) renamed watrate's parameters (see also SW_Site.h)
	 shift,  shift the x-value of the inflection point
	 shape,  slope of the line at the inflection point
	 inflec, y-value of the inflection point
	 range;  max y-val - min y-val at the limits
	 1-Oct-03 - cwb - removed the sumecoeff variable as it should
	 always be 1.0.  Also removed the line
	 avswp = sumswp / sumecoeff;

	 INPUTS:
	 nelyrs    - number of layers to consider in evaporation
	 sumecoeff - sum of evaporation coefficients
	 ecoeff    - array of evaporation coefficients
	 totagb    - sum of abovegraound biomass and litter
	 fbse      - fraction of water loss from bare soil evaporation
	 petday       - potential evapotranspiration rate
	 width     - array containing width of each layer.
	 swc  - array of soil water content per layer.

	 LOCAL:
	 avswp     - average soil water potential over all layers
	 evpar1    - input parameter to watrate.

	 OUTPUTS:
	 bserate   - bare soil evaporation loss rate. (cm/day)

	 FUNCTION CALLS:
	 watrate   - calculate evaporation rate.
	 swpotentl - compute soilwater potential
	 **********************************************************************/

	double x, avswp = 0.0, sumwidth = 0.0;
	unsigned int i;

	/* get the weighted average of swp in the evap layers */
	for (i = 0; i < nelyrs; i++) {
		x = width[i] * ecoeff[i];
		sumwidth += x;
		avswp += x * SWCbulk2SWPmatric(SW_Site.lyr[i]->fractionVolBulk_gravel, swc[i], i);
	}

	avswp /= sumwidth;

	/*  8/27/92 (SLC) if totagb > Es_param_limit, assume soil surface is
	 * completely covered with litter and that bare soil
	 * evaporation is inhibited.
	 */

	if (GE(totagb, Es_param_limit)) {
		*bserate = 0.;
	} else {
		*bserate = petday * watrate(avswp, petday, shift, shape, inflec, range) * (1. - (totagb / Es_param_limit)) * fbse;
	}

}

void pot_soil_evap_bs(double *bserate, unsigned int nelyrs, double ecoeff[], double petday, double shift, double shape, double inflec, double range, double width[],
		double swc[]) {
	/**********************************************************************
	 PURPOSE: Calculate potential bare soil evaporation rate of bare ground.
	 See 2.11 in ELM doc.

	 INPUTS:
	 nelyrs    - number of layers to consider in evaporation
	 sumecoeff - sum of evaporation coefficients
	 ecoeff    - array of evaporation coefficients
	 petday    - potential evapotranspiration rate
	 width     - array containing width of each layer.
	 swc  	    - array of soil water content per layer.

	 LOCAL:
	 avswp     - average soil water potential over all layers
	 evpar1    - input parameter to watrate.

	 OUTPUTS:
	 bserate   - bare soil evaporation loss rate. (cm/day)

	 FUNCTION CALLS:
	 watrate   - calculate evaporation rate.
	 swpotentl - compute soilwater potential
	 **********************************************************************/

	double x, avswp = 0.0, sumwidth = 0.0;
	unsigned int i;

	/* get the weighted average of swp in the evap layers */
	for (i = 0; i < nelyrs; i++) {
		x = width[i] * ecoeff[i];
		sumwidth += x;
		avswp += x * SWCbulk2SWPmatric(SW_Site.lyr[i]->fractionVolBulk_gravel, swc[i], i);
	}

	avswp /= sumwidth;

	*bserate = petday * watrate(avswp, petday, shift, shape, inflec, range);

}

void pot_transp(double *bstrate, double swpavg, double biolive, double biodead, double fbst, double petday, double swp_shift, double swp_shape, double swp_inflec,
		double swp_range, double shade_scale, double shade_deadmax, double shade_xinflex, double shade_slope, double shade_yinflex, double shade_range) {
	/**********************************************************************
	 PURPOSE: Calculate potential transpiration rate.
	 See 2.11 in ELM doc.

	 HISTORY:
	 4/30/92  (SLC)
	 7/1/92   (SLC) Fixed bug.  Equation for bstrate not right.
	 9/1/92   (SLC) Allow transpiration, even if biodead is zero.  This
	 was in the original model, we will compute shade if biodead
	 is greater than deadmax.  Otherwise, shadeaf = 1.0
	 6 Mar 02 (cwb) renamed watrate's parameters (see also SW_Site.h)
	 shift,  shift the x-value of the inflection point
	 shape,  slope of the line at the inflection point
	 inflec, y-value of the inflection point
	 range;  max y-val - min y-val at the limits

	 INPUTS:
	 swpavg    - weighted average of soil water potential (from
	 function "transp_weighted_avg")
	 biolive   - biomass of live
	 biodead   - biomass of dead
	 fbst      - fraction of water loss from transpiration
	 petday       - potential evapotranspiration

	 LOCAL VARIABLES:
	 shadeaf - shade affect on transpiration rate
	 scale1  - scale for shade affect
	 trpar1  - input paramter to watrate
	 deadmax - maximum biomass of dead, before shade has any affect.

	 OUTPUTS:
	 bstrate   - transpiration loss rate. (cm/day)

	 FUNCTION CALLS:
	 watrate - compute transpiration rate.
	 tanfunc - tangent function
	 **********************************************************************/

	double par1, par2, shadeaf;

	if (LE(biolive, 0.)) {
		*bstrate = 0.;

	} else {
		if (GE(biodead, shade_deadmax)) {
			par1 = tanfunc(biolive, shade_xinflex, shade_yinflex, shade_range, shade_slope);
			par2 = tanfunc(biodead, shade_xinflex, shade_yinflex, shade_range, shade_slope);
			shadeaf = (par1 / par2) * (1.0 - shade_scale) + shade_scale;
			shadeaf = fmin(shadeaf, 1.0);
		} else {
			shadeaf = 1.0;
		}

		*bstrate = watrate(swpavg, petday, swp_shift, swp_shape, swp_inflec, swp_range) * shadeaf * petday * fbst;
	}
}

double watrate(double swp, double petday, double shift, double shape, double inflec, double range) {
	/**********************************************************************
	 PURPOSE: Calculate the evaporation (or transpiration) rate, as
	 a function of potential evapotranspiration and soil
	 water potential. The ratio of evaporation (transpiration)
	 rate to PET is inversely proportional to soil water
	 poential (see Fig2.5a,b, pp.39, "Abiotic Section of ELM")

	 HISTORY:
	 4/30/92  (SLC)
	 6 Mar 02 (cwb) - Changed arguments from a/b/c to shift,shape,
	 inflec, range because I finally found the source
	 for tanfunc.

	 INPUTS:
	 swp - soil water potential (-bars)
	 petday - potential evapotranspiration rate for the day.
	 a   - equation parameter (relative to transpiration or evapor. rate)
	 b   - equation parameter (relative to transpiration or evapor. rate)
	 (usually b=.06 for evaporation, and b=.07 for transpiration)

	 OUTPUT:
	 watrate - rate of evaporation (or transpiration) from the
	 soil.

	 **********************************************************************/

	double par1, par2, result;

	if (LT(petday, .2))
		par1 = 3.0;
	else if (LT(petday, .4))
		par1 = (.4 - petday) * -10. + 5.;
	else if (LT(petday, .6))
		par1 = (.6 - petday) * -15. + 8.;
	else
		par1 = 8.;

	par2 = shift - swp;

	result = tanfunc(par2, par1, inflec, range, shape);

	return (fmin( fmax( result, 0.0), 1.0));

}

// void evap_fromSurface( double *evap_pool, double *evap_demand, double *aet) {
// /**********************************************************************
// PURPOSE: Evaporate water from surface water pool, i.e., intercepted water (tree, shrub, grass, litter) or standingWater
// 	call seperately for each pool
//
// INPUTS:
// evap_pool	- pool of surface water to evaporate from
// evap_demand	- yet unmet evaporative demand of atmosphere
//
// OUTPUTS:
// evap_pool	- pool of surface water minus evaporated water
// evap_demand	- evaporative demand of atmosphere  minus evaporated water
// aet			- aet + evaporated water
// **********************************************************************/
//
// 	double evap;
// 	double a=1., b=1.;
//
// 	evap = *evap_demand * a * (1. - exp(- b * (*evap_pool)));
// 	evap = fmax(0., fmin(*evap_demand, fmin(*evap_pool, evap )));
//
// 	*evap_pool -= evap;
// 	*aet += evap;
// 	*evap_demand -= evap;
//
//
// // 	if( GT(*evap_pool, *evap_demand) ) { /* demand is smaller than available water -> entire demand is evaporated */
// // 		*evap_pool -= *evap_demand;
// // 		*aet += *evap_demand;
// // 		*evap_demand = 0.;
// // 	} else { /* demand is larger than available water -> all available is evaporated */
// // 		*evap_demand -= *evap_pool;
// // 		*aet += *evap_pool;
// // 		*evap_pool = 0.;
// // 	}
// }

void evap_fromSurface(double *water_pool, double *evap_rate, double *aet) {
	/**********************************************************************
	 PURPOSE: Evaporate water from surface water pool, i.e., intercepted water (tree, shrub, grass, litter) or standingWater
	 call seperately for each pool

	 INPUTS:
	 water_pool	- pool of surface water to evaporate from
	 evap_rate	- potential evaporation from this pool

	 OUTPUTS:
	 water_pool	- pool of surface water minus evaporated water
	 evap_rate	- actual evaporation from this pool
	 aet			- aet + evaporated water
	 **********************************************************************/

	if (GT(*water_pool, *evap_rate)) { /* potential rate is smaller than available water -> entire potential is evaporated */
		*water_pool -= *evap_rate;
		*aet += *evap_rate;
	} else { /* potential rate is larger than available water -> entire pool is evaporated */
		*evap_rate = *water_pool;
		*aet += *water_pool;
		*water_pool = 0.;
	}
}

void remove_from_soil(double swc[], double qty[], double *aet, unsigned int nlyrs, double coeff[], double rate, double swcmin[]) {
	/**********************************************************************
	 PURPOSE: Remove water from the soil.  This replaces earlier versions'
	 call to separate functions for evaporation and transpiration
	 which did exactly the same thing but simply looked
	 different.  I prefer using one function over two to avoid
	 possible errors in the same transaction.

	 See Eqns. 2.12 - 2.18 in "Abiotic Section of ELM".

	 HISTORY: 10 Jan 2002 - cwb - replaced two previous functions with
	 this one.
	 12 Jan 2002 - cwb - added aet arg.
	 4 Dec 2002  - cwb - Adding STEPWAT code uncovered possible
	 div/0 error. If no transp coeffs, return.
	 INPUTS:
	 nlyrs  - number of layers considered in water removal
	 coeff - coefficients of removal for removal layers, either
	 evap_coeff[] or transp_coeff[].
	 rate - removal rate, either soil_evap_rate or soil_transp_rate.
	 swcmin - lower limit on soilwater content (per layer).

	 OUTPUTS:
	 swc  - soil water content adjusted after evaporation
	 qty - removal quantity from each layer, evap or transp.
	 aet -

	 FUNCTION CALLS:
	 swpotentl - compute soilwater potential of the layer.
	 **********************************************************************/

	unsigned int i;
	double swpfrac[MAX_LAYERS], sumswp = 0.0, swc_avail, q;

	ST_RGR_VALUES *st = &stValues;

	for (i = 0; i < nlyrs; i++) {
		swpfrac[i] = coeff[i] / SWCbulk2SWPmatric(SW_Site.lyr[i]->fractionVolBulk_gravel, swc[i], i);
		sumswp += swpfrac[i];
	}

	if (ZRO(sumswp))
		return;

	for (i = 0; i < nlyrs; i++) {
		if (st->lyrFrozen[i] == 0) { // no evaporation and transpiration from frozen soil layer
			q = (swpfrac[i] / sumswp) * rate;
			swc_avail = fmax(0., swc[i] - swcmin[i]);
			qty[i] = fmin( q, swc_avail);
			swc[i] -= qty[i];
			*aet += qty[i];
		}
	}
}

void infiltrate_water_low(double swc[], double drain[], double *drainout, unsigned int nlyrs, double sdrainpar, double sdraindpth, double swcfc[], double width[],
		double swcmin[], double swcsat[], double impermeability[], double *standingWater) {
	/**********************************************************************
	 PURPOSE:Calculate soilwater drainage for low soil water conditions
	 See Equation 2.9 in ELM doc.

	 HISTORY:
	 4/30/92  (SLC)
	 7/2/92   (fixed bug.  equation for drainlw needed fixing)
	 8/13/92 (SLC) Changed call to function which checks lower bound
	 on soilwater content.  REplaced call to "chkzero" with
	 the function "getdiff".
	 - lower bound is used in place of zero as lower bound
	 here.  Old code used 0.cm water as a lower bound in
	 low water drainage.
	 9/22/01 - (cwb) replaced tr_reg_max[] with transp_rgn[]
	 see INPUTS
	 1/14/02 - (cwb) fixed off by one error in loop.
	 6-Oct-03  (cwb) removed condition disallowing gravitational
	 drainage from transpiration region 1.

	 INPUTS:
	 drain - drainage from each layer
	 ndeeplyr - bottom layer to stop at for drainage
	 transp_rgn - array of transpiration regions of each layer
	 sdrainpar - slow drainage parameter
	 swcfc   - soilwater content at field capacity
	 swcwp   - soilwater content at wilting point
	 swc  - soil water content adjusted by drainage
	 drain - drainage from each soil water layer

	 OUTPUTS:
	 swc  - soil water content adjusted by drainage
	 drain - drainage from each soil water layer
	 drainout - added low drainout (to already calculated high drainout)
	 **********************************************************************/

	unsigned int i;
	int j;
	double drainlw = 0.0, swc_avail, drainpot, d[nlyrs], push, kunsat_rel	;

	ST_RGR_VALUES *st = &stValues;

	// Unsaturated percolation
	for (i = 0; i < nlyrs; i++) {
		/* calculate potential unsaturated percolation */
		if (LE(swc[i], swcmin[i])) { /* in original code was !GT(swc[i], swcwp[i]) equivalent to LE(swc[i], swcwp[i]), but then water is drained to swcmin nevertheless - maybe should be LE(swc[i], swcmin[i]) */
			d[i] = 0.;
		} else {
			if (st->lyrFrozen[i] == 1) {
				kunsat_rel = 0.01; // roughly estimated from Parton et al. 1998 GCB
			} else {
				kunsat_rel = 1.;
			}
			swc_avail = fmax(0., swc[i] - swcmin[i]);
			drainpot = GT(swc[i], swcfc[i]) ? sdrainpar : sdrainpar * exp((swc[i] - swcfc[i]) * sdraindpth / width[i]);
			d[i] = kunsat_rel * (1. - impermeability[i]) * fmin(swc_avail, drainpot);
		}
		drain[i] += d[i];

		if (i < nlyrs - 1) { /* percolate up to next-to-last layer */
			swc[i + 1] += d[i];
			swc[i] -= d[i];
		} else { /* percolate last layer */
			drainlw = fmax( d[i], 0.0);
			(*drainout) += drainlw;
			swc[i] -= drainlw;
		}
	}

	/* adjust (i.e., push water upwards) if water content of a layer is now above saturated water content */
	for (j = nlyrs; j >= 0; j--) {
		if (GT(swc[j], swcsat[j])) {
			push = swc[j] - swcsat[j];
			swc[j] -= push;
			if (j > 0) {
				drain[j - 1] -= push;
				swc[j - 1] += push;
			} else {
				(*standingWater) += push;
			}
		}
	}

}

void hydraulic_redistribution(double swc[], double swcwp[], double lyrRootCo[], double hydred[], unsigned int nlyrs, double maxCondroot, double swp50, double shapeCond,
		double scale) {
	/**********************************************************************
	 PURPOSE:Calculate hydraulic redistribution according to Ryel, Ryel R, Caldwell, Caldwell M, Yoder, Yoder C, Or, Or D, Leffler, Leffler A. 2002. Hydraulic redistribution in a stand of Artemisia tridentata: evaluation of benefits to transpiration assessed with a simulation model. Oecologia 130: 173-184.

	 HISTORY:
	 10/19/2010 (drs)
	 11/13/2010 (drs) limited water extraction for hydred to swp above wilting point
	 03/23/2012 (drs) excluded hydraulic redistribution from top soil layer (assuming that this layer is <= 5 cm deep)

	 INPUTS:
	 swc  - soil water content
	 lyrRootCo - fraction of active roots in layer i
	 nlyrs  - number of soil layers
	 maxCondroot - maximum radial soil-root conductance of the entire active root system for water (cm/-bar/day)
	 swp50 - soil water potential (-bar) where conductance is reduced by 50%
	 shapeCond - shaping parameter for the empirical relationship from van Genuchten to model relative soil-root conductance for water
	 scale - fraction of vegetation type to scale hydred

	 OUTPUTS:
	 swc  - soil water content adjusted by hydraulic redistribution
	 hydred - hydraulic redistribtion for each soil water layer (cm/day/layer)

	 **********************************************************************/

	unsigned int i, j;
	double swp[nlyrs], swpwp[nlyrs], relCondroot[nlyrs], hydredmat[nlyrs][nlyrs], Rx, swa, hydred_sum;

	hydred[0] = 0.; /* no hydred in top layer */

	ST_RGR_VALUES *st = &stValues;

	for (i = 0; i < nlyrs; i++) {
		swp[i] = SWCbulk2SWPmatric(SW_Site.lyr[i]->fractionVolBulk_gravel, swc[i], i);
		relCondroot[i] = fmin( 1., fmax(0., 1./(1. + powe(swp[i]/swp50, shapeCond) ) ) );
		swpwp[i] = SWCbulk2SWPmatric(SW_Site.lyr[i]->fractionVolBulk_gravel, swcwp[i], i);

		hydredmat[0][i] = hydredmat[i][0] = 0.; /* no hydred in top layer */
	}

	for (i = 1; i < nlyrs; i++) {
		hydred[i] = hydredmat[i][i] = 0.; /* no hydred within any layer */

		for (j = i + 1; j < nlyrs; j++) {

			if ((LT(swp[i], swpwp[i]) || LT(swp[j], swpwp[j])) && (st->lyrFrozen[i] == 0) && (st->lyrFrozen[j] == 0)) { /* hydred occurs only if at least one soil layer's swp is above wilting point and both soil layers are not frozen */
				if (GT(swc[i], swc[j])) {
					Rx = lyrRootCo[i];
				} else {
					Rx = lyrRootCo[j];
				}

				hydredmat[i][j] = maxCondroot * 10. / 24. * (swp[j] - swp[i]) * fmax(relCondroot[i], relCondroot[j]) * (lyrRootCo[i] * lyrRootCo[j] / (1. - Rx)); /* assuming a 10-hour night */
				hydredmat[j][i] = -hydredmat[i][j];
			} else {
				hydredmat[i][j] = hydredmat[j][i] = 0.;
			}
		}
	}

	for (i = 0; i < nlyrs; i++) { /* total hydred from layer i cannot extract more than its swa */
		hydred_sum = 0.;
		for (j = 0; j < nlyrs; j++) {
			hydred_sum += hydredmat[i][j];
		}

		swa = fmax( 0., swc[i] - swcwp[i] );
		if (LT(hydred_sum, 0.) && GT( -hydred_sum, swa)) {
			for (j = 0; j < nlyrs; j++) {
				hydredmat[i][j] *= (swa / -hydred_sum);
				hydredmat[j][i] *= (swa / -hydred_sum);
			}
		}
	}

	for (i = 0; i < nlyrs; i++) {
		for (j = 0; j < nlyrs; j++) {
			hydred[i] += hydredmat[i][j] * scale;
		}
		swc[i] += hydred[i];
	}

}

/**********************************************************************
 PURPOSE: Initialize soil temperature values, only needs to be called once (ie the first time the soil_temperature function is called).  this is not included in the header file since it is NOT an external function

 HISTORY:
 05/31/2012 (DLM) initial coding

 INPUTS: they are all defined in the soil_temperature function, so go look there to see their meaning as it would be redundant to explain them all here as well.

 OUTPUT: none, but places the interpolation values in stValues struct for use in the soil_temperature function later
 **********************************************************************/
void lyrTemp_to_lyrSoil_temperature(double cor[MAX_ST_RGR + 1][MAX_LAYERS + 1], unsigned int nlyrTemp, double depth_Temp[], double sTempR[], unsigned int nlyrSoil, double depth_Soil[], double width_Soil[], double sTemp[]){
	unsigned int i = 0, j, n, toDebug = 0;
	double acc;

	// interpolate soil temperature values for depth of soil profile layers
	for (j = 0; j < nlyrSoil + 1; j++) {
		sTemp[j] = 0.0;
		acc = 0.0;
		n = 0;
		while (LT(acc, width_Soil[j]) && i <= nlyrTemp + 1) {
			if (EQ(cor[i][j], 0.0)) {// zero cor values indicate next soil temperature layer
				i++;}
			if (GT(cor[i][j], 0.0)) { // there are soil layers to add; index i = 0 is soil surface temperature
				if (!(i == 0 && LT(acc + cor[i][j], width_Soil[j]))) {//don't use soil surface temperature if there is other sufficient soil temperature to interpolate
					sTemp[j] += interpolation(((i > 0) ? depth_Temp[i - 1] : 0.0), depth_Temp[i], sTempR[i], sTempR[i + 1], depth_Soil[j]);
					n++; // add weighting by layer width
				}
				acc += cor[i][j];
				if (LT(acc, width_Soil[j])) i++;
			} else if (LT(cor[i][j], 0.0)) { // negative cor values indicate copying values from deepest soil layer
				break;
			}
		}
		if(n > 0)
			sTemp[j] = sTemp[j] / n;

		if (toDebug)
			#ifndef RSOILWAT
				printf("\nConf T : i=%i, j=%i, n=%i, sTemp[%i]=%2.2f, acc=%2.2f", i, j, n, j, sTemp[j], acc);
			#else
				Rprintf("\nConf T : i=%i, j=%i, n=%i, sTemp[%i]=%2.2f, acc=%2.2f", i, j, n, j, sTemp[j], acc);
			#endif

	}
}

void lyrSoil_to_lyrTemp_temperature(unsigned int nlyrSoil, double depth_Soil[], double sTemp[], double endTemp, unsigned int nlyrTemp, double depth_Temp[], double maxTempDepth, double sTempR[]){
	unsigned int i, j1=0, j2, toDebug = 0;
	double depth_Soil2[nlyrSoil + 1], sTemp2[nlyrSoil + 1];

	//transfer data to include bottom conditions; do not include surface temperature in interpolations
	for (i = 0; i < nlyrSoil; i++) {
		depth_Soil2[i] = depth_Soil[i];
		sTemp2[i] = sTemp[i];
	}
	depth_Soil2[nlyrSoil] = maxTempDepth;
	sTemp2[nlyrSoil] = endTemp;

	//interpolate soil temperature at soil temperature profile depths
	for (i = 0; i < nlyrTemp; i++) {
		while (LT(depth_Soil2[j1 + 1], depth_Temp[i]) && (j1 + 1) < nlyrSoil) {
			j1++;
		}
		j2 = j1 + 1;
		while (LE(depth_Soil2[j2 + 1], depth_Temp[i]) && (j2 + 1) < nlyrSoil + 1) {
			j2++;
		}

		sTempR[i + 1] = interpolation(depth_Soil2[j1], depth_Soil2[j2], sTemp2[j1], sTemp2[j2], depth_Temp[i]);

		if (toDebug)
			#ifndef RSOILWAT
				printf("\nConf T: i=%i, j1=%i, j2=%i, sTempR[%i]=%2.2f, sTemp2[%i]=%2.2f, sTemp2[%i]=%2.2f, depthT[%i]=%2.2f, depthS2[%i]=%2.2f, depthS2[%i]=%2.2f", i, j1, j2, i, sTempR[i], j1, sTemp2[j1], j2, sTemp2[j2], i, depth_Temp[i], j1, depth_Soil2[j1], j2, depth_Soil2[j2]);
			#else
				Rprintf("\nConf T: i=%i, j1=%i, j2=%i, sTempR[%i]=%2.2f, sTemp2[%i]=%2.2f, sTemp2[%i]=%2.2f, depthT[%i]=%2.2f, depthS2[%i]=%2.2f, depthS2[%i]=%2.2f", i, j1, j2, i, sTempR[i], j1, sTemp2[j1], j2, sTemp2[j2], i, depth_Temp[i], j1, depth_Soil2[j1], j2, depth_Soil2[j2]);
			#endif

	}
	sTempR[nlyrTemp + 1] = endTemp;

	if (toDebug)
		#ifndef RSOILWAT
			printf("\nConf T: sTempR[%i]=%2.2f, sTempR[%i]=%2.2f", i, sTempR[i], i+1, sTempR[i+1]);
		#else
			Rprintf("\nConf T: sTempR[%i]=%2.2f, sTempR[%i]=%2.2f", i, sTempR[i], i+1, sTempR[i+1]);
		#endif

}

void lyrSoil_to_lyrTemp(double cor[MAX_ST_RGR + 1][MAX_LAYERS + 1], unsigned int nlyrSoil, double width_Soil[], double var[], unsigned int nlyrTemp, double width_Temp, double res[]){
	unsigned int i, j = 0, toDebug = 0;
	double acc, ratio, sum;

	for (i = 0; i < nlyrTemp + 1; i++) {
		res[i] = 0.0;
		acc = 0.0;
		sum = 0.0;
		while (LT(acc, width_Temp) && j < nlyrSoil + 1) {
			if (GE(cor[i][j], 0.0)) { // there are soil layers to add
				ratio = cor[i][j] / width_Soil[j];
				res[i] += var[j] * ratio;
				sum += ratio;
				acc += cor[i][j];
				if (LT(acc, width_Temp)) j++;
			} else if (LT(cor[i][j], 0.0)) { // negative cor values indicate end of soil layer profile
				// copying values from deepest soil layer
				ratio = -cor[i][j] / width_Soil[j - 1];
				res[i] += var[j - 1] * ratio;
				sum += ratio;
				acc += (-cor[i][j]);
			}
		}
		res[i] = res[i] / sum;

		if (toDebug)
			#ifndef RSOILWAT
				printf("\nConf A: acc=%2.2f, sum=%2.2f, res[%i]=%2.2f, var[%i]=%2.2f, [%i]=%2.2f, cor[%i][%i]=%2.2f, width_Soil[%i]=%2.2f, [%i]=%2.2f", acc, sum, i, res[i], j, var[j], j-1, var[j-1], i, j, cor[i][j], j, width_Soil[j], j-1, width_Soil[j-1]);
			#else
				Rprintf("\nConf A: acc=%2.2f, sum=%2.2f, res[%i]=%2.2f, var[%i]=%2.2f, [%i]=%2.2f, cor[%i][%i]=%2.2f, width_Soil[%i]=%2.2f, [%i]=%2.2f", acc, sum, i, res[i], j, var[j], j-1, var[j-1], i, j, cor[i][j], j, width_Soil[j], j-1, width_Soil[j-1]);
			#endif


	}
}

/**
 * Determines the average temperature of the soil surface under snow. Based upon
 	 Parton et al. 1998. Equations 5 & 6.
 *
 *
 * @param airTempAvg the average air temperature of the area, in Celsius
 * @param snow the snow-water-equivalent of the area, in cm
 * @return the modified, average temperature of the soil surface
 */
double surface_temperature_under_snow(double airTempAvg, double snow){
  double kSnow; /** the effect of snow based on swe */
  double tSoilAvg; /** the average temeperature of the soil surface */
	/** Parton et al. 1998. Equation 6. */
  if (snow == 0){
    return 0.0;
  }
  else if (snow > 0 && airTempAvg >= 0){
    tSoilAvg = -2.0;
  }
  else if (snow > 0 && airTempAvg < 0){
    kSnow = fmax((-0.15 * snow + 1.0), 0.0); /** Parton et al. 1998. Equation 5. */
    tSoilAvg = 0.3 * airTempAvg * kSnow + -2.0;
  }
	return tSoilAvg;
}

void soil_temperature_init(double bDensity[], double width[], double surfaceTemp, double oldsTemp[], double meanAirTemp, unsigned int nlyrs, double fc[], double wp[], double deltaX, double theMaxDepth,unsigned int nRgr) {
	// local vars
	unsigned int x1 = 0, x2 = 0, j = 0, i, toDebug = 0;
	double d1 = 0.0, d2 = 0.0, acc = 0.0;
	double fc_vwc[nlyrs], wp_vwc[nlyrs];
	// pointers
	ST_RGR_VALUES *st = &stValues; // just for convenience, so I don't have to type as much

	soil_temp_init = 1; // make this value 1 to make sure that this function isn't called more than once... (b/c it doesn't need to be)


	if (toDebug)
		#ifndef RSOILWAT
			printf("\nInit soil layer profile: nlyrs=%i, surfaceTemp=%2.2f, meanAirTemp=%2.2F;\nSoil temperature profile: deltaX=%F, theMaxDepth=%F, nRgr=%i\n", nlyrs, surfaceTemp, meanAirTemp, deltaX, theMaxDepth, nRgr);
		#else
			Rprintf("\nInit soil layer profile: nlyrs=%i, surfaceTemp=%2.2f, meanAirTemp=%2.2F;\nSoil temperature profile: deltaX=%F, theMaxDepth=%F, nRgr=%i\n", nlyrs, surfaceTemp, meanAirTemp, deltaX, theMaxDepth, nRgr);
		#endif


	// init st
	for (i = 0; i < nRgr + 1; i++) {
		st->fcR[i] = 0.0;
		st->wpR[i] = 0.0;
		st->bDensityR[i] = 0.0;
		st->oldsTempR[i] = 0.0;
		for (j = 0; j < nlyrs + 1; j++) // last column is used for soil temperature layers that are deeper than the deepest soil profile layer
			st->tlyrs_by_slyrs[i][j] = 0.0;
	}

	// copy depths of soil layer profile
	for (j = 0; j < nlyrs + 1; j++) {
		acc += width[j];
		st->depths[j] = acc;
	}
	// calculate evenly spaced depths of soil temperature profile
	acc = 0.0;
	for (i = 0; i < nRgr + 1; i++) {
		acc += deltaX;
		st->depthsR[i] = acc;
	}


	// if soil temperature max depth is less than soil layer depth then quit
	if (LT(theMaxDepth, st->depths[nlyrs - 1])) {
		if (!soil_temp_error) { // if the error hasn't been reported yet... print an error to the stderr and one to the logfile

		#ifndef RSOILWAT
			printf("\nSOIL_TEMP FUNCTION ERROR: soil temperature max depth (%5.2f cm) must be more than soil layer depth (%5.2f cm)... soil temperature will NOT be calculated\n", theMaxDepth, st->depths[nlyrs - 1]);
		#else
			Rprintf("\nSOIL_TEMP FUNCTION ERROR: soil temperature max depth (%5.2f cm) must be more than soil layer depth (%5.2f cm)... soil temperature will NOT be calculated\n", theMaxDepth, st->depths[nlyrs - 1]);
		#endif

			soil_temp_error = 1;
		}
		return; // exits the function
	}

	// calculate values of correspondance 'tlyrs_by_slyrs' between soil profile layers and soil temperature layers
	for (i = 0; i < nRgr + 1; i++) {
		acc = 0.0; // cumulative sum towards deltaX
		while (x2 < nlyrs && acc < deltaX) { // there are soil layers to add
			// add from previous (x1) soil layer
			if (GT(d1, 0.0)) {
				j = x1;
				if (GT(d1, deltaX)) { // soil temperatur layer ends within x1-th soil layer
					d2 = deltaX;
					d1 -= deltaX;
				} else {
					d2 = d1;
					d1 = 0.0;
					x2++;
				}
			} else {
				// add from next (x2) soil layer
				j = x2;
				if (LT(st->depthsR[i], st->depths[x2])) { // soil temperatur layer ends within x2-th soil layer
					d2 = fmax(deltaX - acc, 0.0);
					d1 = width[x2] - d2;
				} else {
					d2 = width[x2];
					d1 = 0.0;
					x2++;
				}
			}
			acc += d2;
			st->tlyrs_by_slyrs[i][j] = d2;
		}
		x1 = x2;

		if (x2 >= nlyrs) { // soil temperature profile is deeper than deepest soil layer; copy data from deepest soil layer
			st->tlyrs_by_slyrs[i][x2] = -(deltaX - acc);
		}
	}

	if (toDebug) {
		for (i = 0; i < nRgr + 1; i++) {
			#ifndef RSOILWAT
				 printf("\ntl_by_sl");
					for (j = 0; j < nlyrs + 1; j++)
					   printf("[%i,%i]=%3.2f ", i, j, st->tlyrs_by_slyrs[i][j]);

			#else
				  Rprintf("\ntl_by_sl");
					for (j = 0; j < nlyrs + 1; j++)
					   Rprintf("[%i,%i]=%3.2f ", i, j, st->tlyrs_by_slyrs[i][j]);
			#endif


		}
	}

	// calculate volumetric field capacity, volumetric wilting point, bulk density, and initial soil temperature for layers of the soil temperature profile
	lyrSoil_to_lyrTemp(st->tlyrs_by_slyrs, nlyrs, width, bDensity, nRgr, deltaX, st->bDensityR);
	lyrSoil_to_lyrTemp_temperature(nlyrs, st->depths, oldsTemp, meanAirTemp, nRgr, st->depthsR, theMaxDepth, st->oldsTempR);
	// units of fc and wp are [cm H2O]; units of fcR and wpR are [m3/m3]
	for (i = 0; i < nlyrs; i++){
		fc_vwc[i] = fc[i] / width[i];
		wp_vwc[i] = wp[i] / width[i];
	}
	lyrSoil_to_lyrTemp(st->tlyrs_by_slyrs, nlyrs, width, fc_vwc, nRgr, deltaX, st->fcR);
	lyrSoil_to_lyrTemp(st->tlyrs_by_slyrs, nlyrs, width, wp_vwc, nRgr, deltaX, st->wpR);

	// st->oldsTempR: index 0 is surface temperature
	if (toDebug){

		#ifndef RSOILWAT
			 for (j = 0; j < nlyrs; j++)
						printf("\nConv Soil depth[%i]=%2.2f, fc=%2.2f, wp=%2.2f, bDens=%2.2f, oldT=%2.2f",
							j, st->depths[j], fc[j], wp[j], bDensity[j], oldsTemp[j]);
					printf("\nConv ST oldSurfaceTR=%2.2f", st->oldsTempR[0]);
					for (i = 0; i < nRgr + 1; i++)
					   printf("\nConv ST depth[%i]=%2.2f, fcR=%2.2f, wpR=%2.2f, bDensR=%2.2f, oldTR=%2.2f",
							i, st->depthsR[i], st->fcR[i], st->wpR[i], st->bDensityR[i], st->oldsTempR[i+1]);


		#else
			 for (j = 0; j < nlyrs; j++)
					Rprintf("\nConv Soil depth[%i]=%2.2f, fc=%2.2f, wp=%2.2f, bDens=%2.2f, oldT=%2.2f",
							j, st->depths[j], fc[j], wp[j], bDensity[j], oldsTemp[j]);
				Rprintf("\nConv ST oldSurfaceTR=%2.2f", st->oldsTempR[0]);
					for (i = 0; i < nRgr + 1; i++)
						Rprintf("\nConv ST depth[%i]=%2.2f, fcR=%2.2f, wpR=%2.2f, bDensR=%2.2f, oldTR=%2.2f",
							i, st->depthsR[i], st->fcR[i], st->wpR[i], st->bDensityR[i], st->oldsTempR[i+1]);

		#endif


		}
}

void set_frozen_unfrozen(unsigned int nlyrs, double sTemp[], double swc[], double swc_sat[], double width[]){
/*	01/06/2016 (drs)	function to determine if a soil layer is frozen or not

	Parton, W. J., M. Hartman, D. Ojima, and D. Schimel. 1998. DAYCENT and its land surface submodel: description and testing. Global and Planetary Change 19:35-48.
	A layer was considered frozen if its average soil temperature was below the freezing temperature (-1C), and theta(sat) - theta(cur) < 0.13,
	where theta(sat) was the saturated volumetric wetness of the layer and theta(cur) was the simulated volumetric wetness (Flerchinger and Saxton, 1989).
	The hydraulic conductivity of a frozen layer was reduced to 0.00001 cm/s.


*/

// 	TODO: freeze surfaceWater and restrict infiltration

	unsigned int i;
	ST_RGR_VALUES *st = &stValues;

	for (i = 0; i < nlyrs; i++){
		if (LE(sTemp[i], FREEZING_TEMP_C) && GT(swc[i], swc_sat[i] - width[i] * MIN_VWC_TO_FREEZE) ){
			st->lyrFrozen[i] = 1;
		} else {
			st->lyrFrozen[i] = 0;
		}
	}

}


unsigned int adjust_Tsoil_by_freezing_and_thawing(double oldsTemp[], double sTemp[], double shParam, unsigned int nlyrs, double vwc[], double bDensity[]){
// Calculate fusion pools based on soil profile layers, soil freezing/thawing, and if freezing/thawing not completed during one day, then adjust soil temperature
// based on Eitzinger, J., W. J. Parton, and M. Hartman. 2000. Improvement and Validation of A Daily Soil Temperature Submodel for Freezing/Thawing Periods. Soil Science 165:525-534.

// NOTE: THIS FUNCTION IS CURRENTLY NOT OPERATIONAL: DESCRIPTION BY EITZINGER ET AL. 2000 SEEMS INSUFFICIENT

	unsigned int i, sFadjusted_sTemp;
	double deltaTemp, Cis, sFusionPool[nlyrs], sFusionPool_actual[nlyrs];

	/* local variables explained:
	 toDebug - 1 to print out debug messages & then exit the program after completing the function, 0 to not.  default is 0.
	 deltaTemp - the change in temperature for each day
	 Cis - heat capacity of the i-th non-frozen soil layer (cal cm-3 K-1)
	 sFusionPool[] - the fusion pool for each soil layer
	 sFusionPool_actual[] - the actual fusion pool for each soil layer
	 sFadjusted_sTemp - if soil layer temperature was changed due to freezing/thawing
	 */

	ST_RGR_VALUES *st = &stValues;


	if (!fusion_pool_init) {
		for (i = 0; i < nlyrs; i++)
			st->oldsFusionPool_actual[i] = 0.;
		fusion_pool_init = 1;
	}

	sFadjusted_sTemp = 0;

/*
// THIS FUNCTION IS CURRENTLY NOT OPERATIONAL: DESCRIPTION BY EITZINGER ET AL. 2000 SEEMS INSUFFICIENT
	for (i = 0; i < nlyrs; i++){
		sFusionPool_actual[i] = 0.;

		// only need to do something if the soil temperature is at the freezing point, or the soil temperature is transitioning over the freezing point
		if (EQ(oldsTemp[i], FREEZING_TEMP_C) || (GT(oldsTemp[i], FREEZING_TEMP_C) && LT(sTemp[i], FREEZING_TEMP_C))|| (LT(oldsTemp[i], FREEZING_TEMP_C) && GT(sTemp[i], FREEZING_TEMP_C)) ){

			Cis = (vwc[i] + shParam * (1. - vwc[i])) * bDensity[i]; // Cis = sh * (bulk soil density): "Cis is the heat capacity of the i-th non-frozen soil layer (cal cm-3 K-1)" estimated based on Parton 1978 eq. 2.23 units(sh) = [cal g-1 C-1]; unit conversion factor = 'bulk soil density' with units [g/cm3]
			sFusionPool[i] = - FUSIONHEAT_H2O * vwc[i] / Cis * TCORRECTION; // Eitzinger et al. (2000): eq. 3 wherein sFusionPool[i] = Pi = the fusion energy pool of a soil layer given as a temperature equivalent (K), i.e., Pi = temperature change that must happen to freeze/thaw a soil layer

			// Calculate actual status of the fusion energy pool in [Celsius]
			// Eitzinger et al. (2000): eq. 6 wherein sFusionPool_actual[i] = Pai and sTemp[i] = T(sav-1)i + [delta]T(sav)i
			if( GT(oldsTemp[i], FREEZING_TEMP_C) && LE(sTemp[i], FREEZING_TEMP_C) ){ // Freezing?
				// soil above freezing yesterday; soil at or below freezing today
				sFusionPool_actual[i] = sTemp[i];
			} else {
				if( LT(st->oldsFusionPool_actual[i], FREEZING_TEMP_C) && (LE(sTemp[i], FREEZING_TEMP_C) || GT(sTemp[i], FREEZING_TEMP_C)) ){
					// Thawing?
					// actual fusion pool below freezing yesterday AND soil below or at freezing today
// TODO: I guess the first should be above (and not below freezing yesterday)?
					// actual fusion pool below freezing yesterday AND soil above freezing today
					deltaTemp = sTemp[i] - oldsTemp[i]; // deltaTemp = [delta]T(sav)i; oldsTemp[i] = T(sav-1)i
					sFusionPool_actual[i] = st->oldsFusionPool_actual[i] + deltaTemp;
				} else {
// TODO: What if not? This situation is not covered by Eitzinger et al. 2000
				}
			}

			// Eitzinger et al. (2000): eq. 4
			if( LT(sFusionPool_actual[i], 0.) && LT(sFusionPool[i], sFusionPool_actual[i]) ){
// TODO: No condition for thawing considered?
// TODO: If partial thawing/freezing, shouldn't sTemp[i] bet set to FREEZING_TEMP_C?
				// If the freezing process of a relevant soil layer is not finished within a day, it is assumed that no change in the daily average soil layer temperature (Eq. (4)) [...] can occur.
				// It implies that the state of the soil layer (frozen, partly frozen, or unfrozen) is not changed by the diurnal soil temperature change within the daily time step.
				sTemp[i] = oldsTemp[i];
				sFadjusted_sTemp = 1;
			}
		}

		// updating the values of yesterdays fusion pools for the next time the function is called...
		st->oldsFusionPool_actual[i] = sFusionPool_actual[i];

	}
*/
	return sFadjusted_sTemp;
}

void endCalculations()
{
	soil_temp_error = 1;
	// return;  //Exits the Function
}

/**********************************************************************
 PURPOSE: Calculate soil temperature for each layer as described in Parton 1978, ch. 2.2.2 Temperature-profile Submodel, interpolation values are gotten from a mixture of interpolation & extrapolation
// soil freezing based on Eitzinger, J., W. J. Parton, and M. Hartman. 2000. Improvement and Validation of A Daily Soil Temperature Submodel for Freezing/Thawing Periods. Soil Science 165:525-534.

 *NOTE* There will be some degree of error because the original equation is written for soil layers of 15 cm.  if soil layers aren't all 15 cm then linear regressions are used to estimate the values (error should be relatively small though).
 *NOTE* Function might not work correctly if the maxDepth of the soil is > 180 cm, since Parton's equation goes only to 180 cm
 *NOTE* Function will run if maxLyrDepth > maxDepth of the equation, but the results might be slightly off...

 HISTORY:
 05/24/2012 (DLM) initial coding, still need to add to header file, handle if the layer height is > 15 cm properly, & test
 05/25/2012 (DLM) added all this 'fun' crazy linear interpolation stuff
 05/29/2012 (DLM) still working on the function, linear interpolation stuff should work now.  needs testing
 05/30/2012 (DLM) got rid of nasty segmentation fault error, also tested math seems correct after checking by hand.  added the ability to change the value of deltaX
 05/30/2012 (DLM) added # of lyrs check & maxdepth check at the beginning to make sure code doesn't blow up... if there isn't enough lyrs (ie < 2) or the maxdepth is too little (ie < deltaX * 2), the function quits out and reports an error to the user
 05/31/2012 (DLM) added theMaxDepth variable to allow the changing of the maxdepth of the equation, also now stores most interpolation data in a structure to reduce redundant interpolation calculations & speeds things up
 06/01/2012 (DLM) changed deltaT variable from hours to seconds, also changed some of the interpolation calculations so that swc, fc, & wp regressions are scaled properly... results are actually starting to look usable!
 06/13/2012 (DLM) no longer extrapolating values for interpolation layers that are out of the bounds of the soil layers... instead they are now set to the last soil layers values.  extrapolating code is still in the function and can be commented out and used if wishing to go back to extrapolating the values...
 03/28/2013 (clk) added a check to see if the soil was freezing/thawing and adjusted the soil temperature correctly during this phase change. If the temperature was in this area, also needed to re run soil_temperature_init on next call because you need to also change the regression temperatures to match the change in soil temperature.
 12/23/2014 (drs) re-wrote soil temperature functions:
 					- soil characteristics (fc, wp, bulk density) of soil temperature now integrated across all soil layers (instead of linear interpolation between the two extreme layers;
					- interpolation for top soil layer with surface temperature only if no other information available
					- mean of interpolations if multiple layers affect a soil profile layer
					- reporting of surface soil temperature
					- test iteration steps that they meet the criterion of a stable solution
 11/23/2015 (drs) added call to surface_temperature_under_snow()
 12/24/2015 (drs) fixed code following realization that equations in Parton 1978 were using units of volumetric soil water [cm3/cm3] instead of soil water content per layer [cm] - thanks to a comparison with the soil temperature formulations in R by Caitlin Andrews and Matt Petrie
 01/04/2016 (drs) moved code for freezing/thawing of soil layers to function adjust_Tsoil_by_freezing_and_thawing()

 INPUTS:
 airTemp - the average daily air temperature in celsius
 pet - the potential evapotranspiration rate
 aet - the actual evapotranspiration rate
 biomass - the standing-crop biomass
 swc - soil water content
 bDensity - bulk density of the soil layers
 width - width of layers
 oldsTemp - soil layer temperatures from the previous day in celsius
 nlyrs - number of soil layers, must be greater than 1 or the function won't work right
 fc - field capacity for each soil layer
 wp - wilting point for each soil layer
 bmLimiter - biomass limiter constant (300 g/m^2)
 t1Params - constants for the avg temp at the top of soil equation (15, -4, 600) there is 3 of them
 csParams - constants for the soil thermal conductivity equation (0.00070, 0.00030) there is 2 of them
 shParam - constant for the specific heat capacity equation (0.18)
 sh -  specific heat capacity equation
 snowdepth - the depth of snow cover (cm)
 meanAirTemp - the avg air temperature for the month in celsius
 deltaX - the distance between profile points (default is 15 from Parton's equation, wouldn't recommend changing the value from that).  180 must be evenly divisible by this number.
 theMaxDepth - the lower bound of the equation (default is 180 from Parton's equation, wouldn't recommend changing the value from that).
 nRgr - the number of regressions (1 extra value is needed for the sTempR and oldsTempR for the last layer
 sFadjusted_sTemp - if soil layer temperature was changed due to freezing/thawing; if so, then temperature of the soil temperature layers need to updated as well

 OUTPUT:
 sTemp - soil layer temperatures in celsius
 **********************************************************************/

void soil_temperature(double airTemp, double pet, double aet, double biomass, double swc[], double swc_sat[], double bDensity[], double width[], double oldsTemp[], double sTemp[], double surfaceTemp[2],
		unsigned int nlyrs, double fc[], double wp[], double bmLimiter, double t1Param1, double t1Param2, double t1Param3, double csParam1, double csParam2, double shParam,
		double snowdepth, double meanAirTemp, double deltaX, double theMaxDepth, unsigned int nRgr, double snow) {

	unsigned int i, k, toDebug = 0, sFadjusted_sTemp;
	double T1, cs, sh, pe, parts, part1, part2, vwc[nlyrs], vwcR[nRgr], sTempR[nRgr + 1];

	for (i = 0; i < nlyrs; i++)
		vwc[i] = swc[i] / width[i];

	/* local variables explained:
	 toDebug - 1 to print out debug messages & then exit the program after completing the function, 0 to not.  default is 0.
	 T1 - the average daily temperature at the top of the soil in celsius
	 vwc - volumetric soil-water content
	 pe - ratio of the difference between volumetric soil-water content & soil-water content
	 at the wilting point to the difference between soil water content at field capacity &
	 soil-water content at wilting point.
	 cs - soil thermal conductivity
	 sh - specific heat capacity
	 depths[nlyrs] - the depths of each layer of soil, calculated in the function
	 vwcR[], sTempR[] - anything with a R at the end of the variable name stands for the interpolation of that array
	 */
	if (toDebug){
		#ifndef RSOILWAT
			printf("\n\nNew call to soil_temperature()");
		#else
			Rprintf("\n\nNew call to soil_temperature()");
		#endif
	}


	if (!soil_temp_init) {
		if (toDebug) {
			#ifndef RSOILWAT
				printf("\nCalling soil_temperature_init\n");
			#else
				Rprintf("\nCalling soil_temperature_init\n");
			#endif
		}

		surfaceTemp[Today] = airTemp;
		set_frozen_unfrozen(nlyrs, oldsTemp, swc, swc_sat, width);
		soil_temperature_init(bDensity, width, surfaceTemp[Today], oldsTemp, meanAirTemp, nlyrs, fc, wp, deltaX, theMaxDepth, nRgr);
	}

	// if (soil_temp_error) // if there is an error found in the soil_temperature_init function, return so that the function doesn't blow up later
	// 	return;


	ST_RGR_VALUES *st = &stValues; // just for convenience, so I don't have to type as much

	// calculating T1, the average daily soil surface temperature
	if(GT(snowdepth, 0.0)) {
		T1 = surface_temperature_under_snow(airTemp, snow);
		if(toDebug) printf("\nThere is snow on the ground, T1=%5.4f calculated using new equation from Parton 1998\n", T1);
	} else {
		if (LE(biomass, bmLimiter)) { // bmLimiter = 300
			T1 = airTemp + (t1Param1 * pet * (1. - (aet / pet)) * (1. - (biomass / bmLimiter))); // t1Param1 = 15; drs (Dec 16, 2014): this interpretation of Parton 1978's 2.20 equation (the printed version misses a closing parenthesis) removes a jump of T1 for biomass = bmLimiter
			if (toDebug) {
				#ifndef RSOILWAT
					printf("\nT1 = %5.4f = %5.4f + (%5.4f * %5.4f * (1 - (%5.4f / %5.4f)) * (1 - (%5.4f / %5.4f)) ) )",
						   airTemp, T1, t1Param1, pet, aet, pet, biomass, bmLimiter);
				#else
					Rprintf("\nT1 = %5.4f = %5.4f + (%5.4f * %5.4f * (1 - (%5.4f / %5.4f)) * (1 - (%5.4f / %5.4f)) ) )",
							airTemp, T1, t1Param1, pet, aet, pet, biomass, bmLimiter);
				#endif
			}
		} else {
			T1 = airTemp + ((t1Param2 * (biomass - bmLimiter)) / t1Param3); // t1Param2 = -4, t1Param3 = 600; math is correct
			if (toDebug){
				#ifndef RSOILWAT
					printf("\nT1 = %5.4f = %5.4f + ((%5.4f * (%5.4f - %5.4f)) / %5.4f)", airTemp, T1, t1Param2, biomass, bmLimiter, t1Param3);
				#else
					Rprintf("\nT1 = %5.4f = %5.4f + ((%5.4f * (%5.4f - %5.4f)) / %5.4f)", airTemp, T1, t1Param2, biomass, bmLimiter, t1Param3);
				#endif
			}
		}
	}


	// calculate volumetric soil water content for soil temperature layers
	lyrSoil_to_lyrTemp(st->tlyrs_by_slyrs, nlyrs, width, vwc, nRgr, deltaX, vwcR);

	if (toDebug) {
		#ifndef RSOILWAT
			printf("\nregression values:");
				for (i = 0; i < nRgr; i++)
					printf("\nk %2d width %5.4f depth %5.4f vwcR %5.4f fcR %5.4f wpR %5.4f oldsTempR %5.4f bDensityR %5.4f", i, deltaX, st->depthsR[i], vwcR[i], st->fcR[i], st->wpR[i], st->oldsTempR[i], st->bDensityR[i]);

			printf("\nlayer values:");
				for (i = 0; i < nlyrs; i++)
					printf("\ni %2d width %5.4f depth %5.4f vwc %5.4f fc %5.4f wp %5.4f oldsTemp %5.4f bDensity %5.4f", i, width[i], st->depths[i], vwc[i], fc[i], wp[i], oldsTemp[i], bDensity[i]);
		#else
			Rprintf("\nregression values:");
				for (i = 0; i < nRgr; i++)
					Rprintf("\nk %2d width %5.4f depth %5.4f vwcR %5.4f fcR %5.4f wpR %5.4f oldsTempR %5.4f bDensityR %5.4f", i, deltaX, st->depthsR[i], vwcR[i], st->fcR[i], st->wpR[i], st->oldsTempR[i], st->bDensityR[i]);

			Rprintf("\nlayer values:");
				for (i = 0; i < nlyrs; i++)
					Rprintf("\ni %2d width %5.4f depth %5.4f vwc %5.4f fc %5.4f wp %5.4f oldsTemp %5.4f bDensity %5.4f", i, width[i], st->depths[i], vwc[i], fc[i], wp[i], oldsTemp[i], bDensity[i]);
		#endif
	}

	// calculate the new soil temperature for each layer

	sTempR[0] = T1; //index 0 indicates surface and not first layer
	part1 = SEC_PER_DAY / squared(deltaX);
	for (i = 1; i < nRgr + 1; i++) { // goes to nRgr, because the soil temp of the last interpolation layer (nRgr) is the meanAirTemp
		k = i - 1;
		pe = (vwcR[k] - st->wpR[k]) / (st->fcR[k] - st->wpR[k]); // the units are volumetric!
		cs = csParam1 + (pe * csParam2); // Parton (1978) eq. 2.22: soil thermal conductivity; csParam1 = 0.0007, csParam2 = 0.0003
		sh = vwcR[k] + shParam * (1. - vwcR[k]); // Parton (1978) eq. 2.22: specific heat capacity; shParam = 0.18
			// TODO: adjust thermal conductivity and heat capacity if layer is frozen
		parts = part1 * cs / (sh * st->bDensityR[k]);

		part2 = sTempR[i - 1] - 2 * st->oldsTempR[i] + st->oldsTempR[i + 1];

		/*Parton, W. J. 1984. Predicting Soil Temperatures in A Shortgrass Steppe. Soil Science 138:93-101.
		VWCnew: why 0.5 and not 1? and they use a fixed alpha * K whereas here it is 1/(cs * sh)*/
		if (GT(parts, 1.0)){
			#ifndef RSOILWAT
				printf("\n SOILWAT has encountered an ERROR: Parts Exceeds 1.0 and May Produce Extreme Values");
				soil_temp_error = 1;
			#else
				Rprintf("\n SOILWAT has encountered an ERROR: Parts Exceeds 1.0 and May Produce Extreme Values");
				soil_temp_error = 1;
			#endif
			// return;  //Exits the Function
		}


		sTempR[i] = st->oldsTempR[i] + parts * part2; // Parton (1978) eq. 2.21

		if (toDebug) {
			#ifndef RSOILWAT
				printf("\nk %d cs %5.4f sh %5.4f p1 %5.4f ps %5.4f p2 %5.4f p %5.4f", k, cs, sh, part1, parts, part2, parts * part2);
			#else
				Rprintf("\nk %d cs %5.4f sh %5.4f p1 %5.4f ps %5.4f p2 %5.4f p %5.4f", k, cs, sh, part1, parts, part2, parts * part2);
			#endif
		}
	}

	sTempR[nRgr + 1] = meanAirTemp; // again... the last layer of the interpolation is set to the constant meanAirTemp

	if (toDebug) {
		#ifndef RSOILWAT
			printf("\nSoil temperature profile values:");
			for (i = 0; i <= nRgr + 1; i++)
				printf("\nk %d oldsTempR %5.4f sTempR %5.4f depth %5.4f", i, st->oldsTempR[i], sTempR[i], (i * deltaX)); // *(oldsTempR + i) is equivalent to writing oldsTempR[i]
		#else
			Rprintf("\nSoil temperature profile values:");
			for (i = 0; i <= nRgr + 1; i++)
				Rprintf("\nk %d oldsTempR %5.4f sTempR %5.4f depth %5.4f", i, st->oldsTempR[i], sTempR[i], (i * deltaX)); // *(oldsTempR + i) is equivalent to writing oldsTempR[i]
		#endif
	}


	// convert soil temperature of soil temperature profile 'sTempR' to soil profile layers 'sTemp'
	surfaceTemp[Yesterday] = surfaceTemp[Today];
	surfaceTemp[Today] = T1;
	lyrTemp_to_lyrSoil_temperature(st->tlyrs_by_slyrs, nRgr, st->depthsR, sTempR, nlyrs, st->depths, width, sTemp);


	// Calculate fusion pools based on soil profile layers, soil freezing/thawing, and if freezing/thawing not completed during one day, then adjust soil temperature
	sFadjusted_sTemp = adjust_Tsoil_by_freezing_and_thawing(oldsTemp, sTemp, shParam, nlyrs, vwc, bDensity);

	// update sTempR if sTemp were changed due to soil freezing/thawing
	if(sFadjusted_sTemp){
		lyrSoil_to_lyrTemp_temperature(nlyrs, st->depths, sTemp, meanAirTemp, nRgr, st->depthsR, theMaxDepth, sTempR);
	}

	// determine frozen/unfrozen status of soil layers
	set_frozen_unfrozen(nlyrs, sTemp, swc, swc_sat, width);


	if (toDebug) {
		#ifndef RSOILWAT
			printf("\nsTemp %5.4f surface; soil temperature adjusted by freeze/thaw: %i", surfaceTemp[Today], sFadjusted_sTemp);

			printf("\nSoil temperature profile values:");
			for (i = 0; i <= nRgr + 1; i++)
				printf("\nk %d oldsTempR %5.4f sTempR %5.4f depth %5.4f", i, st->oldsTempR[i], sTempR[i], (i * deltaX)); // *(oldsTempR + i) is equivalent to writing oldsTempR[i]

			printf("\nSoil profile layer temperatures:");
			for (i = 0; i < nlyrs; i++)
				printf("\ni %d oldTemp %5.4f sTemp %5.4f depth %5.4f frozen %d", i, oldsTemp[i], sTemp[i], st->depths[i], st->lyrFrozen[i]);
		#else
			Rprintf("\nsTemp %5.4f surface; soil temperature adjusted by freeze/thaw: %i", surfaceTemp[Today], sFadjusted_sTemp);

			Rprintf("\nSoil temperature profile values:");
			for (i = 0; i <= nRgr + 1; i++)
				Rprintf("\nk %d oldsTempR %5.4f sTempR %5.4f depth %5.4f", i, st->oldsTempR[i], sTempR[i], (i * deltaX)); // *(oldsTempR + i) is equivalent to writing oldsTempR[i]

			Rprintf("\nSoil profile layer temperatures:");
			for (i = 0; i < nlyrs; i++)
				Rprintf("\ni %d oldTemp %5.4f sTemp %5.4f depth %5.4f frozen %d", i, oldsTemp[i], sTemp[i], st->depths[i], st->lyrFrozen[i]);
		#endif
	}

	// updating the values of yesterdays temperature for the next time the function is called...
	for (i = 0; i <= nRgr + 1; i++){
		st->oldsTempR[i] = sTempR[i];
	}

	if (toDebug) {
		#ifndef RSOILWAT
			exit(0); // terminates the program, make sure to take this out later
		#else
			Rprintf("\nEXIT DEBUG IS ON IN SOIL TEMPERATURE. CONSIDER TURNING OFF.\n");
			//exit(0); // terminates the program, make sure to take this out later
			error("@ SW_Flow_lib.c function soil_temperature");
		#endif
	}
}
