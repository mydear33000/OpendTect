/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2003
-*/


#include "wellhdf5writer.h"
#include "hdf5arraynd.h"

#include "welldata.h"
#include "wellinfo.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "welldisp.h"
#include "keystrs.h"
#include "iostrm.h"
#include "dbman.h"
#include "file.h"
#include "filepath.h"



Well::HDF5Writer::HDF5Writer( const char* fnm, const Data& wd,
			      uiString& e )
    : WriteAccess(wd)
    , errmsg_(e)
{
    init( fnm );
}


Well::HDF5Writer::HDF5Writer( const IOObj& ioobj, const Data& wd,
			      uiString& e )
    : WriteAccess(wd)
    , errmsg_(e)
    , wrr_(0)
{
    if ( !useHDF5(ioobj,errmsg_) || !errmsg_.isEmpty() )
	return;

    bool fnmchgd;
    init( ioobj.mainFileName(), &fnmchgd );
    if ( errmsg_.isEmpty() && fnmchgd )
    {
	IOObj* ioobjclone = ioobj.clone();
	mDynamicCastGet( IOStream*, iostrm, ioobjclone );
	if ( iostrm )
	{
	    iostrm->fileSpec().setFileName( filename_ );
	    DBM().setEntry( *iostrm );
	}
    }
}


Well::HDF5Writer::~HDF5Writer()
{
    delete wrr_;
}


bool Well::HDF5Writer::useHDF5( const IOObj& ioobj, uiString& emsg )
{
    const BufferString fnm( ioobj.mainFileName() );
    bool usehdf = HDF5::isEnabled( HDF5::sWellType() );
    emsg.setEmpty();
    if ( !File::isEmpty(fnm) )
    {
	usehdf = HDF5::isHDF5File(fnm);
	if ( usehdf && !HDF5::isAvailable() )
	    emsg = HDF5::Access::sHDF5NotAvailable( fnm );
    }
    return usehdf;
}


HDF5::Reader* Well::HDF5Writer::createCoupledHDFReader() const
{
    return wrr_ ? wrr_->createCoupledReader() : 0;
}


#define mErrRet( s )	{ errmsg_ = s; return; }

void Well::HDF5Writer::init( const char* inpfnm, bool* fnmchgd )
{
    const BufferString orgfnm( inpfnm );
    if ( orgfnm.isEmpty() || !HDF5::isAvailable() )
    {
	errmsg_ = HDF5::Access::sHDF5NotAvailable( orgfnm );
	return;
    }

    wrr_ = HDF5::mkWriter();
    if ( !wrr_ )
	{ pErrMsg("Available but no writer?"); return; }

    filename_.set( orgfnm );
    if ( fnmchgd )
    {
	File::Path fp( orgfnm );
	fp.setExtension( HDF5::Access::sFileExtension() );
	filename_.set( fp.fullPath() );
	*fnmchgd = filename_ != orgfnm;
    }
}


bool Well::HDF5Writer::isFunctional() const
{
    return wrr_ && !filename_.isEmpty();
}


bool Well::HDF5Writer::ensureFileOpen() const
{
    if ( !wrr_ )
	return false;
    else if ( wrr_->isOpen() )
	return true;
    else if ( filename_.isEmpty() )
	return false;

    const bool neededit = HDF5::isHDF5File( filename_ );
    uiRetVal uirv = neededit
		  ? wrr_->open4Edit( filename_ )
		  : wrr_->open( filename_ );

    if ( !uirv.isOK() )
    {
	delete wrr_; const_cast<HDF5Writer*>(this)->wrr_ = 0;
	errmsg_.set( uirv );
	return false;
    }

    return true;
}


bool Well::HDF5Writer::put() const
{
    return putInfoAndTrack()
	&& putD2T()
	&& putLogs()
	&& putMarkers()
	&& putCSMdl()
	&& putDispProps();
}


void Well::HDF5Writer::putDepthUnit( IOPar& iop ) const
{
    iop.set( sKey::DepthUnit(),
	     UnitOfMeasure::surveyDefDepthStorageUnit()->name() );
}


#define mErrRetIfUiRvNotOK(dsky) \
    if ( !uirv.isOK() ) \
	{ errmsg_.set( uirv ); return false; }
