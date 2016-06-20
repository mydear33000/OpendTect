/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/


#include "vislocationdisplay.h"

#include "ioman.h"

#include "picksetmanager.h"
#include "selector.h"

#include "visevent.h"
#include "vismaterial.h"
#include "vissower.h"
#include "vistransform.h"
#include "visplanedatadisplay.h"
#include "vishorizondisplay.h"
#include "visrandomtrackdisplay.h"
#include "zaxistransform.h"


namespace visSurvey {

const char* LocationDisplay::sKeyID()		{ return "Location.ID"; }
const char* LocationDisplay::sKeyMgrName()	{ return "Location.Manager"; }
const char* LocationDisplay::sKeyShowAll()	{ return "Show all"; }
const char* LocationDisplay::sKeyMarkerType()	{ return "Shape"; }
const char* LocationDisplay::sKeyMarkerSize()	{ return "Size"; }

static const float cDistEps = 0.1f;

static float findDistance( Coord3 p1, Coord3 p2, Coord3 p )
{
    const Coord3 vec = p2 - p1;
    const Coord3 newvec = p - p1;
    const float prod = (float) vec.dot(newvec);
    const float sq = (float) vec.sqAbs();
    if ( mIsZero(sq,cDistEps) ) return mUdf(float);	// p1 and p2 coincide.

    const float factor = prod / sq;
    if ( factor<0 || factor>1 )		// projected point outside the segment.
	return (float) mMIN( p1.distTo(p), p2.distTo(p) );

    const Coord3 proj = p1 + vec * factor;
    return (float) proj.distTo( p );
}


LocationDisplay::LocationDisplay()
    : VisualObjectImpl( true )
    , eventcatcher_( 0 )
    , transformation_( 0 )
    , showall_( true )
    , set_( 0 )
    , manip_( this )
    , waitsfordirectionid_(LocID::getInvalid())
    , waitsforpositionid_(LocID::getInvalid())
    , datatransform_( 0 )
    , pickedsobjid_(-1)
    , voiidx_(-1)
    , undoloccoord_( Coord3::udf() )
    , undomove_( false )
    , selectionmodel_(false)
    , ctrldown_(false)
{
    sower_ = new Sower( this );
    addChild( sower_->osgNode() );
}


LocationDisplay::~LocationDisplay()
{
    detachAllNotifiers();
    setSceneEventCatcher( 0 );

    if ( transformation_ )
	transformation_->unRef();

    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		mCB( this, LocationDisplay, fullRedraw) );
	datatransform_->unRef();
    }

    removeChild( sower_->osgNode() );
    delete sower_;
}


void LocationDisplay::setSet( Pick::Set* ps )
{
    if ( !ps || set_.ptr() == ps )
	return;

    if ( set_ )
	mDetachCB( set_->objectChanged() , LocationDisplay::setChgCB );

    set_ = ps;
    setName( toUiString(set_->name()) );

    fullRedraw();
    if ( !showall_ && scene_ )
	scene_->objectMoved( 0 );

    mAttachCB( ps->objectChanged() , LocationDisplay::setChgCB );
}


MultiID LocationDisplay::getMultiID() const
{
    if ( set_ )
	return Pick::SetMGR().getID( *set_ );
    return MultiID::udf();
}


void LocationDisplay::fullRedraw( CallBacker* )
{
    if ( !set_ )
	return;

    if ( datatransform_ && datatransform_->needsVolumeOfInterest() &&
	 !set_->isEmpty() )
    {
	TrcKeyZSampling cs( false );
	Pick::SetIter psiter( *set_ );
	while ( psiter.next() )
	{
	    const Pick::Location& loc = psiter.get();
	    if ( loc.hasPos() )
	    {
		cs.hsamp_.include( loc.binID() );
		cs.zsamp_.include( (float)loc.pos().z, false );
	    }
	}
	psiter.retire();

	if ( cs.isDefined() )
	{
	    if ( voiidx_<0 )
		voiidx_ = datatransform_->addVolumeOfInterest( cs, true );
	    else
		datatransform_->setVolumeOfInterest( voiidx_, cs, true );

	    datatransform_->loadDataIfMissing( voiidx_ );
	}
    }

    getMaterial()->setColor( set_->dispColor() );
    invalidpicks_.erase();

    if ( set_->isEmpty() )
	{ removeAll(); return; }

    Pick::SetIter psiter( *set_ );
    int idx = 0;
    while ( psiter.next() )
    {
	Pick::Location loc = psiter.get();
	if ( !transformPos( loc ) )
	    invalidpicks_ += idx;
	else
	    invalidpicks_ -= idx;

	setPosition( idx, loc );
	idx++;
    }
}


