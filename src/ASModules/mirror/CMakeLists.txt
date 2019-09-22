set(OD_MODULE_DEPS Basic General Seis ASSystem)
set (MODLE_NAME as_mirror)
set (OD_MODULE_SOURCES
           mod_mirror.cc
    csLine3D.cc
    csProfile.cc
    grid2D.cc
    main_grid2D.cc
)
OD_INIT_MODULE()
