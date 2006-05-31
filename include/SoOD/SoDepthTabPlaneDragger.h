#ifndef SoDepthTabPlaneDragger_h
#define SoDepthTabPlaneDragger_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoDepthTabPlaneDragger.h,v 1.4 2006-05-31 08:05:11 cvskris Exp $
________________________________________________________________________


-*/

#include <Inventor/draggers/SoDragger.h>


class SbLineProjector;
class SbPlaneProjector;

/*!\brief
A TabPlaneDragger where the moving on moves the dragger in z direction
(instead of xy as with SoTabPlaneDragger). In addition, there are limits
to how small the dragger is allowed to be.
*/

class SoDepthTabPlaneDragger : public SoDragger
{
    typedef SoDragger inherited;
    SO_KIT_HEADER(SoDepthTabPlaneDragger);

    SO_KIT_CATALOG_ENTRY_HEADER(planeSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(greenTabsHints);
    SO_KIT_CATALOG_ENTRY_HEADER(greenTabsMaterial);
    SO_KIT_CATALOG_ENTRY_HEADER(greenTabsMaterialBinding);
    SO_KIT_CATALOG_ENTRY_HEADER(greenTabsNormal);
    SO_KIT_CATALOG_ENTRY_HEADER(greenTabsNormalBinding);
    SO_KIT_CATALOG_ENTRY_HEADER(greenTabsSep);
    SO_KIT_CATALOG_ENTRY_HEADER(greenTabsOffset);
    SO_KIT_CATALOG_ENTRY_HEADER(greenTabsCoords);
    SO_KIT_CATALOG_ENTRY_HEADER(greenTabs);
    SO_KIT_CATALOG_ENTRY_HEADER(translator);

public:
    static void		initClass();
    			SoDepthTabPlaneDragger();

    SoSFVec3f		translation;
    SoSFVec3f		scaleFactor;

    SoSFVec3f		minSize;
    SoSFVec3f		maxSize;

    SoSFVec3f		minPos;
    SoSFVec3f		maxPos;

protected:
    			~SoDepthTabPlaneDragger();
    virtual void	GLRender(SoGLRenderAction*);

    virtual SbBool	setUpConnections(SbBool onoff,
	    				 SbBool doitalways = false);
    virtual void	setDefaultOnNonWritingFields();

    void		reallyAdjustScaleTabSize(SoGLRenderAction*);

    void		dragStart(void);
    void		drag(void);
    void		dragFinish(void);

private:

    static void		startCB(void* f, SoDragger * d);
    static void		motionCB(void* f, SoDragger * d);
    static void		finishCB(void* f, SoDragger * d);
    static void		fieldSensorCB(void* f, SoSensor * s);
    static void		valueChangedCB(void* f, SoDragger * d);

    void		createPrivateParts();
    SoNode*		getNodeFieldNode(const char *fieldname);

    SoFieldSensor*	scaleFieldSensor_;
    SoFieldSensor*	translFieldSensor_;
    SbLineProjector*	lineProj_;
    int			whatkind_;
    int			constraintState_;
    float		prevsizex_;
    float		prevsizey_;
    SbVec3f		scaleCenter_;
};

#endif

