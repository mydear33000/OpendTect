#ifndef velocitfunctionyasciio_h
#define velocitfunctionyasciio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Aug 2008
 RCS:		$Id: velocityfunctionascio.h,v 1.4 2009-09-28 12:24:04 cvskris Exp $
________________________________________________________________________

-*/

#include "tableascio.h"
#include "task.h"

namespace Table { class FormatDesc; }
class BinIDValueSet;
class TaskRunner;

namespace Vel
{

mClass FunctionAscIO : public Table::AscIO, public SequentialTask
{
public:
    				FunctionAscIO( const Table::FormatDesc& fd,
					       std::istream& stm,
				       	       od_int64 filesizeinkb=-1 );
   static Table::FormatDesc*	getDesc();
   static void			updateDesc(Table::FormatDesc&);

   float 			getUdfVal() const;
   bool				isXY() const;
   void				setOutput(BinIDValueSet& bvs)
       				{ output_ = &bvs; first_ = true; }

protected:
   int				nextStep();
   od_int64			nrDone() const { return nrdone_/1024; }
   const char*			nrDoneText() const { return "KBytes read"; }
   od_int64			totalNr() const { return nrkbytes_; }
   static void			createDescBody(Table::FormatDesc&);

   std::istream&		strm_;   
   BinIDValueSet*		output_;
   bool				first_;
   od_int64			nrdone_;
   od_int64			nrkbytes_;
};

} // namespace Vel


#endif
