/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          Jun 2002
 RCS:		$Id: uiseiscbvsimp.cc,v 1.3 2002-06-21 22:37:04 bert Exp $
________________________________________________________________________

-*/

#include "uiseiscbvsimp.h"
#include "seistrctr.h"
#include "ioman.h"
#include "iostrm.h"
#include "iodir.h"
#include "iopar.h"
#include "ctxtioobj.h"
#include "filegen.h"

#include "uimsg.h"
#include "uifileinput.h"
#include "uiioobjsel.h"


uiSeisImpCBVS::uiSeisImpCBVS( uiParent* p )
	: uiDialog(p,"Make CBVS cube available")
	, ctio_(*new CtxtIOObj(SeisTrcTranslator::ioContext()))
{
    setTitleText( "Create CBVS cube definition" );

    inpfld = new uiFileInput( this, "(First) CBVS file name", GetDataDir(),
	   			true, "*.cbvs;;*" );
    inpfld->valuechanged.notify( mCB(this,uiSeisImpCBVS,inpSel) );

    StringListInpSpec spec;
    spec.addString( "Input data cube" );
    spec.addString( "Generated attribute cube" );
    spec.addString( "Steering cube" );
    typefld = new uiGenInput( this, "Cube type", spec );
    typefld->attach( alignedBelow, inpfld );

    ctio_.ctxt.forread = false;
    ctio_.ctxt.trglobexpr = "CBVS";
    IOM().to( ctio_.ctxt.stdSelKey() );
    seissel = new uiIOObjSel( this, ctio_, "Cube name" );
    seissel->attach( alignedBelow, typefld );
}


uiSeisImpCBVS::~uiSeisImpCBVS()
{
    delete ctio_.ioobj;
    delete &ctio_;
}


void uiSeisImpCBVS::inpSel( CallBacker* )
{
    const char* out = seissel->getInput();
    if ( *out ) return;
    BufferString inp = inpfld->text();
    if ( inp == "" ) return;
    inp = File_getFileName( inp );
    if ( inp == "" ) return;

    // convert underscores to spaces
    char* ptr = inp.buf();
    while ( *ptr )
    {
	if ( *ptr == '_' ) *ptr = ' ';
	ptr++;
    }

    // remove .cbvs extension
    ptr--;
    if ( *ptr == 's' && *(ptr-1) == 'v' && *(ptr-2) == 'b'
     && *(ptr-3) == 'c' && *(ptr-4) == '.' )
	*(ptr-4) = '\0';

    seissel->setInputText( inp );
}


#define mErrRet(s) \
	{ uiMSG().error(s); return false; }

bool uiSeisImpCBVS::acceptOK( CallBacker* )
{
    if ( !seissel->commitInput(true) )
    {
	uiMSG().error( "Please choose a name for the cube" );
	return false;
    }
    const char* fname = inpfld->text();
    if ( !fname || !*fname )
    {
	uiMSG().error( "Please select the input filename" );
	return false;
    }

    const int seltyp = typefld->getIntValue();
    if ( !seltyp )
	ctio_.ioobj->pars().removeWithKey( "Type" );
    else
	ctio_.ioobj->pars().set( "Type",
	    			 seltyp == 1 ? "Attribute" : "Steering" );

    ctio_.ioobj->setTranslator( "CBVS" );
    mDynamicCastGet(IOStream*,iostrm,ctio_.ioobj);
    if ( iostrm )
	iostrm->setFileName( fname );
    IOM().dirPtr()->commitChanges( ctio_.ioobj );
    return true;
}
