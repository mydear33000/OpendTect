/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 1995
 * FUNCTION : Stream Provider functions
-*/

#include "Pmacros.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>

#ifdef __msvc__
# include <windows.h>
# define popen _popen
# define pclose _pclose
# define fileno(s) _fileno(s)
#endif

#include "strmprov.h"
#include "filegen.h"
#include "string2.h"
#include "binidsel.h"
#include "strmoper.h"


static const char* rcsID = "$Id: strmprov.cc,v 1.7 2001-06-02 14:29:06 windev Exp $";

static FixedString<1024> oscommand;
#define exeCmd(comm) system((const char*)comm) ? false : true

DefineClassID(StreamProvider);
static const char* sStdIO = "$std$IO$";


void StreamData::close()
{
    if ( fp && fp != stdin && fp != stdout && fp != stderr )
	{ if ( ispipe ) pclose(fp); else fclose(fp); }
    if ( istrm && istrm != &cin )
	delete istrm;
    if ( ostrm && ostrm != &cout && ostrm != &cerr )
	delete ostrm;

    init();
}


void StreamData::init()
{
    fp = 0; istrm = 0; ostrm = 0; ispipe = false;
}


bool StreamData::usable() const
{
    return ( istrm || ostrm )
	&& ( !istrm || !istrm->bad() )
	&& ( !ostrm || (!ostrm->bad() && !ostrm->eof()) )
	&& ( !ispipe || fp );
}


StreamProvider::StreamProvider( const char* devname )
{
    set( devname );
}


StreamProvider::StreamProvider( const char* hostnm, const char* fnm,
				StreamConn::Type typ )
	: isbad(false)
	, type_(typ)
	, hostname(hostnm)
	, fname(fnm?fnm:sStdIO)
{
    if ( fname == "" ) isbad = true;
}


void StreamProvider::set( const char* devname )
{
    type_ = StreamConn::File;
    isbad = false;
    blocksize = 0;
    hostname = "";

    if ( !devname
      || !strcmp(devname,"stdin")
      || !strcmp(devname,"stdio")
      || !strcmp(devname,"stdout") )
    {
	type_ = StreamConn::File;
	fname = sStdIO;
	return;
    }
    else if ( !*devname )
    {
	isbad = true; fname = "";
	return;
    }

    char* ptr = (char*)devname;
    if ( *ptr == '@' ) { type_ = StreamConn::Command; ptr++; }
    fname = ptr;

    // separate hostname from filename
    ptr = strchr( fname.buf(), ':' );
    if ( ptr )
    {
	*ptr++ = '\0';
	hostname = fname;
    }
    else
	ptr = fname.buf();

    if ( *ptr == '@' ) { type_ = StreamConn::Command; ptr++; }

    char* ptrname = fname.buf();
    while ( *ptr ) *ptrname++ = *ptr++;
    *ptrname = '\0';

    if ( type_ != StreamConn::Command && matchString( "/dev/", fname ) )
	type_ = StreamConn::Device;
}


bool StreamProvider::isNormalFile() const
{
    return type_ == StreamConn::File && hostname == "";
}


bool StreamProvider::rewind() const
{
    if ( isbad ) return false;
    else if ( type_ != StreamConn::Device ) return true;

    if ( hostname[0] )
	sprintf( oscommand.buf(), "rsh %s \"mt -f %s rewind\"",
		 (const char*)hostname, (const char*)fname );
    else
	sprintf( oscommand.buf(), "mt -f %s rewind", (const char*)fname );
    return exeCmd(oscommand);
}


bool StreamProvider::offline() const
{
    if ( isbad ) return false;
    else if ( type_ != StreamConn::Device ) return true;

    if ( hostname[0] )
	sprintf( oscommand.buf(), "rsh %s \"mt -f %s offline\"",
			    (const char*)hostname, (const char*)fname );
    else
	sprintf( oscommand.buf(), "mt -f %s offline", (const char*)fname );
    return exeCmd(oscommand);
}


bool StreamProvider::skipFiles( int nr ) const
{
    if ( isbad ) return false;
    if ( type_ != StreamConn::Device ) return false;

    if ( hostname[0] )
	sprintf( oscommand.buf(), "rsh %s \"mt -f %s fsf %d\"",
			(const char*)hostname, (const char*)fname, nr );
    else
	sprintf( oscommand.buf(), "mt -f %s fsf %d", (const char*)fname, nr );
    return exeCmd(oscommand);
}


const char* StreamProvider::fullName() const
{
    oscommand = "";
    if ( type_ == StreamConn::Command )
	oscommand += "@";
    if ( hostname[0] ) 
    {
	oscommand += hostname;
	oscommand += ":";
    }
    oscommand += fname == sStdIO ? "stdio" : (const char*)fname;

    return oscommand;
}


