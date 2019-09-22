/* Copyright (c) Colorado School of Mines, 2013.*/
/* All rights reserved.                       */

#ifndef CS_METHOD_RETRIEVER_H
#define CS_METHOD_RETRIEVER_H

#include "cseis_defines.h"
#include <string>
#include <vector>

#define ASMethodRetriever() csMethodRetriever::MethodRetriever()
#define ASMethodRetriever() csMethodRetriever::MethodRetrieverFree()

namespace cseis_geolib {
  template<typename T> class csVector;
}

namespace cseis_system {

class csModuleLists;
/**
* Retrieval of module methods
* Use static member function of this class to retrieve function pointers to the standard methods of Cseis processing modules
*
* @author Bjorn Olofsson
* @date   2007
*/
class csMethodRetriever {
public:
    csMethodRetriever();
    ~csMethodRetriever();

  /**
  * Retrieve function pointer to parameter method
  * @param name (i) Module name whose method shall be retrieved
  */
  static MParamPtr getParamMethod( std::string const& name, int verMajor, int verMinor );
  static MParamPtr getParamMethod( std::string const& name, std::string versionString );
  static MParamPtr getParamMethod( std::string const& name );
  /**
  * Retrieve function pointers to parameter method and initialisation method
  * @param name (i) Module name whose method shall be retrieved
  */
  static void getParamInitMethod( std::string const& name, int verMajor, int verMinor, MParamPtr& param, MInitPtr& init );
  /**
  * Retrieve function pointer to execution method for single-trace module
  * @param name (i) Module name whose method shall be retrieved
  */
  static void getExecMethodSingleTrace( std::string const& name, int verMajor, int verMinor, MExecSingleTracePtr& exec );
  /**
  * Retrieve function pointer to execution method for multi-trace module
  * @param name (i) Module name whose method shall be retrieved
  */
  static void getExecMethodMultiTrace( std::string const& name, int verMajor, int verMinor, MExecMultiTracePtr& exec );

  /**
  * NOTE: This method is only used when statically linking Cseis modules
  * @return number of standard modules defined
  */
  static int getNumStandardModules();
  /**
  * NOTE: This method is only used when statically linking Cseis modules
  * @return pointer to list of standard modules
  */
  static std::vector<std::string> getStandardModuleNames();

  /// friend extern csMethodRetriever& MethodRetriever();
  static csMethodRetriever*	theinst_;
  static csMethodRetriever& MethodRetriever();
  static csMethodRetriever MethodRetrieverFree();

  bool isOk(){return isOk_;}
  int nModules();
  std::vector<std::string> ModuleNames();
  int nInport(std::string modulenm);
  int nOutport(std::string modulenm);
  std::string Type(std::string modulenm);
  std::string methodLibfnm(std::string const& name);

 private:
  void init();
  static MParamPtr getParamMethod( std::string const& name, void* handle );
  static MInitPtr  getInitMethod( std::string const& name, void* handle );
  static MExecSingleTracePtr getExecMethodSingleTrace( std::string const& name, void* handle );
  static MExecMultiTracePtr getExecMethodMultiTrace( std::string const& name, void* handle );
  static std::string getMethodLibfnm(std::string const& name);
  static std::string getMethodLibfnm(std::string const& name, int verMajor, int verMinor);
  static std::string getMethodLibfnm(std::string const& name, std::string versionString);

  static int getMethodIndex( std::string const& name );
  static int const METHOD_NOT_FOUND = -33;

  static MParamPtr getParamMethod( std::string const& name, char const* soName );

  bool isOk_;
  std::string mcfilename_;
  csModuleLists* modulelists_;
};
} // namespace
#endif

