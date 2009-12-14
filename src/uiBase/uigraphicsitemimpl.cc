/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigraphicsitemimpl.cc,v 1.33 2009-12-14 07:44:07 cvsnanne Exp $";

#include "uigraphicsitemimpl.h"

#include "angles.h"
#include "odgraphicsitem.h"
#include "pixmap.h"
#include "polygon.h"
#include "uifont.h"
#include "uigroup.h"
#include "uiobj.h"

#include <QBrush>
#include <QFont>
#include <QFontMetrics>
#include <QGraphicsProxyWidget>
#include <QPen>
#include <QTextDocument>


uiObjectItem::uiObjectItem( uiObject* obj )
    : uiGraphicsItem(mkQtObj())
    , obj_(0)
    , grp_(0)
{
    setObject( obj );
}


uiObjectItem::uiObjectItem( uiGroup* grp )
    : uiGraphicsItem(mkQtObj())
    , obj_(0)
    , grp_(0)
{
    setGroup( grp );
}


uiObjectItem::~uiObjectItem()
{
    delete qwidgetitem_;
}


uiObject* uiObjectItem::getObject()
{ return obj_; }


void uiObjectItem::setObject( uiObject* obj )
{
    obj_ = obj;
    qwidgetitem_->setWidget( obj_ ? obj_->qwidget() : 0 );
}


uiGroup* uiObjectItem::getGroup()
{ return grp_; }

void uiObjectItem::setGroup( uiGroup* grp )
{
    grp_ = grp;
    if ( !grp_ ) return;

    setObject( grp->attachObj() );
}


QGraphicsItem* uiObjectItem::mkQtObj()
{
    qwidgetitem_ = new QGraphicsProxyWidget();
    return qwidgetitem_;
}



uiEllipseItem::uiEllipseItem()
    : uiGraphicsItem(mkQtObj())
{}


uiEllipseItem::uiEllipseItem( const uiPoint& center, const uiSize& sz )
    : uiGraphicsItem(mkQtObj())
{
    setPos( center );
    setSize( sz );
}


uiEllipseItem::uiEllipseItem( const uiSize& size )
    : uiGraphicsItem(mkQtObj())
{
    setSize( size );
}


uiEllipseItem::~uiEllipseItem()
{
    delete qellipseitem_;
}


QGraphicsItem* uiEllipseItem::mkQtObj()
{
    qellipseitem_ = new QGraphicsEllipseItem();
    return qellipseitem_;
}


void uiEllipseItem::setSize( const uiSize& size )
{
    const int width = size.hNrPics();
    const int height = size.vNrPics();
    qellipseitem_->setRect( -width/2, -height/2, width, height );
}


uiCircleItem::uiCircleItem()
    : uiEllipseItem()
{}


uiCircleItem::uiCircleItem( const uiPoint& center, int r )
    : uiEllipseItem(center,uiSize(2*r,2*r))
{}


uiCircleItem::uiCircleItem( int r )
    : uiEllipseItem(uiSize(2*r,2*r))
{}


void uiCircleItem::setRadius( int r )
{
    setSize( uiSize(2*r,2*r) );
}


// ***** uiLineItem *****

uiLineItem::uiLineItem()
    : uiGraphicsItem(mkQtObj())
{}


uiLineItem::uiLineItem( const uiPoint& startpos, const uiPoint& endpos,
			bool abspos )
    : uiGraphicsItem(mkQtObj())
{
    setLine( startpos, endpos, abspos );
}


uiLineItem::uiLineItem( float x1, float y1, float x2, float y2, bool abspos )
    : uiGraphicsItem(mkQtObj())
{
    setLine( mNINT(x1), mNINT(y1), mNINT(x2), mNINT(y2), abspos );
}


