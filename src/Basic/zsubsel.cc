/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2018
-*/


#include "zsubsel.h"
#include "iopar.h"
#include "keystrs.h"
#include "posidxsubsel.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "uistrings.h"

mUseType( Pos::ZSubSelData, idx_type );
mUseType( Pos::ZSubSelData, z_type );
mUseType( Pos::IdxSubSelData, pos_steprg_type );


Pos::ZSubSelData::ZSubSelData( const z_steprg_type& rg )
    : ArrRegSubSelData(rg.nrSteps()+1)
    , inpzrg_(rg)
{
    ensureSizeOK();
}


bool Pos::ZSubSelData::operator ==( const Pos::ZSubSelData& oth ) const
{
    return inpzrg_.isEqual( oth.inpzrg_, zEps() )
	&& ArrRegSubSelData::operator ==( oth );
}


bool Pos::ZSubSelData::isAll() const
{
    return !isSubSpaced() && hasFullRange();
}


bool Pos::ZSubSelData::hasFullRange() const
{
    return mIsEqual(zStart(),inpzrg_.start,zEps())
	&& mIsEqual(zStop(),inpzrg_.stop,zEps());
}


idx_type Pos::ZSubSelData::idx4Z( z_type z ) const
{
    const auto fnrz = (z - zStart()) / zStep();
    return mNINT32( fnrz );
}


z_type Pos::ZSubSelData::z4Idx( idx_type idx ) const
{
    return zStart() + zStep() * idx;
}


z_type Pos::ZSubSelData::zStart() const
{
    return inpzrg_.start + inpzrg_.step * offs_;
}


z_type Pos::ZSubSelData::zStep() const
{
    return inpzrg_.step * step_;
}


z_type Pos::ZSubSelData::zStop() const
{
    return zStart() + zStep() * (sz_-1);
}


void Pos::ZSubSelData::setInputZRange( const z_steprg_type& newrg )
{
    inpzrg_ = newrg;
    sz_ = inpzrg_.nrSteps() + 1;
    ensureSizeOK();
}


void Pos::ZSubSelData::setOutputZRange( z_type newstart, z_type newstop,
					z_type newstep )
{
    if ( mIsUdf(newstart) )
	newstart = inpzrg_.start;
    if ( mIsUdf(newstop) )
	newstop = inpzrg_.stop;
    if ( mIsUdf(newstep) || newstep <= 0.f )
	newstep = inpzrg_.step;

    IdxSubSelData newss( pos_steprg_type(0,inpzrg_.nrSteps(),1) );
    z_type fnewstep = newstep / inpzrg_.step;
    ZSubSelData cleanzss( inpzrg_ );
    newss.setOutputPosRange( cleanzss.idx4Z(newstart), cleanzss.idx4Z(newstop),
			     mRounded(z_type,fnewstep) );
    ArrRegSubSelData::operator =( newss );
}


void Pos::ZSubSelData::ensureSizeOK()
{
    if ( zStop() > inpzrg_.stop+zEps() )
    {
	z_type fsz = (inpzrg_.stop - zStart()) / zStep() + 1;
	sz_ = (size_type)(fsz + zEps());
    }
}


void Pos::ZSubSelData::limitTo( const ZSubSelData& oth )
{
    auto outrg = outputZRange();
    outrg.limitTo( oth.outputZRange() );
    setOutputZRange( outrg );
}


void Pos::ZSubSelData::limitTo( const z_rg_type& zrg )
{
    auto outrg = outputZRange();
    outrg.limitTo( zrg );
    setOutputZRange( outrg );
}


void Pos::ZSubSelData::widenTo( const ZSubSelData& oth )
{
    auto outrg = outputZRange();
    const auto othoutrg = oth.outputZRange();
    if ( othoutrg.start < outrg.start )
	outrg.start = outrg.atIndex( outrg.nearestIndex(othoutrg.start) );
    if ( othoutrg.stop > outrg.stop )
	outrg.stop = outrg.atIndex( outrg.nearestIndex(othoutrg.stop) );
    setOutputZRange( outrg );
}


void Pos::ZSubSelData::widen( const z_rg_type& zrg )
{
    auto outrg = outputZRange();
    outrg.start += zrg.start; outrg.stop += zrg.stop;
    setOutputZRange( outrg );
}


