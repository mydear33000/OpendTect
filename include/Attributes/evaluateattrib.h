#ifndef evaluateattrib_h
#define evaluateattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          19-10-2005
 RCS:           $Id: evaluateattrib.h,v 1.6 2009-01-06 10:29:52 cvsranojay Exp $
________________________________________________________________________

-*/


#include "attribprovider.h"


/*!\brief Evaluate Attribute

Attribute built for convenience purpose; only there to allow the computation of
a set of attributes in one go;

*/
    

namespace Attrib
{

mClass Evaluate: public Provider
{
public:
    static void			initClass();
				Evaluate(Desc&);

    static const char*		attribName()		{ return "Evaluate"; }

protected:
    				~Evaluate() {}
    static Provider*		createInstance(Desc&);

    bool			allowParallelComputation() const
    				{ return true; }

    bool			getInputOutput(int inp,TypeSet<int>& res) const;
    bool			getInputData(const BinID&,int zintv);
    bool			computeData(const DataHolder&,
	    				    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const;

    TypeSet<int>		dataidx_;
    ObjectSet<const DataHolder>	inputdata_;
};

}; // namespace Attrib


#endif
