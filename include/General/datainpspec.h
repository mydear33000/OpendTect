#ifndef datainpspec_h
#define datainpspec_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          08/02/2001
 RCS:           $Id: datainpspec.h,v 1.32 2002-01-10 13:43:00 arend Exp $
________________________________________________________________________

-*/

#include <ranges.h>
#include <bufstring.h>
#include <string2.h>
#include <sizepolspec.h>

class BinID2Coord;

/*! \brief handles undefined-ness of variables

This is my first attempt to catch the concept of undefinedness.
I would prefer an implementation based on template specialisation,
such as in ~arend/work/include/NLA/mlimits.h, but I'm pretty
sure M$ vc++ will choke on that.

TODO: check if we can make a real template specialisation based
implementation for both gcc and msvc:(, without the need to instantiate
the Undef class for each class it's used in.

*/
template<class T>
class Undef
{
public:
			Undef() : value_( value__( (T)0 ) ) {}

    inline T		value() const		{ return value_;     }
    inline bool		check( T t ) const	{ return isUndef_( t ); }

private:

    const T		value_;

    inline bool		isUndef_( int i )    const { return i==mUndefIntVal; }
    inline bool		isUndef_( float f )  const { return mIsUndefined(f); }
    inline bool		isUndef_( double d ) const { return mIsUndefined(d); }

    inline int		value__( int i ) 	   { return mUndefIntVal; }
    inline float	value__( float f )	   { return mUndefValue; }
    inline double	value__( double d )	   { return mUndefValue; }
};

/*! \brief Specification of input characteristics

A DataInpSpec is a conceptual specification of intrinsic properties of data.
With it, user interface parts can be constructed (uiGenInput).

*/
class DataInpSpec : public SzPolicySpec
{
public:

    enum		Type { none, intTp, floatTp, doubleTp, boolTp, 
			       intIntervalTp, floatIntervalTp, doubleIntervalTp,
			       binIDCoordTp, stringTp, fileNmTp, stringListTp };

			DataInpSpec( Type t )	: tp_(t), pfw_(-1) {}

    virtual		~DataInpSpec() {}

    Type		type() const			{ return tp_; }

    virtual DataInpSpec* clone() const			=0;
    virtual int 	nElems() const			{ return 1; }

    virtual bool	isUndef( int idx=0 ) const	=0;

    DataInpSpec&	setHSzP( SzPolicy p )		{ hor_=p; return *this;}
    DataInpSpec&	setVSzP( SzPolicy p )		{ ver_=p; return *this;}

			//! Preferred field width in characters
    DataInpSpec&	forcePrefFldWidth( int w )	{ pfw_=w; return *this;}
    int			prefFldWidth() const		{ return pfw_; }

    virtual bool	hasLimits() const		{ return false; }

    virtual const char*	text( int idx=0 ) const		=0;
    virtual void	setText( const char*, int idx=0)=0;

    virtual int		getIntValue( int idx ) const
			    { return text(idx) ? atoi( text(idx) ) : 0; }
    virtual double	getValue( int idx ) const
			    { return text(idx) ? atof( text(idx) ) : 0; }
    virtual float	getfValue( int idx ) const
			    { return text(idx) ? atof( text(idx) ) : 0; }
    virtual bool	getBoolValue( int idx ) const
			    { return (bool)getIntValue(idx); }

    virtual void	setValue( int i, int idx )
			    { setText( toString( i ),idx); }
    virtual void	setValue( double d, int idx )
			    { setText( toString( d ),idx); }
    virtual void	setValue( float f, int idx )
			    { setText( toString( f ),idx); }
    virtual void	setValue( bool b, int idx )
			    { setValue( ((int)b), idx ); }


protected:

    void		setType( Type t ) { tp_ = t; }
    Type		tp_;
    int			pfw_;

};


/*! \brief Specifications for inputs that optionally lie a specified range
*/
template <class T>
class NumInpWithLimitsSpec : public DataInpSpec
{
public:
			NumInpWithLimitsSpec(DataInpSpec::Type t ) 
			    : DataInpSpec( t ) , limits_(0) {}
			NumInpWithLimitsSpec( const NumInpWithLimitsSpec<T>& o )
			    : DataInpSpec( o ) 
			    , limits_( o.limits_?new Interval<T>(*o.limits_):0 )
			    {}

