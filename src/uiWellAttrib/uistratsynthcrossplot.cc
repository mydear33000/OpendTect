/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratsynthcrossplot.cc,v 1.18 2011-02-03 11:59:54 cvsbert Exp $";

#include "uistratsynthcrossplot.h"
#include "uistratlayseqattrsetbuild.h"
#include "uiattribsetbuild.h"
#include "uidatapointset.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "stratlevel.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "stratlayseqattrib.h"
#include "stratlayseqattribcalc.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribprocessor.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "datacoldef.h"
#include "seisbuf.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "survinfo.h"
#include "aimodel.h"
#include "valseriesevent.h"


uiStratSynthCrossplot::uiStratSynthCrossplot( uiParent* p,
					const DataPack::FullID& dpid,
					const Strat::LayerModel& lm,
       					const ObjectSet<AIModel>& aimdls )
    : uiDialog(p,Setup("Layer model/synthetics cross-plotting",
			mNoDlgTitle,mTODOHelpID))
    , packmgrid_(DataPackMgr::getID(dpid))
    , lm_(lm)
    , aimodels_(aimdls)
    , emptylbl_(0)
    , tbpack_(0)
{
    if ( lm.isEmpty() )
	{ emptylbl_ = new uiLabel(this,"No input data"); return; }
    DataPack* dp = DPM(packmgrid_).obtain( DataPack::getID(dpid) );
    mDynamicCastGet(SeisTrcBufDataPack*,tbdp,dp)
    if ( !tbdp )
	{ emptylbl_ = new uiLabel(this,"Missing or invalid datapack"); return; }
    tbpack_ = tbdp;

    uiAttribDescSetBuild::Setup bsu( true );
    bsu.showdepthonlyattrs(false).showusingtrcpos(false).showps(false);
    seisattrfld_ = new uiAttribDescSetBuild( this, bsu );
    TypeSet<DataPack::FullID> fids; fids += dpid;
    seisattrfld_->setDataPackInp( fids );

    uiSeparator* sep = new uiSeparator( this, "sep1", true );
    sep->attach( stretchedBelow, seisattrfld_ );

    layseqattrfld_ = new uiStratLaySeqAttribSetBuild( this, lm_ );
    layseqattrfld_->attach( alignedWith, seisattrfld_ );
    layseqattrfld_->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "sep2" );
    sep->attach( stretchedBelow, layseqattrfld_ );

    uiGroup* extrposgrp = new uiGroup( this, "Extraction pos group" );
    extrposgrp->attach( ensureBelow, sep );
    uiLabeledComboBox* llvlfld = new uiLabeledComboBox( extrposgrp,
	    						"Reference level" );
    reflvlfld_ = llvlfld->box();
    extrposgrp->setHAlignObj( llvlfld );
    const Strat::LevelSet& lvls = Strat::LVLS();
    for ( int idx=0; idx<lvls.size(); idx++ )
	reflvlfld_->addItem( lvls.levels()[idx]->name() );

    BufferStringSet eventnms( VSEvent::TypeNames() ); eventnms.remove(0);
    snapfld_ = new uiGenInput( extrposgrp, "Snap to event",
	    			StringListInpSpec(eventnms) );
    snapfld_->setWithCheck( true );
    snapfld_->checked.notify( mCB(this,uiStratSynthCrossplot,evSnapCheck) );
    snapfld_->setValue( 1 );
    snapfld_->attach( alignedBelow, llvlfld );
    snapfld_->setElemSzPol( uiObject::Small );
    snapoffsfld_ = new uiGenInput( extrposgrp, "Offset (ms)", FloatInpSpec(0) );
    snapoffsfld_->attach( rightOf, snapfld_ );
    snapoffsfld_->setElemSzPol( uiObject::Small );
    snapoffsfld_->setSensitive( false );

    const float defstep = SI().zIsTime() ? SI().zStep() * 1000 : 4;
    extrwinfld_ = new uiGenInput( extrposgrp, "Extraction window",
	  FloatInpIntervalSpec(StepInterval<float>(0,0,defstep)) );
    extrwinfld_->attach( alignedBelow, snapfld_ );
}


