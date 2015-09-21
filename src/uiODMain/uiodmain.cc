/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodmain.h"

#include "uiattribpartserv.h"
#include "uimain.h"
#include "uicolortable.h"
#include "uidockwin.h"
#include "uiempartserv.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiioobjseldlg.h"
#include "uistatusbar.h"
#include "uilabel.h"
#include "uimpepartserv.h"
#include "uimsg.h"
#include "uinlapartserv.h"
#include "uinotsaveddlg.h"
#include "uiodapplmgr.h"
#include "uiodmenumgr.h"
#include "uiodscenemgr.h"
#include "uiodviewer2dmgr.h"
#include "uipixmap.h"
#include "uipluginsel.h"
#include "uiseispartserv.h"
#include "uisetdatadir.h"
#include "uistrattreewin.h"
#include "uisurvey.h"
#include "uisurvinfoed.h"
#include "uitoolbar.h"
#include "ui2dsip.h"
#include "uiviscoltabed.h"
#include "uivispartserv.h"

#include "coltabsequence.h"
#include "ctxtioobj.h"
#include "envvars.h"
#include "ioman.h"
#include "ioobj.h"
#include "moddepmgr.h"
#include "mousecursor.h"
#include "nrbytes2string.h"
#include "oddirs.h"
#include "odinst.h"
#include "odplatform.h"
#include "odsessionfact.h"
#include "odsysmem.h"
#include "odver.h"
#include "plugins.h"
#include "ptrman.h"
#include "settingsaccess.h"
#include "survgeom.h"
#include "survinfo.h"
#include "timer.h"
#include "visdata.h"
#include "od_helpids.h"

#include <iostream>


extern "C" const char* GetSettingsDataDir();

static uiODMain* manODMainWin( uiODMain* i )
{
    mDefineStaticLocalObject( uiODMain*, theinst, = 0 );
    if ( i ) theinst = i;
    return theinst;
}


uiODMain* ODMainWin()
{
    return manODMainWin(0);
}


int ODMain( int argc, char** argv )
{
    OD::ModDeps().ensureLoaded( "AllNonUi" );
    OD::ModDeps().ensureLoaded( "uiBase" );
    uiDialog::setTitlePos( -1 );

    PtrMan<uiODMain> odmain = new uiODMain( *new uiMain(argc,argv) );
    manODMainWin( odmain );

    bool dodlg = true;
    Settings::common().getYN( uiPluginSel::sKeyDoAtStartup(), dodlg );
    ObjectSet<PluginManager::Data>& pimdata = PIM().getData();
    if ( dodlg && pimdata.size() )
    {
	uiPluginSel dlg( odmain );
	if ( dlg.nrPlugins() && !dlg.go() )
	    return 1;
    }

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiODMain" );
    PIM().loadAuto( true );
    if ( !odmain->ensureGoodSurveySetup() )
	return 1;

    odmain->initScene();
    odmain->go();
    return 0;
}


#define mMemStatusFld 4
static BufferString cputxt_;

uiODMain::uiODMain( uiMain& a )
    : uiMainWin(0,uiString::emptyString(),5,true)
    , uiapp_(a)
    , failed_(true)
    , applmgr_(0)
    , menumgr_(0)
    , scenemgr_(0)
    , ctabed_(0)
    , ctabwin_(0)
    , ctabtb_(0)
    , timer_(*new Timer("Session restore timer"))
    , memtimer_(*new Timer("Memory display timer"))
    , lastsession_(*new ODSession)
    , cursession_(0)
    , restoringsess_(false)
    , sessionSave(this)
    , sessionRestoreEarly(this)
    , sessionRestore(this)
    , justBeforeGo(this)
    , programname_( "OpendTect" )
{
    setIconText( getProgramString() );
    uiapp_.setTopLevel( this );

    if ( !ensureGoodDataDir()
      || (IOM().isBad() && !ensureGoodSurveySetup()) )
	::exit( 0 );

    applmgr_ = new uiODApplMgr( *this );
    if ( buildUI() )
	failed_ = false;

    IOM().afterSurveyChange.notify( mCB(this,uiODMain,afterSurveyChgCB) );
    timer_.tick.notify( mCB(this,uiODMain,timerCB) );

    const int systemnrcpus = Threads::getSystemNrProcessors();
    const int odnrcpus = Threads::getNrProcessors();
    const bool useallcpus = systemnrcpus == odnrcpus;

    uiString statustt = tr( "System memory: Free/Available" );
    if ( !useallcpus )
	statustt.append( tr("| CPU: Used/Available") );
    statusBar()->setToolTip( mMemStatusFld, statustt );
    statusBar()->setTxtAlign( mMemStatusFld, Alignment::HCenter );
    memtimer_.tick.notify( mCB(this,uiODMain,memTimerCB) );
    memtimer_.start( 1000 );

    if ( !useallcpus )
	cputxt_.set( "[cpu] " ).add( odnrcpus ).add( "/" ).add( systemnrcpus );
    else
	cputxt_.setEmpty();
}


