/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vistransform.h"
#include "iopar.h"
#include "trigonometry.h"
#include "visosg.h"

#include <osg/MatrixTransform>

#include <osg/Vec3f>
#include <osg/Vec3d>

mCreateFactoryEntry( visBase::Transformation );


using namespace visBase;

Transformation::Transformation()
    : node_( 0 )
    , curtrans_( *new osg::Vec3d )
    , currot_( *new osg::Quat )
    , curscale_( *new osg::Vec3d )
    , curso_( *new osg::Quat )
{
    if ( osggroup_ ) osggroup_->unref();
    
    osggroup_ = node_ = new osg::MatrixTransform;
    osggroup_->ref();

    reset();
}


Transformation::~Transformation()
{
    //node is unreffed in visBase::DataObjectGroup
    delete &curtrans_;
    delete &currot_;
    delete &curscale_;
    delete &curso_;
}


void Transformation::reset()
{
    curtrans_ = osg::Vec3d( 0.0, 0.0, 0.0 );
    currot_ = curso_ = osg::Quat( 0.0, 0.0, 0.0, 1.0 );
    curscale_ = osg::Vec3d( 1.0, 1.0, 1.0 );
    node_->setMatrix( osg::Matrix::identity() );
    return;
}


void Transformation::setA( double a11, double a12, double a13, double a14,
			   double a21, double a22, double a23, double a24,
			   double a31, double a32, double a33, double a34,
			   double a41, double a42, double a43, double a44 )
{
    node_->setMatrix( osg::Matrix(
			a11, a21, a31, a41,
			a12, a22, a32, a42,
			a13, a23, a33, a43,
			a14, a24, a34, a44 ) );

    node_->getMatrix().decompose( curtrans_, currot_, curscale_, curso_ );

    updateNormalizationMode();
}


void Transformation::updateMatrix()
{
    osg::Matrix mat = osg::Matrix::scale( curscale_ );
    if ( !curso_.zeroRotation() )
    {
	mat.preMult( osg::Matrix::inverse(osg::Matrix::rotate(curso_)) );
	mat *= osg::Matrix::rotate( curso_ );
    }
    mat *= osg::Matrix::rotate( currot_ );
    mat *= osg::Matrix::translate( curtrans_ );
    node_->setMatrix( mat );
}


void Transformation::updateNormalizationMode()
{
    const double eps = 1e-5;
    const osg::Vec3d scale = node_->getMatrix().getScale();

    if ( node_->getStateSet() )
    {
	node_->getStateSet()->removeMode( GL_NORMALIZE );
	node_->getStateSet()->removeMode( GL_RESCALE_NORMAL );
    }

    if ( fabs(scale.x()-scale.y()) > eps ||
	 fabs(scale.y()-scale.z()) > eps ||
	 fabs(scale.z()-scale.x()) > eps )
    {
	node_->getOrCreateStateSet()->setMode( GL_NORMALIZE,
					       osg::StateAttribute::ON );
    }
    else if ( fabs(scale.x()-1.0) > eps ||
	      fabs(scale.y()-1.0) > eps ||
	      fabs(scale.z()-1.0) > eps )
    {
	node_->getOrCreateStateSet()->setMode( GL_RESCALE_NORMAL,
					       osg::StateAttribute::ON );
    }
}


void Transformation::setMatrix( const Coord3& trans,
				const Coord3& rotvec,double rotangle,
				const Coord3& scale )
{
    curtrans_ = Conv::to<osg::Vec3d>( trans );
    currot_ = osg::Quat( rotangle, Conv::to<osg::Vec3d>(rotvec) );
    curscale_ = Conv::to<osg::Vec3d>( scale );
    curso_ = osg::Quat( 0.0, 0.0, 0.0, 1.0 );
    updateMatrix();
    updateNormalizationMode();
}


void Transformation::setTranslation( const Coord3& vec )
{
    curtrans_ = Conv::to<osg::Vec3d>( vec );
    updateMatrix();
}


