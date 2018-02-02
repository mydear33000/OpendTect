#pragma once

/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		October 2016
___________________________________________________________________

-*/

#include "geometrymod.h"
#include "uistring.h"
#include "probe.h"
#include "emposid.h"


mExpClass(EarthModel) EMProbe : public Probe
{
public:
				~EMProbe();

    mDeclInstanceCreatedNotifierAccess(EMProbe);
    mDeclAbstractMonitorableAssignment(EMProbe);

    mImplSimpleMonitoredGet(	id,DBKey,objid_)
    void			setID(const DBKey&);
    void			updateAll();

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

protected:

				EMProbe(const DBKey&);

    uiWord			gtDisplayName() const;

    DBKey			objid_;

};


mExpClass(EarthModel) Horizon3DProbe : public EMProbe
{
public:

				Horizon3DProbe(const DBKey&);
				~Horizon3DProbe();
    mDeclInstanceCreatedNotifierAccess(Horizon3DProbe);
    mDeclMonitorableAssignment(Horizon3DProbe);

    mDeclRequiredProbeFns();

    virtual bool		isVertical() const	{ return false; }

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

};
