#ifndef uirockphysform_h
#define uirockphysform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2011
 RCS:           $Id: uirockphysform.h,v 1.5 2012-02-29 13:42:57 cvshelene Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "propertyref.h"
class uiComboBox;
class uiGenInput;
class uiLabel;
class uiTextEdit;
class uiRockPhysCstFld;


mClass uiRockPhysForm : public uiGroup
{
public:

			uiRockPhysForm(uiParent*);
			uiRockPhysForm(uiParent*,PropertyRef::StdType);

    PropertyRef::StdType getType() const;
    void		setType(PropertyRef::StdType);
    				//!< only works when 1st constructor used
    const char*		formulaName() const;
    void		setFormulaName(const char*);

    BufferString	getText() const;
    const char*		errMsg() const		{ return errmsg_.buf(); }
    bool		isOK() const;

protected:

    uiComboBox*		typfld_;
    uiComboBox*		nmfld_;
    uiTextEdit*		descriptionfld_;
    const PropertyRef::StdType fixedtype_;

    void		typSel(CallBacker*);
    void		nameSel(CallBacker*);

    void		createFlds(uiObject*);

    ObjectSet<uiRockPhysCstFld>	cstflds_;

    BufferString	errmsg_;
};


mClass uiRockPhysCstFld : public uiGroup
{
public:

			uiRockPhysCstFld(uiParent*);

    float		getCstVal() const;
    void		updField(BufferString,Interval<float>,
	    			 float val = mUdf(float));


protected:

    uiGenInput*		valfld_;
    uiLabel*		nmlbl_;
    uiLabel*		rangelbl_;
};


#endif
