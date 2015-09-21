/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiseiswvltman.h"

#include "ioman.h"
#include "ioobj.h"
#include "survinfo.h"
#include "wavelet.h"
#include "waveletio.h"
#include "waveletattrib.h"

#include "uiaxishandler.h"
#include "uitoolbutton.h"
#include "uifunctiondisplay.h"
#include "uilabel.h"
#include "uiioobjselgrp.h"
#include "uiioobjmanip.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseiswvltattr.h"
#include "uiseiswvltgen.h"
#include "uiseiswvltimpexp.h"
#include "uiselobjothersurv.h"
#include "uistrings.h"
#include "uitextedit.h"
#include "uiwaveletextraction.h"
#include "uiwaveletmatchdlg.h"
#include "od_helpids.h"


#define mErrRet(s) { uiMSG().error(s); return; }

mDefineInstanceCreatedNotifierAccess(uiSeisWvltMan)


uiSeisWvltMan::uiSeisWvltMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup(uiStrings::sManWav(),mNoDlgTitle,
                                     mODHelpKey(mSeisWvltManHelpID) )
				     .nrstatusflds(1).modal(false),
		   WaveletTranslatorGroup::ioContext() )
    , wvltext_(0)
    , wvltpropdlg_(0)
{
    createDefaultUI();

    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();
    manipgrp->addButton( "impfromothsurv", "Get from other survey",
			mCB(this,uiSeisWvltMan,getFromOtherSurvey) );
    disppropbut_ = manipgrp->addButton( "info", "Display properties",
				mCB(this,uiSeisWvltMan,dispProperties) );
    revpolbut_ = manipgrp->addButton( "revpol", "Reverse polarity",
				mCB(this,uiSeisWvltMan,reversePolarity) );
    rotatephbut_  = manipgrp->addButton( "phase", "Rotate phase",
				mCB(this,uiSeisWvltMan,rotatePhase) );
    taperbut_ = manipgrp->addButton( "wavelet_taper", "Taper",
				     mCB(this,uiSeisWvltMan,taper) );
    addButtons();
    uiGroup* wvltdispgrp = new uiGroup( listgrp_,"Wavelet Display" );
    wvltdispgrp->attach( rightOf, selgrp_ );

    uiFunctionDisplay::Setup fdsu;
    fdsu.noy2axis(true).noy2gridline(true);
    
    waveletdisplay_ = new uiFunctionDisplay( wvltdispgrp, fdsu );
    const BufferString ztxt( (SI().zIsTime() ? "Time " : "Depth "), 
			      SI().getZUnitString() );
    waveletdisplay_->xAxis()->setCaption( ztxt );
    waveletdisplay_->yAxis(false)->setCaption( uiStrings::sAmplitude() );

    wvnamdisp_ = new uiLabel( wvltdispgrp, "Wavelet" );
    wvnamdisp_->attach(centeredAbove, waveletdisplay_);
    wvnamdisp_->setAlignment( Alignment::HCenter );
      
    selChg( this );
    mTriggerInstanceCreatedNotifier();
    windowClosed.notify( mCB(this,uiSeisWvltMan,closeDlg) );
}


uiSeisWvltMan::~uiSeisWvltMan()
{
    if ( wvltext_ )
	wvltext_->extractionDone.remove( mCB(this,uiSeisWvltMan,updateCB) );

    delete wvltext_;

    if ( wvltpropdlg_ )
	delete wvltpropdlg_;
}


static void reSampleWavelet( Wavelet &resampledwvlt )
{
    const float minstepval = 1.f/SI().zDomain().userFactor();
    resampledwvlt.reSample(minstepval);
}


void uiSeisWvltMan::addButtons()
{
    uiButtonGroup* grp = extraButtonGroup();
    uiPushButton* impbut = new uiPushButton( grp, uiStrings::sImport(), false );
    impbut->activated.notify( mCB(this,uiSeisWvltMan,impPush) );

    uiPushButton* crbut = new uiPushButton( grp, tr("Generate"), false );
    crbut->activated.notify( mCB(this,uiSeisWvltMan,crPush) );
    crbut->attach( rightOf, impbut );

    uiPushButton* mergebut = new uiPushButton( grp, tr("Stack"), false );
    mergebut->activated.notify( mCB(this,uiSeisWvltMan,mrgPush) );
    mergebut->attach( rightOf, crbut );

    uiPushButton* extractbut = new uiPushButton( grp, tr("Extract"), false );
    extractbut->activated.notify( mCB(this,uiSeisWvltMan,extractPush) );
    extractbut->attach( rightOf, mergebut );

    uiPushButton* matchbut = new uiPushButton( grp, tr("Match"), false );
    matchbut->activated.notify( mCB(this,uiSeisWvltMan,matchPush) );
    matchbut->attach( rightOf, extractbut );
}


