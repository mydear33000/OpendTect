#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "uiodmainmod.h"
#include "uioddatatreeitem.h"
#include "datapack.h"

class AttribProbeLayer;

/*! Implementation of uiODDataTreeItem for standard attribute displays. */

mExpClass(uiODMain) uiODAttribTreeItem : public uiODDataTreeItem
{ mODTextTranslationClass(mODTextTranslationClass);
public:
    			uiODAttribTreeItem( const char* parenttype );
			~uiODAttribTreeItem();

    const AttribProbeLayer* attribProbeLayer() const;
    AttribProbeLayer*	attribProbeLayer();

    static uiString	createDisplayName( int visid, int attrib );
    static uiString	sKeySelAttribMenuTxt();
    static uiString	sKeyColSettingsMenuTxt();

protected:

    bool		anyButtonClick(uiTreeViewItem*);

    void		createMenu(MenuHandler*,bool istb);
    void		handleMenuCB(CallBacker*);
    virtual void	updateDisplay();
    void		updateColumnText(int col);
    uiString		createDisplayName() const;
    void		createSelMenu(MenuItem&);
    bool		handleSelMenu(int mnuid);
    ConstRefMan<DataPack> calculateAttribute();
    virtual DataPackMgr& getDPM();

    MenuItem		selattrmnuitem_;
    MenuItem		colsettingsmnuitem_;
};
