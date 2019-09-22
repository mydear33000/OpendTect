/* Copyright (c) Colorado School of Mines, 2013.*/
/* All rights reserved.                       */

#include <cstring>
#include "cseis_includes.h"
#include "csFXDecon.h"

using namespace cseis_system;
using namespace cseis_geolib;
using namespace std;

/**
 * CSEIS - Seabed Seismic Processing System
 * Module: FXDECON
 *
 */
namespace mod_fxdecon {
  struct VariableStruct {
    csFXDecon* fxdecon;
  };
  static int const MODE_ENSEMBLE = 11;
  static int const MODE_TRACE    = 12;
  static int const MODE_ALL      = 13;
}
using namespace mod_fxdecon;

//*************************************************************************************************
// Init phase
//
//
//
//*************************************************************************************************
void init_mod_fxdecon_( csParamManager* param, csInitPhaseEnv* env, csLogWriter* log )
{
  csExecPhaseDef*   edef = env->execPhaseDef;
  //  csTraceHeaderDef* hdef = env->headerDef;
  csSuperHeader*    shdr = env->superHeader;
  VariableStruct* vars = new VariableStruct();
  edef->setVariables( vars );

  vars->fxdecon = NULL;

  edef->setExecType( EXEC_TYPE_MULTITRACE );
  env->execPhaseDef->setTraceSelectionMode( TRCMODE_ENSEMBLE );

  mod_fxdecon::Attr attr;
  attr.fmin           = 0;
  attr.fmax           = 0;
  attr.winLen_samp    = 0;
  attr.taperLen_samp  = 0;
  attr.taperLen_s     = 0;
  attr.numWin         = 0;
  attr.ntraces_design = 0;
  attr.ntraces_filter = 0;

  if( param->exists( "freq_range" ) ) {
    param->getFloat( "freq_range", &attr.fmin, 0 );
    param->getFloat( "freq_range", &attr.fmax, 1 );
  }

  float windowLength_ms = shdr->numSamples * shdr->sampleInt;
  if( param->exists( "win_len" ) ) {
    param->getFloat( "win_len", &windowLength_ms, 0 );
    attr.winLen_samp = (int)round( windowLength_ms / shdr->sampleInt ) + 1;
  }
  attr.numWin = (int)round( (shdr->numSamples-1) * shdr->sampleInt / windowLength_ms );

  attr.taperLen_samp  = (int)round( 0.1 * attr.winLen_samp );
  if( param->exists( "taper_len" ) ) {
    float taperLength_ms;
    param->getFloat( "taper_len", &taperLength_ms, 0 );
    attr.taperLen_samp = (int)round( taperLength_ms / shdr->sampleInt );
    attr.taperLen_samp = ( attr.taperLen_samp%2 ? attr.taperLen_samp+1 : attr.taperLen_samp ); 
  }

  if( param->exists( "win_traces" ) ) {
    param->getInt( "win_traces", &attr.ntraces_design, 0 );
    param->getInt( "win_traces", &attr.ntraces_filter, 1 );
  }

  vars->fxdecon = new mod_fxdecon::csFXDecon();
  attr.taperLen_s = attr.taperLen_samp * shdr->sampleInt / 1000.0;
  if( attr.numWin == 0 ) attr.taperLen_s = 0;

  vars->fxdecon->initialize( shdr->sampleInt, shdr->numSamples, attr );

  if( edef->isDebug() ) {
    vars->fxdecon->dump();
  }
}

//*************************************************************************************************
// Exec phase
//
//
//
//*************************************************************************************************
void exec_mod_fxdecon_(
  csTraceGather* traceGather,
  int* port,
  int* numTrcToKeep,
  csExecPhaseEnv* env,
  csLogWriter* log )
{
  VariableStruct* vars = reinterpret_cast<VariableStruct*>( env->execPhaseDef->variables() );
  csExecPhaseDef* edef = env->execPhaseDef;
  //  csSuperHeader const* shdr = env->superHeader;

  if( edef->isCleanup() ) {
    if( vars->fxdecon != NULL ) {
      delete vars->fxdecon;
      vars->fxdecon = NULL;
    }
    delete vars; vars = NULL;
    return;
  }

  int numSamples = traceGather->trace(0)->numSamples();
  int numTraces  = traceGather->numTraces();

  float** samplesIn  = new float*[numTraces];
  float** samplesOut = new float*[numTraces];
  for( int itrc = 0; itrc < numTraces; itrc++ ) {
    samplesIn[itrc]  = traceGather->trace(itrc)->getTraceSamples();
    samplesOut[itrc] = new float[numSamples];
    memset( samplesOut[itrc], 0, numSamples*sizeof(float) );
  }

  vars->fxdecon->apply( samplesIn, samplesOut, numTraces, numSamples );
  for( int itrc = 0; itrc < numTraces; itrc++ ) {
    memcpy( samplesIn[itrc], samplesOut[itrc], numSamples*sizeof(float) );
  }

  for( int itrc = 0; itrc < numTraces; itrc++ ) {
    delete [] samplesOut[itrc];
    //    for( int isamp = 0; isamp < shdr->numSamples; isamp++ ) {
    //   samplesIn[itrc][isamp] = 1.0;
    //  }
  }
  delete [] samplesIn;
  delete [] samplesOut;
}

//*************************************************************************************************
// Parameter definition
//
//
//*************************************************************************************************
void params_mod_fxdecon_( csParamDef* pdef ) {
  pdef->setModule( "FXDECON", "FX deconvolution");

  pdef->addParam( "freq_range", "Frequency range to filter", NUM_VALUES_FIXED );
  pdef->addValue( "1", VALTYPE_NUMBER, "Minimum frequency [Hz]" );
  pdef->addValue( "12", VALTYPE_NUMBER, "Maximum frequency [Hz]" );

  pdef->addParam( "win_len", "Window length [ms]", NUM_VALUES_FIXED );
  pdef->addValue( "2000", VALTYPE_NUMBER );

  pdef->addParam( "win_traces", "Spatial/filter window", NUM_VALUES_FIXED );
  pdef->addValue( "10", VALTYPE_NUMBER, "Number of traces in design window" );
  pdef->addValue( "4", VALTYPE_NUMBER, "Number of traces for filter (smaller than window size)" );

  pdef->addParam( "taper_len", "Taper length [ms]", NUM_VALUES_FIXED );
  pdef->addValue( "100", VALTYPE_NUMBER );
}

extern "C" void _params_mod_fxdecon_( csParamDef* pdef ) {
  params_mod_fxdecon_( pdef );
}
extern "C" void _init_mod_fxdecon_( csParamManager* param, csInitPhaseEnv* env, csLogWriter* log ) {
  init_mod_fxdecon_( param, env, log );
}
extern "C" void _exec_mod_fxdecon_( csTraceGather* traceGather, int* port, int* numTrcToKeep, csExecPhaseEnv* env, csLogWriter* log ) {
  exec_mod_fxdecon_( traceGather, port, numTrcToKeep, env, log );
}

