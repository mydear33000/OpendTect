#ifndef ioman_H
#define ioman_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		3-8-1995
 RCS:		$Id: ioman.h,v 1.18 2004-02-13 17:03:00 bert Exp $
________________________________________________________________________

-*/
 

#include <uidobj.h>
#include <multiid.h>
#include <sets.h>
class IOLink;
class IOParList;
class IOPar;
class CtxtIOObj;
class Translator;


/*!> Class IOMan manages the Meta-data store for the IOObj's. This info
is read from the .omf files.

There will be one IOMan available through the gloabal function IOM(). Creating
more instances is probably not be a good idea, but may work.

A current IODir is maintained. Auxiliary info, not needed for read/write the
object, but useful info can be stored in .aux files.

Access to the parameter save files (e.g. '.Process_Seismic') is also provided
through getParList().

*/

class IOMan : public UserIDObject
{
    friend class	IOObj;
    friend class	IODir;
    friend IOMan&	IOM();

public:

    enum State		{ Bad, NeedInit, Good };
    bool		bad() const		{ return state_ != Good; }
    State		state() const		{ return state_; }

    bool		to(const IOLink*);	//!< NULL -> ".."
    bool		to(const MultiID&);
    void		back();

    //! The following functions return a cloned IOObj (=mem man by caller)
    IOObj*		get(const MultiID&) const;
    IOObj*		getLocal(const char* objname) const;
    IOObj*		getOfGroup(const char* tgname,bool first=true,
	    			   bool onlyifsingle=false) const;
    IOObj*		getIfOnlyOne( const char* trgroupname )
			{ return getOfGroup(trgroupname,true,true); }
    IOObj*		getByName(const char* objname,
			      const char* partrgname=0,const char* parname=0);

    IODir*		dirPtr() const		{ return (IODir*)dirptr; }
    MultiID		key() const;		//!< of current IODir
    const char*		curDir() const;		//!< OS dir name
    int			curLevel() const	{ return curlvl; }
    const char*		rootDir() const		{ return rootdir; }
    int			levelOf(const char* dirnm) const;
    const char*		nameOf(const char* keystr,bool inc_parents=false) const;

    void		getEntry(CtxtIOObj&,MultiID parentid="");
			//!< will create a new entry if necessary
    bool		haveEntries(const MultiID& dirid,const char* trgrpnm=0,
				     const char* trnm=0) const;
    IOParList*		getParList(const char* typ=0) const;
			//!< Reads the file on the root of the survey
    bool		commitChanges(const IOObj&);
    bool		permRemove(const MultiID&);

    IOPar*		getAux(const MultiID&) const;
    bool		putAux(const MultiID&,const IOPar*) const;
    IOParList*		getAuxList(const MultiID&) const;
    bool		putAuxList(const MultiID&,const IOParList*) const;
    bool		hasAux(const MultiID&) const;
    bool		removeAux(const MultiID&) const;

    const char*		surveyName() const;
    static bool		newSurvey();
			/*!< if an external source has changed
				the .od/survey, force re-read it. */
    static void		setSurvey(const char*);
			/*!< will remove a possible existing IO manager and
			     set the survey to 'name', thus bypassing the
			     .od/survey file */

    bool		setRootDir(const char*);
    bool		setFileName(MultiID,const char*);
    const char*		generateFileName(Translator*,const char*);
    static bool		validSurveySetup(BufferString& errmsg);
    MultiID		newKey() const;

private:

    State		state_;
    IODir*		dirptr;
    int			curlvl;
    MultiID		prevkey;
    FileNameString	rootdir;

    static IOMan*	theinst_;
			IOMan(const char* rd=0);
    			~IOMan();
    void		init();
    static void		stop();

    bool		setDir(const char*);
    bool		getAuxfname(const MultiID&,FileNameString&) const;

};


inline IOMan& IOM()
{
    if ( !IOMan::theinst_ )
	{ IOMan::theinst_ = new IOMan; IOMan::theinst_->init(); }
    return *IOMan::theinst_;
}


/*!\mainpage
  \section intro Introduction

  This module uses the services from the Basic module and adds services that
  are (in general) more or less OpendTect specific. Just like the Basic module
  the services are used by all other modules.


  \section cont Content
  Some of the groups of services are:

  -# I/O management system
  - ioman.h : the IOM() object provides a lookup of objects in the data store
  - iooobj.h : Subclasses of IOObj hold all data necessary to access a stored
    object.
  - iostrm.h : IOStream is the most common subclass of IOObj because files
    are most commonly where data is stored.
  -# Translators
  - transl.h : Translators are the objects that know file and database formats.
    All normal data will be put into and written from in-memory objects via
    subclasses of Translator.
  -# ArrayND
  - arraynd.h and arrayndinfo.h : interface for multi-dimensional but flexible
    arrays of any (simple) type.
  - arrayndimpl.h : implementation with specific classed for 1, 2, and 3D.
  - arrayndslice.h and arrayndutils.h : Slices and other utlities.
  -# CBVS
  - cbvsreadmgr.h : reads the 'Common Basic Volume Storage' format
  - cbvswritemgr.h : writes CBSV format.
  -# Transformations
  - transform.h and transfact.h : Interface and factory for transforms
  - costrans.h, fft.h, wavelettrans.h : transforms
*/


#endif
