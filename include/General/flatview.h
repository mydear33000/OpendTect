#ifndef flatview_h
#define flatview_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "coltabmapper.h"
#include "datapackbase.h"
#include "draw.h"
#include "mousecursor.h"

class FlatView_CB_Rcvr;
class ZAxisTransform;
namespace ZDomain { class Def; }
namespace FlatView
{

typedef Geom::Point2D<double> Point;
typedef Geom::PosRectangle<double> Rect;

/*!
\brief Class that represents non-bitmap data to be displayed in a flatviewer,
such as markers, lines and more.
*/

mExpClass(General) AuxData
{
public:

    /*!\brief Explains what part of an AuxData's appearance may be edited by
    the user.*/

    mExpClass(General) EditPermissions
    {
    public:
			EditPermissions();

	bool		onoff_;
	bool		namepos_;
	bool		linestyle_;
	bool		linecolor_;
	bool		fillcolor_;
	bool		markerstyle_;
	bool		markercolor_;
	bool		x1rg_;
	bool		x2rg_;
    };


    virtual			~AuxData();
    virtual AuxData*		clone() const { return new AuxData(*this); }
    virtual void		touch()					{}

    EditPermissions*		editpermissions_;//!<If null no editing allowed

    bool			enabled_;	//!<Turns on/off everything
    BufferString		name_;
    Alignment			namealignment_;
    int				namepos_;	//!<nodraw=udf, before first=-1,
					    //!< center=0, after last=1
    bool			fitnameinview_;
    Interval<double>*		x1rg_;		//!<if 0, use viewer's rg & zoom
    Interval<double>*		x2rg_;		//!<if 0, use viewer's rg & zoom

    TypeSet<Point>		poly_;
    TypeSet<MarkerStyle2D>	markerstyles_;
				/*!<- No markerstyles means no markers will be
				      displayed.
				    - If number of markerstyles is more than
				      zero, but less than number of points, the
				      last markerstyle will be used for the
				      excess points.
				*/

    OD::LineStyle		linestyle_;
    Color			fillcolor_;
    FillPattern			fillpattern_;

    mExpClass(General) FillGradient
    {
    public:
			FillGradient()
			    : from_(Point::udf()),to_(Point::udf())	{}
			FillGradient(const Point& fr,const Point& to,
				     const TypeSet<float>& stops,
				     const TypeSet<Color>& colors)
			    : from_(fr),to_(to)
			    , stops_(stops),colors_(colors)	{}

	bool		hasGradient()	{ return !stops_.isEmpty(); }

	Point		from_;
	Point		to_;
	TypeSet<float>	stops_;
	TypeSet<Color>	colors_;
    };

    FillGradient		fillgradient_;    

    int				zvalue_; //!<overlay zvalue ( max=on top )
    MouseCursor			cursor_;

    bool			close_;

    void			setFillPattern( const FillPattern& fp )
						{ fillpattern_ = fp; }
    bool			isEmpty() const;
    void			empty();
    bool			turnon_;
    bool			needsupdatelines_;

    // should be protected, don't use.
				AuxData( const char* nm );
				AuxData( const AuxData& );
				friend class Viewer;
};


/*!
\brief Annotation data for flatviewers.
*/

mExpClass(General) Annotation
{
public:


    /*!\brief Things like well tracks, cultural data, 2-D line positions.*/

    mStruct(General) AxisData
    {
				AxisData();

	BufferString		name_;
	SamplingData<float>	sampling_;
	bool			showannot_;
	bool			showgridlines_;
	bool			reversed_;
	bool			annotinint_;
	int			factor_;
	bool			showauxannot_;
	uiString		auxlabel_;
	OD::LineStyle		auxlinestyle_;
	OD::LineStyle		auxhllinestyle_;
	TypeSet<PlotAnnotation> auxannot_;
	int			auxPosIdx(float pos,float eps) const;
	void			showAll(bool yn);
    };


				Annotation(bool darkbg);
				~Annotation();

    BufferString		title_; //!< color not settable
    Color			color_; //!< For axes
    AxisData			x1_;
    AxisData			x2_;

    bool			showaux_;
    bool			showscalebar_;
    bool			editable_;
    bool			allowuserchange_;
    bool			allowuserchangereversedaxis_;

    inline void		setAxesAnnot( bool yn ) //!< Convenience all or nothing
			{ x1_.showAll(yn); x2_.showAll(yn); }
    inline bool		haveTitle() const
			{ return !title_.isEmpty(); }
    inline bool		haveAxisAnnot( bool x1dir ) const
			{ return color_.isVisible()
			      && ( ( x1dir && x1_.showannot_)
				|| (!x1dir && x2_.showannot_)); }
    inline bool		haveGridLines( bool x1dir ) const
			{ return color_.isVisible()
			      && ( ( x1dir && x1_.showgridlines_)
				|| (!x1dir && x2_.showgridlines_)); }
    //bool		haveAux() const;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    static const char*	sKeyAxes();
    static const char*	sKeyX1Sampl();
    static const char*	sKeyX2Sampl();
    static const char*	sKeyShwAnnot();
    static const char*	sKeyShwGridLines();
    static const char*	sKeyIsRev();
    static const char*	sKeyShwAux();
};


/*!
\brief Data display parameters.

  When data needs to be displayed, there is a load of parameters and options
  for display. The two main display modes are:
  Variable Density = display according to color table
  Wiggle/Variable Area = wiggles with (possibly) filled amplitude
*/

mExpClass(General) DataDispPars
{
public:

    //!\brief Common to VD and WVA
    mExpClass(General) Common
    {
    public:
			Common();

	bool			show_;	   // default=true
	bool			blocky_;   // default=false
	bool			allowuserchange_;
	bool			allowuserchangedata_;
	ColTab::MapperSetup	mappersetup_;
    };

    //!\brief Variable Density (=color-bar driven) parameters
    mExpClass(General) VD : public Common
    {
    public:

			VD()
			: lininterp_(false) {}

	BufferString	ctab_;
	bool		lininterp_; // Use bi-linear interpol, not poly
    };
    //!\brief Wiggle/Variable Area parameters
    mExpClass(General) WVA : public Common
    {
    public:

		    WVA()
			: wigg_(Color::Black())
			, refline_(Color::NoColor())
			, lowfill_(Color::NoColor())
			, highfill_(Color::Black())
			, overlap_(1)
			, reflinevalue_(mUdf(float))	{}

	Color		wigg_;
	Color		refline_;
	Color		lowfill_;
	Color		highfill_;
	float		overlap_;
	float		reflinevalue_;
    };

			DataDispPars()		{}

    VD			vd_;
    WVA			wva_;
    void		show( bool wva, bool vd )
			{ wva_.show_ = wva; vd_.show_ = vd; }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    static const char*	sKeyVD();
    static const char*	sKeyWVA();
    static const char*	sKeyShow();
    static const char*	sKeyDispRg();
    static const char*	sKeyColTab();
    static const char*  sKeyFlipSequence();
    static const char*	sKeyLinearInter();
    static const char*	sKeyBlocky();
    static const char*  sKeyAutoScale();
    static const char*	sKeyClipPerc();
    static const char*	sKeyWiggCol();
    static const char*	sKeyRefLineCol();
    static const char*	sKeyLowFillCol();
    static const char*	sKeyHighFillCol();
    static const char*	sKeyOverlap();
    static const char*	sKeySymMidValue();
    static const char*	sKeyRefLineValue();
};


/*!
\brief Flatviewer appearance.
*/

mExpClass(General) Appearance
{
public:
			Appearance( bool drkbg=true )
			    : darkbg_(drkbg)
			    , annot_(drkbg)
			    , secondsetaxes_(drkbg)
			    , anglewithset1_(0)		{}

    virtual		~Appearance()			{}

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    Annotation		annot_;
    DataDispPars	ddpars_;

    //Second set of axes, especially handy in case flat viewer is horizontal
    Annotation		secondsetaxes_;
    float		anglewithset1_;

    void		setDarkBG(bool yn);
    bool		darkBG() const		{ return darkbg_; }

    void		setGeoDefaults(bool isvert);

protected:

    bool		darkbg_;	//!< Two styles: dark (=black)
					//!< and lite (=white) background
					//!< Impacts a lot of display choices
};


/*!
\brief Flat Viewer using FlatView::Data and FlatView::Appearance.

  Interface for displaying data and related annotations where at least one of
  the directions is sampled regularly.

  The viewer works with FlatDataPacks - period. in previous versions, you could
  pass 'loose' data but DataPacks are so neat&clean we got rid of this
  possibility.

  You can attach many datapacks to the viewer; the different modes (WVA, VD)
  can be attached to zero or one of those packs.

  addPack() -> add a DataPack to the list of available data packs
  usePack() -> sets one of the available packs for wva of vd display
  setPack() -> Combination of addPack and usePack.
  removePack() -> removes this pack from the available packs, if necessary
		  it also clears the wva or vd display to no display.
*/

mExpClass(General) Viewer
{
public:

			Viewer();
    virtual		~Viewer();

    virtual Appearance&	appearance();
    const Appearance&	appearance() const
			{ return const_cast<Viewer*>(this)->appearance(); }

    ZAxisTransform*	getZAxisTransform() const
			{ return datatransform_; }
    bool		setZAxisTransform(ZAxisTransform*);
    bool		hasZAxisTransform() const
			{ return datatransform_; }

    void		addPack(::DataPack::ID);
			/*!< Adds to list and obtains the DataPack, but does not
			 use for WVA or VD. DataPack gets released in the
			 destructor of this class. */
    void		usePack(bool wva,::DataPack::ID,bool usedefs=true );
			//!< Does not add new packs, just selects from added
    void		removePack(::DataPack::ID);
			//!< Releases DataPack after removing from the list.
    void		setPack(bool wva,::DataPack::ID id,bool usedefs=true)
			{ addPack( id ); usePack( wva, id, usedefs ); }
    void		clearAllPacks();

    const FlatDataPack* obtainPack(bool wva,bool checkother=false) const;
			/*!< Obtains DataPack before returning the pointer. Has
			 to be released after it is used. For convenience use
			 ConstDataPackRef which releases the DataPack in its
			 destructor.
			 \param checkother if true, the datapack of other
			 display (i.e. variable density or wiggles) is returned
			 if the specified display has no datapack. */
    bool		hasPack(bool wva) const
			{ return packID(wva)!=DataPack::cNoID(); }
    DataPack::ID	packID(bool wva) const;

    const TypeSet< ::DataPack::ID>&	availablePacks() const	{ return ids_; }

    virtual bool	isVertical() const		{ return true; }
    bool		isVisible(bool wva) const;
    void		setVisible(bool wva, bool visibility);
			//!< Will also handleChange.
			//!< So, do not use unless you want both.

    Coord3		getCoord(const Point&) const;

    enum DataChangeType	{ All=0xFFFFFFFF, BitmapData=0x0001, DisplayPars=0x0002,
			  Annot=0x0004, Auxdata=0x0008 };
    virtual void	handleChange(unsigned int datachangetype)	= 0;
			/*!<\param datachangetype can be any combination of
				   DataChangeType. */

			//!Does not store any data, just how data is displayed
    virtual void	fillAppearancePar( IOPar& iop ) const
			{ appearance().fillPar( iop ); }
			/*!Does not retrieve any data, just how data is
			  displayed */
    virtual void	useAppearancePar( const IOPar& iop )
			{ appearance().usePar( iop ); }

    void		storeDefaults(const char* key) const;
    void		useStoredDefaults(const char* key);

    void		getAuxInfo(const Point&,IOPar&) const;
    virtual void	showAuxDataObjects(AuxData&,bool)	{}
    virtual void	updateProperties(const AuxData&)	{}
    virtual void	reGenerate(AuxData&)		{}
    virtual void	remove(const AuxData&)		{}

    StepInterval<double> getDataPackRange(bool forx1) const;
    virtual Interval<float> getDataRange(bool wva) const;

    virtual AuxData*		createAuxData(const char* nm) const	= 0;

    virtual int			nrAuxData() const			= 0;
    virtual AuxData*		getAuxData(int idx)			= 0;
    virtual const AuxData*	getAuxData(int idx) const		= 0;
    virtual void		addAuxData(AuxData* a)			= 0;
    virtual AuxData*		removeAuxData(AuxData* a)		= 0;
    virtual AuxData*		removeAuxData(int idx)			= 0;
    void			removeAuxDatas(ObjectSet<AuxData>&);
    void			removeAllAuxData();
    virtual void		setAnnotChoice(int selannot)		{}
    virtual int			getAnnotChoice(BufferStringSet&) const
				{ return -1; }
    void			enableStatusBarUpdate()
				{ needstatusbarupd_ = true; }
    void			disableStatusBarUpdate()
				{ needstatusbarupd_ = false; }
    bool			needStatusBarUpdate() const
				{ return needstatusbarupd_; }
    void			setSeisGeomidsToViewer(TypeSet<Pos::GeomID>&);
    const TypeSet<Pos::GeomID>&	getAllSeisGeomids() const;
    void			setZDomain(const ZDomain::Def&);
    const ZDomain::Info&	zDomain() const;

protected:

    TypeSet< ::DataPack::ID>	ids_;
    Appearance*			defapp_;
    DataPackMgr&		dpm_;
    ZAxisTransform*		datatransform_;
    FlatView_CB_Rcvr*		cbrcvr_;
    mutable Threads::Lock	lock_;
    bool			needstatusbarupd_;
    ZDomain::Info*		zdinfo_;

    void			addAuxInfo(bool,const Point&,IOPar&) const;

private:

    const FlatDataPack*		wvapack_;
    const FlatDataPack*		vdpack_;
    TypeSet<Pos::GeomID>	geom2dids_;

public:
};

} // namespace FlatView

#endif