uiLineItem::uiLineItem( const uiPoint& pt, double angle, double len,
			bool abspos )
    : uiGraphicsItem(mkQtObj())
{
    uiPoint endpt( pt );
    double delta = len * cos( angle );
    endpt.x += mNINT(delta);
    delta = -len * sin( angle );
    endpt.y += mNINT(delta);

    setLine( pt, endpt, abspos );
}


uiLineItem::~uiLineItem()
{
    delete qlineitem_;
}


QGraphicsItem* uiLineItem::mkQtObj()
{
    qlineitem_ = new QGraphicsLineItem();
    return qlineitem_;
}


void uiLineItem::setLine( const uiPoint& start, const uiPoint& end, bool abs )
{
    setLine( start.x, start.y, end.x, end.y, abs );
}


void uiLineItem::setLine( int x1, int y1, int x2, int y2, bool abs )
{
    if ( !abs )
	qlineitem_->setLine( x1, y1, x2, y2 );
    else
    {
	qlineitem_->setLine( 0, 0, x2-x1, y2-y1 );
	qlineitem_->setPos( x1, y1 );
    }
}


void uiLineItem::setStartPos( const uiPoint& start, bool abspos )
{
    QLineF qline = qlineitem_->line();
}


void uiLineItem::setEndPos( const uiPoint& end, bool abspos )
{
    QLineF qline = qlineitem_->line();
}


uiRect uiLineItem::lineRect() const
{
    QLineF qline = qlineitem_->line();
    return uiRect( (int)qline.x1(), (int)qline.y1(),
		   (int)qline.x2(), (int)qline.y2() );
}


void uiLineItem::setPenColor( const Color& col )
{
    QPen qpen = qlineitem_->pen();
    qpen.setColor( QColor(col.rgb()) );
    qlineitem_->setPen( qpen );
}


void uiLineItem::setPenStyle( const LineStyle& ls )
{
    QBrush qbrush( QColor(QRgb(ls.color_.rgb())) );
    QPen qpen( qbrush, ls.width_, (Qt::PenStyle)ls.type_ );
    qlineitem_->setPen( qpen );
}



uiPixmapItem::uiPixmapItem()
    : uiGraphicsItem(mkQtObj())
{}


uiPixmapItem::uiPixmapItem( const ioPixmap& pm )
    : uiGraphicsItem(mkQtObj())
{
    setPixmap( pm );
}


uiPixmapItem::uiPixmapItem( const uiPoint& pos, const ioPixmap& pm )
    : uiGraphicsItem(mkQtObj())
{
    setPos( pos );
    setPixmap( pm );
}


uiPixmapItem::~uiPixmapItem()
{
    delete qpixmapitem_;
}


QGraphicsItem* uiPixmapItem::mkQtObj()
{
    qpixmapitem_ = new ODGraphicsPixmapItem();
    return qpixmapitem_;
}


void uiPixmapItem::setOffset( int left, int top )
{
    qpixmapitem_->setOffset( QPointF(left,top) );
}


void uiPixmapItem::setPixmap( const ioPixmap& pixmap )
{
    qpixmapitem_->setPixmap( *pixmap.qpixmap());
}



uiPolygonItem::uiPolygonItem( QGraphicsPolygonItem* qtobj )
    : uiGraphicsItem(qtobj)
    , qpolygonitem_(qtobj)
{}


uiPolygonItem::uiPolygonItem()
    : uiGraphicsItem(mkQtObj())
{}


uiPolygonItem::uiPolygonItem( const ODPolygon<int>& polygon, bool dofill )
    : uiGraphicsItem(mkQtObj())
{
    setPolygon( polygon );
    if ( dofill )
	fill();
}


uiPolygonItem::uiPolygonItem( const TypeSet<uiPoint>& polygon, bool dofill )
    : uiGraphicsItem(mkQtObj())
{
    setPolygon( polygon );
    if ( dofill )
	fill();
}

uiPolygonItem::~uiPolygonItem()
{
    delete qpolygonitem_;
}