uiODMain::~uiODMain()
{
    memtimer_.stop();
    if ( ODMainWin()==this )
	manODMainWin( 0 );

    delete ctabwin_;
    delete &lastsession_;
    delete &timer_;
    delete &memtimer_;

    delete menumgr_;
    delete viewer2dmgr_;
    delete scenemgr_;
    delete applmgr_;
}



bool uiODMain::ensureGoodDataDir()
{
    while ( !IOMan::isValidDataRoot(GetBaseDataDir()) )
    {
	uiSetDataDir dlg( this );
	if ( !dlg.go() )
	{
	    if ( uiMSG().askGoOn( tr("Without a valid data root, %1 "
				     "cannot start.\nDo you wish to exit?")
					.arg(programname_) ) )
		return false;
	}
	else if ( uiSetDataDir::setRootDataDir(this,dlg.selectedDir()) )
	    break;
    }

    return true;
}


bool uiODMain::ensureGoodSurveySetup()
{
    BufferString errmsg;
    if ( !IOMan::validSurveySetup(errmsg) )
    {
	std::cerr << errmsg << std::endl;
	uiMSG().error( errmsg );
	return false;
    }
    else if ( !IOM().isReady() )
    {
	while ( !uiODApplMgr::manageSurvey() )
	{
	    if ( uiMSG().askGoOn( tr("Without a valid survey, %1 "
				     "cannot start.\nDo you wish to exit?")
				     .arg( programname_ )) )
		return false;
	}
    }

    return true;
}


bool uiODMain::buildUI()
{
    scenemgr_ = new uiODSceneMgr( this );
    viewer2dmgr_ = new uiODViewer2DMgr( this );
    menumgr_ = new uiODMenuMgr( this );
    menumgr_->initSceneMgrDepObjs( applmgr_, scenemgr_ );

    uiColorTableToolBar* tb = new uiColorTableToolBar( this );
    ctabtb_ = tb;
    ctabed_ = new uiVisColTabEd( *tb );
    ctabed_->seqChange().notify( mCB(applmgr_,uiODApplMgr,colSeqChg) );
    ctabed_->mapperChange().notify( mCB(applmgr_,uiODApplMgr,colMapperChg));

    return true;
}


void uiODMain::initScene()
{
    scenemgr_->initMenuMgrDepObjs();
    readSettings();

    justBeforeGo.trigger();
}


IOPar& uiODMain::sessionPars()
{
    return cursession_->pluginpars();
}


CtxtIOObj* uiODMain::getUserSessionIOData( bool restore )
{
    CtxtIOObj* ctio = mMkCtxtIOObj(ODSession);
    ctio->ctxt_.forread_ = restore;
    ctio->setObj( cursessid_ );
    uiIOObjSelDlg dlg( this, *ctio );
    if ( !dlg.go() )
	{ delete ctio->ioobj_; delete ctio; ctio = 0; }
    else
    {
	delete ctio->ioobj_; ctio->ioobj_ = dlg.ioObj()->clone();
	const MultiID id( ctio->ioobj_ ? ctio->ioobj_->key() : MultiID("") );
	cursessid_ = id;
    }

    return ctio;
}


