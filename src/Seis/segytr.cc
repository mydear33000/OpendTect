/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-3-1996
 * FUNCTION : Seis trace translator
-*/

static const char* rcsID = "$Id: segytr.cc,v 1.72 2008-11-13 11:33:21 cvsbert Exp $";

#include "segytr.h"
#include "seistrc.h"
#include "segyhdr.h"
#include "segyfiledef.h"
#include "datainterp.h"
#include "conn.h"
#include "errh.h"
#include "iopar.h"
#include "iostrm.h"
#include "timefun.h"
#include "scaler.h"
#include "survinfo.h"
#include "seisselection.h"
#include "envvars.h"
#include "separstr.h"
#include "keystrs.h"
#include "settings.h"
#include <math.h>
#include <ctype.h>

#define mBPS(cd) (int)cd->datachar.nrBytes()

static const int cSEGYWarnBadFmt = 1;
static const int cSEGYWarnPos = 2;
static const int cSEGYWarnZeroSampIntv = 3;
static const int cSEGYWarnDataReadIncomplete = 4;


SEGYSeisTrcTranslator::SEGYSeisTrcTranslator( const char* nm, const char* unm )
	: SeisTrcTranslator(nm,unm)
	, trchead_(*new SEGY::TrcHeader(headerbuf,true,fileopts_.thdef_))
	, txthead_(0)
	, binhead_(*new SEGY::BinHeader)
	, trcscale_(0)
	, curtrcscale_(0)
	, forcerev0(false)
	, storinterp(0)
	, blockbuf(0)
	, headerbufread(false)
	, headerdone(false)
	, userawdata(false)
	, useinpsd(false)
	, inpcd(0)
	, outcd(0)
	, curoffs(-1)
	, prevoffs(0)
	, offsdef(0,1)
	, iotype(StreamConn::File)
{
    curtrcnr = prevtrcnr = -1;
    mSetUdf(curbinid.inl); mSetUdf(prevbinid.inl);
}


SEGYSeisTrcTranslator::~SEGYSeisTrcTranslator()
{
    SeisTrcTranslator::cleanUp();

    delete storinterp; storinterp = 0;
    delete [] blockbuf; blockbuf = 0;
    headerbufread = headerdone = false;

    delete txthead_;
    delete &binhead_;
    delete &trchead_;
}


int SEGYSeisTrcTranslator::dataBytes() const
{
    return SEGY::BinHeader::formatBytes(
	    	filepars_.fmt_ > 0 ? filepars_.fmt_ : 1 );
}


#define mErrRet(s) { fillErrMsg(s,false); return false; }
#define mPosErrRet(s) { fillErrMsg(s,true); return false; }


bool SEGYSeisTrcTranslator::readTapeHeader()
{
    if ( !txthead_ )
	txthead_ = new SEGY::TxtHeader;
    if ( !sConn().doIO(txthead_->txt_,SegyTxtHeaderLength) )
	mErrRet( "Cannot read SEG-Y Text header" )
    txthead_->setAscii();

    binhead_.needswap = filepars_.byteswap_ > 1;
    unsigned char binheaderbuf[400];
    if ( !sConn().doIO( binheaderbuf, SegyBinHeaderLength ) )
	mErrRet( "Cannot read SEG-Y Text header" )
    binhead_.getFrom( binheaderbuf );

    trchead_.setNeedSwap( filepars_.byteswap_ > 1 );
    trchead_.isrev1 = forcerev0 ? false : binhead_.isrev1;
    if ( trchead_.isrev1 )
    {
	if ( binhead_.nrstzs > 100 ) // protect against wild values
	    binhead_.nrstzs = 0;
	for ( int idx=0; idx<binhead_.nrstzs; idx++ )
	{
	    if ( !sConn().doIO(txthead_->txt_,SegyTxtHeaderLength) )
		mErrRet( "No traces found in the SEG-Y file" )
	}
    }

    if ( filepars_.fmt_ == 0 )
    {
	filepars_.fmt_ = binhead_.format;
	if ( filepars_.fmt_ == 4 && read_mode != Seis::PreScan )
	    mErrRet( "SEG-Y format '4' (fixed point/gain code) not supported" )
	else if ( filepars_.fmt_<1 || filepars_.fmt_>8
		|| filepars_.fmt_==6 || filepars_.fmt_==7 )
	{
	    BufferString nrstr = filepars_.fmt_;
	    addWarn( cSEGYWarnBadFmt, nrstr );
	    filepars_.fmt_ = 1;
	}
    }

    txthead_->getText( pinfo.usrinfo );
    pinfo.nr = binhead_.lino;
    pinfo.zrg.step = binhead_.hdt * (0.001 / SI().zFactor());
    insd.step = binhead_dpos_ = pinfo.zrg.step;
    innrsamples = binhead_ns_ = binhead_.hns;
    return true;
}


