/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
________________________________________________________________________

-*/

#include "uitoolbutton.h"
#include "i_qbutton.h"

#include "uiaction.h"
#include "uibuttongroup.h"
#include "uiicon.h"
#include "uimain.h"
#include "uimenu.h"
#include "uipixmap.h"
#include "uieventfilter.h"
#include "uitoolbar.h"
#include "uistrings.h"

#include "objdisposer.h"
#include "odiconfile.h"
#include "perthreadrepos.h"


#include <QApplication>
#include <QCheckBox>
#include <QEvent>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QResizeEvent>
#include <QToolButton>

mUseQtnamespace

bool uiButton::havecommonpbics_ = false;

// uiButton


/*
void uiCheckBoxBody::nextCheckState()
{
    Qt::CheckState state = checkState();
    if ( state==Qt::Unchecked )
	setCheckState( Qt::Checked );
    else
	setCheckState( Qt::Unchecked );
}
 */


#define muiButBody() dynamic_cast<uiButtonBody&>( *body() )

uiButton::uiButton( uiParent* parnt, const uiString& nm, const CallBack* cb )
    : uiSingleWidgetObject(parnt,nm.getFullString())
    , activated(this)
    , iconscale_(0.75)
    , text_(nm)
    , messenger_( 0 )
{
    if ( cb ) activated.notify(*cb);
    
    setHSzPol( uiObject::SmallVar );

    mDynamicCastGet(uiButtonGroup*,butgrp,parnt)
    if ( butgrp )
	butgrp->addButton( this );
    
    eventFilter()->addEventType( uiEventFilter::Resize );
    eventFilter()->addEventType( uiEventFilter::KeyPress );
    mAttachCB( eventFilter()->eventhappened, uiButton::eventCB );
}


uiButton::~uiButton()
{
    detachAllNotifiers();
    
    delete messenger_;
    CallBack::removeFromMainThread( this );
}


void uiButton::eventCB(CallBacker*)
{
    const uiEventFilter::EventType type = eventFilter()->getCurrentEventType();
    if ( type==uiEventFilter::Resize )
    {
        uiParent* hpar = parent();
        mDynamicCastGet(uiToolBar*,tb,hpar);
        if ( hpar && !tb )
            updateIconSize();
    }
    else if ( type==uiEventFilter::KeyPress )
    {
        mDynamicCastGet(QKeyEvent*, qke, eventFilter()->getCurrentEvent());
        if ( qke && qke->key() == Qt::Key_Space )
        {
            eventFilter()->setBlockEvent( true );
        }
    }
}

void uiButton::setButtonWidget( QAbstractButton* b )
{
    delete messenger_;
    messenger_ = new i_ButMessenger( b, this );
    b->setText( text_.getQString() );
    setSingleWidget( b );
    button_ = b;
}


void uiButton::setIcon( const char* iconnm )
{
    icon_ = iconnm;
    updateIconCB(0);
}


void uiButton::updateIconCB(CallBacker*)
{
    mEnsureExecutedInMainThread( uiButton::updateIconCB );
    
    uiIcon icon( icon_ );
    button_->setIcon( icon.qicon() );
    updateIconSize();
}


void uiButton::doTrigger()
{
    const int refnr = beginCmdRecEvent();
    activated.trigger();
    endCmdRecEvent( refnr );
}


void uiButton::setPM( const uiPixmap& pm )
{
    if ( !isMainThreadCurrent() )
	return;

    button_->setIcon( *pm.qpixmap() );
    updateIconSize();
}


void uiButton::setIconScale( float val )
{
    if ( val<=0.0f || val>1.0f )
	val = 1.0f;

    iconscale_ = val;
    updateIconSize();
}


void uiButton::setText( const uiString& txt )
{
    text_ = txt;
    button_->setText( text_.getQString() );
}


void uiButton::translateText()
{
    uiObject::translateText();
    button_->setText( text_.getQString() );
}


