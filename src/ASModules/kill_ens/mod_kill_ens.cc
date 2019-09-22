/* Copyright (c) Colorado School of Mines, 2013.*/
/* All rights reserved.                       */

#include "cseis_includes.h"
#include "csSelection.h"

using namespace cseis_system;
using namespace cseis_geolib;
using namespace std;

/**
 * CSEIS - Seabed Seismic Processing System
 * Module: KILL_ENS
 *
 * @author Bjorn Olofsson
 * @date   2007
 */
namespace mod_kill_ens {
  struct VariableStruct {
    csSelection* selection;
    bool isModeInclude;
  };
}
using mod_kill_ens::VariableStruct;

//*************************************************************************************************
// Init phase
//
//
//
//*************************************************************************************************
void init_mod_kill_ens_( csParamManager* param, csInitPhaseEnv* env, csLogWriter* log )
{
//  csTraceHeaderDef* hdef = env->headerDef;
  csExecPhaseDef*   edef = env->execPhaseDef;
//  csSuperHeader*    shdr = env->superHeader;
  VariableStruct* vars = new VariableStruct();
  edef->setVariables( vars );

  edef->setExecType( EXEC_TYPE_SINGLETRACE );
  edef->setExecType( EXEC_TYPE_MULTITRACE );
  edef->setTraceSelectionMode( TRCMODE_ENSEMBLE );

  //---------------------------------------------------------

  vars->selection     = NULL;
  vars->isModeInclude = true;

  //---------------------------------------------------------
  std::string text;
  param->getString( "select", &text );

  try {
    type_t type = TYPE_INT;
    vars->selection = new csSelection( 1, &type );
    vars->selection->add( text );
  }
  catch( csException& e ) {
    vars->selection = NULL;
    log->error( "%s", e.getMessage() );
  }
  if( edef->isDebug() ) vars->selection->dump();

  if( param->exists( "mode" ) ) {
    param->getString( "mode", &text );
    if( !text.compare("include") ) {
     vars->isModeInclude = true;
    }
    else if( !text.compare("exclude") ) {
     vars->isModeInclude = false;
    }
    else {
     log->error( "Mode option not recognised: '%s'", text.c_str() );
    }
  }
}

//*************************************************************************************************
// Exec phase
//
//
//
//*************************************************************************************************
void exec_mod_kill_ens_(
                      csTraceGather* traceGather,
                      int* port,
                      int* numTrcToKeep,
                      csExecPhaseEnv* env,
                      csLogWriter* log )
{
  VariableStruct* vars = reinterpret_cast<VariableStruct*>( env->execPhaseDef->variables() );
  csExecPhaseDef* edef = env->execPhaseDef;

  if( edef->isCleanup() ) {
    if( vars->selection ) {
      delete vars->selection; vars->selection = NULL;
    }
    delete vars; vars = NULL;
    return;
  }

  int nTraces = traceGather->numTraces();
  cseis_geolib::csFlexNumber value( nTraces );
  bool isSelected = vars->selection->contains( &value );
  if( edef->isDebug() ) log->line("Number of traces: %d, selected: %s.\n", nTraces, isSelected ? "true" : "false" );
  if( (isSelected && vars->isModeInclude) || (!isSelected && !vars->isModeInclude) ) {
    // Kill this ensemble --> kill all traces in this ensemble
    traceGather->freeAllTraces();
  }
}

//*************************************************************************************************
// Parameter definition
//
//
//
//*************************************************************************************************
void params_mod_kill_ens_( csParamDef* pdef ) {
  pdef->setModule( "KILL_ENS", "Kill ensembles", "Kill ensembles that have the specified number of traces" );

  pdef->addParam( "select", "Selection of number of traces", NUM_VALUES_VARIABLE );
  pdef->addValue( "", VALTYPE_STRING, "Selection string. For example: <4 : Kill ensembles with less than 4 traces", "See documentation for more detailed description of selection syntax." );

  pdef->addParam( "mode", "Mode of selection", NUM_VALUES_FIXED );
  pdef->addValue( "include", VALTYPE_OPTION );
  pdef->addOption( "include", "Kill ensembles matching the specified selection criteria" );
  pdef->addOption( "exclude", "Kill ensembles NOT matching the specified selection criteria" );
}

extern "C" void _params_mod_kill_ens_( csParamDef* pdef ) {
  params_mod_kill_ens_( pdef );
}
extern "C" void _init_mod_kill_ens_( csParamManager* param, csInitPhaseEnv* env, csLogWriter* log ) {
  init_mod_kill_ens_( param, env, log );
}
extern "C" void _exec_mod_kill_ens_( csTraceGather* traceGather, int* port, int* numTrcToKeep, csExecPhaseEnv* env, csLogWriter* log ) {
  exec_mod_kill_ens_( traceGather, port, numTrcToKeep, env, log );
}

