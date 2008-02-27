/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 18-8-2000
 * FUNCTION : Help viewing
-*/
 
static const char* rcsID = "$Id: helpview.cc,v 1.34 2008-02-27 10:22:06 cvsnanne Exp $";

#include "helpview.h"

#include "ascstream.h"
#include "envvars.h"
#include "errh.h"
#include "filegen.h"
#include "filepath.h"
#include "multiid.h"
#include "oddirs.h"
#include "strmprov.h"


static const char* sMainIndex = "MainIndex";
static const char* sODDir = "base";
static const char* sIndexHtml = "index.html";
static const char* sBookHtml = "book1.htm";
static const char* sNotFoundHtml = "notfound.html";
static const char* sNotInstHtml = "docnotinst.html";
static const char* sToDoHtml = "todo.html";


static StreamData getStreamData( const char* fnm )
{
    StreamData sd = StreamProvider( fnm ).makeIStream();
    if ( !sd.usable() )
    {
	BufferString msg( "Help file '" );
	msg += fnm; msg += "' not available";
	ErrMsg( msg ); sd.close();
    }
    return sd;
}


static const char* unScoped( const char* inp )
{
    if ( !inp || !*inp ) return inp;

    char* ptr = strchr( inp, ':' );
    return ptr ? ptr + 1 : inp;
}


static BufferString getScope( const char* inp )
{
    BufferString ret;
    BufferString winid( inp );
    char* ptr = strchr( winid.buf(), ':' );
    if ( ptr )
    {
	*ptr = '\0';
	ret = winid;
    }

    if ( ret.isEmpty() )
	ret = sODDir;
    return ret;
}


BufferString HelpViewer::getLinkNameForWinID( const char* inpwinid,
       					      const char* docdir )
{
    if ( !inpwinid || !*inpwinid ) return BufferString( sMainIndex );

    BufferString winid( unScoped(inpwinid) );

    const BufferString fnm =
	FilePath( docdir ).add( "WindowLinkTable.txt" ).fullPath();
    StreamData sd = getStreamData( fnm );
    if ( !sd.usable() )
	return BufferString("");
    std::istream& strm = *sd.istrm;

    ascistream astream( strm );
    MultiID code[3];
    MultiID wid;
    int lvl;
    const char* ptr = 0;
    while ( 1 )
    {
	char c = strm.peek();
	while ( c == '\n' ) { strm.ignore( 1 ); c = strm.peek(); }
	lvl = 0;
	if ( c == '\t' )
	{
	    lvl++;
	    strm.ignore( 1 );
	    c = strm.peek();
	    if ( c == '\t' ) lvl++;
	}
	astream.next();
	if ( atEndOfSection(astream) ) break;

	code[lvl] = astream.keyWord();
	for ( int idx=2; idx>lvl; idx-- )
	    code[idx] = "";
	wid = code[0];
	if ( !code[1].isEmpty() )
	{
	    wid += code[1];
	    if ( !code[2].isEmpty() )
		wid += code[2];
	}
	if ( wid != winid.buf() ) continue;

	ptr = astream.value();
	mSkipBlanks(ptr);

	// Skip object name
	mSkipNonBlanks( ptr );
	if ( ! *ptr ) { strm.ignore(10000,'\n'); ptr = 0; continue; }
	mSkipBlanks(ptr);

	const char* endptr = ptr;
	mSkipNonBlanks( endptr );
	*(char*)endptr = '\0';
	break;
    }

    if ( !ptr || ! *ptr )
    {
	BufferString msg = "No specific help for this window (ID=";
	msg += inpwinid; msg += ").";
	UsrMsg( msg );
	ptr = sMainIndex;
    }
    else if ( GetEnvVarYN("DTECT_SHOW_HELP") )
    {
	BufferString msg = inpwinid; msg += " -> "; msg += ptr;
	UsrMsg( msg );
    }

    sd.close();
    return BufferString( ptr );
}


BufferString HelpViewer::getURLForLinkName( const char* lnm, const char* docdir)
{
    BufferString linknm( lnm );
    if ( linknm.isEmpty() )
	linknm = sMainIndex;

    const bool ismainidx = linknm == sMainIndex;
    const bool istodo = linknm == "TODO";
    BufferString url;
    if ( ismainidx || istodo )
    {
	url = ismainidx ? FilePath( docdir ).add( sIndexHtml ).fullPath().buf()
			: GetDocFileDir( sToDoHtml );
	if ( ismainidx && !File_exists(url) )
	{
	    url = FilePath( docdir ).add( sBookHtml ).fullPath();
	    if ( !File_exists(url) )
		url = GetDocFileDir( sNotFoundHtml );
	}
	return url;
    }

    const BufferString ftnm =
	FilePath( docdir ).add( "LinkFileTable.txt" ).fullPath();
    StreamData sd = getStreamData( ftnm );
    BufferString htmlfnm;
    if ( sd.usable() )
    {
	std::string lnk, fnm;
	std::istream& strm = *sd.istrm;
	while ( strm.good() )
	{
	    strm >> lnk >> fnm;
	    if ( caseInsensitiveEqual(lnk.c_str(),linknm.buf(),0) )
	    {
		htmlfnm = fnm.c_str();
		linknm = lnk.c_str();
		break;
	    }
	}
	sd.close();
    }

    const char* fnm = htmlfnm.isEmpty() ? sIndexHtml : htmlfnm.buf();
    url = FilePath( docdir ).add( fnm ).fullPath();
    if ( !File_exists(url) )
	url = GetDocFileDir( sNotFoundHtml );
    else
    { url += "#"; url += linknm; }

    return url;
}


BufferString HelpViewer::getURLForWinID( const char* winid )
{
    const BufferString docdir =
	FilePath( mGetUserDocDir() ).add( getScope(winid) ).fullPath();
    if ( !File_exists(docdir) )
	return BufferString( GetDocFileDir(sNotInstHtml) );

    const BufferString lnm = getLinkNameForWinID( winid, docdir );
    return getURLForLinkName( lnm.buf(), docdir );
}