QGraphicsItem* uiPolygonItem::mkQtObj()
{
    qpolygonitem_ = new QGraphicsPolygonItem();
    return qpolygonitem_;
}


void uiPolygonItem::fill()
{
    qpolygonitem_->setFillRule( Qt::OddEvenFill );
}


void uiPolygonItem::setPolygon( const TypeSet<uiPoint>& ptlist )
{
    QPolygon qpolygon( ptlist.size() );
    for ( int idx=0; idx<ptlist.size(); idx++ )
	qpolygon.setPoint( (unsigned int)idx, ptlist[idx].x, ptlist[idx].y );
    QPolygonF qpolygonf(qpolygon);
    qpolygonitem_->setPolygon( qpolygonf );
}


void uiPolygonItem::setPolygon( const ODPolygon<int>& polygon )
{
    TypeSet<uiPoint> ptlist;
    for ( int idx=0; idx<polygon.size(); idx++ )
	ptlist += polygon.data()[idx];
    setPolygon( ptlist );
}


uiPolyLineItem::uiPolyLineItem()
    : uiGraphicsItem(mkQtObj())
{}


uiPolyLineItem::uiPolyLineItem( const TypeSet<uiPoint>& ptlist )
    : uiGraphicsItem(mkQtObj())
{
    setPolyLine( ptlist );
}


uiPolyLineItem::~uiPolyLineItem()
{
    delete qpolylineitem_;
}


void uiPolyLineItem::setPolyLine( const TypeSet<uiPoint>& ptlist )
{
    QPolygon qpolygon( ptlist.size() );
    for ( int idx=0; idx<ptlist.size(); idx++ )
	qpolygon.setPoint( (unsigned int)idx, ptlist[idx].x, ptlist[idx].y );
    qpolylineitem_->setPolyLine( qpolygon );
}


QGraphicsItem* uiPolyLineItem::mkQtObj()
{
    qpolylineitem_ = new ODGraphicsPolyLineItem();
    return qpolylineitem_;
}


uiRectItem::uiRectItem()
    : uiGraphicsItem(mkQtObj())
{}


uiRectItem::uiRectItem( QGraphicsRectItem* qtobj )
    : uiGraphicsItem(qtobj)
    , qrectitem_(qtobj)
{}


uiRectItem::uiRectItem( int x, int y, int width, int height )
    : uiGraphicsItem(mkQtObj())
{
    setRect( x, y, width, height );
}


uiRectItem::~uiRectItem()
{
    delete qrectitem_;
}


QGraphicsItem* uiRectItem::mkQtObj()
{
    qrectitem_ = new QGraphicsRectItem();
    return qrectitem_;
}


void uiRectItem::setRect( int x, int y, int width, int height )
{
    qrectitem_->setRect( 0, 0, width, height );
    qrectitem_->setPos( x, y );
}


uiTextItem::uiTextItem()
    : uiGraphicsItem(mkQtObj())
    , pos_(0,0)
    , al_(Alignment::Left,Alignment::Top)
{
}


uiTextItem::uiTextItem( const char* txt, const Alignment& al )
    : uiGraphicsItem(mkQtObj())
    , pos_(0,0)
    , al_(al)
{
    setText( txt );
}



uiTextItem::uiTextItem( const uiPoint& pos, const char* txt,
			const Alignment& al )
    : uiGraphicsItem(mkQtObj())
    , pos_(pos)
    , al_(al)
{
    setText( txt );
    updatePos();
}


uiTextItem::~uiTextItem()
{
    delete qtextitem_;
}


QGraphicsItem* uiTextItem::mkQtObj()
{
    qtextitem_ = new QGraphicsTextItem();
    return qtextitem_;
}


uiRect uiTextItem::getTextRect() const
{
    QRect rect( qtextitem_->boundingRect().toRect().topLeft(),
	    	qtextitem_->boundingRect().toRect().bottomRight() );
    return uiRect( rect.topLeft().x(), rect.topLeft().y(),
	    	   rect.bottomRight().x(), rect.bottomRight().y() );
}


