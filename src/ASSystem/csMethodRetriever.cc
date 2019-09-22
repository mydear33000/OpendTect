/* Copyright (c) Colorado School of Mines, 2013.*/
/* All rights reserved.                       */

#include <string>
#include <fstream>
#include <cstring>
#include <sstream>
#include "csMethodRetriever.h"
#include "csException.h"
#include "csVector.h"
#include "geolib_string_utils.h"
#include "csModuleLists.h"
#include "geolib_platform_dependent.h"

#include "oddirs.h"
#include "file.h"
#include "glog/logging.h"

#include "cseis_modules.h"
#include <dlfcn.h>

using namespace cseis_system;
using namespace cseis_geolib;

csMethodRetriever* csMethodRetriever::theinst_ = nullptr;
static std::string libdir_;

csMethodRetriever& csMethodRetriever::MethodRetriever(){
    if(!csMethodRetriever::theinst_){
        csMethodRetriever::theinst_ = new csMethodRetriever();
    }

    return *csMethodRetriever::theinst_;
}
csMethodRetriever csMethodRetriever::MethodRetrieverFree(){
    if(csMethodRetriever::theinst_) delete csMethodRetriever::theinst_;
    csMethodRetriever::theinst_ = nullptr;
}
csMethodRetriever::csMethodRetriever()
    : isOk_(false)
    , modulelists_(nullptr)
{
    init();
}
csMethodRetriever::~csMethodRetriever(){
    if(modulelists_)delete modulelists_;
}
void csMethodRetriever::init(){
    isOk_ = false;

    /*mcfilename_ = GetASModelListFileName();

    if(!File::exists(mcfilename_.c_str()))
        LOG(ERROR) << "Cant find module configure file " << mcfilename_ << "!";

    if(modulelists_)delete modulelists_;
    modulelists_ = new csModuleLists(mcfilename_);

    libdir_ = modulelists_->libPath();*/

    isOk_ = true;
}
int csMethodRetriever::nModules(){
    if(modulelists_)return modulelists_->nModules();

    return 0;
}
std::vector<std::string> csMethodRetriever::ModuleNames(){
    if(!isOk())LOG(ERROR) << "!!!BUGS: WTF";

    return modulelists_->modulesNMList();
}
int csMethodRetriever::nInport(std::string modulenm){
    if(modulelists_)return modulelists_->nInport(modulenm);
    return 0;
}
int csMethodRetriever::nOutport(std::string modulenm){
    if(modulelists_)return modulelists_->nOutport(modulenm);
    return 0;
}
std::string csMethodRetriever::Type(std::string modulenm){
    if(modulelists_)return modulelists_->Type(modulenm);
    return std::string("unknow");
}
std::string csMethodRetriever::methodLibfnm(std::string const& name){
    if(!isOk())LOG(ERROR) << "!!!BUGS: WTF";

    std::string nameLower = cseis_geolib::toLowerCase( name );
    std::stringstream ss;

    ss << modulelists_->modelFullNM(nameLower);
    return ss.str();
}
std::string csMethodRetriever::getMethodLibfnm(std::string const& name){
    return MethodRetriever().methodLibfnm(name);
}
std::string csMethodRetriever::getMethodLibfnm(std::string const& name, int verMajor, int verMinor){
    std::stringstream ss;
    ss << getMethodLibfnm(name);

    ss << "." << verMajor << "." << verMinor;

    return ss.str();
}
std::string csMethodRetriever::getMethodLibfnm(std::string const& name, std::string versionString){
    std::stringstream ss;
    ss << getMethodLibfnm(name);

    ss << versionString;

    return ss.str();
}
void csMethodRetriever::getParamInitMethod( std::string const& name, int verMajor, int verMinor, MParamPtr& param, MInitPtr& init ) {
  std::string nameLower = cseis_geolib::toLowerCase( name );
  std::string soName = getMethodLibfnm(nameLower, verMajor, verMinor);

  void* handle = dlopen( soName.c_str(), RTLD_LAZY );
  const char *dlopen_error = dlerror();

  if( dlopen_error ) {
    throw( cseis_geolib::csException("Error occurred while opening shared library. ...does module '%s' exist? Does version '%d.%d' exist?\nSystem message: %s\n",
                       name.c_str(), verMajor, verMinor, dlopen_error ) );
  }

  param = getParamMethod( nameLower, handle );
  init  = getInitMethod( nameLower, handle );

  //  dlclose(handle);
  //  fprintf(stderr,"Param method pointer found: %x\n", param);
}
//----------------------------------------------------------------------------
//
MParamPtr csMethodRetriever::getParamMethod( std::string const& nameLower, char const* soName ) {
  void* handle = dlopen( soName, RTLD_LAZY );
  const char *dlopen_error = dlerror();

  if( dlopen_error ) {
    fprintf(stdout, "Error occurred while opening shared library. ...does module '%s' exist?\nSystem message: %s\n\n",
            nameLower.c_str(), dlopen_error );
    fflush(stdout);
    throw( cseis_geolib::csException("Error occurred while opening shared library. ...does module '%s' exist?\nSystem message: %s\n",
      				     nameLower.c_str(), dlopen_error ) );
  }
  return getParamMethod( nameLower, handle );
}
MParamPtr csMethodRetriever::getParamMethod( std::string const& name ) {
  std::string soName = getMethodLibfnm(name);
  return getParamMethod( cseis_geolib::toLowerCase( name ), soName.c_str() );
}
MParamPtr csMethodRetriever::getParamMethod( std::string const& name, int verMajor, int verMinor ) {
  std::string soName = getMethodLibfnm(name, verMajor, verMinor);
  return getParamMethod( cseis_geolib::toLowerCase( name ), soName.c_str() );
}
MParamPtr csMethodRetriever::getParamMethod( std::string const& name, std::string versionString ) {
  std::string soName = getMethodLibfnm(name, versionString);
  return getParamMethod( cseis_geolib::toLowerCase( name ), soName.c_str() );
}
//----------------------------------------------------------------------------
//
MInitPtr csMethodRetriever::getInitMethod( std::string const& nameLower, void* handle  ) {
  char* methodName = new char[200];
  sprintf( methodName, "_init_mod_%s_", nameLower.c_str() );
  
  //  fprintf(stderr,"Init method name:  '%s'\n", methodName );

  //  MInitPtr method = reinterpret_cast<MInitPtr>( dlsym(handle,methodName) );
  MInitPtr method;
  void *ptr = dlsym(handle,methodName);
  memcpy(&method, &ptr, sizeof(void *));

  const char *dlsym_error = dlerror();
  delete [] methodName;
  if( dlsym_error ) {
    throw( cseis_geolib::csException("Cannot find init definition method. System message:\n%s\n", dlsym_error ) );
  }
  return method;
}
//----------------------------------------------------------------------------
//
MParamPtr csMethodRetriever::getParamMethod( std::string const& nameLower, void* handle ) {
  char* methodName = new char[200];
  sprintf( methodName, "_params_mod_%s_", nameLower.c_str() );

  //  MParamPtr method = reinterpret_cast<MParamPtr>( dlsym(handle,methodName) );
  MParamPtr method;
  void *ptr = dlsym(handle,methodName);
  memcpy(&method, &ptr, sizeof(void *));

  const char *dlsym_error = dlerror();

  delete [] methodName;
  if( dlsym_error ) {
    throw( cseis_geolib::csException("Cannot find parameter definition method. System message:\n%s\n", dlsym_error) );
  }

  return method;
}
//--------------------------------------------------------------------
//
void csMethodRetriever::getExecMethodSingleTrace( std::string const& name, int verMajor, int verMinor, MExecSingleTracePtr& exec ) {
  std::string nameLower = cseis_geolib::toLowerCase( name );

  std::string soName = getMethodLibfnm(name, verMajor, verMinor);

  void* handle = dlopen( soName.c_str(), RTLD_LAZY );
  const char *dlopen_error = dlerror();

  if( dlopen_error ) {
    throw( cseis_geolib::csException("Error occurred while opening shared library. ...does module '%s' exist? Does version '%d.%d' exist?\nSystem message: %s\n",
                       name.c_str(), verMajor, verMinor, dlopen_error ) );
  }

  exec  = getExecMethodSingleTrace( nameLower, handle );

  //  myMethodNameList.insertEnd(nameLower);
  //  myExecList.insertEnd(exec);
}
//----------------------------------------------------------
//
void csMethodRetriever::getExecMethodMultiTrace( std::string const& name, int verMajor, int verMinor, MExecMultiTracePtr& exec ) {

  std::string nameLower = cseis_geolib::toLowerCase( name );

  std::string soName = getMethodLibfnm(name, verMajor, verMinor);

  void* handle = dlopen( soName.c_str(), RTLD_LAZY );
  const char *dlopen_error = dlerror();

  if( dlopen_error ) {
    //    dlclose(handle);
    throw( cseis_geolib::csException("Error occurred while opening shared library. ...does module '%s' exist? Does version '%d.%d' exist?\nSystem message: %s\n",
                       name.c_str(), verMajor, verMinor, dlopen_error ) );
  }

  exec  = getExecMethodMultiTrace( nameLower, handle );

  //  myMethodNameList.insertEnd(nameLower);
  // myExecList.insertEnd(exec);
}
//
//---------------------------------------------------------
MExecSingleTracePtr csMethodRetriever::getExecMethodSingleTrace( std::string const& nameLower, void* handle  ) {
  char* methodName = new char[200];
  sprintf( methodName, "_exec_mod_%s_", nameLower.c_str() );
  
  //  fprintf(stderr,"Exec method name:  '%s'\n", methodName );

  //  MExecSingleTracePtr method = reinterpret_cast<MExecSingleTracePtr>( dlsym(handle,methodName) );
  MExecSingleTracePtr method;
  void *ptr = dlsym(handle,methodName);
  memcpy(&method, &ptr, sizeof(void *));

  const char *dlsym_error = dlerror();
  delete [] methodName;
  if( dlsym_error ) {
    throw( cseis_geolib::csException("Cannot find exec definition method. System message:\n%s\n", dlsym_error) );
  }
  return method;
}
//---------------------------------------------------------
MExecMultiTracePtr csMethodRetriever::getExecMethodMultiTrace( std::string const& nameLower, void* handle  ) {
  char* methodName = new char[200];
  sprintf( methodName, "_exec_mod_%s_", nameLower.c_str() );
  
  //  fprintf(stderr,"Exec method name:  '%s'\n", methodName );

  //  MExecMultiTracePtr method = reinterpret_cast<MExecMultiTracePtr>( dlsym(handle,methodName) );
  MExecMultiTracePtr method;
  void *ptr = dlsym(handle,methodName);
  memcpy(&method, &ptr, sizeof(void *));

  const char *dlsym_error = dlerror();
  delete [] methodName;
  if( dlsym_error ) {
    throw( cseis_geolib::csException("Cannot find exec definition method. System message:\n%s\n", dlsym_error) );
  }
  return method;
}


//--------------------------------------------------------------------
//
int csMethodRetriever::getNumStandardModules() {
  return MethodRetriever().nModules();
}
//--------------------------------------------------------------------
//
std::vector<std::string> csMethodRetriever::getStandardModuleNames() {
  return MethodRetriever().ModuleNames();
}


