#ifndef uiempartserv_h
#define uiempartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Sep 2002
 RCS:           $Id: uiempartserv.h,v 1.51 2007-08-07 05:05:32 cvsraman Exp $
________________________________________________________________________

-*/

#include "emposid.h"
#include "multiid.h"
#include "uiapplserv.h"


class BinID;
class BinIDRange;
class BinIDValueSet;
class BufferStringSet;
class MultiID;
class SurfaceInfo;
class uiPopupMenu;

namespace Pick { class Set; }

template <class T> class Interval;

namespace EM { class EMManager; class SurfaceIODataSelection; };

/*! \brief Earth Model UI Part Server */

class uiEMPartServer : public uiApplPartServer
{
public:
			uiEMPartServer(uiApplService&);
			~uiEMPartServer();

    const char*		name() const			{ return "EarthModel"; }

			// Services
    bool		importHorizon();
    bool		importHorizonAttribute();
    bool		exportHorizon();

    MultiID		getStorageID( const EM::ObjectID& ) const;
    EM::ObjectID	getObjectID( const MultiID& ) const;

    BufferString	getName(const EM::ObjectID&) const;
    const char*		getType(const EM::ObjectID&) const;

    bool		isChanged(const EM::ObjectID&) const;
    bool		isEmpty(const EM::ObjectID&) const;
    bool		isFullResolution(const EM::ObjectID&) const;
    bool		isFullyLoaded(const EM::ObjectID&) const;
    bool		isShifted(const EM::ObjectID&) const;

    void		fillHoles(const EM::ObjectID&);
    void		filterSurface(const EM::ObjectID&);
    void		fillPickSet(Pick::Set&,MultiID);
    void		deriveHor3DFrom2D(const EM::ObjectID&);
    void		askUserToSave(const EM::ObjectID&) const;
    			/*!< If object has changed, user is asked whether
			    to save it or not, and if so, the object is saved */

    void		selectHorizons(TypeSet<EM::ObjectID>&);
    void		select2DHorizons(TypeSet<EM::ObjectID>&);
    void		selectFaults(TypeSet<EM::ObjectID>&);
    bool		showLoadAuxDataDlg(const EM::ObjectID&);
    int			loadAuxData(const EM::ObjectID&,const char*);
    			/*!<Loads the specified data into memory and returns
			    its auxdatanr. */

    bool		importLMKFault();

    void		manageSurfaces(const char* typ);
    bool		loadSurface(const MultiID&,
	    			    const EM::SurfaceIODataSelection* s=0);
    void		getSurfaceInfo(ObjectSet<SurfaceInfo>&);
    void                getAllSurfaceInfo(ObjectSet<SurfaceInfo>&);
    void		getSurfaceDef(const TypeSet<EM::ObjectID>&,
	    			      BinIDValueSet&,
				      const BinIDRange* br=0) const;

    bool		storeObject(const EM::ObjectID&,
	    			    bool storeas=false) const;
    bool		storeAuxData(const EM::ObjectID&,
	    			     bool storeas=false) const;
    int			setAuxData(const EM::ObjectID&,
	    			   ObjectSet<const BinIDValueSet>&,
				   const char* nm,int valnr);
    bool		getAuxData(const EM::ObjectID&,int auxdatanr,
	    			   BufferString& auxdataname,
	    			   ObjectSet<BinIDValueSet>&) const;
    bool		getAllAuxData(const EM::ObjectID&,BufferStringSet&,
	    			      ObjectSet<BinIDValueSet>&) const;

    void		removeUndo();

    static const int	evDisplayHorizon;
    static const int	evRemoveTreeObject;

			// Interaction stuff
    const EM::ObjectID&	selEMID() const			{ return selemid_; }

    void		removeTreeObject(const EM::ObjectID&);  

protected:
    void		selectSurfaces(TypeSet<EM::ObjectID>&,
				       const BufferString& typ);
    bool		loadAuxData(const EM::ObjectID&, const TypeSet<int>& );

    EM::ObjectID	selemid_;
    EM::EMManager&	em_;

    bool		ioHorizon(bool);
};


/*!\mainpage Earth Model User Interface

 The earth model objects are visualised by the visXX classes. The I/O,
 parameters and other data-related issues also require standard user interface
 elements. The classes in this module provide these services via the
 uiEMPartServer interface.

*/

#endif