void LocationDisplay::removeAll()
{
    // has to be done by subclass, nothing 2 do 4 me
}


void LocationDisplay::showAll( bool yn )
{
    showall_ = yn;
    if ( scene_ )
    {
	scene_->objectMoved(0);
	return;
    }
}


void LocationDisplay::setOnlyAtSectionsDisplay( bool yn )
{ showAll( !yn); }

bool LocationDisplay::displayedOnlyAtSections() const
{ return !allShown(); }


void LocationDisplay::pickCB( CallBacker* cb )
{
    if ( !set_ || !isSelected() || !isOn() || isLocked() )
	return;

    mCBCapsuleUnpack( const visBase::EventInfo&, eventinfo, cb );
    ctrldown_ = OD::ctrlKeyboardButton( eventinfo.buttonstate_ );

    if ( eventinfo.dragging )
	updateDragger();

    if ( eventinfo.type == visBase::MouseClick && !eventinfo.pressed )
    {
	if ( !draggerNormal() )
	{
	    const Coord3 normal = getActivePlaneNormal( eventinfo );
	    setDraggerNormal( normal );
	}
	updateDragger();
    }

    const Pick::Set::Disp::Connection connection = set_->connection();
    const bool sowerenabled = connection != Pick::Set::Disp::None;

    if ( eventinfo.type == visBase::MouseDoubleClick && sowerenabled )
    {
	set_->setConnection( Pick::Set::Disp::Close );
	return;
    }

    if ( waitsfordirectionid_.isValid() )
    {
	Coord3 newpos, normal;
	if ( getPickSurface(eventinfo,newpos,normal) )
	{
	    Pick::Location pl( set_->get(waitsfordirectionid_) );
	    Coord3 dir = newpos - pl.pos();
	    const float zscale = scene_ ? scene_->getZScale(): SI().zScale();
	    dir.z *= -zscale; //convert to right dir-domain
	    if ( dir.sqAbs()>=0 )
	    {
		pl.setDir( cartesian2Spherical(dir,true) );
		set_->set( waitsfordirectionid_, pl );
	    }
	}

	eventcatcher_->setHandled();
    }
    else if ( !waitsforpositionid_.isUdf() ) // dragging
    {
	// when dragging it will receive multi times coords from visevent:
	// mouse move and mouse release. we need the last one and the begin
	// one for undo issue
	Coord3 newpos, normal;
	if ( getPickSurface(eventinfo,newpos,normal) )
	{
	    if ( eventinfo.type==visBase::MouseClick )
	    {
		if ( undoloccoord_.isDefined() )
		{
		    Pick::Location pl( set_->get(waitsfordirectionid_) );
		    const ::Sphere dir = pl.dir();
		    const Pick::Location undoloc( undoloccoord_, dir );
		    const Pick::Location newloc( newpos, dir );
		    undomove_ = false;
		    set_->set( waitsforpositionid_, newloc );
		    Pick::SetMGR().addLocEvent( Pick::SetMGR().getID(*set_),
				Pick::SetManager::LocEvent( waitsforpositionid_,
				undoloc, newloc ) );
		}
	    }
	    else
	    {
		Pick::Location pl = set_->get( waitsforpositionid_ );
		if ( !undomove_ )
		{
		    undoloccoord_ = pl.pos();
		    undomove_ =  true;
		}
		pl.setPos( newpos );
		set_->set( waitsforpositionid_, pl );
	    }
	}

	eventcatcher_->setHandled();
    }
    else if ( sowerenabled && sower_->accept(eventinfo) )
	return;

    if ( eventinfo.type != visBase::MouseClick ||
	 !OD::leftMouseButton( eventinfo.buttonstate_ ) )
	return;

    int eventid = -1;
    pickedsobjid_ = -1;
    for ( int idx=0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	visBase::DataObject* dataobj =
			visBase::DM().getObject( eventinfo.pickedobjids[idx] );
	if ( !dataobj || dataobj == this )
	    continue;

	if ( dataobj->isPickable() )
	    eventid = eventinfo.pickedobjids[idx];

	mDynamicCastGet(const SurveyObject*,so,dataobj);
	if ( so && so->allowsPicks() )
	    pickedsobjid_ = eventid;

	if ( pickedsobjid_ != -1 )
	    break;
    }

    if ( eventid == -1 )
	return;

    if ( waitsforpositionid_.isValid() || waitsfordirectionid_.isValid() )
    {
	setPickable( true );
	waitsforpositionid_.setInvalid();
	waitsfordirectionid_.setInvalid();
	mousepressid_ = -1;
    }
    else if ( eventinfo.pressed )
    {
	mousepressid_ = eventid;
	if ( !OD::ctrlKeyboardButton( eventinfo.buttonstate_ ) &&
	     !OD::altKeyboardButton( eventinfo.buttonstate_ ) &&
	     !OD::shiftKeyboardButton( eventinfo.buttonstate_ ) )
	{
	    const int selfpickidx = clickedMarkerIndex( eventinfo );
	    if ( selfpickidx >= 0 )
	    {
		setPickable( false, false );
		waitsforpositionid_ = set_->locIDFor( selfpickidx );
	    }
	    const int selfdirpickidx
			    = isDirMarkerClick(eventinfo.pickedobjids);
	    if ( selfdirpickidx >= 0 )
	    {
		waitsfordirectionid_ = set_->locIDFor( selfpickidx );
		setPickable( false, false );
	    }

	    //Only set handled if clicked on marker. Otherwise
	    //we may interfere with draggers.
	    if ( selfdirpickidx>=0 || selfpickidx>=0 )
		eventcatcher_->setHandled();
	    else
	    {
		const Color& color = set_->dispColor();
		if ( sowerenabled && sower_->activate(color, eventinfo) )
		    return;
	    }
	}
    }
    else
    {
	if ( OD::ctrlKeyboardButton( eventinfo.buttonstate_ ) &&
	     !OD::altKeyboardButton( eventinfo.buttonstate_ ) &&
	     !OD::shiftKeyboardButton( eventinfo.buttonstate_ ) )
	{
	    if ( eventinfo.pickedobjids.size() && eventid==mousepressid_ )
	    {
		const int removeidx = clickedMarkerIndex( eventinfo );
		if ( removeidx >= 0 )
		    set_->remove( set_->locIDFor(removeidx) );
	    }

	    eventcatcher_->setHandled();
	}
	else if ( !OD::ctrlKeyboardButton( eventinfo.buttonstate_ ) &&
	          !OD::altKeyboardButton( eventinfo.buttonstate_ ) &&
		  !OD::shiftKeyboardButton( eventinfo.buttonstate_ ) )
	{
	    if ( eventinfo.pickedobjids.size() &&
		 eventid==mousepressid_ )
	    {
		Coord3 newpos, normal;
		if ( getPickSurface(eventinfo,newpos,normal) )
		{
		    const Sphere dir = normal.isDefined()
			? cartesian2Spherical(
				Coord3(normal.y,-normal.x,normal.z), true)
			: Sphere( 1, 0, 0 );

		    LocID locid = addPick( newpos, dir );
		    if ( locid.isValid() )
		    {
			if ( hasDirection() )
			{
			    setPickable( false, false );
			    waitsfordirectionid_ = locid;
			}

			eventcatcher_->setHandled();
		    }
		}
	    }
	}
    }
}


