#ifndef uimathattrib_h
#define uimathattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2001
 RCS:           $Id: uimathattrib.h,v 1.1 2005-05-31 12:35:24 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };
class uiAttrSel;
class uiGenInput;
class uiPushButton;

/*! \brief Math Attribute description editor */

class uiMathAttrib : public uiAttrDescEd
{
public:

			uiMathAttrib(uiParent*);

protected:

    uiGenInput*         inpfld;
    uiPushButton*	parsebut;
    ObjectSet<uiAttrSel> attribflds;

    int			nrvariables;
    void 		parsePush(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);
};

#endif
