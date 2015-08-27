#ifndef segyuiscandata_h
#define segyuiscandata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
 RCS:           $Id: $
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "ranges.h"
#include "bufstring.h"
#include "datachar.h"

class DataClipSampler;
class uiParent;


namespace SEGY
{

class TrcHeader;
class TrcHeaderDef;


/*!\brief Basic data from a SEG-Y fle */

mExpClass(uiSEGY) BasicFileInfo
{
public:

			BasicFileInfo()			{ init(); }
    void		init();

    int			revision_;
    int			ns_;
    short		format_;
    SamplingData<float>	sampling_;
    bool		hdrsswapped_;
    bool		dataswapped_;

    bool		isValid() const			{ return ns_ > 0; }
    bool		isRev0() const			{ return revision_ < 1;}
    int			bytesPerSample() const;
    int			traceDataBytes() const;
    DataCharacteristics	getDataChar() const;

    int			nrTracesIn(const od_istream&,od_stream_Pos p=-1) const;
    void		goToTrace(od_istream&,od_stream_Pos,int) const;

};


/*!\brief definition for SEG-Y loading */

mExpClass(uiSEGY) LoadDef : public BasicFileInfo
{
public:

			LoadDef();
			~LoadDef();
    void		reInit(bool alsohdef);

    float		coordscale_;

    TrcHeaderDef*	hdrdef_;

    TrcHeader*		getTrcHdr(od_istream&) const;
    bool		getData(od_istream&,char*,float* vals=0) const;
    TrcHeader*		getTrace(od_istream&,char*,float*) const;

};


/*!\brief info collected by scanning SEG-Y file */

mExpClass(uiSEGY) ScanInfo
{
public:

			ScanInfo(const char* fnm);

    BufferString	filenm_;
    bool		usable_;
    bool		fullscan_;
    bool		isEmpty() const		{ return nrtrcs_ < 1; }
    bool		isUsable() const	{ return usable_ && !isEmpty();}

    int			nrtrcs_;
    Interval<double>	xrg_;
    Interval<double>	yrg_;
    Interval<float>	offsrg_;
    Interval<int>	inls_;
    Interval<int>	crls_;
    Interval<int>	trcnrs_;
    Interval<float>	refnrs_;
    bool		infeet_;

    BasicFileInfo	basicinfo_;

    void		getFromSEGYBody(od_istream&,const LoadDef&,
				    bool isfirst,bool is2d,DataClipSampler&,
				    bool full,uiParent*);
    void		merge(const ScanInfo&);

    void		reInit();

protected:

    bool		addTrace(od_istream&,bool,char*,float*,const LoadDef&,
				 DataClipSampler&);
    void		addValues(DataClipSampler&,const float*, int);

    friend class	FullUIScanner;

};

} // namespace SEGY


#endif
