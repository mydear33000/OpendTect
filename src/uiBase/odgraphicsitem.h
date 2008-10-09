#ifndef odgraphicsitem_h
#define odgraphicsitem_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		April 2008
 RCS:		$Id: odgraphicsitem.h,v 1.3 2008-10-09 06:35:33 cvssatyaki Exp $
________________________________________________________________________

-*/

#include <QGraphicsItem>

#include "uienums.h"
#include "draw.h"

class QTextOption;
class ioPixmap;

class ODGraphicsPointItem : public QAbstractGraphicsShapeItem
{
public:
    				ODGraphicsPointItem()
				    : highlight_(false)
				    , penwidth_(2)
				    , pencolor_(Color::Black)	{}

    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);
    void 			drawPoint(QPainter*);
    void			setHighLight( bool hl )
				{ highlight_ = hl ; }

protected:
    bool			highlight_;
    int				penwidth_;
    Color			pencolor_;
};



class ODGraphicsMarkerItem : public QAbstractGraphicsShapeItem
{
public:
    				ODGraphicsMarkerItem();

    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);
    void 			drawMarker(QPainter&);
    void			setMarkerStyle(const MarkerStyle2D&);
    void			setAngle( float angle )	  { angle_ = angle; }
    void			setSideLength( int side ) { side_ = side; }

protected:
    QRectF			boundingrect_;
    MarkerStyle2D*		mstyle_;	
    float			angle_;	
    int 			side_;	
    const QPoint* 		qpoint_;	
};


class ODGraphicsPixmapItem : public QGraphicsPixmapItem
{
public:
    				ODGraphicsPixmapItem();
    				ODGraphicsPixmapItem(const ioPixmap&);
    void                        paint(QPainter*,const QStyleOptionGraphicsItem*,
				      QWidget*);
};


class ODGraphicsArrowItem : public QAbstractGraphicsShapeItem
{
public:
    				ODGraphicsArrowItem();
    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);
    void 			drawArrow(QPainter&);
    double 			getAddedAngle(double,float);
    QPoint 			getEndPoint(const QPoint&,double,double);
    void 			drawArrowHead(QPainter&,const QPoint&,
	    				      const QPoint&);
    void			setArrowStyle( const ArrowStyle& arrowstyle )
    				{ arrowstyle_ = arrowstyle ; }
    void			setArrowSize( const int arrowsz )
    				{ arrowsz_ = arrowsz ; }
    void			setLineStyle(QPainter&,const LineStyle&);

protected:
    QRectF*			boundingrect_;
    ArrowStyle			arrowstyle_;
    QPoint*			qpointhead_;
    QPoint*			qpointtail_;
    int				arrowsz_;
};


class ODGraphicsTextItem : public QGraphicsTextItem
{
public:
    				ODGraphicsTextItem();
    QRectF			boundingRect() const;
    void 			paint(QPainter*,const QStyleOptionGraphicsItem*,
	    		              QWidget*);
    void 			setTextAlignment(OD::Alignment);
    void			setText(const char*);
protected:
    QRectF			boundingrect_;
    QString*			text_;
    QTextOption*		alignoption_;
};

#endif