void SEGYSeisTrcTranslator::addWarn( int nr, const char* detail )
{
    if ( warnnrs_.indexOf(nr) >= 0 ) return;

    BufferString msg;
    if ( nr == cSEGYWarnBadFmt )
    {
	msg = "SEG-Y format '"; msg += detail;
	msg += "' found.\n\tReplaced with '1' (4-byte floating point)";
    }
    else if ( nr == cSEGYWarnPos )
    {
	msg = "Bad position found. Such traces are ignored.\nFirst occurrence ";
	msg += detail;
    }
    else if ( nr == cSEGYWarnZeroSampIntv )
    {
	msg = "Zero sample interval found. Replaced with survey default.\n"
	      "First occurrence ";
	msg += detail;
    }
    else if ( nr == cSEGYWarnDataReadIncomplete )
    {
	msg = detail;
    }

    SeisTrcTranslator::addWarn( nr, msg );
}


void SEGYSeisTrcTranslator::updateCDFromBuf()
{
    SeisTrcInfo info;
    trchead_.fill( info, fileopts_.coordscale_ );
    insd = info.sampling;
    if ( !insd.step ) insd.step = binhead_dpos_;
    if ( !mIsUdf(fileopts_.timeshift_) )
	insd.start = fileopts_.timeshift_;
    if ( !mIsUdf(fileopts_.sampleintv_) )
	insd.step = fileopts_.sampleintv_;

    innrsamples = filepars_.ns_;
    if ( innrsamples <= 0 )
    {
	innrsamples = binhead_ns_;
	if ( innrsamples <= 0 )
	{
	    innrsamples = trchead_.nrSamples();
	    if ( innrsamples <= 0 )
		innrsamples = SI().zRange(false).nrSteps() + 1;
	}
    }

    addComp( getDataChar(filepars_.fmt_)  );
    DataCharacteristics& dc = tarcds[0]->datachar;
    dc.fmt = DataCharacteristics::Ieee;
    const float scfac = trchead_.postScale(filepars_.fmt_ ? filepars_.fmt_ : 1);
    if ( !mIsEqual(scfac,1,mDefEps)
      || (dc.isInteger() && dc.nrBytes() == BinDataDesc::N4) )
	dc = DataCharacteristics();
}


int SEGYSeisTrcTranslator::nrSamplesRead() const
{
    int ret = trchead_.nrSamples();
    return ret ? ret : binhead_ns_;
}


void SEGYSeisTrcTranslator::interpretBuf( SeisTrcInfo& ti )
{
    trchead_.fill( ti, fileopts_.coordscale_ );
    if ( is_prestack && fileopts_.psdef_ == SEGY::FileReadOpts::SrcRcvCoords )
    {
	Coord c1( trchead_.getCoord(true,fileopts_.coordscale_) );
	Coord c2( trchead_.getCoord(false,fileopts_.coordscale_) );
	ti.setPSFlds( c1, c2 );
	ti.coord = Coord( (c1.x+c2.x)*.5, (c1.y+c2.y)*.5 );
    }
    float scfac = trchead_.postScale( filepars_.fmt_ ? filepars_.fmt_ : 1 );
    if ( mIsEqual(scfac,1,mDefEps) )
	curtrcscale_ = 0;
    else
    {
	if ( !trcscale_ ) trcscale_ = new LinScaler( 0, scfac );
	else		 trcscale_->factor = scfac;
	curtrcscale_ = trcscale_;
    }

    if ( !mIsUdf(fileopts_.timeshift_) )
	ti.sampling.start = fileopts_.timeshift_;
    if ( !mIsUdf(fileopts_.sampleintv_) )
	ti.sampling.step = fileopts_.sampleintv_;
}