			~NumInpWithLimitsSpec()	{ delete limits_; }

    virtual bool	hasLimits() const	{ return limits_ ? true:false; }
    const Interval<T>*	limits()		{ return limits_; }
    void		setLimits( const Interval<T>& r)
			{
			    if ( limits_ ) delete limits_;
			    limits_ = new Interval<T>( r );
			}
protected:

    const Undef<T>	undef;
    Interval<T>*	limits_;

};


/*! \brief Specifications for numerical inputs
*/
template <class T>
class NumInpSpec : public NumInpWithLimitsSpec<T>
{
public:
			NumInpSpec(DataInpSpec::Type t ) 
			    : NumInpWithLimitsSpec<T>( t )	
			    { value_=undef.value(); }
			NumInpSpec(DataInpSpec::Type t, T val ) 
			    : NumInpWithLimitsSpec<T>( t )
			    , value_( val )			{}
			NumInpSpec( const NumInpSpec<T>& o )
			    : NumInpWithLimitsSpec<T>( o )
			    , value_( o.value_ )		{}

    virtual bool	isUndef( int idx=0 ) const	
			    { return undef.check(value_); }

    virtual void	setText( const char* s, int idx )
			    { getFromString( value_, s ); }

    T			value() const	
			{
			    if ( undef.check(value_) ) return undef.value();
			    return value_;
			}

    virtual const char*	text( int idx ) const
			{
			    if ( isUndef() )	return "";
			    else		return toString( value() );
			}

protected:

    T			value_;


}; 


