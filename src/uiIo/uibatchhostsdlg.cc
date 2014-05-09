/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uibatchhostsdlg.h"

#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitable.h"
#include "uitoolbutton.h"

#include "file.h"
#include "hostdata.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "systeminfo.h"


static const int sIPCol		= 0;
static const int sHostNameCol	= 1;
static const int sDispNameCol	= 2;
static const int sPlfCol	= 3;
static const int sDataRootCol	= 4;


uiBatchHostsDlg::uiBatchHostsDlg( uiParent* p )
    : uiDialog(p,Setup("Setup Multi Machine Processing",mNoDlgTitle,
		       mODHelpKey(mBatchHostsDlgHelpID)))
    , hostdatalist_(*new HostDataList(true,false))
{
    const FilePath bhfp = hostdatalist_.getBatchHostsFilename();
    const BufferString bhfnm = bhfp.fullPath();
    bool writeallowed = true;
    if ( File::exists(bhfnm) )
	writeallowed = File::isWritable( bhfnm );
    else
    {
	const BufferString& datadir = bhfp.pathOnly();
	writeallowed = File::isWritable( datadir );
    }

    if ( writeallowed )
	setOkText( uiStrings::sSave(true) );
    else
	setCtrlStyle( CloseOnly );

    uiGenInput* filefld = new uiGenInput( this, tr("BatchHosts file") );
    filefld->setElemSzPol( uiObject::Wide );
    filefld->setText( bhfnm );
    filefld->setReadOnly();

    uiPushButton* advbut =
	new uiPushButton( this, tr("Advanced Settings"), false );
    advbut->activated.notify( mCB(this,uiBatchHostsDlg,advbutCB) );
    advbut->attach( rightTo, filefld );
    advbut->attach( rightBorder );

    uiTable::Setup tsu( -1, 5 );
    tsu.rowdesc("Host").defrowlbl(true).selmode(uiTable::SingleRow);
    table_ = new uiTable( this, tsu, "Batch Hosts" );
    BufferStringSet collbls;
    collbls.add( "IP address" ).add( "Hostname" ).add( "Display Name" )
	   .add( "Platform" ).add("Survey Data Root");
    table_->setColumnLabels( collbls );
    table_->setPrefWidth( 800 );
    table_->resizeHeaderToContents( true );
    table_->setTableReadOnly( !writeallowed );
    table_->valueChanged.notify( mCB(this,uiBatchHostsDlg,changedCB) );
    table_->selectionChanged.notify( mCB(this,uiBatchHostsDlg,hostSelCB) );
    table_->attach( leftAlignedBelow, filefld );

    uiButtonGroup* buttons = new uiButtonGroup( this, "", OD::Vertical );
    new uiToolButton( buttons, "addnew", tr("Add Host"),
			mCB(this,uiBatchHostsDlg,addHostCB) );
    new uiToolButton( buttons, "stop", tr("Remove Host"),
			mCB(this,uiBatchHostsDlg,rmHostCB) );
    upbut_ = new uiToolButton( buttons, uiToolButton::UpArrow, tr("Move up"),
			mCB(this,uiBatchHostsDlg,moveUpCB) );
    downbut_ = new uiToolButton( buttons, uiToolButton::DownArrow,
			tr("Move down"),
			mCB(this,uiBatchHostsDlg,moveDownCB) );
    new uiToolButton( buttons, "checkgreen", tr("Test Hosts"),
			mCB(this,uiBatchHostsDlg,testHostsCB) );
    buttons->attach( rightTo, table_ );

    fillTable();
}


uiBatchHostsDlg::~uiBatchHostsDlg()
{
    delete &hostdatalist_;
}


