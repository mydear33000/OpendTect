#ifndef welldahobj_h
#define welldahobj_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		May 2004
 RCS:		$Id: welldahobj.h,v 1.9 2009-08-11 21:31:29 cvskris Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "namedobj.h"

namespace Well
{

mClass DahObj : public ::NamedObject
{
public:

			DahObj( const char* nm=0 )
			: ::NamedObject(nm)		{}
			DahObj( const DahObj& d )
			    : ::NamedObject(d.name())
			    , dah_(d.dah_)		{}

    inline int		size() const			{ return dah_.size(); }
    inline float	dah(int idx) const		{ return dah_[idx]; }
    virtual float	value(int idx) const		= 0;
    virtual void	remove( int idx )
			{ dah_.remove(idx); removeAux(idx); }
    virtual void	erase()
			{ dah_.erase(); eraseAux(); }
    inline bool		isEmpty() const			{ return size() == 0; }

    float		dahStep(bool min_else_average) const;

    void		addToDahFrom(int fromidx,float extradah);
    void		removeFromDahFrom(int fromidx,float extradah);

    void		deInterpolate();
    			//!< Remove unnecessary points
protected:

    TypeSet<float>	dah_;

    virtual void	removeAux(int)			= 0;
    virtual void	eraseAux()			= 0;
};


}; // namespace Well

#endif