uiStratSynthCrossplot::~uiStratSynthCrossplot()
{
    if( tbpack_ )
	DPM(packmgrid_).release( tbpack_->id() );
}


DataPointSet* uiStratSynthCrossplot::getData( const Attrib::DescSet& seisattrs,
					const Strat::LaySeqAttribSet& seqattrs,
					const Strat::Level& lvl,
					const StepInterval<float>& extrwin )
{
    DataPointSet* dps = seisattrs.createDataPointSet(Attrib::DescSetup());
    if ( !dps )
	{ uiMSG().error(seisattrs.errMsg()); return false; }
    dps->dataSet().add( new DataColDef(sKey::Depth) );
    const int depthcol = dps->nrCols() - 1;
    for ( int iattr=0; iattr<seqattrs.size(); iattr++ )
	dps->dataSet().add(
		new DataColDef(seqattrs.attr(iattr).name(),toString(iattr,0)) );

    const int nraimdls = aimodels_.size();
    const SeisTrcBuf& tbuf = tbpack_->trcBuf();
    if ( tbuf.size() != nraimdls )
	{ pErrMsg("DataPack nr of traces != nr of aimodels"); return 0; }

    TypeSet<float> lvltms( nraimdls, 0 );
    const bool dosnap = snapfld_->isChecked();
    const int evtyp = snapfld_->getIntValue();
    const float snapoffs = snapoffsfld_->getfValue() *.001;
    for ( int imod=0; imod<nraimdls; imod++ )
    {
	const float dpth = lm_.sequence(imod).depthOf( lvl );
	lvltms[imod] = aimodels_[imod]->convertTo( dpth, AIModel::TWT );
	if ( dosnap )
	{
	    lvltms[imod] += snapoffs;
	    snapLevelTime( *tbuf.get(imod), evtyp, lvltms[imod] );
	}
    }

    const int nrextr = extrwin.nrSteps() + 1;
    for ( int iextr=0; iextr<nrextr; iextr++ )
    {
	const float relz = extrwin.atIndex( iextr );
	for ( int itrc=0; itrc<nraimdls; itrc++ )
	{
	    const SeisTrc& trc = *tbuf.get( itrc );
	    DataPointSet::DataRow dr;
	    dr.pos_.nr_ = trc.info().nr;
	    dr.pos_.set( trc.info().coord );
	    dr.pos_.z_ = lvltms[itrc] + relz;
	    dr.data_.setSize( dps->nrCols(), mUdf(float) );
	    dr.data_[depthcol] = aimodels_[itrc]->convertTo( dr.pos_.z_,
							     AIModel::Depth );
	    dps->addRow( dr );
	}
    }
    dps->dataChanged();

    if ( dps->isEmpty() )
    {
	uiMSG().error( "No positions for data extraction" );
	delete dps; dps = 0;
    }
    else if ( !extractSeisAttribs(*dps,seisattrs)
	    || !extractLayerAttribs(*dps,seqattrs) )
	{ delete dps; dps = 0; }

    return dps;
}


void uiStratSynthCrossplot::snapLevelTime( const SeisTrc& trc, int et,
					   float& reftm ) const
{
    const VSEvent::Type evtyp = (VSEvent::Type)et;
    const SeisTrcValueSeries tvs( trc, 0 );
    const SamplingData<float> sd( trc.info().sampling );
    const int trcsz = trc.size();

    ValueSeriesEvFinder<float,float> evf( tvs, trcsz-1, sd );
    const Interval<float> trcwin( trc.startPos(), trc.samplePos(trcsz-1) );
    for ( int iwdth=1; iwdth<trcsz; iwdth++ )
    {
	Interval<float> findwin( reftm - iwdth*sd.step, reftm + iwdth*sd.step );
	if ( findwin.start < trcwin.start ) findwin.start = trcwin.start;
	if ( findwin.stop > trcwin.stop ) findwin.stop = trcwin.stop;

	ValueSeriesEvent<float,float> ev = evf.find( evtyp, findwin );
	if ( !mIsUdf(ev.pos) )
	    { reftm = ev.pos; break; }
    }
}