void StreamProvider::addPathIfNecessary( const char* path )
{
    if ( isbad ) return;
    if ( type_ != StreamConn::File || !path || ! *path
      || File_isAbsPath(fname) )
	return;
    else if ( fname == sStdIO ) return;

    FileNameString pth( path );
    fname = File_getFullPath( pth, fname );
}


StreamData StreamProvider::makeIStream() const
{
    StreamData sd;
    if ( isbad ) return sd;
    if ( !*(const char*)fname ) return sd;

    if ( type_ == StreamConn::Command || hostname[0] )
    {
	if ( hostname[0] )
	{
	    switch ( type_ )
	    {
	    case StreamConn::Device:
		if ( blocksize )
		    sprintf( oscommand.buf(), "rsh %s dd if=%s ibs=%ld",
				(const char*)hostname, (const char*)fname,
				blocksize );
		else
		    sprintf( oscommand.buf(), "rsh %s dd if=%s",
				(const char*)hostname, (const char*)fname );
	    break;
	    case StreamConn::Command:
		sprintf( oscommand.buf(), "rsh %s %s", (const char*)hostname,
					(const char*)fname );
	    break;
	    case StreamConn::File:
		sprintf( oscommand.buf(), "rsh %s cat %s",
				(const char*)hostname, (const char*)fname );
	    break;
	    }
	}
	else
	    strcpy( oscommand.buf(), (const char*)fname );

	sd.fp = popen( oscommand, "r" );
	sd.ispipe = true;
#ifndef __msvc__
	if ( sd.fp )
	    sd.istrm = new ifstream( fileno(sd.fp) );
#endif
    }
    else
    {
	if ( fname == sStdIO )
	    sd.istrm = &cin;
	else if ( File_exists(fname) )
	    sd.istrm = new ifstream( fname );
    }

    return sd;
}


StreamData StreamProvider::makeOStream() const
{
    StreamData sd;
    if ( isbad ||  !*(const char*)fname ) return sd;

    if ( type_ == StreamConn::Command || hostname[0] )
    {
	if ( hostname[0] )
	{
	    switch ( type_ )
	    {
	    case StreamConn::Device:
		if ( blocksize )
		    sprintf( oscommand.buf(), "rsh %s dd of=%s obs=%ld",
				      (const char*)hostname, (const char*)fname,
					blocksize );
		else
		    sprintf( oscommand.buf(), "rsh %s dd of=%s",
				    (const char*)hostname, (const char*)fname );
	    break;
	    case StreamConn::Command:
		sprintf( oscommand.buf(), "rsh %s %s", (const char*)hostname,
					(const char*)fname );
	    break;
	    case StreamConn::File:
		sprintf( oscommand.buf(), "rsh %s tee %s > /dev/null",
				  (const char*)hostname, (const char*)fname );
	    break;
	    }
	}
	else
	    strcpy( oscommand.buf(), (const char*)fname );

	sd.fp = popen( oscommand, "w" );
	sd.ispipe = true;
#ifndef __msvc__
	if ( sd.fp )
	    sd.ostrm = new ofstream( fileno(sd.fp) );
#endif
    }
    else
    {
	if ( fname == sStdIO )
	    sd.ostrm = &cout;
	else
	    sd.ostrm = new ofstream( fname );
    }

    return sd;
}


bool StreamProvider::exists( int fr ) const
{
    if ( isbad ) return false;

    if ( type_ == StreamConn::Command )
	return fr;

    if ( hostname[0] )
    {
	sprintf( oscommand.buf(), "rsh %s 'test -%c %s && echo 1'",
		  (const char*)hostname, fr ? 'r' : 'w', (const char*)fname );
	FILE* fp = popen( oscommand, "r" );
	int i = 0;
	fscanf( fp, "%d", &i );
	pclose( fp );
	return i;
    }
    else
	return fname == sStdIO ? true : File_exists( (const char*)fname );
}


bool StreamProvider::remove() const
{
    if ( isbad || type_ != StreamConn::File ) return false;

    if ( !hostname[0] )
	return fname == sStdIO ? false :
			File_remove( (const char*)fname, YES, YES );
    else
    {
	sprintf( oscommand.buf(), "rsh %s '/bin/rm -f %s && echo 1'",
		  (const char*)hostname, (const char*)fname );
	FILE* fp = popen( oscommand, "r" );
	char c;
	fscanf( fp, "%c", &c );
	pclose( fp );
	return c == '1';
    }
}