bool LocationDisplay::getPickSurface( const visBase::EventInfo& evi,
				      Coord3& newpos, Coord3& normal ) const
{
    const int sz = evi.pickedobjids.size();
    bool validpicksurface = false;
    int eventid = -1;

    for ( int idx=0; idx<sz; idx++ )
    {
	const DataObject* pickedobj =
	    visBase::DM().getObject( evi.pickedobjids[idx] );
	if ( pickedobj == this )
	    continue;

	if ( eventid==-1 && pickedobj->isPickable() )
	{
	    eventid = evi.pickedobjids[idx];
	    if ( validpicksurface )
		break;
	}

	mDynamicCastGet(const SurveyObject*,so,pickedobj);
	if ( so && so->allowsPicks() )
	{
	    validpicksurface = true;
	    normal = so->getNormal( evi.displaypickedpos );
	    if ( eventid!=-1 )
		break;
	}
    }

    if ( !validpicksurface )
	return false;

    newpos = evi.worldpickedpos;
    if ( datatransform_ )
    {
	newpos.z = datatransform_->transformBack( newpos );
	if ( mIsUdf(newpos.z) )
	    return false;
    }

    mDynamicCastGet( SurveyObject*,so, visBase::DM().getObject(eventid))
    if ( so ) so->snapToTracePos( newpos );

    return true;
}