const Pos::ZSubSel& Pos::ZSubSel::surv3D()
{
    static ZSubSel ret( z_steprg_type(0.f,0.f,1.f) );
    ret.setInputZRange( SI().zRange() );
    return ret;
}


Pos::ZSubSel& Pos::ZSubSel::dummy()
{
    static ZSubSel ret( z_steprg_type(0.f,0.f,1.f) );
    ret = surv3D();
    return ret;
}


Pos::ZSubSel::ZSubSel( GeomID gid )
    : data_(Survey::Geometry::get(gid).zRange())
{
}


bool Pos::ZSubSel::usePar( const IOPar& iop )
{
    z_steprg_type zrg;
    if ( !iop.get(sKey::ZRange(),zrg) )
	return false;

    data_.setOutputZRange( zrg );
    return true;
}


void Pos::ZSubSel::fillPar( IOPar& iop ) const
{
    iop.set( sKey::ZRange(), zStart(), zStop(), zStep() );
}


Survey::FullZSubSel::FullZSubSel()
    : geomids_(GeomID::get3D())
    , zsss_(ZSubSel(SI().zRange()))
{
}


Survey::FullZSubSel::FullZSubSel( const ZSubSel& zss )
    : geomids_(GeomID::get3D())
    , zsss_(zss)
{
}


Survey::FullZSubSel::FullZSubSel( const z_steprg_type& zrg )
    : geomids_(GeomID::get3D())
    , zsss_(ZSubSel(zrg))
{
}


Survey::FullZSubSel::FullZSubSel( GeomID gid )
    : FullZSubSel( GeomIDSet(gid) )
{
}


Survey::FullZSubSel::FullZSubSel( const GeomIDSet& gids )
{
    for ( auto gid : gids )
	setFull( gid );
}


Survey::FullZSubSel::FullZSubSel( GeomID gid, const z_steprg_type& zrg )
{
    set( gid, ZSubSel(zrg) );
}


Survey::FullZSubSel::FullZSubSel( const FullZSubSel& oth )
    : geomids_(oth.geomids_)
    , zsss_(oth.zsss_)
{
}


Survey::FullZSubSel& Survey::FullZSubSel::operator =( const FullZSubSel& oth )
{
    if ( this != &oth )
    {
	geomids_ = oth.geomids_;
	zsss_ = oth.zsss_;
    }
    return *this;
}


bool Survey::FullZSubSel::operator ==( const FullZSubSel& oth ) const
{
    return geomids_ == oth.geomids_ && zsss_ == oth.zsss_;
}


bool Survey::FullZSubSel::is2D() const
{
    return geomids_.isEmpty() || geomids_.first().is2D();
}


Pos::ZSubSel& Survey::FullZSubSel::getFor( GeomID gid )
{
    const auto idx = indexOf( gid );
    return idx < 0 ? ZSubSel::dummy() : zsss_.get( idx );
}


bool Survey::FullZSubSel::isAll() const
{
    GeomIDSet gids;
    Survey::Geometry2D::getGeomIDs( gids );
    for ( auto gid : gids )
	if ( !isPresent(gid) || !getFor(gid).isAll() )
	    return false;

    return true;
}


bool Survey::FullZSubSel::hasFullRange() const
{
    for ( auto zss : zsss_ )
	if ( !zss.hasFullRange() )
	    return false;
    return true;
}


void Survey::FullZSubSel::setFull( GeomID gid )
{
    if ( !gid.isValid() )
	return;
    const auto& geom = Survey::Geometry::get( gid );
    if ( !geom.isEmpty() )
	set( gid, ZSubSel(geom.zRange()) );
}


void Survey::FullZSubSel::set( const ZSubSel& zss )
{
    set( GeomID::get3D(), zss );
}


void Survey::FullZSubSel::set( GeomID gid, const ZSubSel& zss )
{
    if ( !gid.isValid() )
	return;

    const auto idxof = indexOf( gid );
    if ( idxof >= 0 )
	zsss_.get(idxof) = zss;
    else
    {
	geomids_.add( gid );
	zsss_.add( zss );
    }
}


void Survey::FullZSubSel::setToNone( bool is2d )
{
    geomids_.setEmpty();
    zsss_.setEmpty();
    if ( !is2d )
	setFull( GeomID::get3D() );
}