void Transformation::setRotation( const Coord3& vec, double angle )
{
    currot_ = osg::Quat( angle, Conv::to<osg::Vec3d>(vec) );
    updateMatrix();
}


void Transformation::setScale( const Coord3& vec )
{
    curscale_ = Conv::to<osg::Vec3d>( vec );
    updateMatrix();
    updateNormalizationMode();
}


void Transformation::setScaleOrientation( const Coord3& vec, double angle )
{
    curso_ = osg::Quat( angle, Conv::to<osg::Vec3d>(vec) );
    updateMatrix();
}


Coord3 Transformation::getTranslation() const
{
    return Conv::to<Coord3>( curtrans_ );
}


Coord3 Transformation::getScale() const
{
    return Conv::to<Coord3>( curscale_ );
}


void Transformation::setAbsoluteReferenceFrame()
{
    node_->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
}
    

#define mDeclTransType( func, tp, mat, post ) \
void Transformation::func( tp& inp ) const \
{ inp = mFromOsgVec( node_->mat().preMult( mToOsgVec(inp) ) post ); } \
void Transformation::func( const tp& inp, tp& to ) const \
{ to = mFromOsgVec( node_->mat().preMult( mToOsgVec(inp) ) post ); }


#define mDeclTrans( tp ) \
mDeclTransType( transform, tp, getMatrix, ) \
mDeclTransType( transformBack, tp, getInverseMatrix, ) \
mDeclTransType( transformDir, tp, getMatrix, -node_->getMatrix().getTrans() ) \
mDeclTransType( transformBackDir, tp, getInverseMatrix, -node_->getInverseMatrix().getTrans() )

#define mToOsgVec( inp ) inp
#define mFromOsgVec( inp ) inp

mDeclTrans( osg::Vec3f )
mDeclTrans( osg::Vec3d )

#undef mToOsgVec
#undef mFromOsgVec

#define mToOsgVec( inp ) Conv::to<osg::Vec3d>( inp )
#define mFromOsgVec( inp ) Conv::to<Coord3>( inp )

mDeclTrans( Coord3 )

#undef mToOsgVec
#undef mFromOsgVec

#define mDeclConvTransType( func, frtp, totp, mat, post ) \
void Transformation::func( const frtp& inp, totp& to ) const \
{ to = mFromOsgVec( node_->mat().preMult( mToOsgVec(inp) ) post ); }


#define mDeclConvTrans( frtp, totp ) \
mDeclConvTransType( transform, frtp, totp, getMatrix, ) \
mDeclConvTransType( transformBack, frtp, totp, getInverseMatrix, ) \
mDeclConvTransType( transformDir, frtp, totp, getMatrix, -node_->getMatrix().getTrans() ) \
mDeclConvTransType( transformBackDir, frtp, totp, getInverseMatrix, -node_->getInverseMatrix().getTrans() )

#define mToOsgVec( inp ) inp
#define mFromOsgVec( inp ) Conv::to<Coord3>( inp )
mDeclConvTrans( osg::Vec3d, Coord3 )
#undef mToOsgVec
#undef mFromOsgVec

#define mToOsgVec( inp ) Conv::to<osg::Vec3d>( inp )
#define mFromOsgVec( inp ) inp 
mDeclConvTrans( Coord3, osg::Vec3d )
#undef mToOsgVec
#undef mFromOsgVec

#define mToOsgVec( inp ) Conv::to<osg::Vec3d>( inp )
#define mFromOsgVec( inp ) inp
mDeclConvTrans( Coord3, osg::Vec3f )
#undef mToOsgVec
#undef mFromOsgVec

#define mToOsgVec( inp ) osg::Vec3d(inp)
#define mFromOsgVec( inp ) Conv::to<Coord3>( inp )
mDeclConvTrans( osg::Vec3f, Coord3 )
#undef mToOsgVec
#undef mFromOsgVec



