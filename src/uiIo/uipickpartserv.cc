/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uipickpartserv.cc,v 1.39 2006-08-03 18:56:52 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uipickpartserv.h"

#include "uicursor.h"
#include "uifetchpicks.h"
#include "uiimppickset.h"
#include "uimsg.h"
#include "uipicksetman.h"

#include "pickset.h"
#include "picksettr.h"
#include "surfaceinfo.h"
#include "color.h"
#include "ioobj.h"
#include "survinfo.h"
#include "stats.h"
#include "ptrman.h"

const int uiPickPartServer::evGetHorInfo = 0;
const int uiPickPartServer::evGetHorDef = 1;


uiPickPartServer::uiPickPartServer( uiApplService& a )
	: uiApplPartServer(a)
	, uiPickSetMgr(Pick::Mgr())
    	, gendef_(2,true)
{
}


uiPickPartServer::~uiPickPartServer()
{
    deepErase( selhorids_ );
}


void uiPickPartServer::managePickSets()
{
    uiPickSetMan dlg( appserv().parent() );
    dlg.go();
}


void uiPickPartServer::impexpSet( bool import )
{
    uiImpExpPickSet dlg( parent(), import );
    dlg.go();
}


bool uiPickPartServer::fetchSets()
{
    deepErase( hinfos_ );
    sendEvent( evGetHorInfo );
    BufferStringSet hornms;
    for ( int idx=0; idx<hinfos_.size(); idx++ )
	hornms.add( hinfos_[idx]->name );

    uiFetchPicks dlg( parent(), setmgr_, hornms );
    if ( !dlg.go() )
	{ deepErase( hinfos_ ); return false; }

    bool rv = false;
    if ( dlg.mkNew() )
    {
	Pick::Set* newps = new Pick::Set( dlg.getName() );
	newps->disp_.color_ = dlg.getPickColor();
	if ( dlg.genRand() )
	{
	    if ( !mkRandLocs(*newps,dlg.randPars()) )
		{ delete newps; newps = 0; }
	}
	if ( newps )
	    rv = storeNewSet( newps );
    }
    else
    {
	for ( int idx=0; dlg.ioobj(idx); idx++ )
	{
	    const IOObj* ioobj = dlg.ioobj( idx );
	    if ( setmgr_.indexOf(ioobj->name()) >= 0 )
		continue;

	    Pick::Set* newps = new Pick::Set;
	    BufferString bs;
	    if ( PickSetTranslator::retrieve(*newps,ioobj,bs) )
	    {
		rv = true;
		setmgr_.set( ioobj->key(), newps );
	    }
	    else
	    {
		delete newps;
		if ( idx == 0 )
		{
		    uiMSG().error( bs );
		    return false;
		}
		else
		{
		    BufferString msg( dlg.ioobj(idx)->name() );
		    msg += ": "; msg += bs;
		    uiMSG().warning( msg );
		}
	    }
	}
    }

    return rv;
}


bool uiPickPartServer::mkRandLocs( Pick::Set& ps, const RandLocGenPars& rp )
{
    uiCursorChanger cursorlock( uiCursor::Wait );

    Stat_initRandom(0);
    selbr_ = &rp.bidrg;
    const bool do2hors = !rp.iscube && rp.horidx2 >= 0 && 
			 rp.horidx2 != rp.horidx;
    gendef_.empty();
    deepErase( selhorids_ );
    if ( !rp.iscube )
    {
	selhorids_ += new MultiID( hinfos_[rp.horidx]->multiid );
	if ( do2hors )
	    selhorids_ += new MultiID( hinfos_[rp.horidx2]->multiid );
	sendEvent( evGetHorDef );
    }
    else
    {
	const BinID stp = BinID( SI().inlStep(), SI().crlStep() );
	BinID bid;
	for ( bid.inl=selbr_->start.inl; bid.inl<=selbr_->stop.inl;
		bid.inl +=stp.inl )
	{
	    for ( bid.crl=selbr_->start.crl; bid.crl<=selbr_->stop.crl;
		    	bid.crl += stp.crl )
		gendef_.add( bid, rp.zrg.start, rp.zrg.stop );
	}
    }

    const int nrpts = gendef_.totalSize();
    if ( !nrpts ) return true;

    BinID bid; Interval<float> zrg;
    for ( int ipt=0; ipt<rp.nr; ipt++ )
    {
	const int ptidx = Stat_getIndex( nrpts );
	BinIDValueSet::Pos pos = gendef_.getPos( ptidx );
	gendef_.get( pos, bid, zrg.start, zrg.stop );
	float val = zrg.start + Stat_getRandom() * zrg.width();

	ps += Pick::Location( SI().transform(bid), val );
    }

    gendef_.empty();
    return true;
}


void uiPickPartServer::setMisclassSet( const BinIDValueSet& bivs )
{
    static const char* sKeyMisClass = "Misclassified [NN]";
    int setidx = setmgr_.indexOf( sKeyMisClass );
    const bool isnew = setidx < 0;
    Pick::Set* ps = isnew ? 0 : &setmgr_.get( setidx );
    if ( ps )
	ps->erase();
    else
    {
	ps = new Pick::Set( sKeyMisClass );
	ps->disp_.color_.set( 240, 0, 0 );
    }

    BinIDValueSet::Pos pos; BinIDValue biv;
    while ( bivs.next(pos,false) )
    {
	bivs.get( pos, biv );
	Coord pos = SI().transform( biv.binid );
	*ps += Pick::Location( pos.x, pos.y, biv.value );
    }

    if ( isnew )
	storeNewSet( ps );
    else
    {
	PtrMan<IOObj> ioobj = getSetIOObj( *ps );
	if ( ioobj )
	    doStore( *ps, *ioobj );
    }
}