Coord3 LocationDisplay::display2World( const Coord3& pos ) const
{
    Coord3 res = pos;
    if ( scene_ )
	scene_->getTempZStretchTransform()->transformBack( res );
    if ( transformation_ )
	transformation_->transformBack( res );
    return res;
}


Coord3 LocationDisplay::world2Display( const Coord3& pos ) const
{
    Coord3 res;
    mVisTrans::transform( transformation_, pos, res );
    if ( scene_ )
	scene_->getTempZStretchTransform()->transform( res );
    return res;
}


bool LocationDisplay::transformPos( Pick::Location& loc ) const
{
    if ( !datatransform_ )
	return true;

    const float newdepth = datatransform_->transform( loc.pos() );
    if ( mIsUdf(newdepth) )
	return false;

    loc.setZ( newdepth );

    if ( hasDirection() )
	{ pErrMsg("Direction not impl"); }

    return true;
}


void LocationDisplay::setChgCB( CallBacker* cb )
{
    mGetMonitoredChgData( cb, chgdata );

    if ( chgdata.changeType() == Pick::Set::cDispChange() )
	dispChg();
    else
	locChg( chgdata );
}


void LocationDisplay::locChg( const Monitorable::ChangeData& chgdata )
{
    if ( chgdata.changeType() == Pick::Set::cEntireObjectChangeType() )
	{ fullRedraw( 0 ); return; }

    const LocID locid = LocID::get( (LocID::IDType)chgdata.ID() );
    const int locidx = set_->idxFor( locid );
    if ( chgdata.changeType() == Pick::Set::cLocationInsert() )
    {
	Pick::Location loc = set_->get( locid );
	if ( !transformPos( loc ) )
	    invalidpicks_ += locidx;

	setPosition( locidx, loc, true );
    }
    else if ( chgdata.changeType() == Pick::Set::cLocationRemove() )
    {
	removePosition( locidx );
	invalidpicks_ -= locidx;
    }
    else if ( chgdata.changeType() == Pick::Set::cLocationChange() )
    {
	Pick::Location loc = set_->get( locid );
	if ( transformPos( loc ) )
	    invalidpicks_ -= locidx;
	else
	{
	    if ( invalidpicks_.indexOf(locidx) < 0 )
		invalidpicks_ += locidx;
	}

	setPosition( locidx, loc );
    }
}


void LocationDisplay::dispChg()
{
    if ( set_ )
	getMaterial()->setColor( set_->dispColor() );
}


void LocationDisplay::setColor( Color nc )
{
    if ( set_ )
	set_->setDispColor( nc );
}


Color LocationDisplay::getColor() const
{
    return set_ ? set_->dispColor() : Color::DgbColor();
}


bool LocationDisplay::isPicking() const
{
    return isSelected() && !isLocked();
}


