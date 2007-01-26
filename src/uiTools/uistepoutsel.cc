/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uistepoutsel.cc,v 1.6 2007-01-26 12:00:29 cvsbert Exp $";

#include "uistepoutsel.h"
#include "uispinbox.h"
#include "uilabel.h"


inline static BufferString mkPrefx( const char* lbl )
{
    BufferString ret( lbl );
    if ( !ret.isEmpty() )
	ret += ":";
    return ret;
}


uiStepOutSel::uiStepOutSel( uiParent* p, const uiStepOutSel::Setup& setup )
    : uiGroup(p,setup.seltxt_)
    , valueChanged(this)
    , fld2(0)
{
    init( setup );
}


uiStepOutSel::uiStepOutSel( uiParent* p, bool single, const char* seltxt )
    : uiGroup(p,seltxt)
    , valueChanged(this)
    , fld2(0)
{
    uiStepOutSel::Setup setup( single );
    setup.seltxt( seltxt );
    init( setup );
}


void uiStepOutSel::init( const uiStepOutSel::Setup& setup )
{
    const StepInterval<int> intv( setup.allowneg_ ? -999 : 0, 999, 1 );

    uiLabel* lbl = new uiLabel( this, setup.seltxt_ );

    fld1 = new uiSpinBox( this, 0, "spinbox 1" );
    fld1->setPrefix( mkPrefx(setup.lbl1_) );
    fld1->attach( rightOf, lbl );
    fld1->setInterval( intv );
    fld1->valueChanged.notify( mCB(this,uiStepOutSel,valChg) );

    if ( !setup.single_ )
    {
	fld2 = new uiSpinBox( this, 0, "spinbox 2" );
	fld2->setPrefix( mkPrefx(setup.lbl2_) );
	fld2->attach( rightOf, fld1 );
	fld2->setInterval( intv );
	fld2->valueChanged.notify( mCB(this,uiStepOutSel,valChg) );
    }

    setHAlignObj( fld1 );
}


int uiStepOutSel::val( bool dir1 ) const
{
    return dir1 ? fld1->getValue() : (fld2 ? fld2->getValue() : mUdf(int));
}


void uiStepOutSel::setVal( bool dir1, int v )
{
    if ( dir1 )
	fld1->setValue( v );
    else if ( fld2 )
	fld2->setValue( v );
}


void uiStepOutSel::setVals( int v )
{
    fld1->setValue( v );
    if ( fld2 )
	fld2->setValue( v );
}


bool uiStepOutSel::dir2Active() const
{
    return fld2 && fld2->sensitive();
}


void uiStepOutSel::setBinID( const BinID& bid )
{
    if ( dir2Active() )
	{ setVal(true,bid.inl); setVal(false,bid.crl); }
    else
	setVal(true,bid.crl);
}


BinID uiStepOutSel::getBinID() const
{
    return dir2Active() ? BinID( val(true), val(false) )
			: BinID( 0, val(true) );
}


void uiStepOutSel::valChg( CallBacker* cb )
{
    valueChanged.trigger( cb );
}
