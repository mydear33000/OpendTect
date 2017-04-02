#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2008
________________________________________________________________________

-*/

#include "segyfiledef.h"
#include "executor.h"
#include "bufstringset.h"
#include "seisstatscollector.h"
#include "uistring.h"
class SeisTrc;
class DataClipSampler;
class SEGYSeisTrcTranslator;
namespace PosInfo { class Detector; }


namespace SEGY
{
class FileDataSet;

/*!\brief Scans SEG-Y file(s). For reports, you'd want to set rich info. */

mExpClass(Seis) Scanner : public Executor
{ mODTextTranslationClass(Scanner);
public:

			Scanner(const IOPar&,Seis::GeomType);
			Scanner(const FileSpec&,Seis::GeomType,const IOPar&);
			~Scanner();

    Seis::GeomType	geomType() const	{ return geom_; }
    const IOPar&	pars() const		{ return pars_; }
    void		setMaxNrtraces( int n )	{ nrtrcs_ = n; }
    void		setForceRev0( bool yn )	{ forcerev0_ = yn; }
    void		setRichInfo( bool yn )	{ richinfo_ = yn; }
    void		collectInfoPerTrace( bool yn )	{ notrcinfo_ = !yn; }
    uiRetVal		fillStats(IOPar&) const;

    int			nextStep();
    uiString		message() const	{ return msg_; }
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const;
    uiString		nrDoneText() const	{ return tr("Traces scanned"); }

    const FileDataSet&	fileDataSet() const	{ return fds_; }
    FileDataSet&	fileDataSet()		{ return fds_; }

    BufferStringSet	fnms_;		//!< Actually used, possibly with errs
    BufferStringSet	failedfnms_;	//!< Failed to open or read
    uiStringSet	failerrmsgs_;	//!< Err Msgs for failed
    BufferStringSet	scanerrfnms_;	//!< Error during scan (but in fnms_)
    uiStringSet	scanerrmsgs_;	//!< Err Msgs for 'Error during scan'

    void			getReport(IOPar&,const IOPar* inppars=0) const;
    StepInterval<float>		zRange() const;
    const PosInfo::Detector&	posInfoDetector() const { return dtctor_; }
    const FileSpec&		fileSpec() const	{ return filespec_; }

    const BufferStringSet&	warnings() const	{ return trwarns_; }
    const SEGYSeisTrcTranslator* translator() const	{ return tr_; }

protected:

    Seis::GeomType	geom_;
    const IOPar&	pars_;
    FileDataSet&	fds_;
    FileSpec		filespec_;
    SEGYSeisTrcTranslator* tr_;
    SeisStatsCollector	seisstatscollector_;
    DataClipSampler&	clipsmplr_;
    int			nrtrcs_;
    bool		forcerev0_;
    bool		richinfo_;

    SeisTrc&		trc_;
    int			curfidx_;
    uiString		msg_;
    od_int64		nrdone_;
    mutable od_int64	totnr_;
    PosInfo::Detector&	dtctor_;
    BufferStringSet	trwarns_;
    bool		notrcinfo_;

    int			openNext();
    int			readNext();
    void		closeTr();

    void		init();
    int			finish(bool);
    void		addFailed(const uiString&);
    void		initFileData();
    void		addErrReport(IOPar&) const;
    uiString Openff()	{ return tr("Opening first file"); }
};

} // namespace