LocationDisplay::LocID LocationDisplay::addPick( const Coord3& pos,
						 const Sphere& dir )
{
    if ( selectionmodel_ )
	return LocID::getInvalid();

    mDefineStaticLocalObject( TypeSet<Coord3>, sowinghistory, );

    LocID locid = LocID::getInvalid();
    bool insertpick = false;
    if ( set_->connection() != Pick::Set::Disp::Close )
	sower_->alternateSowingOrder( false );
    else
    {
	sower_->alternateSowingOrder( true );
	Coord3 displaypos = world2Display( pos );
	if ( sower_->mode() == Sower::FirstSowing )
	{
	    displaypos = sower_->pivotPos();
	    sowinghistory.erase();
	}

	float mindist = mUdf(float);
	MonitorLock ml( *set_ );
	const int setsz = set_->size();
	for ( int idx=0; idx<setsz; idx++ )
	{
	    int pidx = idx>0 ? idx-1 : setsz-1;

	    const Coord3 pcoord = set_->getByIndex( idx ).pos();
	    const Coord3 prevpcoord = set_->getByIndex( pidx ).pos();
	    int nrmatches = sowinghistory.indexOf( pcoord ) >= 0;
	    nrmatches += sowinghistory.indexOf( prevpcoord ) >= 0;
	    if ( nrmatches != sowinghistory.size() )
		continue;

	    const float dist = findDistance( world2Display(prevpcoord),
					     world2Display(pcoord),
					     displaypos );
	    if ( mIsUdf(dist) )
		continue;

	    if ( mIsUdf(mindist) || dist<mindist )
	    {
		mindist = dist;
		locid = set_->locIDFor( idx );
	    }
	}
	ml.unlockNow();
	insertpick = locid.isValid();

	sowinghistory.insert( 0, pos );
	sowinghistory.removeSingle( 2 );
    }

    Pick::Location newloc( pos, dir );
    const SurveyObject* so = getPickedSurveyObject();
    if ( so )
	newloc.setGeomID( so->getGeomID() );

    if ( insertpick )
	locid = set_->insertBefore( locid, newloc );
    else
	locid = set_->add( newloc );

    if ( hasText() )
    {
	if ( !set_->get(locid).hasText() )
	    { set_->remove( locid ); locid.setInvalid(); }
    }

    return locid;
}


BufferString LocationDisplay::getManipulationString() const
{
    BufferString str = set_ && set_->isPolygon() ? "Polygon: " : "PickSet: ";
    str += mFromUiStringTodo(name());
    return str;
}


void LocationDisplay::getObjectInfo( BufferString& info ) const
{
    info = getManipulationString();
}


void LocationDisplay::getMousePosInfo( const visBase::EventInfo&,
				      Coord3& pos, BufferString& val,
				      BufferString& info ) const
{
    val.setEmpty();
    info = getManipulationString();
}


void LocationDisplay::otherObjectsMoved(
			const ObjectSet<const SurveyObject>& objs, int )
{
    if ( showall_ && invalidpicks_.isEmpty() )
	return;
    // Ehhh? anything?
}


void LocationDisplay::setDisplayTransformation( const mVisTrans* newtr )
{
    if ( transformation_==newtr )
	return;

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = newtr;

    if ( transformation_ )
	transformation_->ref();

    sower_->setDisplayTransformation( newtr );
}


const mVisTrans* LocationDisplay::getDisplayTransformation() const
{
    return transformation_;
}


void LocationDisplay::setRightHandSystem( bool yn )
{
    visBase::VisualObjectImpl::setRightHandSystem( yn );
}


void LocationDisplay::setSceneEventCatcher( visBase::EventCatcher* nevc )
{
    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.remove(mCB(this,LocationDisplay,pickCB));
	eventcatcher_->unRef();
    }

    eventcatcher_ = nevc;
    sower_->setEventCatcher( nevc );

    if ( eventcatcher_ )
    {
	eventcatcher_->eventhappened.notify(mCB(this,LocationDisplay,pickCB));
	eventcatcher_->ref();
    }

}


int LocationDisplay::getPickID(
			    visBase::DataObject* dataobj ) const
{
    return -1; // to be implemented
}


bool LocationDisplay::setZAxisTransform( ZAxisTransform* zat, TaskRunner* tr )
{
    if ( datatransform_==zat )
	return true;

    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->remove(
		mCB( this, LocationDisplay, fullRedraw) );
	datatransform_->unRef();
    }

    datatransform_ = zat;

    if ( datatransform_ )
    {
	if ( datatransform_->changeNotifier() )
	    datatransform_->changeNotifier()->notify(
		mCB( this, LocationDisplay, fullRedraw) );

	datatransform_->ref();
    }


    fullRedraw();
    showAll( datatransform_ && datatransform_->needsVolumeOfInterest() );
    return true;
}


const ZAxisTransform* LocationDisplay::getZAxisTransform() const
{
    return datatransform_;
}


int LocationDisplay::clickedMarkerIndex( const visBase::EventInfo& evi ) const
{ return -1; }


bool LocationDisplay::isMarkerClick( const visBase::EventInfo& evi ) const
{
    return false;
}


int LocationDisplay::isDirMarkerClick( const TypeSet<int>& ) const
{ return -1; }


