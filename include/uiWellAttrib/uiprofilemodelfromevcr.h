#ifndef uiprofilemodelfromevcr_h
#define uiprofilemodelfromevcr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		December 2016
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uigroup.h"
#include "uidialog.h"
#include "datapack.h"

namespace FlatView { class AuxData; }
class uiGenInput;
class uiToolButton;
class uiListBox;
class uiFlatViewer;

class ProfileModelBase;
class ProfileModelFromEventData;
class ProfileModelBaseAuxDataMgr;
class ZValueProvider;

mExpClass(uiWellAttrib) uiProfileModelFromEvCrGrp : public uiGroup
{ mODTextTranslationClass(uiProfileModelFromEvCrGrp);
public:
				uiProfileModelFromEvCrGrp(
					uiParent*, ProfileModelFromEventData&);
				~uiProfileModelFromEvCrGrp();
    int				nrProfs() const;
    virtual void		updateDisplay();

protected:

    uiGroup*				paramgrp_;
    uiGenInput*				nrprofsfld_;
    uiFlatViewer*			viewer_;
    uiListBox*				evlistbox_;
    uiToolButton*			addevbut_;
    uiToolButton*			rmevbut_;
    uiToolButton*			tiemarkerbut_;
    uiToolButton*			applybut_;
    ProfileModelFromEventData&		data_;
    ObjectSet<FlatView::AuxData>	horauxdatas_;
    ProfileModelBaseAuxDataMgr*		modeladmgr_;;

    void				addEventCB(CallBacker*);
    void				removeEventCB(CallBacker*);
    void				tieEventsCB(CallBacker*);
    void				createModelCB(CallBacker*);
    void				showCtrlProfilesCB(CallBacker*);

    void				updateProfileModelDisplay();
    virtual void			getEvents()			=0;
    void				drawEvents();
    void				checkAndRemoveEvents();
};


mExpClass(uiWellAttrib) uiProfileModelFromEvCrGrpFactory
{
public:
    typedef uiProfileModelFromEvCrGrp*
				(*CreateFunc)(uiParent*,
					      ProfileModelFromEventData&);
    void			addCreateFunc(CreateFunc,const char*);
    uiProfileModelFromEvCrGrp*	create(const char*,uiParent*,
				       ProfileModelFromEventData&);
    const BufferStringSet&	factoryNames() const	{ return keys_; }
protected:
    TypeSet<CreateFunc>		createfuncs_;
    BufferStringSet		keys_;
};


mGlobal(uiWellAttrib) uiProfileModelFromEvCrGrpFactory& uiPMCrGrpFac();


mExpClass(uiWellAttrib) uiProfileModelFromEvCrDlg : public uiDialog
{ mODTextTranslationClass(uiProfileModelFromEvCrDlg);
public:
				uiProfileModelFromEvCrDlg(uiParent*,
					ProfileModelFromEventData&,
					const char* typenm);
protected:
    void			finaliseCB(CallBacker*);

    uiProfileModelFromEvCrGrp*	profscrgrp_;
};


#endif
