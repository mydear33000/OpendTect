#ifndef eventfreqattrib_h
#define eventfreqattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Jul 2007
 RCS:           $Id: eventfreqattrib.h,v 1.3 2007-08-10 11:57:19 cvsbert Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "valseries.h"

/*!\brief Computes a simple frequency from distance between events */

namespace Attrib
{

class EventFreq : public Provider
{
public:

    static void		initClass();
			EventFreq(Desc&);

    static const char*	attribName()	   { return "EventFreq"; }

protected:

    static Provider*	createInstance(Desc&);

    bool		allowParallelComputation() const	{ return true; }
    bool		getInputData(const BinID&,int);
    bool		computeData(const DataHolder&,const BinID&,
	    			    int,int,int) const;

    const DataHolder*	inpdata_;
    ValueSeries<float>*	inpseries_;
    Interval<int>	dessamps_;
    Interval<int>	cubeintv_;

    mutable TypeSet<float> evposns_;
    mutable bool	firstevmax_;

    void		findEvents(int,int) const;
    void		fillFreqOutput(const DataHolder&,int,int) const;
    void		fillPhaseOutput(const DataHolder&,int,int) const;

    const Interval<int>* desZSampMargin(int,int) const;
    float		getPDz(float*,int) const;

};

} // namespace Attrib


#endif