#define mDefNumInpClass( clssNm, type ) \
class  clssNm : public NumInpSpec<type>\
{\
public:\
				clssNm() \
				    : NumInpSpec<type>( type##Tp ) {} \
				clssNm( type var ) \
				    : NumInpSpec<type>( type##Tp, var )	{} \
				clssNm( const clssNm& o ) \
				    : NumInpSpec<type>(o)		{} \
\
    virtual DataInpSpec*	clone() const \
				{ return new clssNm( *this ); } \
};

/*! \brief Specifications for float inputs.  */
mDefNumInpClass( FloatInpSpec, float )

/*! \brief Specifications for double inputs.  */
mDefNumInpClass( DoubleInpSpec, double )

/*! \brief Specifications for integer inputs.  */
mDefNumInpClass( IntInpSpec, int )

#undef mDefNumInpClass

/*! \brief Specifications for numerical intervals
*/
template <class T>
class NumInpIntervalSpec : public NumInpWithLimitsSpec<T>
{
public:
			NumInpIntervalSpec(DataInpSpec::Type t, bool withstep )
			    : NumInpWithLimitsSpec<T>( t )
			    , interval_( withstep ?  new StepInterval<T>( 
							undef.value(), 
							undef.value(), 
							undef.value() ) 
						  : new Interval<T>(
							undef.value(), 
							undef.value() ) )
			    {}

			NumInpIntervalSpec(DataInpSpec::Type t,
			    const Interval<T>& interval ) 
			    : NumInpWithLimitsSpec<T>( t )
			    , interval_( interval.clone() )	{}

			NumInpIntervalSpec( const NumInpIntervalSpec<T>& o )
			    : NumInpWithLimitsSpec<T>( o )
			    , interval_( o.interval_ ? o.interval_->clone() : 0)
			    {}

			~NumInpIntervalSpec()	{ delete interval_; }

    virtual int 	nElems()  const	{ return hasStep() ? 3 : 2; }
    inline bool		hasStep() const	{ return stpi()    ? true : false; }

    virtual bool	isUndef( int idx=0 ) const
			{	
			    if ( !interval_ ) return true;
			    return undef.check( value_(idx) ); 
			}

    virtual void	setValue( const Interval<T>& intval )
			{
			    if ( interval_ ) delete interval_;
			    interval_ = intval.clone();
			}

    virtual void	setText( const char* s, int idx )
			{ 
			    if ( pt_value_(idx) ) 
				getFromString( *pt_value_(idx), s ); 
			}

    T			value( int idx=0 ) const
			{
			    T retval = value_(idx);
			    if ( undef.check(retval) ) return undef.value();
			    return retval;
			}

    virtual const char*	text( int idx ) const
			{
			    if ( isUndef(idx) )	return "";
			    else		return toString( value(idx) );
			}

protected:

    T			value_( int idx=0 ) const
			{
			    if ( pt_value_(idx) ) return *pt_value_(idx);
			    return undef.value();
			}

    T*			pt_value_( int idx=0 ) const
			{
			    if ( !interval_ )	return 0;
			    if ( idx == 0 )	return &interval_->start;
			    if ( idx == 1 )	return &interval_->stop;
			    if ( hasStep() )	return &stpi()->step; 
			    return 0;
			}

    StepInterval<T>*	stpi() const
			{
			    mDynamicCastGet(const StepInterval<T>*,si,interval_)
			    return const_cast<StepInterval<T>*>( si );
			}

    Interval<T>*	interval_;

private:

    BufferString	value__;

}; 


#define mDefIntervalClass( clssNm, type ) \
class  clssNm : public NumInpIntervalSpec<type> \
{ \
public: \
		    clssNm( bool withstep=false) \
		    : NumInpIntervalSpec<type>( type##IntervalTp, withstep )\
		    {} \
		    clssNm( const Interval<type>& var ) \
		    : NumInpIntervalSpec<type>( type##IntervalTp,var )\
		    {} \
		    clssNm( const clssNm& o ) \
		    : NumInpIntervalSpec<type>(o) \
		    {} \
\
    virtual DataInpSpec*	clone() const \
				{ return new clssNm( *this ); } \
};

/*! \brief Specifications for float inputs.  */
mDefIntervalClass( FloatInpIntervalSpec, float )

/*! \brief Specifications for double inputs.  */
mDefIntervalClass( DoubleInpIntervalSpec, double )

/*! \brief Specifications for integer inputs.  */
mDefIntervalClass( IntInpIntervalSpec, int )

#undef mDefIntervalClass

/*! \brief Specifications for character string inputs. */

class StringInpSpec : public DataInpSpec
{
public:
			StringInpSpec( const char* s=0 )
			    : DataInpSpec( stringTp )
			    , isUndef_(s?false:true), str( s ) {}

    virtual bool	isUndef( int idx=0 ) const	{ return isUndef_; }

    virtual DataInpSpec* clone() const	{ return new StringInpSpec( *this ); }
    const char*		text() const			{ return str; }

    virtual void	setText( const char* s, int idx ) 
			    { str = s; isUndef_ = s ? false : true; }
    virtual const char*	text( int idx ) const
			{
			    if ( isUndef() )	return "";
			    else		return (const char*) str;
			}
protected:

    bool		isUndef_;
    BufferString	str;

};

/*! \brief Specifications for file-name inputs.
*/
class FileNameInpSpec : public StringInpSpec
{
public:
			FileNameInpSpec( const char* fname=0 )
			    : StringInpSpec( fname )
			    { setType( fileNmTp ); }

    virtual DataInpSpec* clone() const  
			    { return new FileNameInpSpec( *this ); }
};


/*! \brief Specifications for boolean inputs.

This specifies a boolean input field. If the second char string is "" or NULL, 
then a checkbox will be created. Otherwise two connected radio buttons
will do the job. Default will create two radio buttons "Yes" and "No".
When calling setText("xx") on the resulting uiGenInput, it will try to set the
boolean value according to the given true/false text's or "Yes"/"No".
It does not change the underlying true/false texts.

*/

class BoolInpSpec : public DataInpSpec
{
public:
			BoolInpSpec( const char* truetxt="Yes"
				, const char* falsetxt="No" , bool yesno=true )
			    : DataInpSpec( boolTp )
			    , truetext( truetxt ), falsetext( falsetxt )
			    , yn( yesno ) 
			    { setHSzP( SzPolicySpec::smallvar ); }

    virtual bool	isUndef( int idx=0 ) const	{ return false; }

    virtual DataInpSpec* clone() const  
			    { return new BoolInpSpec( *this ); }
    const char*		trueFalseTxt( bool tf = true ) const
			    { return tf ? truetext : falsetext; }
    void 		setTrueFalseTxt( bool tf, const char* txt )
			    { if ( tf ) truetext=txt; else falsetext=txt; }

    bool		checked() const			{ return yn; }
    void		setChecked( bool yesno )	{ yn=yesno; }
    virtual const char*	text( int idx=0 ) const
			{ 
			    return yn ? (const char*)truetext 
				      : (const char*)falsetext; 
			}

    virtual void	setText( const char* s, int idx=0 )
			    { yn = s && strcmp(s,falsetext); }
    virtual bool	getBoolValue( int idx=0 ) const
			    { return yn; }
    virtual void	setValue( bool b, int idx=0 )
			    { yn = b; }

protected:

    BufferString	truetext;
    BufferString	falsetext;
    bool		yn;

};




/*! \brief Specifications for list of character string inputs.
*/
class StringListInpSpec : public DataInpSpec
{
public:
				StringListInpSpec( const char** sl=0 )
				    : DataInpSpec( stringListTp )
				    , cur_(0)
				{
				    if ( !sl ) return;
				    while( *sl )
					strings_ += new BufferString( *sl++ );
				}

				StringListInpSpec( const StringListInpSpec& oth)
				    : DataInpSpec( oth )
				    , cur_(oth.cur_)
				    { deepCopy( strings_, oth.strings_ ); }

				~StringListInpSpec() { deepErase(strings_); }

    virtual bool		isUndef( int idx=0 ) const	
				    { return strings_.size() && cur_ >= 0
					   ? false : true; }
    virtual DataInpSpec*	clone() const	
				    { return new StringListInpSpec( *this ); }
    const ObjectSet<BufferString>& strings() const	{ return strings_; }
    void			addString( const char* txt )
				    { strings_ += new BufferString( txt ); }

    virtual const char*		text( int idx ) const
				    {
					if ( isUndef() ) return "";
					else return (const char*)*strings_[cur_];
				    }
    void		setItemText( int idx, const char* s ) 
			{ *strings_[cur_] = s; }

    virtual void	setText( const char* s, int nr ) 
			{
			    for ( int idx=0; idx<strings_.size(); idx++ )
				if ( *strings_[idx] == s ) { cur_ = idx;return;}
			}

    virtual int		getIntValue( int idx ) const
			    { return cur_; }
    virtual double	getValue( int idx ) const
			    { return cur_; }
    virtual float	getfValue( int idx ) const
			    { return cur_; }

    virtual void	setValue( int i, int idx )
			    { if ( i < strings_.size() ) cur_ = i; }
    virtual void	setValue( double d, int idx )
			    { if ( (int)(d+.5) < strings_.size() )
				cur_ = (int)(d+.5); }
    virtual void	setValue( float f, int idx )
			    { if ( (int)(f+.5) < strings_.size() )
				cur_ = (int)(f+.5); }

protected:

    ObjectSet<BufferString>	strings_;
    int				cur_;

};


/*! \brief Specifications for BinID/Coordinate inputs.
*/
class BinIDCoordInpSpec : public DataInpSpec
{
public:
			BinIDCoordInpSpec( bool doCoord=false
					 , bool isRelative=false
					 , double inline_x = mUndefValue
					 , double crossline_y = mUndefValue
					 , bool withOtherBut=false
					 , const BinID2Coord* b2c=0 )
			    : DataInpSpec( binIDCoordTp )
			    , withOtherBut_( withOtherBut )
			    , inl_x( inline_x )
			    , crl_y( crossline_y )
			    , doCoord_( doCoord )
			    , isRelative_( isRelative )
			    , b2c_( b2c ) {}

    virtual DataInpSpec* clone() const  
			{ return new BinIDCoordInpSpec( *this ); }

    virtual int 	nElems()  const	{ return 2; }

    double		value( int idx ) const { return idx ? crl_y : inl_x; }
    virtual bool	isUndef( int idx=0 ) const		
			{ 
			    if ( idx<0 || idx>1 ) return true;
			    return idx ? mIsUndefined( crl_y )
				       : mIsUndefined( inl_x );
			}
    virtual const char*	text( int idx ) const
			{
			    if ( isUndef() )	return "";
			    else		return toString( value(idx) );
			}
    virtual void	setText( const char* s, int idx ) 
			    { getFromString( (idx ? crl_y : inl_x), s); }

    const char*		otherTxt() const
			{
			    if ( !withOtherBut_ ) return 0;
			    if ( doCoord_ ) { return "Inline/Xline ..."; }
			    return isRelative_? "Distance ..." : "Coords ...";
			}
    const BinID2Coord*	binID2Coord() const
			{ return b2c_; }

protected:

    double		inl_x;
    double		crl_y;

    bool		doCoord_;
    bool		isRelative_;
    bool		withOtherBut_;
    const BinID2Coord*	b2c_;
};

#endif
