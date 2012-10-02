#ifndef uimultiflatviewcontrol_h
#define uimultiflatviewcontrol_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Feb 2012
 RCS:           $Id: uimultiflatviewcontrol.h,v 1.9 2012/05/23 15:09:37 cvsbruno Exp $
________________________________________________________________________

-*/

/*! brief : a control for FlatViewers with different zoom properties and settings !*/

#include "uiflatviewstdcontrol.h"
#include "flatview.h"

mClass uiMultiFlatViewControl : public uiFlatViewStdControl
{
public:
    			uiMultiFlatViewControl(uiFlatViewer&,const Setup&);
    			~uiMultiFlatViewControl();

    void                setNewView(Geom::Point2D<double>& centre,
	                                       Geom::Size2D<double>& sizes);

    uiToolBar*		getToolBar(int idx) { return toolbars_[idx]; }

    uiFlatViewer*	activeVwr() const   { return activevwr_; }

    void		setZoomCoupled( bool yn ) { iszoomcoupled_ = yn; }
    void		setDrawZoomBoxes( bool yn ) { drawzoomboxes_ = yn; }

    void		reInitZooms();

protected:
    ObjectSet<FlatView::ZoomMgr> zoommgrs_;
    ObjectSet<uiToolBar> toolbars_;
    ObjectSet<uiToolButton> parsbuts_;
    ObjectSet<FlatView::AuxData> zoomboxes_;

    bool		handleUserClick();
    bool		iszoomcoupled_;
    bool		drawzoomboxes_;

    uiFlatViewer*	activevwr_;

    void		rubBandCB(CallBacker*);
    void		parsCB(CallBacker*);
    void		dataChangeCB(CallBacker*);
    void		setZoomAreasCB(CallBacker*);
    void		setZoomBoxesCB(CallBacker*);
    void		vwrAdded(CallBacker*);
    void		zoomCB(CallBacker*);
    void		wheelMoveCB(CallBacker*);
};

#endif