static bool hasSceneItems( uiVisPartServer* visserv )
{
    TypeSet<int> sceneids;
    visserv->getChildIds( -1, sceneids );
    if ( sceneids.isEmpty() ) return false;

    int nrchildren = 0;
    TypeSet<int> visids;
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visserv->getChildIds( sceneids[0], visids );
	nrchildren += visids.size();
    }

    return nrchildren>sceneids.size()*3;
}


bool uiODMain::hasSessionChanged()
{
    if ( !hasSceneItems(applMgr().visServer()) )
	return false;

    ODSession sess;
    cursession_ = &sess;
    updateSession();
    cursession_ = &lastsession_;
    return !( sess == lastsession_ );
}


#define mDelCtioRet()	{ delete ctio->ioobj_; delete ctio; return; }

void uiODMain::saveSession()
{
    CtxtIOObj* ctio = getUserSessionIOData( false );
    if ( !ctio ) { delete ctio; return; }
    ODSession sess; cursession_ = &sess;
    if ( !updateSession() ) mDelCtioRet()
    uiString bs;
    if ( !ODSessionTranslator::store(sess,ctio->ioobj_,bs) )
	{ uiMSG().error( bs ); mDelCtioRet() }

    lastsession_ = sess; cursession_ = &lastsession_;
    mDelCtioRet()
}


void uiODMain::restoreSession()
{
    CtxtIOObj* ctio = getUserSessionIOData( true );
    if ( !ctio ) { delete ctio; return; }
    restoreSession( ctio->ioobj_ );
    mDelCtioRet()
}


class uiODMainAutoSessionDlg : public uiDialog
{
public:

uiODMainAutoSessionDlg( uiODMain* p )
    : uiDialog(p,uiDialog::Setup("Auto-load session",mNoDlgTitle,
                                 mODHelpKey(mODMainAutoSessionDlgHelpID) ))
{
    bool douse = false; MultiID id;
    ODSession::getStartupData( douse, id );

    usefld_ = new uiGenInput( this, "Auto-load session mode",
			      BoolInpSpec(douse,"Enabled","Disabled") );
    usefld_->valuechanged.notify( mCB(this,uiODMainAutoSessionDlg,useChg) );
    doselfld_ = new uiGenInput( this, "Use one for this survey",
			        BoolInpSpec(!id.isEmpty()) );
    doselfld_->valuechanged.notify( mCB(this,uiODMainAutoSessionDlg,useChg) );
    doselfld_->attach( alignedBelow, usefld_ );

    IOObjContext ctxt = mIOObjContext( ODSession );
    ctxt.forread_ = true;
    sessionfld_ = new uiIOObjSel( this, ctxt );
    sessionfld_->setInput( id );
    sessionfld_->attach( alignedBelow, doselfld_ );

    loadnowfld_ = new uiGenInput( this, "Load selected session now",
				  BoolInpSpec(true) );
    loadnowfld_->attach( alignedBelow, sessionfld_ );

    postFinalise().notify( mCB(this,uiODMainAutoSessionDlg,useChg) );
}

void useChg( CallBacker* )
{
    const bool douse = usefld_->getBoolValue();
    const bool dosel = douse ? doselfld_->getBoolValue() : false;
    doselfld_->display( douse );
    sessionfld_->display( dosel );
    loadnowfld_->display( dosel );
}


bool acceptOK( CallBacker* )
{
    const bool douse = usefld_->getBoolValue();
    const bool dosel = douse ? doselfld_->getBoolValue() : false;
    if ( !dosel )
    {
	ODSession::setStartupData( douse, MultiID::udf() );
	return true;
    }

    const IOObj* ioobj = sessionfld_->ioobj();
    if ( !ioobj ) return false;

    ODSession::setStartupData( douse, sessionfld_->key() );
    return true;
}

    uiGenInput*		usefld_;
    uiGenInput*		doselfld_;
    uiIOObjSel*		sessionfld_;
    uiGenInput*		loadnowfld_;
};


void uiODMain::autoSession()
{
    uiODMainAutoSessionDlg dlg( this );
    if ( dlg.go() )
    {
	if ( dlg.loadnowfld_->getBoolValue() )
	    handleStartupSession();
    }
}