static uiButton* crStd( uiParent* p, OD::StdActionType typ,
	const CallBack& cb, bool immediate, const uiString* buttxt,
	bool pbics=false )
{
    uiString txt = uiString::emptyString();
    uiString tt = uiString::emptyString();
    const char* icid = 0;

#   define mGetDefs(typ) \
    case OD::typ: { \
	if ( !buttxt ) \
	    txt = uiStrings::s##typ(); \
	else if ( !buttxt->isEmpty() )\
	{ \
	    txt = *buttxt; \
	    tt = uiStrings::phrThreeDots( uiStrings::s##typ(), immediate ); \
	} \
	icid = OD::IconFile::getIdentifier( OD::typ ); \
    break; }

    switch( typ )
    {
	mGetDefs(Apply)
	mGetDefs(Cancel)
	mGetDefs(Define)
	mGetDefs(Delete)
	mGetDefs(Edit)
	mGetDefs(Examine)
	mGetDefs(Help)
	mGetDefs(Ok)
	mGetDefs(Options)
	mGetDefs(Properties)
	mGetDefs(Rename)
	mGetDefs(Remove)
	mGetDefs(Save)
	mGetDefs(SaveAs)
	mGetDefs(Select)
	mGetDefs(Settings)
	mGetDefs(Unload)
	default:
	break;
    }

    uiButton* ret = 0;
    if ( txt.isEmpty() )
	ret = new uiToolButton( p, icid, tt, cb );
    else
    {
	ret = new uiPushButton( p, txt, cb, immediate );
	if ( pbics )
	    ret->setIcon( icid );
    }

    return ret;
}

uiButton* uiButton::getStd( uiParent* p, OD::StdActionType typ,
	const CallBack& cb, bool immediate )
{
    return crStd( p, typ, cb, immediate, 0, havecommonpbics_ );
}

uiButton* uiButton::getStd( uiParent* p, OD::StdActionType typ,
	const CallBack& cb, bool immediate, const uiString& buttxt )
{
    return crStd( p, typ, cb, immediate, &buttxt, havecommonpbics_ );
}


#define mQBut(typ) dynamic_cast<Q##typ&>( *body() )

// uiPushButton
uiPushButton::uiPushButton( uiParent* parnt, const uiString& nm, bool ia )
    : uiButton( parnt, nm, 0 )
    , immediate_(ia)
    , pbbody_( new QPushButton( getParentWidget( parnt )))
{
    setButtonWidget( pbbody_ );
    updateText();
}


uiPushButton::uiPushButton( uiParent* parnt, const uiString& nm,
			    const CallBack& cb, bool ia )
    : uiButton( parnt, nm, &cb )
    , immediate_(ia)
    , pbbody_( new QPushButton( getParentWidget( parnt ) ) )
{
    setButtonWidget( pbbody_ );
    updateText();
}


uiPushButton::uiPushButton( uiParent* parnt, const uiString& nm,
			    const uiPixmap& pm, bool ia )
    : uiButton( parnt, nm, 0 )
    , immediate_(ia)
{
    setButtonWidget( pbbody_ );
    updateText();
    setPixmap( pm );
}


uiPushButton::uiPushButton( uiParent* parnt, const uiString& nm,
			    const uiPixmap& pm, const CallBack& cb, bool ia )
    : uiButton( parnt, nm, &cb )
    , immediate_(ia)
    , pbbody_( new QPushButton( getParentWidget( parnt ) ) )
{
    setButtonWidget( pbbody_ );
    updateText();
    setPixmap( pm );
}


void uiPushButton::setMenu( uiMenu* menu )
{
    pbbody_->setMenu( menu ? menu->getQMenu() : 0 );
}


void uiPushButton::setFlat( bool yn )
{
    pbbody_->setFlat( yn );
}


void uiPushButton::updateIconSize()
{
    const QString buttxt = text_.getQString();
    int butwidth = pbbody_->width();
    const int butheight = pbbody_->height();
    if ( !buttxt.isEmpty() )
	butwidth = butheight;

    pbbody_->setIconSize( QSize(mNINT32(butwidth*iconscale_),
                                mNINT32(butheight*iconscale_)) );
}


void uiPushButton::translateText()
{
    updateText();
}


void uiPushButton::updateText()
{
    QString newtext = text_.getQString();
    if ( !newtext.isEmpty() && !immediate_ )
	newtext.append( " ..." );

    pbbody_->setText( newtext );
}


void uiPushButton::setDefault( bool yn )
{
    pbbody_->setDefault( yn );
    setFocus();
}


void uiPushButton::click()
{
    activated.trigger();
}


// uiRadioButton
uiRadioButton::uiRadioButton( uiParent* p, const uiString& nm )
    : uiButton(p,nm,0)
    , rbbody_( new QRadioButton(nm.getQString(), getParentWidget(p)) )
{
    setButtonWidget( rbbody_ );
}


uiRadioButton::uiRadioButton( uiParent* p, const uiString& nm,
			      const CallBack& cb )
    : uiButton(p,nm,&cb)
    , rbbody_( new QRadioButton(nm.getQString(), getParentWidget(p)))
{
    setButtonWidget( rbbody_ );
}


bool uiRadioButton::isChecked() const
{
    return rbbody_->isChecked ();
}

void uiRadioButton::setChecked( bool check )
{
    mBlockCmdRec;
    rbbody_->setChecked( check );
}


void uiRadioButton::click()
{
    setChecked( !isChecked() );
    activated.trigger();
}


// uiCheckBox
uiCheckBox::uiCheckBox( uiParent* p, const uiString& nm )
    : uiButton(p,nm,0)
    , cbbody_( new QCheckBox( nm.getQString(), getParentWidget(p)))
{
    setButtonWidget( cbbody_ );
}


uiCheckBox::uiCheckBox( uiParent* p, const uiString& nm, const CallBack& cb )
    : uiButton(p,nm,&cb)
    , cbbody_( new QCheckBox( nm.getQString(), getParentWidget(p)))
{
    setButtonWidget( cbbody_ );
}


bool uiCheckBox::isChecked() const
{
    return cbbody_->isChecked();
}


void uiCheckBox::setChecked( bool yn )
{
    mBlockCmdRec;
    cbbody_->setChecked( yn );
}


void uiCheckBox::setTriState( bool yn )
{
    cbbody_->setTristate( yn );
}


void uiCheckBox::setCheckState( OD::CheckState cs )
{
    Qt::CheckState qcs = cs==OD::Unchecked ? Qt::Unchecked :
	(cs==OD::Checked ? Qt::Checked : Qt::PartiallyChecked);
    cbbody_->setCheckState( qcs );
}


OD::CheckState uiCheckBox::getCheckState() const
{
    Qt::CheckState qcs = cbbody_->checkState();
    return qcs==Qt::Unchecked ? OD::Unchecked :
	(qcs==Qt::Checked ? OD::Checked : OD::PartiallyChecked);
}


void uiCheckBox::click()
{
    setChecked( !isChecked() );
    activated.trigger();
}


uiButton* uiToolButtonSetup::getButton( uiParent* p, bool forcetb ) const
{
    const FixedString nm( name_.getFullString() );
    const bool istoolbut = nm == tooltip_.getFullString();
    if ( forcetb || istoggle_ || istoolbut )
	return new uiToolButton( p, *this );

    return getPushButton( p, true );
}


uiToolButton* uiToolButtonSetup::getToolButton( uiParent* p ) const
{
    return new uiToolButton( p, *this );
}


uiPushButton* uiToolButtonSetup::getPushButton( uiParent* p, bool wic ) const
{
    uiPushButton* ret = 0;
    if ( wic )
	ret = new uiPushButton( p, name_, uiPixmap(icid_), cb_, isimmediate_ );
    else
	ret = new uiPushButton( p, name_, cb_, isimmediate_ );
    ret->setToolTip( tooltip_ );
    return ret;
}


// For some reason it is necessary to set the preferred width. Otherwise the
// button will reserve +- 3 times it's own width, which looks bad

#define mSetDefPrefSzs() \
    mDynamicCastGet(uiToolBar*,tb,parnt) \
    if ( !tb ) setPrefWidth( prefVNrPics() );

#define mInitTBList \
    id_(-1), uimenu_(0), tbbody_( new QToolButton( getParentWidget(parnt) ) )

uiToolButton::uiToolButton( uiParent* parnt, const uiToolButtonSetup& su )
    : uiButton( parnt, su.name_, &su.cb_ )
    , mInitTBList
{
    setButtonWidget( tbbody_ );
    
    setToolTip( su.tooltip_ );
    setIcon( su.icid_ );
    setText( su.name_ );
    
    if ( su.istoggle_ )
    {
	setToggleButton( true );
	setOn( su.ison_ );
    }
    if ( su.arrowtype_ != NoArrow )
	setArrowType( su.arrowtype_ );
    if ( !su.shortcut_.isEmpty() )
	setShortcut( su.shortcut_ );

    mSetDefPrefSzs();
}


uiToolButton::uiToolButton( uiParent* parnt, const char* fnm,
			    const uiString& tt, const CallBack& cb )
    : uiButton( parnt, tt, &cb )
    , mInitTBList
{
    tbbody_->setFocusPolicy( Qt::ClickFocus );
    setButtonWidget( tbbody_ );
    
    setToolTip( tt );
    setIcon( fnm );
    
    mSetDefPrefSzs();
    setToolTip( tt );
}


uiToolButton::uiToolButton( uiParent* parnt, uiToolButton::ArrowType at,
			    const uiString& tt, const CallBack& cb )
    : uiButton( parnt, tt, &cb )
    , mInitTBList
{
    tbbody_->setFocusPolicy( Qt::ClickFocus );
    setButtonWidget( tbbody_ );
    setToolTip( tt );

    mSetDefPrefSzs();
    setArrowType( at );
    setToolTip( tt );
}


uiToolButton::~uiToolButton()
{
    delete uimenu_;
}


uiToolButton* uiToolButton::getStd( uiParent* p, OD::StdActionType typ,
				    const CallBack& cb, const uiString& tt )
{
    uiButton* but = uiButton::getStd( p, typ, cb, true,
					uiString::emptyString() );
    if ( !but )
	return 0;

    mDynamicCastGet(uiToolButton*,tb,but)
    if ( !tb )
	{ pFreeFnErrMsg("uiButton::getStd delivered PB"); return 0; }

    tb->setToolTip( tt );
    return tb;
}


bool uiToolButton::isOn() const { return tbbody_->isChecked(); }

void uiToolButton::setOn( bool yn )
{
    mBlockCmdRec;
    tbbody_->setChecked( yn );
}


bool uiToolButton::isToggleButton() const     { return tbbody_->isCheckable();}
void uiToolButton::setToggleButton( bool yn ) { tbbody_->setCheckable( yn ); }


void uiToolButton::click()
{
    if ( isToggleButton() )
	setOn( !isOn() );
    activated.trigger();
}


void uiToolButton::setArrowType( ArrowType type )
{
#ifdef __win__
    switch ( type )
    {
	case UpArrow: setPixmap( "uparrow" ); break;
	case DownArrow: setPixmap( "downarrow" ); break;
	case LeftArrow: setPixmap( "leftarrow" ); break;
	case RightArrow: setPixmap( "rightarrow" ); break;
    }
#else
    tbbody_->setArrowType( (Qt::ArrowType)(int)type );
#endif
}


void uiToolButton::setShortcut( const char* sc )
{
    tbbody_->setShortcut( QString(sc) );
}


void uiToolButton::setMenu( uiMenu* mnu, PopupMode mode )
{
    const bool hasmenu = mnu && mnu->nrActions() > 0;
    tbbody_->setMenu( hasmenu ? mnu->getQMenu() : 0 );

    mDynamicCastGet(uiToolBar*,tb,parent())
    if ( !tb )
    {
	if ( finalised() )
	{
	    QSize size = tbbody_->size();
	    const int wdth =
		hasmenu ? mCast(int,1.5*size.height()) : size.height();
	    size.setWidth( wdth );
	    tbbody_->resize( size );
	}
    }

    if ( !hasmenu ) mode = DelayedPopup;
    tbbody_->setPopupMode( (QToolButton::ToolButtonPopupMode)mode );

    OBJDISP()->go( uimenu_ );
    uimenu_ = mnu;
}
