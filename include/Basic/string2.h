#ifndef string2_h
#define string2_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		11-4-1994
 Contents:	Extra string functions
 RCS:		$Id: string2.h,v 1.37 2010-12-29 15:49:20 cvskris Exp $
________________________________________________________________________
-*/



#include "gendefs.h"
#include "plftypes.h"
#include <string.h>
#include <ctype.h>

#ifdef __cpp__
extern "C" {
#endif


/*!> bluntly puts a '\0' on trailing white space. */
mGlobal void removeTrailingBlanks(char*);
/*!> advances given pointer to first non-whitespace. */
#define mSkipBlanks(ptr) \
    { if ( ptr ) { while ( *(ptr) && isspace(*(ptr)) ) (ptr)++; } }
/*!> advances given pointer to first whitespace. */
#define mSkipNonBlanks(ptr) \
    { if ( ptr ) { while ( *(ptr) && !isspace(*(ptr)) ) (ptr)++; } }
/*!> advances to first whitespace and removes trailing whitespace */
#define mTrimBlanks(ptr) \
    { mSkipBlanks(ptr); removeTrailingBlanks(ptr); }


/*!> stricmp with option to compare part */
mGlobal int caseInsensitiveEqual(const char*,const char*,
				     int nr_chars_to_match_0_is_all);
/*!> checks whether a string is the start of another string. */
mGlobal int matchString(const char* startstring,const char* maybebigger);
/*!> is a case insensitive version of matchString */
mGlobal int matchStringCI(const char*,const char*);
mGlobal int stringEndsWith(const char* endstring,const char* maybebigger);
mGlobal int stringEndsWithCI(const char*,const char*);

/*!> fills a buffer with the next word (delimited by whitespace) in string.
     It returns a ptr just after the word. */
mGlobal const char* getNextWord(const char*,char*);

/*!> counts occurrences of a char in string */
mGlobal int countCharacter(const char*,char);
/*!> replaces all occurrences of a char with another */
mGlobal void replaceCharacter(char*,char from,char to);
/*!> replaces all occurrences of a string with another */
mGlobal void replaceString(char*,const char* from,const char* to);
/*!> removes all occurrences of a char */
mGlobal void removeCharacter(char*,char);
/*!> cleans a string from non-alpha numeric by replacing with underscores.
     params: allow whitespace, allow slashes, allow dots */
mGlobal void cleanupString(char*,int,int,int);
/*!> tells whether a string holds a parseable number */
mGlobal int isNumberString(const char*,int int_only);

/*!> Fills string with string for an int */
mGlobal void getStringFromInt(od_int32,char*);
mGlobal void getStringFromUInt(od_uint32,char*);
mGlobal void getStringFromInt64(od_int64,char*);
mGlobal void getStringFromUInt64(od_uint64,char*);

/*!> Normally, pass null for fmt. Then it will do removal of
     trailing zeros and use %lf in more cases than std. */
mGlobal void getStringFromDouble(const char* fmt,double,char*);
/*!> is like getStringFromDouble, with special %f treatment. */
mGlobal void getStringFromFloat(const char* fmt,float,char*);
/*!> removes unwanted zeros and dots from a floating point in string. */
mGlobal void prettyNumber(char*,int is_float);

/*!> returns ptr to static buffer with "yes" or "No". */
mGlobal const char* getYesNoString(int);
/*!> returns 1 or 0 by inspecting string */
mGlobal int yesNoFromString(const char*);
/*!> returns "th" or "st" or "nd" or "rd"; like in 1st, 2nd, 3rd etc. */
mGlobal const char* getRankPostFix(int);
/*!> returns a nicely, readable size, in bytes, KB, MB, GB, or TB */
mGlobal const char* getBytesString(od_uint64);
/*!> returns a string for display, never larger than specified nr of chars */
mGlobal const char* getLimitedDisplayString(const char*,int nrchars,
					    int trimright);

mGlobal int getIndexInStringArrCI(const char*,const char* const* arr,
				  int startnr,int nr_chars_to_match,
				  int notfoundidx);


#ifdef __cpp__
}
#include <stdlib.h>
#include "undefval.h"

inline void toString(od_int32 i,char* r)	{ getStringFromInt( i, r ); }
inline void toString(od_uint32 i,char* r)	{ getStringFromUInt( i, r ); }
inline void toString(od_int64 i,char* r)	{ getStringFromInt64( i, r ); }
inline void toString(od_uint64 i,char* r)	{ getStringFromUInt64(i,r); }
inline void toString( float f,char* r )		{ getStringFromFloat(0,f,r); }
inline void toString( double d,char* r )	{ getStringFromDouble(0,d,r); }
inline void toString( short i,char* r)		{ getStringFromInt((int)i,r); }
inline void toString( unsigned short i,char* r )
				{ getStringFromUInt( (unsigned int)i, r ); }
inline const char* toString(bool b,char*r=0)	
{
    const char* res = getYesNoString(b);
    if ( r ) strcpy( r, res );
    return res;
}

#define mImplGetFromStrFunc( type, func, udfv ) \
inline bool getFromString( type& i, const char* s, type undef=udfv ) \
{ \
    if ( s && *s ) \
    { \
	char* e; \
	i = (type)func; \
	if ( e==s ) \
	{ \
	    i = undef; \
	    return false;\
	}\
	return true; \
    } \
 \
    i = undef; \
    return false; \
}



// inline bool getFromString( double& d, const char* s, double udefval );
mImplGetFromStrFunc(double, strtod(s,&e), mUdf(double) )
// inline bool getFromString( float& d, const char* s, float udefval );
mImplGetFromStrFunc(float, strtod(s,&e), mUdf(float) )
// inline bool getFromString( int& d, const char* s, int udefval );
mImplGetFromStrFunc(int, strtol(s,&e,10), mUdf(int) )
#undef mImplGetFromStrFunc

inline bool getFromString( bool& b, const char* s )
{
    if ( s )
    {
	b = ( yesNoFromString( s ) ? true : false );
	return true;
    }

    b = false;
    return false;
}

inline bool toBool( const char* s, bool defval=true )
{
    return s && *s ? yesNoFromString(s) : defval;
}

inline float toFloat( const char* s, float defval=0 )
{
    float ret = defval; getFromString( ret, s, ret ); return ret;
}

inline double toDouble( const char* s, double defval=0 )
{
    double ret = defval; getFromString( ret, s, ret ); return ret;
}

inline int toInt( const char* s, int defval=0 )
{
    int ret = defval; getFromString( ret, s, ret ); return ret;
}


#endif


#endif
