#ifndef cubicbeziersurface_h
#define cubicbeziersurface_h
                                                                                
/*+
________________________________________________________________________
CopyRight:     (C) dGB Beheer B.V.
Author:        K. Tingdahl
Date:          December 2004
RCS:           $Id: cubicbeziersurface.h,v 1.3 2005-02-17 10:25:47 cvskris Exp $
________________________________________________________________________

-*/

#include "parametricsurface.h"

template <class T> class TypeSet;

namespace Geometry
{


class CubicBezierSurface : public ParametricSurface
{
public:
    			CubicBezierSurface( const Coord3&, const Coord3&,
				bool posonsamerow,
				const RCol& origo=RowCol(0,0),
				const RCol& step=RowCol(1,1));
    			CubicBezierSurface( const CubicBezierSurface& );
			~CubicBezierSurface();

    CubicBezierSurface*	clone() const;

    IntervalND<float>	boundingBox(bool approx) const;

    Coord3	computePosition(const Coord&) const;
    Coord3	computeNormal(const Coord&) const;

    Coord3	getBezierVertex( const RCol& knot, const RCol& relpos ) const;

    bool	insertRow(int row);
    bool	insertColumn(int col);
    bool	removeRow(int row);
    bool	removeColumn(int col);
    Coord3	getKnot( const RCol&, bool estimateifundef=false ) const;

    Coord3	getRowDirection(const RCol&, bool computeifudf ) const;
    Coord3	getColDirection(const RCol&, bool computeifudf ) const;
    float	directionInfluence() const;
    void	setDirectionInfluence(float);
protected:

    Coord3	computeRowDirection(const RCol&) const;
    Coord3	computeColDirection(const RCol&) const;

    void	_setKnot( int idx, const Coord3& );

    int		nrRows() const;
    int		nrCols() const;

    Array2D<Coord3>*	rowdirections;
    Array2D<Coord3>*	coldirections;
    Array2D<Coord3>*	positions;

    float		directioninfluence;
};

};

#endif