void uiBatchHostsDlg::advbutCB( CallBacker* )
{
    uiDialog dlg( this,
	uiDialog::Setup("Advanced Settings",mNoDlgTitle,mNoHelpKey) );

    uiLabel* albl = new uiLabel( &dlg, tr("Settings for all platforms:") );
    albl->attach( leftBorder );
    const int portnr = hostdatalist_.firstPort();
    uiGenInput* portnrfld = new uiGenInput( &dlg, tr("First Port"),
					    IntInpSpec(portnr) );
    portnrfld->attach( ensureBelow, albl );

    uiSeparator* sep = new uiSeparator( &dlg );
    sep->attach( stretchedBelow, portnrfld );

    uiLabel* ulbl = new uiLabel( &dlg, tr("Settings for UNIX only:") );
    ulbl->attach( leftBorder );
    ulbl->attach( ensureBelow, sep );

    const char* cmds[] = { "ssh", "rsh", 0 };
    uiGenInput* remoteshellfld = new uiGenInput( &dlg,
				tr("Remote shell command"),
				BoolInpSpec(true,cmds[0],cmds[1]) );
    remoteshellfld->setText( hostdatalist_.loginCmd() );
    remoteshellfld->attach( alignedBelow, portnrfld );
    remoteshellfld->attach( ensureBelow, ulbl );

    const StepInterval<int> nicelvlrg( -19, 19, 1 );
    const int nicelvl = hostdatalist_.niceLevel();
    uiGenInput* nicelvlfld = new uiGenInput( &dlg, tr("Nice level"),
				  IntInpSpec(nicelvl,nicelvlrg) );
    nicelvlfld->attach( alignedBelow, remoteshellfld );

    if ( !dlg.go() ) return;

    const int cmdres = (int)(!remoteshellfld->getBoolValue());
    hostdatalist_.setLoginCmd( cmds[cmdres] );
    hostdatalist_.setNiceLevel( nicelvlfld->getIntValue() );
    hostdatalist_.setFirstPort( portnrfld->getIntValue() );
}


void setIPAddress( uiTable& tbl, int row, const HostData& hd )
{ tbl.setText( RowCol(row,sIPCol), hd.getIPAddress() ); }

bool getIPAddress( const uiTable& tbl, int row, HostData& hd )
{
    const BufferString ip = tbl.text( RowCol(row,sIPCol) );
    if ( ip.isEmpty() ) return false;

    hd.setIPAddress( ip );
    return true;
}

void setDisplayName( uiTable& tbl, int row, const HostData& hd )
{
    const char* nm = hd.nrAliases()>0 ? hd.alias(0) : hd.getHostName();
    tbl.setText( RowCol(row,sDispNameCol), nm );
}

void getDisplayName( const uiTable& tbl, int row, HostData& hd )
{ hd.setAlias( tbl.text(RowCol(row,sDispNameCol)) ); }

void setHostName( uiTable& tbl, int row, const HostData& hd )
{ tbl.setText( RowCol(row,sHostNameCol), hd.getHostName() ); }

bool getHostName( const uiTable& tbl, int row, HostData& hd )
{
    const BufferString hostname = tbl.text( RowCol(row,sHostNameCol) );
    if ( hostname.isEmpty() )
	return false;

    hd.setHostName( hostname );
    return true;
}


void setPlatform( uiTable& tbl, int row, const HostData& hd )
{
    uiObject* cellobj = tbl.getCellObject( RowCol(row,2) );
    mDynamicCastGet(uiComboBox*,cb,cellobj)
    if ( !cb )
    {
	cb = new uiComboBox( 0, OD::Platform::TypeNames(), "Platforms" );
	tbl.setCellObject( RowCol(row,sPlfCol), cb );
    }

    cb->setValue( hd.getPlatform().type() );
}


void getPlatform( const uiTable& tbl, int row, HostData& hd )
{
    uiObject* cellobj = tbl.getCellObject( RowCol(row,sPlfCol) );
    mDynamicCastGet(uiComboBox*,cb,cellobj)
    if ( !cb ) return;

    OD::Platform plf( (OD::Platform::Type)cb->getIntValue() );
    hd.setPlatform( plf );
}


void setDataRoot( uiTable& tbl, int row, const HostData& hd )
{
    const FilePath& fp = hd.prefixFilePath( HostData::Data );
    const BufferString dataroot = fp.fullPath();
    tbl.setText( RowCol(row,sDataRootCol), dataroot );
}

void getDataRoot( const uiTable& tbl, int row, HostData& hd )
{ hd.setDataRoot( tbl.text(RowCol(row,sDataRootCol)) ); }


