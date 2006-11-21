#ifndef tabledef_h
#define tabledef_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Oct 2006
 RCS:		$Id: tabledef.h,v 1.8 2006-11-21 14:00:06 cvsbert Exp $
________________________________________________________________________

-*/

#include "namedobj.h"
#include "sets.h"
#include "rowcol.h"
#include "bufstringset.h"
#include <iostream>

namespace Table
{

    enum ReqSpec	{ Optional=0, Required=1 };

/*!\brief Logical piece of information, present in tables.

 In most simple situations, you need to know the column of some data, or the
 row/col of a header. Then you just describe it as:
 FormatInfo fi( Table::Optional, "Sample rate" );

 In some cases, data can be present or offered in various ways. For example, a
 position in the survey can be given as inline/crossline or X and Y. This would
 be specified as follows:

 FormatInfo fi( "Position", Table::Required );
 fi.add( "Inl/Crl", new BufferStringSet( {"Inline","Xline"}, 2 ) );
 fi.add( "X/Y", new BufferStringSet( {"X-coord","Y-coord"}, 2 ) );

*/

class FormatInfo : public NamedObject
{
public:

    			FormatInfo( ReqSpec rs, const char* elemnm )
			    : NamedObject((const char*)0)
			    , req_(rs)		{ init(elemnm); }
    			FormatInfo( const char* nm, ReqSpec rs )
			    : NamedObject(nm)
			    , req_(rs)		{ init(0); }
			~FormatInfo()		{ deepErase( elemdefs_ ); }

    void		add( const char* elmnm, BufferStringSet* elmdef=0 )
    						{ elemnms_.add( elmnm );
						  elemdefs_ += elmdef; }

    bool		isOptional() const	{ return req_ == Optional; }
    int			nrElements() const	{ return elemnms_.size(); }
    const char*		elementName( int ielem ) const
						{ return elemnms_.get(ielem); }
    const BufferStringSet* elementDef( int ielem ) const
						{ return elemdefs_[ielem]; }
				//!< may return null
    int			nrSubElements( int ielem ) const
			{ return !elemdefs_[ielem] ? 1
			    			   : elemdefs_[ielem]->size(); }
    const char*		subElementName( int ielem, int isub ) const
			{ return isub < nrSubElements(ielem)
			       ? elemdefs_[ielem]->get(isub).buf() : ""; }

    /*!\brief Selected element/positioning */
    struct Selection
    {
			Selection() : elem_(0)	{}

	int		elem_;
	TypeSet<RowCol>	pos_;
	BufferStringSet	vals_;	//!< when !havePos(isub)

	bool		havePos( int isub ) const
	    		{ return isub < pos_.size() && pos_[isub].c() >= 0; }
	const char*	getVal( int isub ) const
	    		{ return isub >= vals_.size() ? "" :
				 vals_.get(isub).buf(); }

    };

    mutable Selection	selection_;

protected:

    ReqSpec		req_;
    BufferStringSet	elemnms_;
    ObjectSet<BufferStringSet> elemdefs_;

    void		init( const char* elemnm )
    			{
			    elemdefs_.allowNull();
			    if ( elemnm && *elemnm )
				add( elemnm, 0 );
			}

};


/*!\brief description of input our output table format */

class FormatDesc : public NamedObject
{
public:
    			FormatDesc( const char* nm )
			    : NamedObject(nm)
			    , nrhdrlines_(0)
			    , tokencol_(-1)		{}
			~FormatDesc()
			{
			    deepErase( headerinfos_ );
			    deepErase( bodyinfos_ );
			}

    ObjectSet<FormatInfo> headerinfos_;
    ObjectSet<FormatInfo> bodyinfos_;

    int			nrhdrlines_;	//!< if < 0 token will be used
    BufferString	token_;
    int			tokencol_;	//!< if < 0 token can be in any col

    bool		needToken() const
    			{ return nrhdrlines_ < 0 && !token_.isEmpty(); }
    int			nrHdrLines() const
			{ return needToken() ? mUdf(int)
			       : nrhdrlines_ > 0 ? nrhdrlines_ : 0; }
};

}; // namespace Table


#endif