void uiTextItem::setText( const char* txt )
{
    qtextitem_->setPlainText( QString(txt) );
}


void uiTextItem::setHtmlText( const char* txt )
{
    qtextitem_->setHtml( QString(txt) );
}


void uiTextItem::setFont( const uiFont& font )
{
    qtextitem_->setFont( font.qFont() );
}


int uiTextItem::getTextWidth() const
{
    return (int)qtextitem_->textWidth();
}


void uiTextItem::setAlignment( const Alignment& al )
{
    al_ = al;
    updatePos();
}


void uiTextItem::setPos( const uiPoint& pos )
{
    pos_ = pos;
    updatePos();
}


void uiTextItem::setPos( int x, int y )
{
    setPos( uiPoint(x,y) );
}


void uiTextItem::updatePos()
{
    QFontMetrics qfm( qtextitem_->font() );
    const float txtwidth = qtextitem_->document()->size().width();
    const float txtheight = qfm.height();
    float movex = 0, movey = 0;
    switch ( al_.hPos() )
    {
	case Alignment::Right:
	    movex = -txtwidth;
	    break;
	case Alignment::HCenter:
	    movex = -txtwidth/2;
	    break;
	case Alignment::Left:
	    break;
    }
    
    switch ( al_.vPos() )
    {
	case Alignment::Bottom:
	    movey = -txtheight;
	    break;
	case Alignment::VCenter:
	    movey = -txtheight/2;
	    break;
	case Alignment::Top:
	    break;
    }

    uiPoint newpos( mNINT(pos_.x+movex), mNINT(pos_.y+movey) );
    qtextitem_->setPos( newpos.x, newpos.y );
}


void uiTextItem::setTextColor( const Color& col )
{
    qtextitem_->setDefaultTextColor( QColor(QRgb(col.rgb())) );
}

uiMarkerItem::uiMarkerItem( bool fill )
    : uiGraphicsItem( mkQtObj() )
{
    setFill( fill );
}


uiMarkerItem::uiMarkerItem( const uiPoint& pos, const MarkerStyle2D& mstyle,
			    bool fill )
    : uiGraphicsItem( mkQtObj() )
{
    setPos( pos );
    qmarkeritem_->setMarkerStyle( mstyle );
    setFill( fill );
}


uiMarkerItem::uiMarkerItem( const MarkerStyle2D& mstyle, bool fill )
    : uiGraphicsItem( mkQtObj() )
{
    qmarkeritem_->setMarkerStyle( mstyle );
    setFill( fill ); // ToDo: Get fill info from mstyle
}


QGraphicsItem* uiMarkerItem::mkQtObj()
{
    qmarkeritem_ = new ODGraphicsMarkerItem();
    return qmarkeritem_;
}


uiMarkerItem::~uiMarkerItem()
{
    delete qmarkeritem_;
}


void uiMarkerItem::setMarkerStyle( const MarkerStyle2D& mstyle )
{
    qmarkeritem_->setMarkerStyle( mstyle );
}


void uiMarkerItem::setFill( bool fill )
{
    qmarkeritem_->setFill( fill );
}



uiPointItem::uiPointItem( const uiPoint& pos )
    : uiGraphicsItem(mkQtObj())
{
    setPos( pos );
}


uiPointItem::uiPointItem()
    : uiGraphicsItem(mkQtObj())
{}


uiPointItem::~uiPointItem()
{
    delete qpointitem_;
}


QGraphicsItem* uiPointItem::mkQtObj()
{
    qpointitem_ = new ODGraphicsPointItem();
    return qpointitem_;
}


uiArrowItem::uiArrowItem()
    : uiGraphicsItem(mkQtObj())
{}


uiArrowItem::uiArrowItem( const uiPoint& tail, const uiPoint& head,
			  const ArrowStyle& style )
    : uiGraphicsItem(mkQtObj())
{
    tailpos_ = tail;
    headpos_ = head;
    setArrowStyle( style );
    update();
}