void uiSeisWvltMan::impPush( CallBacker* )
{
    uiSeisWvltImp dlg( this );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.selKey() );
}


void uiSeisWvltMan::crPush( CallBacker* )
{
    uiSeisWvltGen dlg( this );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.storeKey() );
}


void uiSeisWvltMan::mrgPush( CallBacker* )
{
    if ( selgrp_->getListField()->size()<2 )
	mErrRet( tr("At least two wavelets are needed to merge wavelets") );

    uiSeisWvltMerge dlg( this, curioobj_ ? curioobj_->name().str() : 0 );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.storeKey() );
}


void uiSeisWvltMan::extractPush( CallBacker* cb )
{
    bool is2d = SI().has2D();
    if ( is2d && SI().has3D() )
    {
	int res = uiMSG().askGoOnAfter( tr("Use 2D or 3D data?"),
		0, uiStrings::s2D(), uiStrings::s3D() );
	if ( res == -1 )
	    return;
	else
	    is2d = res == 1;
    }

    wvltext_ = new uiWaveletExtraction( parent(), is2d );
    wvltext_->extractionDone.notify( mCB(this,uiSeisWvltMan,updateCB) );
    wvltext_->show();
}


void uiSeisWvltMan::matchPush( CallBacker* )
{
    uiWaveletMatchDlg dlg( this );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.getMultiID() );
}


void uiSeisWvltMan::updateCB( CallBacker* )
{
    selgrp_->fullUpdate( wvltext_->storeKey() );
}


void uiSeisWvltMan::closeDlg( CallBacker* )
{
   if ( !wvltext_ ) return;
   wvltext_->close();
}


#define mSetButToolTip(but,str1,curwvltnm,str2,deftt) \
    if ( !but->sensitive() ) \
	but->setToolTip( deftt ); \
    else \
    { \
	tt.setEmpty(); \
	tt.add( str1 ).add( " '" ).add( curwvltnm ).add( "' " ).add( str2 ); \
	but->setToolTip( tr(tt) ); \
    }

void uiSeisWvltMan::ownSelChg()
{
    BufferString tt,curwvlt;
    if ( curioobj_ )
	curwvlt.add( curioobj_->name() );

    revpolbut_->setSensitive( curioobj_ );
    rotatephbut_->setSensitive( curioobj_ );
    taperbut_->setSensitive( curioobj_ );
    disppropbut_->setSensitive( curioobj_ );

    mSetButToolTip(revpolbut_,"Reverse", curwvlt, "polarity",
		   "Reverse polarity");
    mSetButToolTip(rotatephbut_,"Rotate", curwvlt, "phase","Rotate phase");
    mSetButToolTip(taperbut_,"Taper", curwvlt,"", "Taper" );
    mSetButToolTip(disppropbut_,"Display", curwvlt, "properties",
		   "Display properties");
}


void uiSeisWvltMan::mkFileInfo()
{
    BufferString txt;
    Wavelet* wvlt = Wavelet::get( curioobj_ );
    dispWavelet( wvlt );
    if ( wvlt )
    {
	const float zfac = mCast( float, SI().zDomain().userFactor() );
	WaveletAttrib wvltattrib( *wvlt );

	BufferString msg;
	msg.add( "Number of samples: " ).add( wvlt->size() ).addNewLine();
	msg.add( "Sample interval " ).add( SI().getZUnitString(true) )
	   .add( ": " ).add( wvlt->sampleRate() * zfac ).addNewLine();
	Interval<float> extremevals;
	wvlt->getExtrValues( extremevals );
	msg.add( "Min/Max amplitude: " ).add( extremevals.start )
	   .add( "/" ).add( extremevals.stop ).addNewLine();
	float avgphase = wvltattrib.getAvgPhase( true );
	if ( mIsZero(avgphase,1e-3f) ) avgphase = 0.f;
	msg.add( "Average phase (deg): ").add( avgphase, 2 ).addNewLine();
	txt.add( msg );
	delete wvlt;

	MultiID orgid; MultiID horid; MultiID seisid; BufferString lvlnm;
	if ( Wavelet::isScaled(curioobj_->key(),orgid,horid,seisid,lvlnm) )
	{
	    msg = "Scaled: ";
	    if ( orgid == MultiID("0") )
		msg.add( "Outside OpendTect" );
	    else
	    {
		msg.add( "'").add( IOM().nameOf(orgid) ).add( "'" );
		msg.add( " scaled to '").add( IOM().nameOf(seisid) ).add( "'" );
		msg.add( "\n\t(along '").add( IOM().nameOf(horid) ).add( "'" );
		msg.add( " at '").add( lvlnm ).add( "')" );
	    }
	    txt.add( msg ).addNewLine();
	}
    }

    txt += getFileInfo();
    setInfo( txt );
}


