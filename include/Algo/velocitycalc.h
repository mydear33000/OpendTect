#ifndef velocitycalc_h
#define velocitycalc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		Dec 2007
 RCS:		$Id: velocitycalc.h,v 1.7 2009-01-20 04:38:45 cvsranojay Exp $
________________________________________________________________________

-*/


#include "samplingdata.h"
#include "veldesc.h"

template <class T> class ValueSeries;


/*!Converts between time and depth given a velocity model. The velocity model
   can be either RMO-velocities in time, or interval velocity in either depth or
   time. */

mClass TimeDepthConverter
{
public:
    			TimeDepthConverter();
			~TimeDepthConverter();

    bool		isOK() const;

    bool		setVelocityModel(const ValueSeries<float>&, int sz,
	    				 const SamplingData<double>&,
					 const VelocityDesc&,bool istime);

    bool		calcDepths(ValueSeries<float>&, int sz,
	    			   const SamplingData<double>& timesamp) const;
    bool		calcTimes(ValueSeries<float>&, int sz,
	    			   const SamplingData<double>& depthsamp) const;

    static bool		calcDepths(const ValueSeries<float>& vels, int velsz,
	    			   const SamplingData<double>&,
				   VelocityDesc::SampleSpan, float* depths );
    static bool		calcTimes(const ValueSeries<float>& vels, int velsz,
	    			   const SamplingData<double>&,
				   VelocityDesc::SampleSpan, float* depths );
protected:

    float			firstvel_;
    float			lastvel_;
    float*			depths_;
    float*			times_;
    int				sz_;
    SamplingData<double>	sd_;
};


/*! Computes moveout with anisotropy, according to the equation
by Alkhalifah and Tsvankin 1995. */


bool computeMoveout( float t0, float Vrms, float effectiveanisotropy,
		     int nroffsets, const float* offsets, float* res );


/*!Converts a series of Vrms to Vint. Vrms may contain undefined values, as
   long as at least one is define. */

bool computeDix(const float* Vrms,const SamplingData<double>& sd,int nrvels,
		VelocityDesc::SampleSpan,float* Vint);

/*!Converts a number of layers with Vrms to interval velocities.
   Note that the times in t refers to the bottom of each layer, and t0
   has the start time of the top layer. */

bool computeDix(const float* Vrms, float t0, const float* t, int nrlayers,
		float* Vint);


bool computeVrms(const float* Vint,const SamplingData<double>& sd, int nrvels,
		 VelocityDesc::SampleSpan,float* Vrms);

/*!Converts a number of layers with Vint to rms velocities.
   Note that the times in t refers to the bottom of each layer, and t0
   has the start time of the top layer. */

bool computeVrms(const float* Vint,float t0,const float* t,int nrlayers,
		 float* Vrms);

/*!Given an irregularly sampled Vrms, create a regularly sampled one. The
   function assumes constant interval velocity before and after the input
   interval.*/

bool sampleVrms(const float* Vin,float t0_in,const float* t_in,int nr_in,
		const SamplingData<double>& sd_out,float* Vout, int nr_out);
	        
	        
#endif
