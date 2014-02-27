/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          January 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiattremout.h"

#include "attribdescset.h"
#include "attribdesc.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "nladesign.h"
#include "nlamodel.h"
#include "multiid.h"

#include "uiattrsel.h"
#include "uibatchjobdispatchersel.h"
#include "uimsg.h"

using namespace Attrib;

uiAttrEMOut::uiAttrEMOut( uiParent* p, const DescSet& ad,
			  const NLAModel* n, const MultiID& mid,
			  const char* dlgnm )
    : uiDialog(p,Setup(dlgnm,mNoDlgTitle,""))
    , ads_(const_cast<DescSet&>(ad))
    , nlamodel_(n)
    , nlaid_(mid)
    , nladescid_( -1, true )
{
    setTitleText( "" );

    attrfld_ = new uiAttrSel( this, ads_, "Quantity to output" );
    attrfld_->setNLAModel( nlamodel_ );
    attrfld_->selectionDone.notify( mCB(this,uiAttrEMOut,attribSel) );

    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					     Batch::JobSpec::AttribEM );
}


bool uiAttrEMOut::prepareProcessing()
{
    attrfld_->processInput();
    if ( !attrfld_->attribID().isValid() && attrfld_->outputNr() < 0 )
    {
	uiMSG().error( "Please select the output quantity" );
	return false;
    }

    return true;
}


bool uiAttrEMOut::fillPar()
{
    if ( nlamodel_ && attrfld_->outputNr() >= 0 )
    {
	if ( !nlaid_ || !(*nlaid_) )
	{
	    uiMSG().message( "NN needs to be stored before creating volume" );
	    return false;
	}
	if ( !addNLA( nladescid_ ) )	return false;
    }

    const DescID targetid = nladescid_.isValid() ? nladescid_
			  : attrfld_->attribID();
    DescSet* clonedset = ads_.optimizeClone( targetid );
    if ( !clonedset )
	return false;

    IOPar attrpar( "Attribute Descriptions" );
    clonedset->fillPar( attrpar );
    IOPar& iopar = batchfld_->jobSpec().pars_;
    iopar.mergeComp( attrpar, SeisTrcStorOutput::attribkey() );

    if ( attrfld_->is2D() )
    {
	DescSet descset(true);
	if ( nlamodel_ )
	    descset.usePar( nlamodel_->pars() );

	const Desc* desc = nlamodel_ ? descset.getFirstStored()
				     : clonedset->getFirstStored();
	BufferString storedid = desc ? desc->getStoredID() : "";
	if ( !storedid.isEmpty() )
	    iopar.set( "Input Line Set", storedid.buf() );
    }

    ads_.removeDesc( nladescid_ );
    return true;
}


void uiAttrEMOut::fillOutPar( IOPar& iopar, const char* outtyp,
			      const char* idlbl, const char* outid )
{
    iopar.set( IOPar::compKey( sKey::Output(), sKey::Type()), outtyp );

    BufferString key;
    BufferString tmpkey;
    mDefineStaticLocalObject( const BufferString, keybase,
			      ( IOPar::compKey(sKey::Output(), 0) ) );
    tmpkey = IOPar::compKey( keybase.buf(), SeisTrcStorOutput::attribkey() );
    key = IOPar::compKey( tmpkey.buf(), DescSet::highestIDStr() );
    iopar.set( key, 1 );

    tmpkey = IOPar::compKey( keybase.buf(), SeisTrcStorOutput::attribkey() );
    key = IOPar::compKey( tmpkey.buf(), 0 );
    iopar.set( key, nladescid_.isValid() ? nladescid_.asInt()
	    				 : attrfld_->attribID().asInt() );

    key = IOPar::compKey( keybase.buf(), idlbl );
    iopar.set( key, outid );
}

#define mErrRet(str) { uiMSG().message( str ); return false; }

bool uiAttrEMOut::addNLA( DescID& id )
{
    BufferString defstr("NN specification=");
    defstr += nlaid_;

    const int outputnr = attrfld_->outputNr();
    BufferString errmsg;
    EngineMan::addNLADesc( defstr, id, ads_, outputnr, nlamodel_, errmsg );
    if ( errmsg.size() )
	mErrRet( errmsg );

    return true;
}


bool uiAttrEMOut::acceptOK( CallBacker* )
{
    return prepareProcessing() && fillPar() ? batchfld_->start() : false;
}