void SEGYSeisTrcTranslator::setTxtHeader( SEGY::TxtHeader* th )
{
    delete txthead_; txthead_ = th;
}


bool SEGYSeisTrcTranslator::writeTapeHeader()
{
    if ( filepars_.fmt_ == 0 )
	// Auto-detect
	filepars_.fmt_ = nrFormatFor( storinterp->dataChar() );

    trchead_.isrev1 = !forcerev0;

    if ( !txthead_ )
    {
	txthead_ = new SEGY::TxtHeader( trchead_.isrev1 );
	txthead_->setUserInfo( pinfo.usrinfo );
	fileopts_.thdef_.linename = curlinekey.buf();
	fileopts_.thdef_.pinfo = &pinfo;
	txthead_->setPosInfo( fileopts_.thdef_ );
	txthead_->setStartPos( outsd.start );
	if ( Settings::common().isTrue("SEGY.Text Header EBCDIC") )
	    txthead_->setEbcdic();
    }
    if ( !sConn().doIO( txthead_->txt_, SegyTxtHeaderLength ) )
	mErrRet("Cannot write SEG-Y textual header")

    SEGY::BinHeader binhead( trchead_.isrev1 );
    binhead.format = filepars_.fmt_ < 2 ? 1 : filepars_.fmt_;
    filepars_.fmt_ = binhead.format;
    binhead.lino = pinfo.nr;
    static int jobid = 0;
    binhead.jobid = ++jobid;
    binhead.hns = (short)outnrsamples;
    binhead.hdt = (short)(outsd.step * SI().zFactor() * 1e3 + .5);
    binhead.tsort = is_prestack ? 0 : 4; // To make Strata users happy
    unsigned char binheadbuf[400];
    binhead.putTo( binheadbuf );
    if ( !sConn().doIO( binheadbuf, SegyBinHeaderLength ) )
	mErrRet("Cannot write SEG-Y binary header")

    return true;
}


void SEGYSeisTrcTranslator::fillHeaderBuf( const SeisTrc& trc )
{
    trchead_.use( trc.info() );
    if ( useinpsd )
	trchead_.putSampling( trc.info().sampling, trc.size() );
    else
	trchead_.putSampling( outsd, outnrsamples );
}


void SEGYSeisTrcTranslator::usePar( const IOPar& iopar )
{
    SeisTrcTranslator::usePar( iopar );

    filepars_.usePar( iopar );
    fileopts_.usePar( iopar );
    fileopts_.setGeomType( Seis::geomTypeOf(is_2d,is_prestack) );

    iopar.getYN( SEGY::FileDef::sKeyForceRev0, forcerev0 );
}


bool SEGYSeisTrcTranslator::isRev1() const
{
    return trchead_.isrev1;
}


void SEGYSeisTrcTranslator::toPreferred( DataCharacteristics& dc ) const
{
    dc = getDataChar( nrFormatFor(dc) );
}


void SEGYSeisTrcTranslator::toSupported( DataCharacteristics& dc ) const
{
    if ( dc.isInteger() || !dc.isIeee() )
	dc = getDataChar( nrFormatFor(dc) );
}


void SEGYSeisTrcTranslator::toPreSelected( DataCharacteristics& dc ) const
{
    if ( filepars_.fmt_ > 0 ) dc = getDataChar( filepars_.fmt_ );
}


int SEGYSeisTrcTranslator::nrFormatFor( const DataCharacteristics& dc ) const
{
    int nrbytes = dc.nrBytes();
    if ( nrbytes > 4 ) nrbytes = 4;
    else if ( !dc.isSigned() && nrbytes < 4 )
	nrbytes *= 2;

    int nf = 1;
    if ( nrbytes == 1 )
	nf = 8;
    else if ( nrbytes == 2 )
	nf = 3;
    else
	nf = dc.isInteger() ? 2 : 1;

    return nf;
}