#define mEnsureFileOpen() \
    if ( !ensureFileOpen() ) \
	return false


bool Well::HDF5Writer::putInfoAndTrack() const
{
    mEnsureFileOpen();

    IOPar iop;
    wd_.info().fillPar( iop );
    putDepthUnit( iop );
    const HDF5::DataSetKey rootdsky;
    uiRetVal uirv = wrr_->putInfo( rootdsky, iop );
    mErrRetIfUiRvNotOK( rootdsky );

    const size_type sz = wd_.track().size();
    Array2DImpl<double> arr( 4, sz );
    TrackIter iter( wd_.track() );
    while ( iter.next() )
    {
	const Coord3 c = iter.pos();
	const idx_type idx = iter.curIdx();
	arr.set( 0, idx, iter.dah() );
	arr.set( 1, idx, c.x_ );
	arr.set( 2, idx, c.y_ );
	arr.set( 3, idx, c.z_ );
    }

    HDF5::ArrayNDTool<double> arrtool( arr );
    const HDF5::DataSetKey trackdsky( "", sTrackDSName() );
    uirv = arrtool.put( *wrr_, trackdsky );
    mErrRetIfUiRvNotOK( trackdsky );

    return true;
}


bool Well::HDF5Writer::doPutD2T( bool csmdl ) const
{
    const D2TModel& d2t = csmdl ? wd_.checkShotModel(): wd_.d2TModel();
    const HDF5::DataSetKey dsky( "", csmdl ? sCSMdlDSName() : sD2TDSName() );

    D2TModelIter iter( d2t ); // this locks
    const size_type sz = iter.size();
    Array2DImpl<ZType> arr( 2, sz );
    while ( iter.next() )
    {
	const idx_type idx = iter.curIdx();
	arr.set( 0, idx, iter.dah() );
	arr.set( 1, idx, iter.t() );
    }
    HDF5::ArrayNDTool<ZType> arrtool( arr );
    uiRetVal uirv = arrtool.put( *wrr_, dsky );
    mErrRetIfUiRvNotOK( trackdsky );

    IOPar iop;
    iop.set( sKey::Name(), d2t.name() );
    iop.set( sKey::Desc(), d2t.desc() );
    iop.set( D2TModel::sKeyDataSrc(), d2t.dataSource() );
    putDepthUnit( iop );
    uirv = wrr_->putInfo( dsky, iop );
    mErrRetIfUiRvNotOK( dsky );

    return true;
}


bool Well::HDF5Writer::putD2T() const
{
    mEnsureFileOpen();

    return doPutD2T( false );
}


bool Well::HDF5Writer::putLogs() const
{
    mEnsureFileOpen();

    errmsg_.set( mTODONotImplPhrase() );
    // In a group
    // got to remove 'extra' data sets
    return true;
}


bool Well::HDF5Writer::putMarkers() const
{
    mEnsureFileOpen();

    const MarkerSet& ms = wd_.markers();
    HDF5::DataSetKey dsky( sMarkersGrpName(), "" );
    typedef MarkerSet::LevelID::IDType LvlIDType;

    BufferStringSet nms, colors;
    TypeSet<ZType> mds; TypeSet<LvlIDType> lvlids;
    MarkerSetIter iter( ms );
    while ( iter.next() )
    {
	const Marker& mrkr = iter.get();
	nms.add( mrkr.name() );
	colors.add( mrkr.color().getStdStr() );
	mds.add( mrkr.dah() );
	lvlids.add( mrkr.levelID().getI() );
    }
    iter.retire();

    dsky.setDataSetName( sMDsDSName() );
    uiRetVal uirv = wrr_->put( dsky, mds );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( sColorsDSName() );
    uirv = wrr_->put( dsky, colors );
    mErrRetIfUiRvNotOK( dsky );

    dsky.setDataSetName( sLvlIDsDSName() );
    uirv = wrr_->put( dsky, lvlids );
    mErrRetIfUiRvNotOK( dsky );

    return true;
}


bool Well::HDF5Writer::putCSMdl() const
{
    mEnsureFileOpen();

    return doPutD2T( true );
}


bool Well::HDF5Writer::putDispProps() const
{
    mEnsureFileOpen();

    errmsg_.set( mTODONotImplPhrase() );
    // Group DispProps with only attributes
    return false;
}