void uiODMain::restoreSession( const IOObj* ioobj )
{
    ODSession sess; uiString bs;
    if ( !ODSessionTranslator::retrieve(sess,ioobj,bs) )
	{ uiMSG().error( bs ); return; }

    cursession_ = &sess;
    doRestoreSession();
    cursession_ = &lastsession_; lastsession_.clear();
    timer_.start( 200, true );
    sceneMgr().setToViewMode( true );
    sceneMgr().updateTrees();
}


bool uiODMain::updateSession()
{
    cursession_->clear();
    applMgr().EMServer()->fillPar( cursession_->empars() );
    applMgr().seisServer()->fillPar( cursession_->seispars() );
    applMgr().visServer()->fillPar( cursession_->vispars() );
    applMgr().attrServer()->fillPar( cursession_->attrpars(true,false),
				     true, false );
    applMgr().attrServer()->fillPar( cursession_->attrpars(true, true),
				     true, true );
    applMgr().attrServer()->fillPar( cursession_->attrpars(false, false),
				     false, false );
    applMgr().attrServer()->fillPar( cursession_->attrpars(false, true),
				     false, true );
    sceneMgr().getScenePars( cursession_->scenepars() );
    if ( applMgr().nlaServer()
      && !applMgr().nlaServer()->fillPar( cursession_->nlapars() ) )
	return false;
    applMgr().mpeServer()->fillPar( cursession_->mpepars() );
    viewer2DMgr().fillPar( cursession_->vwr2dpars() );

    sessionSave.trigger();
    return true;
}


void uiODMain::doRestoreSession()
{
    MouseCursorManager::setOverride( MouseCursor::Wait );
    sceneMgr().cleanUp( false );
    applMgr().resetServers();
    restoringsess_ = true;

    sessionRestoreEarly.trigger();
    applMgr().EMServer()->usePar( cursession_->empars() );
    applMgr().seisServer()->usePar( cursession_->seispars() );
    if ( applMgr().nlaServer() )
	applMgr().nlaServer()->usePar( cursession_->nlapars() );
    if ( SI().has2D() )
    {
	applMgr().attrServer()->usePar( cursession_->attrpars(true,false),
					true, false );
	applMgr().attrServer()->usePar( cursession_->attrpars(true,true),
					true, true );
    }
    if ( SI().has3D() )
    {
	applMgr().attrServer()->usePar( cursession_->attrpars(false,false),
					false, false );
	applMgr().attrServer()->usePar( cursession_->attrpars(false,true),
					false, true );
    }
    applMgr().mpeServer()->usePar( cursession_->mpepars() );
    const bool visok = applMgr().visServer()->usePar( cursession_->vispars() );
    if ( visok )
	sceneMgr().useScenePars( cursession_->scenepars() );

    sessionRestore.trigger();
    if ( visok )
	applMgr().visServer()->calculateAllAttribs();
    else
    {
	MouseCursorManager::restoreOverride();
	uiMSG().error( tr("An error occurred while reading session file.\n"
		          "A new scene will be launched") );
	MouseCursorManager::setOverride( MouseCursor::Wait );
	sceneMgr().cleanUp( true );
    }
    if ( visok )
	viewer2DMgr().usePar( cursession_->vwr2dpars() );

    restoringsess_ = false;
    MouseCursorManager::restoreOverride();
}


void uiODMain::handleStartupSession()
{
    bool douse = false;
    MultiID id;
    ODSession::getStartupData( douse, id );
    if ( !douse || id.isEmpty() )
	return;

    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) return;
    cursessid_ = id;
    restoreSession( ioobj );
}


void uiODMain::timerCB( CallBacker* )
{
    sceneMgr().layoutScenes();
}


void uiODMain::translateText()
{ applMgr().sceneMgr().translateText(); }


void uiODMain::memTimerCB( CallBacker* )
{
    mDefineStaticLocalObject( Threads::Lock, memtimerlock, (true) );
    Threads::Locker locker( memtimerlock, Threads::Locker::DontWaitForLock );
    if ( !locker.isLocked() )
	return;

    od_int64 tot, free;
    OD::getSystemMemory( tot, free );
    NrBytesToStringCreator converter;
    converter.setUnitFrom( tot );
    uiString txt = tr( "[free mem] %1/%2%3" );

    //Use separate calls to avoid the reuse of the converter's buffer
    txt.arg( converter.getString( free, 1, false ) );
    txt.arg( converter.getString( tot,1,true ));
    txt.arg( cputxt_.isEmpty() ? cputxt_ : toUiString(" | %1").arg(cputxt_));

    statusBar()->message( txt, mMemStatusFld );
}


