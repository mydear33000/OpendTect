#_______________________CMake___________________________________________________
#
#	Makefile : 	Basic
# 	July 1995	Bert
#_______________________________________________________________________________


set ( OD_MODULE_DEPS Basic Seis General )

list ( APPEND OD_MODULE_SOURCES cseis_help.cc 
           cseis_methods.cc
           csExecPhaseDef.cc 
           csHelp.cc 
           csInitExecEnv.cc
           csMethodRetriever.cc 
           csModule.cc 
           csParamDef.cc 
           csParamManager.cc 
           csRunManager.cc            
           csSelectionManager.cc 
           csStackUtil.cc            
           csTableManager.cc 
           csTableManagerNew.cc
           csUserConstant.cc 
           csUserParam.cc
           csModuleLists.cc
        )

SET( OD_MODULE_PROGS as_jobsubmit.cc )

OD_INIT_MODULE()