void uiBatchHostsDlg::fillTable()
{
    NotifyStopper ns( table_->valueChanged );

    const int nrhosts = hostdatalist_.size();
    table_->setNrRows( nrhosts );

    for ( int idx=0; idx<nrhosts; idx++ )
    {
	HostData* hd = hostdatalist_[idx];
	setIPAddress( *table_, idx, *hd );
	setHostName( *table_, idx, *hd );
	setDisplayName( *table_, idx, *hd );
	setPlatform( *table_, idx, *hd );
	setDataRoot( *table_, idx, *hd );
    }

    table_->resizeColumnsToContents();
    table_->setColumnResizeMode( uiTable::ResizeToContents );
    table_->setColumnStretchable( sDataRootCol, true );
}


void uiBatchHostsDlg::changedCB( CallBacker* )
{
    const RowCol rc = table_->notifiedCell();
    fillHostData( rc );
}


void uiBatchHostsDlg::addHostCB( CallBacker* )
{
    HostData* hd = new HostData( 0 );
    hostdatalist_ += hd;
    fillTable();
    table_->selectRow( hostdatalist_.size()-1 );
}


void uiBatchHostsDlg::rmHostCB( CallBacker* )
{
    if ( hostdatalist_.isEmpty() ) return;

    const int row = table_->currentRow();
    const BufferString hostname = table_->text( RowCol(row,1) );
    const BufferString msg( "Host ", hostname,
			    " will be removed from this list" );
    const bool res = uiMSG().askContinue( msg );
    if ( !res ) return;

    table_->removeRow( row );
    delete hostdatalist_.removeSingle( row );

    const int lastrow = hostdatalist_.size()-1;
    table_->selectRow( row>=lastrow ? row-1 : row );
}


void uiBatchHostsDlg::moveUpCB( CallBacker* )
{
    const int row = table_->currentRow();
    if ( row==0 || !hostdatalist_.validIdx(row) ) return;

    hostdatalist_.swap( row, row-1 );
    fillTable();
    table_->selectRow( row-1 );
}


void uiBatchHostsDlg::moveDownCB( CallBacker* )
{
    const int row = table_->currentRow();
    if ( row==hostdatalist_.size()-1 ||
	 !hostdatalist_.validIdx(row) ) return;

    hostdatalist_.swap( row, row+1 );
    fillTable();
    table_->selectRow( row+1 );
}


void uiBatchHostsDlg::testHostsCB( CallBacker* )
{
    BufferStringSet msgs;
    for ( int idx=0; idx<hostdatalist_.size(); idx++ )
    {
	const char* hostname = hostdatalist_[idx]->getHostName();
	BufferString msg;
	System::lookupHost( hostname, msg );
	msgs.add( msg );
    }

    const BufferString endmsg = msgs.cat();
    if ( !endmsg.isEmpty() )
	uiMSG().message( endmsg );
}


void uiBatchHostsDlg::hostSelCB( CallBacker* )
{
    const int row = table_->currentRow();
    upbut_->setSensitive( row>0 );
    downbut_->setSensitive( row!=hostdatalist_.size()-1 );
}


bool uiBatchHostsDlg::fillHostData( const RowCol& rc )
{
    const int row = rc.row();
    const int col = rc.col();
    const bool all = col<0;

    if ( !hostdatalist_.validIdx(row) )
	return false;

    HostData& hd = *hostdatalist_[row];
    if ( all || col==sIPCol )
    {
	if ( !getIPAddress(*table_,row,hd) )
	{
	    uiMSG().error(table_->rowLabel(row),": IP address cannot be empty");
	    return false;
	}

	if ( !all )
	{
	    const FixedString hostnm = hd.getHostName();
	    if ( hostnm.isEmpty() )
		table_->setText( RowCol(row,sHostNameCol),
				 System::hostName(hd.getIPAddress()) );
	}
    }

    if ( all || col==sHostNameCol )
    {
	if ( !getHostName(*table_,row,hd) )
	{
	    uiMSG().error(table_->rowLabel(row),": Hostname cannot be empty");
	    return false;
	}
    }

    if ( all || col==sDispNameCol )
	getDisplayName( *table_, row, hd );
    if ( all || col==sPlfCol )
	getPlatform( *table_, row, hd );
    if ( all || col==sDataRootCol )
	getDataRoot( *table_, row, hd );

    return true;
}


bool uiBatchHostsDlg::acceptOK( CallBacker* )
{
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	if ( !fillHostData(RowCol(idx,-1)) )
	    return false;
    }

    // TODO: Support BatchHosts file selection?
    return hostdatalist_.writeHostFile( hostdatalist_.getBatchHostsFilename() );
}