DataCharacteristics SEGYSeisTrcTranslator::getDataChar( int nf ) const
{
    DataCharacteristics dc( true, true, BinDataDesc::N4,
			DataCharacteristics::Ibm,
			filepars_.byteswap_ ? !__islittle__ : __islittle__ );

    switch ( nf )
    {
    case 3:
        dc.setNrBytes( 2 );
    case 2:
    break;
    case 8:
        dc.setNrBytes( 1 );
    break;
    case 5:
	dc.fmt = DataCharacteristics::Ieee;
	dc.setInteger( false );
	dc.littleendian = false;
    break;
    default:
	dc.setInteger( false );
    break;
    }

    return dc;
}


bool SEGYSeisTrcTranslator::commitSelections_()
{
    if ( conn && sConn().ioobj )
    {
	mDynamicCastGet(IOStream*,iostrm,sConn().ioobj)
	if ( iostrm )
	    iotype = iostrm->type();
    }
    const bool forread = conn->forRead();
    fileopts_.forread_ = forread;
    fileopts_.setGeomType( Seis::geomTypeOf( is_2d, is_prestack ) );

    inpcd = inpcds[0];
    outcd = outcds[0];
    if ( !forread )
	toPreferred( outcd->datachar );
    storinterp = new TraceDataInterpreter( forread ? inpcd->datachar
	    					   : outcd->datachar );
    if ( mIsEqual(outsd.start,insd.start,mDefEps)
      && mIsEqual(outsd.step,insd.step,mDefEps) )
	useinpsd = true;
    userawdata = inpcd->datachar == outcd->datachar;

    if ( blockbuf )
	{ delete [] blockbuf; blockbuf = 0; }
    int bufsz = innrsamples;
    if ( outnrsamples > bufsz ) bufsz = outnrsamples;
    bufsz += 10;
    int nbts = inpcd->datachar.nrBytes();
    if ( outcd->datachar.nrBytes() > nbts ) nbts = outcd->datachar.nrBytes();
    
    blockbuf = new unsigned char [ nbts * bufsz ];
    return forread || writeTapeHeader();
}


bool SEGYSeisTrcTranslator::initRead_()
{
    if ( !readTapeHeader() || !readTraceHeadBuffer() )
	return false;

    if ( tarcds.isEmpty() )
	updateCDFromBuf();
    return true;
}


bool SEGYSeisTrcTranslator::initWrite_( const SeisTrc& trc )
{
    for ( int idx=0; idx<trc.data().nrComponents(); idx++ )
    {
	DataCharacteristics dc(trc.data().getInterpreter(idx)->dataChar());
	addComp( dc );
	toSupported( dc );
	toPreSelected( dc );
	tarcds[idx]->datachar = dc;
	if ( idx ) tarcds[idx]->destidx = -1;
    }

    return true;
}


bool SEGYSeisTrcTranslator::goToTrace( int nr )
{
    std::streamoff so = nr;
    so *= (240 + dataBytes() * innrsamples);
    so += 3600;
    sConn().iStream().seekg( so, std::ios::beg );
    headerbufread = false;
    return sConn().iStream().good();
}



const char* SEGYSeisTrcTranslator::getTrcPosStr() const
{
    static BufferString msg;
    int usecur = 1; const bool is2d = Seis::is2D(fileopts_.geomType());
    if ( is2d )
    {
	if ( curtrcnr < 0 )
	    usecur = prevtrcnr < 0 ? -1 : 0;
    }
    else
    {
	if ( mIsUdf(curbinid.inl) )
	    usecur = mIsUdf(prevbinid.inl) ? -1 : 0;
    }

    msg = usecur ? "at " : "after ";
    if ( usecur < 0 )
	{ msg += "first trace"; return msg.buf(); }

    if ( is2d )
	{ msg += "trace number "; msg += usecur ? curtrcnr : prevtrcnr; }
    else
    {
	const BinID bid( usecur ? curbinid : prevbinid );
	msg += "position "; msg += bid.inl; msg += "/"; msg += bid.crl;
    }

    if ( Seis::isPS(fileopts_.geomType()) )
	{ msg += " (offset "; msg += usecur ? curoffs : prevoffs; msg += ")"; }

    return msg.buf();
}


#define mBadCoord(ti) \
	( mIsZero(ti.coord.x,0.01) && mIsZero(ti.coord.y,0.01) \
	  && !SI().isReasonable(ti.coord) )