bool uiStratSynthCrossplot::extractSeisAttribs( DataPointSet& dps,
						const Attrib::DescSet& attrs )
{
    BufferString errmsg;                                                        
    PtrMan<Attrib::EngineMan> aem = createEngineMan( attrs );

    PtrMan<Executor> exec = aem->getTableExtractor( dps, attrs, errmsg,0,false);
    if ( !exec )                                                                
    {                                                                           
	uiMSG().error( errmsg );                                                
	return false;
    }

    exec->setName( "Attributes from Traces" );                       
    uiTaskRunner dlg( this );                                                   
    dlg.execute(*exec);
    return true;
}


bool uiStratSynthCrossplot::extractLayerAttribs( DataPointSet& dps,
				     const Strat::LaySeqAttribSet& seqattrs )
{
    Strat::LayModAttribCalc lmac( lm_, seqattrs, dps );
    uiTaskRunner tr( this );
    return tr.execute( lmac );
}


bool uiStratSynthCrossplot::launchCrossPlot( const DataPointSet& dps,
					const Strat::Level& lvl,
					const StepInterval<float>& extrwin )
{
    BufferString wintitl( "Attributes at ", lvl.name() );
    wintitl.add( " [" ).add( extrwin.start )
	   .add( "-" ).add( extrwin.stop ).add( "]" );
    uiDataPointSet::Setup su( wintitl, false );
    uiDataPointSet* uidps = new uiDataPointSet( this, dps, su, 0 );
    seisattrfld_->descSet().fillPar( uidps->storePars() );
    uidps->setDeleteOnClose( true );
    uidps->show();
    return false;
}


Attrib::EngineMan* uiStratSynthCrossplot::createEngineMan(
					    const Attrib::DescSet& attrs ) const
{                                                                               
    Attrib::EngineMan* aem = new Attrib::EngineMan;

    //If default Desc(s) present remove it
    int idx = -1;
    while( true )
    {
	idx++;
	const Attrib::Desc* tmpdesc = attrs.desc(idx);
	if ( tmpdesc && tmpdesc->isStored() && !tmpdesc->isStoredInMem() )
	    const_cast<Attrib::DescSet*>(&attrs)->removeDesc( tmpdesc->id() );
	else
	    break;
    }

    TypeSet<Attrib::SelSpec> attribspecs;
    attrs.fillInSelSpecs( Attrib::DescSetup().hidden(false), attribspecs );

    aem->setAttribSet( &attrs ); 
    aem->setAttribSpecs( attribspecs );                                         
    return aem;                                                                 
}


void uiStratSynthCrossplot::setRefLevel( const char* lvlnm )
{
    reflvlfld_->setText( lvlnm );
}


void uiStratSynthCrossplot::evSnapCheck( CallBacker* )
{
    snapoffsfld_->setSensitive( snapfld_->isChecked() );
}


#define mErrRet(s) { if ( s && *s ) uiMSG().error(s); return false; }

bool uiStratSynthCrossplot::acceptOK( CallBacker* )
{
    if ( emptylbl_ )
	return true;

    const Attrib::DescSet& seisattrs = seisattrfld_->descSet();
    if ( seisattrs.isEmpty() )
	mErrRet("Please define a seismic attribute")
    const Strat::LaySeqAttribSet& seqattrs = layseqattrfld_->attribSet();
    if ( seqattrs.isEmpty() )
	mErrRet("Please define a layer attribute")

    const Strat::Level* lvl = Strat::LVLS().get( reflvlfld_->text() );
    if ( !lvl )
	mErrRet("Cannot find selected stratigraphic level")

    StepInterval<float> extrwin( extrwinfld_->getFStepInterval() );
    if ( mIsUdf(extrwin.start) || mIsUdf(extrwin.stop) || mIsUdf(extrwin.step) )
	mErrRet("Please enter all extraction window parameters")

    extrwin.start *= 0.001; extrwin.stop *= 0.001; extrwin.step *= 0.001;
    DataPointSet* dps = getData( seisattrs, seqattrs, *lvl, extrwin );
    return dps ? launchCrossPlot( *dps, *lvl, extrwin ) : false;
}