uiArrowItem::~uiArrowItem()
{
    delete qarrowitem_;
}


QGraphicsItem* uiArrowItem::mkQtObj()
{
    qarrowitem_ = new ODGraphicsArrowItem();
    return qarrowitem_;
}


void uiArrowItem::setHeadPos( const uiPoint& pt )
{
    headpos_ = pt;
    update();
}


void uiArrowItem::setTailPos( const uiPoint& pt )
{
    tailpos_ = pt;
    update();
}


void uiArrowItem::setTailHeadPos( const uiPoint& tail, const uiPoint& head )
{
    tailpos_ = tail;
    headpos_ = head;
    update();
}


void uiArrowItem::setArrowStyle( const ArrowStyle& arrowstyle )
{
    qarrowitem_->setArrowStyle( arrowstyle );
}


void uiArrowItem::setArrowSize( int arrowsz )
{
    qarrowitem_->setArrowSize( arrowsz );
}


void uiArrowItem::update()
{
    qarrowitem_->resetTransform();

    float diffx = headpos_.x-tailpos_.x;
    float diffy = headpos_.y-tailpos_.y;
    const float arrsz = sqrt( diffx*diffx + diffy*diffy );
    setArrowSize( mNINT(arrsz) );
    setPos( headpos_ );
    const uiPoint relvec( mNINT(diffx), mNINT(diffy) );
    const float ang = atan2((float)relvec.y,(float)relvec.x) * 180/M_PI;
    rotate( ang );
}



uiCurvedItem::uiCurvedItem( const uiPoint& pt )
    : uiGraphicsItem(mkQtObj())
{
    qppath_->moveTo( QPointF(pt.x,pt.y) );
}


uiCurvedItem::uiCurvedItem( const Geom::Point2D<float>& pt )
    : uiGraphicsItem(mkQtObj())
{
    qppath_->moveTo( QPointF(pt.x,pt.y) );
    qpathitem_->setPath( *qppath_ );
}


uiCurvedItem::~uiCurvedItem()
{
    delete qpathitem_;
    delete qppath_;
}


void uiCurvedItem::drawTo( const uiPoint& pt )
{
    drawTo( Geom::Point2D<float>(pt.x,pt.y) );
}
void uiCurvedItem::drawTo( const Geom::Point2D<float>& pt )
{
    qppath_->lineTo( QPointF(pt.x,pt.y) );
    qpathitem_->setPath( *qppath_ );
}


void uiCurvedItem::drawTo( const ArcSpec& as )
{
    Interval<float> angs( Angle::rad2deg(as.angles_.start),
			  Angle::rad2deg(as.angles_.stop) );
    QRectF qr( as.center_.x - as.radius_, as.center_.y - as.radius_,
	       2*as.radius_, 2*as.radius_ );
    qppath_->arcTo( qr, angs.start, angs.stop - angs.start );
    qpathitem_->setPath( *qppath_ );
}


void uiCurvedItem::drawTo( const SplineSpec& ss )
{
    if ( ss.cubic_ )
	qppath_->cubicTo( QPointF(ss.cp1_.x,ss.cp1_.y),
			  QPointF(ss.cp2_.x,ss.cp2_.y),
			  QPointF(ss.end_.x,ss.end_.y) );
    else
	qppath_->quadTo( QPointF(ss.cp1_.x,ss.cp1_.y),
	       		 QPointF(ss.end_.x,ss.end_.y) );
    qpathitem_->setPath( *qppath_ );
}


void uiCurvedItem::closeCurve()
{
    qppath_->closeSubpath();
    qpathitem_->setPath( *qppath_ );
}


QGraphicsItem* uiCurvedItem::mkQtObj()
{
    qppath_ = new QPainterPath();
    qpathitem_ = new QGraphicsPathItem();
    return qpathitem_;
}