#define mBadBid(ti) \
    ( ti.binid.inl == 0 && ti.binid.crl == 0 )
#define mGetNextTrcHdr(ti) \
	{ \
	    addWarn(cSEGYWarnPos,getTrcPosStr()); \
	    sConn().iStream().seekg( nrSamplesRead() * mBPS(inpcd), \
		    		     std::ios::cur ); \
	    if ( !readTraceHeadBuffer() ) \
		return false; \
	    interpretBuf( ti ); \
	}

bool SEGYSeisTrcTranslator::readInfo( SeisTrcInfo& ti )
{
    if ( !storinterp ) commitSelections();
    if ( headerdone ) return true;

    if ( read_mode != Seis::Scan )
	{ mSetUdf(curbinid.inl); mSetUdf(curtrcnr); }

    if ( !headerbufread && !readTraceHeadBuffer() )
	return false;
    interpretBuf( ti );

    bool canuse = true;
    if ( fileopts_.icdef_ == SEGY::FileReadOpts::XYOnly )
    {
	if ( read_mode == Seis::Scan )
	    canuse = !mBadCoord(ti);
	else if ( read_mode == Seis::Prod )
	{
	    while ( mBadCoord(ti) )
		mGetNextTrcHdr(ti)
	}
	ti.binid = SI().transform( ti.coord );
    }
    else if ( fileopts_.icdef_ == SEGY::FileReadOpts::ICOnly )
    {
	if ( read_mode == Seis::Scan )
	    canuse = !mBadBid(ti);
	else if ( read_mode == Seis::Prod )
	{
	    while ( mBadBid(ti) )
		mGetNextTrcHdr(ti)
	}
	ti.coord = SI().transform( ti.binid );
    }
    else
    {
	if ( read_mode == Seis::Scan )
	    canuse = !mBadBid(ti) || !mBadCoord(ti);
	if ( read_mode == Seis::Prod )
	{
	    while ( mBadBid(ti) && mBadCoord(ti) )
		mGetNextTrcHdr(ti)
	    if ( mBadBid(ti) )
		ti.binid = SI().transform( ti.coord );
	    else if ( mBadCoord(ti) )
		ti.coord = SI().transform( ti.binid );
	}
    }

    if ( trchead_.isusable )
	trchead_.isusable = canuse;

    if ( !useinpsd ) ti.sampling = outsd;

    if ( fileopts_.psdef_ == SEGY::FileReadOpts::UsrDef )
    {
	const bool is2d = Seis::is2D(fileopts_.geomType());
	if ( (is2d && ti.nr != prevtrcnr) || (!is2d && ti.binid != prevbinid) )
	    curoffs = -1;

	if ( curoffs < 0 )
	    curoffs = offsdef.start;
	else
	    curoffs += offsdef.step;

	ti.offset = curoffs;
    }

    if ( canuse )
    {
	prevtrcnr = curtrcnr; curtrcnr = ti.nr;
	prevbinid = curbinid; curbinid = ti.binid;
	prevoffs = curoffs; curoffs = ti.offset;
    }

    if ( mIsZero(ti.sampling.step,mDefEps) )
    {
	addWarn(cSEGYWarnZeroSampIntv,getTrcPosStr());
	ti.sampling.step = SI().zStep();
    }
    return (headerdone = true);
}


bool SEGYSeisTrcTranslator::read( SeisTrc& trc )
{
    if ( !readInfo(trc.info()) )
	return false;

    return readData( trc );
}


bool SEGYSeisTrcTranslator::skip( int ntrcs )
{
    if ( ntrcs < 1 ) return true;
    if ( !storinterp ) commitSelections();

    std::istream& strm = sConn().iStream();
    int nrsamples = innrsamples;
    if ( !headerdone )	strm.seekg( mSEGYTraceHeaderBytes, std::ios::cur );
    else		nrsamples = nrSamplesRead();
    strm.seekg( nrsamples * mBPS(inpcd), std::ios::cur );
    if ( ntrcs > 1 )
	strm.seekg( (ntrcs-1) * (mSEGYTraceHeaderBytes
		    		+ nrsamples * mBPS(inpcd)) );

    headerbufread = headerdone = false;

    if ( !strm.good() )
	mPosErrRet("Read error during trace skipping")
    return true;
}


