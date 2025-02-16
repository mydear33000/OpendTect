/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2018
-*/


#include "serverprogtool.h"
#include "wellmanager.h"
#include "wellinfo.h"
#include "welltrack.h"
#include "welllog.h"
#include "commandlineparser.h"
#include "keystrs.h"
#include "odjson.h"
#include "prog.h"
#include "scaler.h"
#include "unitofmeasure.h"


static const int cProtocolNr = 1;

static const char* sListWellsCmd	= ServerProgTool::sListUsrCmd();
static const char* sInfoCmd		= ServerProgTool::sInfoUsrCmd();
static const char* sListLogsCmd		= "list-logs";
static const char* sReadTrackCmd	= "read-track";
static const char* sReadLogCmd		= "read-log";

static const char* sNoTVDArg		= "no-tvd";


class WellServerTool : public ServerProgTool
{
public:

    mUseType( Well, Data );
    mUseType( Well, Info );
    mUseType( Well, LoadReqs );
    mUseType( Well, Track );

		    WellServerTool(int,char**);

    void	    listWells();
    void	    getWellInfo();
    void	    getTrack();
    void	    listLogs();
    void	    readLog(const DBKey&,const char*,bool notvd);

protected:

    ConstRefMan<Data>	wd_;

    BufferString    getSpecificUsage() const override;
    void	    getWD(const DBKey&,const LoadReqs&);
    void	    getWD(const char*,const LoadReqs&);

};


WellServerTool::WellServerTool( int argc, char** argv )
    : ServerProgTool(argc,argv,"Well")
{
    initParsing( cProtocolNr );
}


void WellServerTool::listWells()
{
    DBKeySet wellids;
    Well::MGR().getAll( wellids, false );
    set( sKey::Size(), wellids.size() );
    set( sKey::ID(mPlural), wellids );
    BufferStringSet nms;
    for ( auto wellid : wellids )
	nms.add( wellid->name() );
    set( sKey::Name(mPlural), nms );
    respondInfo( true );
}


void WellServerTool::getWD( const DBKey& wellid, const LoadReqs& lreqs )
{
    uiRetVal uirv;
    wd_ = Well::MGR().fetch( wellid, lreqs, uirv );
    if ( !wd_ )
	respondError( uirv );
}


void WellServerTool::getWD( const char* cmd, const LoadReqs& lreqs )
{
    getWD( getDBKey(cmd), lreqs );
}


void WellServerTool::getWellInfo()
{
    getWD( sInfoCmd, LoadReqs(Well::Inf) );

    set( sKey::ID(), wd_->dbKey() );
    set( sKey::Name(), wd_->name() );

    const auto& inf = wd_->info();
    const BufferString uwi = inf.UWI();
    if ( !uwi.isEmpty() )
	set( Info::sKeyUwid(), uwi );
    const auto wt = inf.wellType();
    if ( wt != Info::None )
	set( sKey::Type(), Info::toString(wt) );

    set( sKey::X(), inf.surfaceCoord().x_ );
    set( sKey::Y(), inf.surfaceCoord().y_ );

    respondInfo( true );
}


void WellServerTool::listLogs()
{
    const DBKey wellid = getDBKey( sListLogsCmd );
    BufferStringSet lognms;
    Well::MGR().getLogNames( wellid, lognms );
    set( sKey::ID(), wellid );
    set( sKey::Size(), lognms.size() );
    set( sKey::Name(mPlural), lognms );
    respondInfo( true );
}


void WellServerTool::getTrack()
{
    getWD( sReadTrackCmd, LoadReqs(Well::Trck) );

    const auto& track = wd_->track();
    TypeSet<float> mds, tvds;
    TypeSet<double> xs, ys;
    const auto sz = track.size();
    for ( auto idx=0; idx<sz; idx++ )
    {
	const auto md = track.dahByIdx( idx );
	const Coord3 pos = track.posByIdx( idx );
	mds += md;
	xs += pos.x_;
	ys += pos.y_;
	tvds += (float)pos.z_;
    }
    set( sKey::Size(), mds.size() );
    set( sKey::MD(mPlural), mds );
    set( sKey::TVD(mPlural), tvds );
    set( sKey::XCoord(mPlural), xs );
    set( sKey::YCoord(mPlural), ys );

    respondInfo( true );
}


void WellServerTool::readLog( const DBKey& wellid, const char* lognm,
			      bool notvd )
{
    auto wl = Well::MGR().getLog( wellid, lognm );
    if ( !wl )
	respondError( "Log not found" );

    if ( !notvd )
	getWD( wellid, LoadReqs(Well::Inf,Well::Trck) );

    const auto sz = wl->size();
    set( sKey::Well(), wellid.name() );
    set( sKey::Name(), wl->name() );
    set( sKey::Size(), wl->size() );
    const auto* uom = wl->unitOfMeasure();
    set( sKey::Unit(), uom ? uom->symbol() : "" );
    set( sKey::Scale(), uom ? uom->scaler().scale(1) : double(1) );

    TypeSet<float> mds, vals, tvds;
    const Track* trck = wd_ ? &wd_->track() : nullptr;
    set( sKey::Size(), sz );
    for ( auto idx=0; idx<sz; idx++ )
    {
	const auto md = wl->dahByIdx( idx );
	mds += md;
	vals += wl->valueByIdx( idx );
	if ( trck )
	    tvds += trck->valueAt( md );
    }
    set( sKey::MD(mPlural), mds );
    set( sKey::Value(mPlural), vals );
    if ( trck )
	set( sKey::TVD(mPlural), tvds );

    respondInfo( true );
}


BufferString WellServerTool::getSpecificUsage() const
{
    BufferString ret;
    addToUsageStr( ret, sListWellsCmd, "" );
    addToUsageStr( ret, sInfoCmd, "well_id" );
    addToUsageStr( ret, sListLogsCmd, "well_id" );
    addToUsageStr( ret, sReadTrackCmd, "well_id" );
    BufferString argstr( "well_id log_name [--", sNoTVDArg, "]" );
    addToUsageStr( ret, sReadLogCmd, argstr );
    return ret;
}


int main( int argc, char** argv )
{
    WellServerTool st( argc, argv );
    auto& clp = st.clp();

    if ( clp.hasKey(sListWellsCmd) )
	st.listWells();

    if ( clp.hasKey(sInfoCmd) )
	st.getWellInfo();
    if ( clp.hasKey(sReadTrackCmd) )
	st.getTrack();
    else if ( clp.hasKey(sListLogsCmd) )
	st.listLogs();
    else if ( clp.hasKey(sReadLogCmd) )
    {
	clp.setKeyHasValues( sReadLogCmd, 2 );
	DBKey wellid;
	clp.getKeyedInfo( sReadLogCmd, wellid );
	BufferString lognm;
	clp.getKeyedInfo( sReadLogCmd, lognm, false, 1 );
	const bool notvd = clp.hasKey( sNoTVDArg );
	st.readLog( wellid, lognm, notvd );
    }

    pFreeFnErrMsg( "Should not reach" );
    return ExitProgram( 0 );
}
