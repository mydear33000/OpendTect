#ifndef uiflatviewcontrol_h
#define uiflatviewcontrol_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewcontrol.h,v 1.16 2007-05-16 16:30:20 cvskris Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "iopar.h"
class uiFlatViewer;
class uiFlatViewPropDlg;
class MouseEventHandler;
namespace FlatView { class ZoomMgr; class Viewer; }


/*!\brief Tools to control uiFlatViewer(s). */

class uiFlatViewControl : public uiGroup
{
public:

			~uiFlatViewControl();

    void		addViewer(uiFlatViewer&);
    			//!< No attaching done. Viewer may be in other window.

    uiWorldRect		getBoundingBox() const;
    void		setNewView(Geom::Point2D<double>& centre,
	    			   Geom::Size2D<double>& sizes);
    			//!< retains uiWorldRect's LR/TB swapping
    			//!< Changes the input to the actual new values
    void		flip(bool hor);
    			//!< reverses uiWorldRect's LR or TB swapping

    static bool		havePan(Geom::Point2D<double> oldcentre,
	    			Geom::Point2D<double> newcentre,
				Geom::Size2D<double> sz);
    static bool		haveZoom(Geom::Size2D<double> oldsz,
				 Geom::Size2D<double> newsz);

    void		setCategory( const char* cat )	{ category_ = cat; }
    			//!< Set the display pars defaults' `category`
   			//!< Needed if the main viewer uses no data pack
    const char*		category() const;
    			//!< If not set, returns data pack's category()

    Notifier<uiFlatViewControl>  infoChanged;	// CallBacker: CBCapsule<IOPar>

protected:

    			uiFlatViewControl(uiFlatViewer&,uiParent*,bool);

    ObjectSet<uiFlatViewer> vwrs_;
    FlatView::ZoomMgr&	zoommgr_;
    bool		haverubber_;
    BufferString	category_;
    IOPar		infopars_;

    uiFlatViewPropDlg*  propdlg_;

    MouseEventHandler&	mouseEventHandler(int);
    uiWorldRect		getZoomAndPanRect(Geom::Point2D<double>,
	    				  Geom::Size2D<double>,
					  const uiWorldRect& bbox) const;
    uiWorldRect		getZoomOrPanRect(Geom::Point2D<double>,
	    				 Geom::Size2D<double>,
					 const uiWorldRect& bbox) const;

    virtual void	finalPrepare()			{}
    virtual void	onFinalise(CallBacker*);
    
    virtual void	dataChangeCB(CallBacker*);
    virtual void	rubBandCB(CallBacker*);
    virtual void	mouseMoveCB(CallBacker*);
    void		usrClickCB(CallBacker*);
    virtual bool	handleUserClick()		{ return false; }
    
    void		doPropertiesDialog(int vieweridx=0);
    void		propDlgClosed(CallBacker*);
    void		applyProperties(CallBacker* cb);
    void		saveProperties(FlatView::Viewer&);

};

#endif
