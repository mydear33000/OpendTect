/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emobject.cc,v 1.19 2004-05-12 08:38:08 kristofer Exp $";

#include "emobject.h"

#include "color.h"
#include "emhorizontransl.h"
#include "emfaulttransl.h"
#include "emsticksettransl.h"
#include "emmanager.h"
#include "ioobj.h"
#include "ptrman.h"
#include "ioman.h"


int EM::EMObject::sPermanentControlNode	= 0;
int EM::EMObject::sTemporaryControlNode	= 1;
int EM::EMObject::sEdgeControlNode	= 2;

EM::EMObject* EM::EMObject::create( const IOObj& ioobj, EM::EMManager& manager )
{
    EM::EMObject* res = 0;
    const char* group = ioobj.group();

    const EM::ObjectID id = EM::EMManager::multiID2ObjectID(ioobj.key());

    if ( !strcmp( group, EMHorizonTranslatorGroup::keyword ))
	res = new EM::Horizon( manager, id );
    else if ( !strcmp( group, EMFaultTranslatorGroup::keyword ))
	res = new EM::Fault( manager, id );
    else if ( !strcmp( group, EMStickSetTranslatorGroup::keyword ))
	res = new EM::StickSet( manager, id );

    return res;
}


void EM::EMObject::ref() const
{
    const_cast<EM::EMObject*>(this)->manager.ref(id());
}


void EM::EMObject::unRef() const
{
    const_cast<EM::EMObject*>(this)->manager.unRef(id());
}


void EM::EMObject::unRefNoDel() const
{
    const_cast<EM::EMObject*>(this)->manager.unRefNoDel(id());
}


EM::EMObject::EMObject( EMManager& emm_, const EM::ObjectID& id__ )
    : manager( emm_ )
    , poschnotifier( this )
    , id_( id__ )
    , prefColorChange( this )
    , preferredcolor( *new Color(255, 0, 0) )
{
    posattrchnotifiers.allowNull();
}


EM::EMObject::~EMObject()
{
    deepErase( posattribs );
    deepErase( posattrchnotifiers );
    delete &preferredcolor;
}


BufferString EM::EMObject::name() const
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    return ioobj ? ioobj->name() : BufferString("");
}


const Color& EM::EMObject::preferredColor() const
{ return preferredcolor; }


void EM::EMObject::setPreferredColor(const Color& col)
{
    if ( col==preferredcolor )
	return;

    preferredcolor = col;
    prefColorChange.trigger();
}


MultiID EM::EMObject::multiID() const
{
    MultiID res = getIOObjContext().stdSelKey();
    res.add(id());
    return res;
}


void EM::EMObject::setPosAttrib( const EM::PosID& pid, int attr, bool yn )
{
    const int idx=attribs.indexOf(attr);
    if ( idx==-1 )
    {
	if ( yn )
	{
	    attribs += attr;
	    posattribs += new TypeSet<EM::PosID>(1,pid);
	    posattrchnotifiers += 0;
	}
    }
    else
    {
	TypeSet<EM::PosID>& posids = *posattribs[idx];
	const int idy=posids.indexOf(pid);

	if ( idy==-1 )
	{
	    if ( yn ) posids += pid;
	}
	else if ( !yn )
	{
	    posids.removeFast(idy);
	}
    }

    CNotifier<EMObject, PosID>* notifier = getPosAttribChNotifier(attr,false);
    if ( notifier ) notifier->trigger( pid, this );
}


bool EM::EMObject::isPosAttrib( const EM::PosID& pid, int attr ) const
{
    const int idx=attribs.indexOf(attr);
    if ( idx==-1 )
	return false;

    TypeSet<EM::PosID>& posids = *posattribs[idx];
    const int idy=posids.indexOf(pid);

    if ( idy==-1 )
	return false;

    return true;
}


const char* EM::EMObject::posAttribName( int idx ) const
{
    return 0;
}


int EM::EMObject::nrPosAttribs() const
{
    return -1;
}


int EM::EMObject::addPosAttribName( const char* name )
{
    return -1;
}


CNotifier<EM::EMObject, EM::PosID>*
EM::EMObject::getPosAttribChNotifier( int attr, bool create )
{
    int idx=attribs.indexOf(attr);
    if ( idx==-1 )
    {
	if ( !create ) return 0;

	attribs += attr;
	posattribs += new TypeSet<EM::PosID>;
	posattrchnotifiers += 0;
	idx = 0;
    }

    if ( !posattrchnotifiers[idx] && create )
	posattrchnotifiers.replace( new CNotifier<EMObject, PosID>(this),idx );

    return posattrchnotifiers[idx];
}