void LocationDisplay::triggerDeSel()
{
    setPickable( true );
    waitsfordirectionid_.setInvalid();
    waitsforpositionid_.setInvalid();
    VisualObject::triggerDeSel();
}


const SurveyObject* LocationDisplay::getPickedSurveyObject() const
{
    const DataObject* pickedobj = visBase::DM().getObject( pickedsobjid_ );
    mDynamicCastGet(const SurveyObject*,so,pickedobj);
    return so;
}


bool LocationDisplay::removeSelections( TaskRunner* taskr )
{
    bool changed = false;
    const Selector< Coord3>* selector = scene_ ? scene_->getSelector() : 0;
    if ( selector && selector->isOK() )
    {
	RefMan<Pick::Set> workps = new Pick::Set( *set_ );
	Pick::SetIter4Edit psiter( *workps );
	while ( psiter.next() )
	{
	    const Pick::Location& loc = psiter.get();
	    if ( selector->includes(loc.pos()) )
	    {
		psiter.removeCurrent();
		changed = true;
	    }
	}
	psiter.retire();
	if ( changed )
	    *set_ = *workps;
    }
    return changed;
}


void LocationDisplay::fillPar( IOPar& par ) const
{
    visBase::VisualObjectImpl::fillPar( par );
    visSurvey::SurveyObject::fillPar( par );

    par.setYN( sKeyShowAll(), showall_ );

    if ( set_ )
    {
	par.set( sKeyID(), Pick::SetMGR().getID(*set_) );
	par.set( sKeyMgrName(), set_->category() );
	BufferString mkststr;
	set_->markerStyle().toString( mkststr );
	par.set( sKey::MarkerStyle(), mkststr );
    }
}


bool LocationDisplay::usePar( const IOPar& par )
{
    if ( !visBase::VisualObjectImpl::usePar( par ) ||
	 !visSurvey::SurveyObject::usePar( par ) )
	 return false;

    bool shwallpicks = true;
    par.getYN( sKeyShowAll(), shwallpicks );
    showAll( shwallpicks );

    MultiID setid;
    if ( !par.get(sKeyID(),setid) )
	return false;
    RefMan<Pick::Set> newps = Pick::SetMGR().fetchForEdit( setid );
    if ( !newps )
	return false;

    const bool v6_or_earlier =
	( par.majorVersion()+par.minorVersion()*0.1 )>0 &&
	( par.majorVersion()+par.minorVersion()*0.1 )<=6;

    Pick::Set::Disp disp = newps->getDisp();
    if ( v6_or_earlier )
    {
	int markertype = 0;
	int pixsize = 3;
	par.get( sKeyMarkerType(), markertype );
	markertype ++;
	par.get( sKeyMarkerSize(), pixsize );
	disp.mkstyle_.type_=OD::MarkerStyle3D::Type(markertype);
	disp.mkstyle_.size_ = pixsize;
    }
    else
    {
	BufferString mkststr;
	par.get( sKey::MarkerStyle(), mkststr );
	disp.mkstyle_.fromString( mkststr );
    }
    newps->setDisp( disp );

    setSet( newps );
    return true;
}


const Coord3 LocationDisplay::getActivePlaneNormal(
    const visBase::EventInfo& eventinfo ) const
{
    Coord3 normal = Coord3::udf();
    for ( int idx = 0; idx<eventinfo.pickedobjids.size(); idx++ )
    {
	visBase::DataObject* dataobj =
	    visBase::DM().getObject(eventinfo.pickedobjids[idx]);
	if ( !dataobj ) continue;
	mDynamicCastGet( PlaneDataDisplay*, plane, dataobj );
	mDynamicCastGet( RandomTrackDisplay*, sdtd, dataobj );
	mDynamicCastGet( HorizonDisplay*,hord,dataobj);
	if ( plane && plane->isOn() )
	{
	    normal = plane->getNormal(Coord3::udf()).normalize();
	    break;
	}
	if ( sdtd && sdtd->isOn() )
	{
	    normal = sdtd->getNormal(eventinfo.displaypickedpos).normalize();
	    break;
	}
	if ( hord && hord->isOn() )
	{
	    normal = hord->getNormal(Coord3::udf()).normalize();
	    break;
	}
    }

    return normal;
}

} // namespace visSurvey
