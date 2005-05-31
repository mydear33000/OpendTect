/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February 2005
 ________________________________________________________________________

-*/

#include "uieventattrib.h"
#include "eventattrib.h"
#include "attribdesc.h"
#include "uigeninput.h"
#include "uiattrsel.h"
#include "datainpspec.h"

using namespace Attrib;

static const char* evtypestrs[] =
{
    "Extremum",
    "Maximum",
    "Minimum",
    "Zero Crossing",
    "Negative to Positive ZC",
    "Positive to Negative ZC",
    "Maximum within the Gate",
    "Minimum within the Gate",
    0
};


static const char* outpstrs[] =
{
    "Peakedness",
    "Steepness",
    "Asymmetry",
    0	
};


uiEventAttrib::uiEventAttrib( uiParent* p )
        : uiAttrDescEd(p)
{
    inpfld = getInpFld();

    issinglefld = new uiGenInput( this, "calculating attribute involving", 
				BoolInpSpec("single event","multiple events") );
    issinglefld->attach( alignedBelow, inpfld );
    issinglefld->valuechanged.notify( mCB(this,uiEventAttrib,isSingleSel) );

    evtypefld = new uiGenInput( this, "choose event type", 
				StringListInpSpec(evtypestrs) );
    evtypefld->attach( alignedBelow, issinglefld );
    evtypefld->valuechanged.notify( mCB(this,uiEventAttrib,isGateSel) );
    evtypefld->display(false);
    
    tonextfld = new uiGenInput( this, "computes distance to",
	                BoolInpSpec("next event","previous event") );
    tonextfld->attach( alignedBelow, evtypefld );
    tonextfld->display(false);
    
    outpfld = new uiGenInput( this, "Output", StringListInpSpec(outpstrs) );
    outpfld->attach( alignedBelow, issinglefld);

    gatefld = new uiGenInput( this, gateLabel(), FloatInpIntervalSpec() );
    gatefld->attach( alignedBelow, tonextfld);
    gatefld->display(false);

    setHAlignObj( issinglefld );
}


void uiEventAttrib::isSingleSel( CallBacker* )
{
    const bool issingle = issinglefld-> getBoolValue();
    const int val = evtypefld-> getIntValue();
    evtypefld->display( !issingle );
    tonextfld->display( !issingle && val != 6 && val != 7 );
    gatefld->display( !issingle && ( val == 6 || val == 7 ) );
    outpfld->display( issingle );
}


void uiEventAttrib::isGateSel( CallBacker* )
{
    const int val = evtypefld-> getIntValue();
    const bool issingle = issinglefld-> getBoolValue();
    const bool tgdisplay =  (val == 6 || val == 7 ) ? true : false;
    gatefld->display( tgdisplay && !issingle );
    tonextfld->display( !tgdisplay && !issingle );
}


bool uiEventAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Event::attribName()) )
	return false;

    mIfGetBool( Event::singleevStr(), issingleevent, 
	        issinglefld->setValue(issingleevent) );
    mIfGetBool( Event::tonextStr(), tonext, tonextfld->setValue(tonext) );
    mIfGetEnum( Event::eventtypeStr(), eventtype, 
	        evtypefld->setValue(eventtype) );
    mIfGetFloatInterval( Event::gateStr(), gate, gatefld->setValue(gate) );

    isSingleSel(0);
    isGateSel(0);
}


bool uiEventAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiEventAttrib::setOutput( const Desc& desc )
{
    outpfld->setValue( desc->selectedOutput() );
    return true;
}


bool uiEventAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),Event::attribName()) )
	return false;

    mSetBool( Event::singleevStr(), issinglefld->getBoolValue() );
    mSetBool( Event::tonextStr(), tonextfld->getBoolValue() );
    mSetEnum( Event::eventtypeStr(), evtypefld->getIntValue() );
    mSetFloatInterval( Event::gateStr(), gatefld->getFInterval() );

    return true;
}


bool uiEventAttrib::getInput( Desc& desc )
{
    inpfld->processInput();
    fillInp( inpfld, desc, 0 );
    return true;
}


bool uiEventAttrib::getOutput( Desc& desc )
{
    const int outp = issinglefld->getBoolValue() ? outpfld->getIntValue() : 0;
    fillOutput( desc, outp );
}