void uiSeisWvltMan::dispProperties( CallBacker* )
{
    Wavelet* wvlt = Wavelet::get( curioobj_ );
    if ( !wvlt ) return;
    
    wvlt->setName( curioobj_->name().buf() );

    Wavelet resampledwvlt( *wvlt );
    reSampleWavelet( resampledwvlt );
    wvltpropdlg_ = new uiWaveletDispPropDlg( this, resampledwvlt );
    if ( wvltpropdlg_ ->go() )
    { delete wvltpropdlg_; wvltpropdlg_ = 0; }

    delete wvlt;
}


#define mRet(s) \
	{ ctio.setObj(0); if ( s ) uiMSG().error(s); return; }

void uiSeisWvltMan::getFromOtherSurvey( CallBacker* )
{
    CtxtIOObj ctio( mIOObjContext(Wavelet) );
    ctio.ctxt_.forread_ = true;

    uiSelObjFromOtherSurvey dlg( this, ctio );
    dlg.setHelpKey(mODHelpKey(mSeisWvltMangetFromOtherSurveyHelpID) );
    Wavelet* wvlt = 0;
    bool didsel = true;
    if ( dlg.go() )
	wvlt = Wavelet::get( ctio.ioobj_ );
    else
	didsel = false;

    dlg.setDirToCurrentSurvey();
    if ( !wvlt )
	mRet((didsel ? "Could not read wavelet" : 0))
    IOM().getEntry( ctio );
    if ( !ctio.ioobj_ )
	mRet("Cannot create new entry in Object Management")
    else if ( !wvlt->put(ctio.ioobj_) )
	mRet("Cannot write wavelet to disk")

    selgrp_->fullUpdate( ctio.ioobj_->key() );
    mRet( 0 )
}


void uiSeisWvltMan::reversePolarity( CallBacker* )
{
    Wavelet* wvlt = Wavelet::get( curioobj_ );
    if ( !wvlt ) return;
    
    float* samps = wvlt->samples();
    for ( int idx=0; idx<wvlt->size(); idx++ )
	samps[idx] *= -1;

    if ( !wvlt->put(curioobj_) )
	uiMSG().error(tr("Cannot write new polarity reversed wavelet to disk"));
    else
	selgrp_->fullUpdate( curioobj_->key() );

    delete wvlt;
}


void uiSeisWvltMan::rotatePhase( CallBacker* )
{
    Wavelet* wvlt = Wavelet::get( curioobj_ );
    if ( !wvlt ) return;

    uiSeisWvltRotDlg dlg( this, *wvlt );
    dlg.setCaption( curioobj_->name() );
    dlg.acting.notify( mCB(this,uiSeisWvltMan,rotUpdateCB) );
    if ( dlg.go() )
    {
	if ( !wvlt->put(curioobj_) )
	    uiMSG().error(tr("Cannot write rotated phase wavelet to disk"));
	else
	    selgrp_->fullUpdate( curioobj_->key() );
    }

    dlg.acting.remove( mCB(this,uiSeisWvltMan,rotUpdateCB) );
    mkFileInfo();

    delete wvlt;
}


void uiSeisWvltMan::taper( CallBacker* )
{
    Wavelet* wvlt = Wavelet::get( curioobj_ );
    if ( !wvlt ) return;
    
    uiSeisWvltTaperDlg dlg( this, *wvlt );
    BufferString title( "Taper '", curioobj_->name(), "'" );
    dlg.setCaption( title.buf() );
    if ( dlg.go() )
    {
	if ( !wvlt->put(curioobj_) )
	    uiMSG().error(tr("Cannot write tapered wavelet to disk"));
	else
	    selgrp_->fullUpdate( curioobj_->key() );
    }
}


#define mErr() mErrRet( "Cannot draw wavelet" );

void uiSeisWvltMan::rotUpdateCB( CallBacker* cb )
{
    mDynamicCastGet(uiSeisWvltRotDlg*,dlg,cb);
    if ( !dlg ) mErr();
    
    const Wavelet* wvlt = dlg->getWavelet();
    if ( !wvlt ) mErr();

    dispWavelet( wvlt );
}


void uiSeisWvltMan::dispWavelet( const Wavelet* wvlt )
{
    wvnamdisp_->setText( curioobj_->name() );
    wvnamdisp_->setPrefWidthInChar( 60 );
    if( !wvlt )
    {
	waveletdisplay_->setEmpty();
	return;
    }	
    Wavelet resampledwvlt( *wvlt );
    reSampleWavelet( resampledwvlt );
    const int wvltsz = resampledwvlt.size();
    StepInterval<float> intxval;
    intxval.setFrom( resampledwvlt.samplePositions() );
    const float zfac = mCast(float,SI().zDomain().userFactor());
    intxval.scale( zfac );
    waveletdisplay_->setVals( intxval, resampledwvlt.samples() , wvltsz );
} 