void Survey::FullZSubSel::remove( idx_type idx )
{
    if ( !geomids_.validIdx(idx) )
	{ pErrMsg("idx bad"); return; }
    geomids_.removeSingle( idx );
    zsss_.removeSingle( idx );
}


void Survey::FullZSubSel::remove( GeomID gid )
{
    const auto idx = indexOf( gid );
    if ( idx >= 0 )
	remove( idx );
}


void Survey::FullZSubSel::merge( const FullZSubSel& oth )
{
    for ( int idx=0; idx<oth.geomids_.size(); idx++ )
    {
	const auto gid = oth.geomids_.get( idx );
	const auto& othzss = oth.zsss_.get( idx );
	const auto idxof = indexOf( gid );
	if ( idxof >= 0 )
	    zsss_.get(idxof).merge( othzss );
	else
	    set( gid, othzss );
    }
}


void Survey::FullZSubSel::limitTo( const FullZSubSel& oth )
{
    for ( int idx=0; idx<geomids_.size(); idx++ )
    {
	const auto gid = geomids_.get( idx );
	const auto idxof = oth.indexOf( gid );
	if ( idxof >= 0 )
	    zsss_.get(idx).limitTo( oth.get(idxof) );
	else
	{
	    geomids_.removeSingle( idx );
	    zsss_.removeSingle( idx );
	    idx--;
	}
    }
}



static const char* sKeyZSS = "Z Subsel";


void Survey::FullZSubSel::fillPar( IOPar& iop ) const
{
    iop.set( IOPar::compKey(sKeyZSS,sKey::Size()), size() );
    iop.setYN( IOPar::compKey(sKeyZSS,sKey::Is2D()), is2D() );
    for ( auto idx=0; idx<size(); idx++ )
    {
	const BufferString baseky( IOPar::compKey(sKeyZSS,idx) );
	iop.set( IOPar::compKey(baseky,sKey::GeomID()), geomids_.get(idx) );
	iop.set( IOPar::compKey(baseky,sKey::Range()),
				    zsss_.get(idx).outputZRange() );
    }
}


void Survey::FullZSubSel::usePar( const IOPar& inpiop )
{
    PtrMan<IOPar> iop = inpiop.subselect( sKeyZSS );
    if ( !iop || iop->isEmpty() )
	return;

    int sz = 0;
    iop->get( IOPar::compKey(sKeyZSS,sKey::Size()), sz );
    if ( sz < 1 )
	return;

    bool is2d = is2D();
    iop->getYN( IOPar::compKey(sKeyZSS,sKey::Is2D()), is2d );

    setToNone( is2d );
    for ( auto idx=0; idx<sz; idx++ )
    {
	const BufferString baseky( IOPar::compKey(sKeyZSS,idx) );
	GeomID gid; z_steprg_type zrg;
	iop->get( IOPar::compKey(baseky,sKey::GeomID()), gid );
	iop->get( IOPar::compKey(baseky,sKey::Range()), zrg );
	if ( gid.isValid() )
	    set( gid, ZSubSel(zrg) );
    }
}


uiString Survey::FullZSubSel::getUserSummary() const
{
    uiString ret;
    if ( isEmpty() )
	return ret;

    const auto& zss0 = get( 0 );
    z_rg_type minmaxrg = zss0.outputZRange();
    bool varying = false;
    bool isall = zss0.isAll();
    for ( auto idx=1; idx<size(); idx++ )
    {
	const auto& zss = get( idx );
	minmaxrg.include( zss.outputZRange(), false );
	isall = isall && zss.isAll();
	varying = varying || zss != zss0;
    }

    uiString rgstr = uiStrings::sRangeTemplate( true );
    rgstr.arg( minmaxrg.start ).arg( minmaxrg.stop ).arg( zss0.zStep() );
    ret.set( toUiString("%1: %2").arg( uiStrings::sZRange() ).arg( rgstr ) );
    if ( size() > 1 )
    {
	if ( isall )
	    ret.appendPhraseSameLine( uiStrings::sFull().parenthesize() );
	else if ( varying )
	    ret.appendPhraseSameLine(
			uiStrings::sVariable(false).parenthesize() );
    }

    return ret;
}