bool uiODMain::go()
{
    if ( failed_ ) return false;

    show();
    Timer tm( "Handle startup session" );
    tm.tick.notify( mCB(this,uiODMain,afterSurveyChgCB) );
    tm.start( 200, true );
    int rv = uiapp_.exec();
    return rv ? false : true;
}


void uiODMain::setProgramName( const char* nm )
{
    programname_ = nm;
    setIconText( getProgramString() );
    updateCaption();
}


bool uiODMain::askStore( bool& askedanything, const uiString& actiontype )
{
    if ( !applmgr_->attrServer() ) return false;

    if ( !NotSavedPrompter::NSP().doTrigger( uiMainWin::activeWindow(), true,
					     actiontype ) )
	return false;

    bool doask = false;
    Settings::common().getYN( "dTect.Ask store session", doask );
    if ( doask && hasSessionChanged() )
    {
	askedanything = true;
	int res = uiMSG().askSave( tr("Do you want to save this session?"),
                                   true );
	if ( res == 1 )
	    saveSession();
	else if ( res == -1 )
	    return false;
    }

    if ( SI().has2D() && !askStoreAttribs( true, askedanything ) )
	return false;
    if ( SI().has3D() && !askStoreAttribs( false, askedanything ) )
	return false;

    return true;
}


bool uiODMain::askStoreAttribs( bool is2d, bool& askedanything )
{
    bool doask = true;
    Settings::common().getYN( "dTect.Ask store attributeset", doask );
    if ( doask && !applmgr_->attrServer()->setSaved( is2d ) )
    {
	askedanything = true;
	int res = uiMSG().askSave( tr("Current attribute set has changed.\n"
				      "Store the changes now?") );
	if ( res == 1 )
	    applmgr_->attrServer()->saveSet( is2d );
	else if ( res == -1 )
	    return false;
    }

    return true;
}


void uiODMain::afterSurveyChgCB( CallBacker* )
{
    updateCaption();
    handleStartupSession();
}


void uiODMain::updateCaption()
{
    uiString capt = uiString( "%1/%2" )
	.arg( getProgramString() )
	.arg( OD::Platform::local().shortName() );

    if ( ODInst::getAutoInstType() == ODInst::InformOnly
	&& ODInst::updatesAvailable() )
	capt.append( tr(" *UPDATE AVAILABLE*") );

    const char* usr = GetSoftwareUser();
    if ( usr && *usr )
    {
	capt.append( tr(" [%1] ").arg( usr ) );
    }

    if ( !SI().name().isEmpty() )
	capt.append( ": %1" ).arg( SI().name() );

    setCaption( capt );
}


bool uiODMain::closeOK()
{
    saveSettings();

    bool askedanything = false;
    if ( !askStore(askedanything,uiString( "%1 %2")
		   .arg( uiStrings::sClose() ).arg( programname_ ) ) )
    {
	uiMSG().message(tr("Closing cancelled"));
	return false;
    }

    if ( failed_ ) return true;

    if ( !askedanything )
    {
	bool doask = true;
	Settings::common().getYN( "dTect.Ask close", doask );
	if ( doask && !uiMSG().askGoOn( tr("Do you want to close %1?")
				       .arg(programname_)) )
	    return false;
    }

    IOM().applClosing();

    removeDockWindow( ctabwin_ );
    timer_.tick.remove( mCB(this,uiODMain,timerCB) );
    delete menumgr_; menumgr_ = 0;
    delete scenemgr_; scenemgr_ = 0;

    return true;
}


uiString uiODMain::getProgramString() const
{
    return uiString( "%1 V%2" ).arg( programname_ ).arg( GetFullODVersion() );
}


void uiODMain::exit()
{
    if ( !closeOK() ) return;

    uiapp_.exit(0);
}