bool SEGYSeisTrcTranslator::writeTrc_( const SeisTrc& trc )
{
    memset( headerbuf, 0, mSEGYTraceHeaderBytes );
    fillHeaderBuf( trc );

    if ( !sConn().doIO( headerbuf, mSEGYTraceHeaderBytes ) )
	mErrRet("Cannot write trace header")

    return writeData( trc );
}


bool SEGYSeisTrcTranslator::readTraceHeadBuffer()
{
    if ( !sConn().doIO( headerbuf, mSEGYTraceHeaderBytes )
	    || sConn().iStream().eof() )
    {
	if ( !sConn().iStream().eof() )
	    mPosErrRet("Error reading trace header")
	return noErrMsg();
    }

    return (headerbufread = true);
}


bool SEGYSeisTrcTranslator::readDataToBuf( unsigned char* tdptr )
{
    std::istream& strm = sConn().iStream();
    if ( samps.start > 0 )
	strm.seekg( samps.start * mBPS(inpcd), std::ios::cur );

    unsigned char* ptr = userawdata ? tdptr : blockbuf;
    int rdsz = (samps.width()+1) *  mBPS(inpcd);
    if ( !sConn().doIO(ptr,rdsz) )
    {
	if ( strm.gcount() != rdsz )
	    addWarn( cSEGYWarnDataReadIncomplete, strm.gcount()
		    ? "Last trace is incomplete" : "No data in last trace" );
	return noErrMsg();
    }

    if ( samps.stop < innrsamples-1 )
	strm.seekg( (innrsamples-samps.stop-1) * mBPS(inpcd), std::ios::cur );

    return strm.good();
}


bool SEGYSeisTrcTranslator::readData( SeisTrc& trc )
{
    noErrMsg();
    const int curcomp = selComp();
    prepareComponents( trc, outnrsamples );
    headerbufread = headerdone = false;

    unsigned char* tdptr = trc.data().getComponent(curcomp)->data();
    if ( !readDataToBuf(tdptr) ) return false;

    if ( !userawdata )
    {
	const unsigned char* ptr = blockbuf;
	// Convert data into other format
	for ( int isamp=0; isamp<outnrsamples; isamp++ )
	    trc.set( isamp, storinterp->get(ptr,isamp), curcomp );
    }

    if ( curtrcscale_ )
    {
	const int tsz = trc.size();
	for ( int idx=0; idx<tsz; idx++ )
	    trc.set(idx,curtrcscale_->scale(trc.get(idx,curcomp)),curcomp);
    }

    return true;
}


bool SEGYSeisTrcTranslator::writeData( const SeisTrc& trc )
{
    const int curcomp = selComp();

    static bool allowudfs = GetEnvVarYN( "OD_SEIS_SEGY_ALLOW_UDF" );
    static float udfreplaceval = GetEnvVarDVal( "OD_SEIS_SEGY_UDF_REPLACE", 0 );
    for ( int idx=0; idx<outnrsamples; idx++ )
    {
	float val = trc.getValue( outsd.atIndex(idx), curcomp );
	if ( !allowudfs && mIsUdf(val) )
	    val = udfreplaceval;
	storinterp->put( blockbuf, idx, val );
    }

    if ( !sConn().doIO( (void*)blockbuf,
			 outnrsamples * outcd->datachar.nrBytes() ) )
	mErrRet("Cannot write trace data")

    headerdone = headerbufread = false;
    return true;
}


void SEGYSeisTrcTranslator::fillErrMsg( const char* s, bool withpos )
{
    static BufferString msg;
    msg = "";

    const char* fnm = sConn().streamData().fileName();
    if ( !fnm || !*fnm )
	msg = usrname_;
    else
	{ msg = "In file '"; msg += fnm; msg += "'"; }
    if ( withpos )
	{ msg += " "; msg += getTrcPosStr(); }
    msg += ":\n"; msg += s;
    errmsg = msg.buf();
}


bool SEGYSeisTrcTranslator::noErrMsg()
{
    errmsg = ""; return false;
}
