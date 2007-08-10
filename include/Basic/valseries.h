#ifndef valseries_h
#define valseries_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril & Kris Tingdahl
 Date:          Mar 2005
 RCS:           $Id: valseries.h,v 1.8 2007-08-10 12:06:35 cvsnanne Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
#include "errh.h"

/*\brief Interface to a series of values

  If the values are in contiguous memory, arr() should return non-null.
 
 */

template <class T>
class ValueSeries
{
public:

    virtual		~ValueSeries()		{}

    virtual bool	isOK() const			{ return true; }

    virtual T		value(int64) const		= 0;
    virtual bool	writable() const		{ return false; }
    virtual void	setValue(int64,T)		{}

    virtual bool	reSizeable() const		{ return false; }
    virtual bool	setSize(int64) 			{ return false; }

    virtual T*		arr()				{ return 0; }
    virtual const T*	arr() const			{ return 0; }

    inline T		operator[]( int64 idx ) const	{ return value(idx); }

};


template <class T>
class OffsetValueSeries : public ValueSeries<T>
{
public:
    inline		OffsetValueSeries( ValueSeries<T>& src, int64 off );
    inline		OffsetValueSeries( const ValueSeries<T>& src,int64 off);

    inline T		value( int64 idx ) const;
    inline void		setValue( int64 idx, T v ) const;
    inline T*		arr();
    inline const T*	arr() const;
    inline bool		writable() const;

    inline int64	getOffset() const;
    inline void		setOffset(int64 no);

protected:
    ValueSeries<T>&	src_;
    int64		off_;
    bool		writable_;
};


/*\brief series of values from a pointer to some kind of array */

template <class T>
class ArrayValueSeries : public ValueSeries<T>
{
public:

    		ArrayValueSeries( T* ptr, bool memmine );
    		ArrayValueSeries( int64 sz );
    		~ArrayValueSeries()		{ if ( mine_ ) delete [] ptr_; }

    bool	isOK()				{ return ptr_; }

    T		value( int64 idx ) const	{ return ptr_[idx]; }
    bool	writable() const		{ return true; }
    void	setValue( int64 idx, T v )	{ ptr_[idx] = v; }

    const T*	arr() const			{ return ptr_; }
    T*		arr()				{ return ptr_; }

    bool	reSizeable() const		{ return mine_; }
    inline bool	setSize(int64);
    int64	size() const			{ return cursize_; }

protected:

    T*		ptr_;
    bool	mine_;
    int64	cursize_;
};



template <class T> inline
OffsetValueSeries<T>::OffsetValueSeries( ValueSeries<T>& src, int64 off )
    : src_( src ), off_( off ), writable_(true) 
{}


template <class T> inline
OffsetValueSeries<T>::OffsetValueSeries( const ValueSeries<T>& src, int64 off )
    : src_( const_cast<ValueSeries<T>& >(src) ), off_( off ), writable_(false) 
{}


template <class T> inline
T OffsetValueSeries<T>::value( int64 idx ) const
{ return src_.value(idx+off_); }

template <class T> inline
void OffsetValueSeries<T>::setValue( int64 idx, T v ) const
{
    if ( writable_ )
	src_.setValue(idx+off_,v);
    else
	pErrMsg("Attempting to write to write-protected array");
}


template <class T> inline
T* OffsetValueSeries<T>::arr()
{ T* p = src_.arr(); return p ? p+off_ : 0; }


template <class T> inline
const T* OffsetValueSeries<T>::arr() const
{ T* p = src_.arr(); return p ? p+off_ : 0; }


template <class T> inline
int64 OffsetValueSeries<T>::getOffset() const
{ return off_; }


template <class T> inline
void OffsetValueSeries<T>::setOffset(int64 no)
{ off_ = no; }


template <class T> inline
bool OffsetValueSeries<T>::writable() const
{ return writable_; }


template <class T>
ArrayValueSeries<T>::ArrayValueSeries( T* ptr, bool memmine )
    : ptr_(ptr), mine_(memmine), cursize_( -1 )
{}


template <class T>
ArrayValueSeries<T>::ArrayValueSeries( int64 sz )
    : mine_(true), cursize_( sz )
{
    mTryAlloc( ptr_, T[sz] )
}


template <class T>
bool ArrayValueSeries<T>::setSize( int64 sz )
{
    if ( !mine_ ) return false;
    if ( cursize_!=-1 && cursize_==sz && ptr_ ) return true;

    delete [] ptr_;
    mTryAlloc( ptr_, T[sz] )
    cursize_ = sz;
    return ptr_;
}


#endif
