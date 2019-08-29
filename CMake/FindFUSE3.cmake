# This module can find FUSE3 Library
#
# The following variables will be defined for your use:
# - FUSE3_FOUND : was FUSE3 found?
# - FUSE3_INCLUDE_DIRS : FUSE3 include directory
# - FUSE3_LIBRARIES : FUSE3 library
#
# In addition, an imported target "FUSE3" will be defined. USE THIS TARGET and
# avoid directly working with the variables defined here.

find_package(PkgConfig)
if(PkgConfig_FOUND)
  pkg_check_modules(PC_FUSE3 fuse3)
endif()

find_path(
    FUSE3_INCLUDE_DIRS
    NAMES fuse_common.h fuse_lowlevel.h fuse.h
    PATHS /usr/local/include/osxfuse /usr/local/include
    PATH_SUFFIXES fuse3
    HINTS ${PC_FUSE3_INCLUDEDIR} ${PC_FUSE3_INCLUDE_DIRS})

set(fuse3_names fuse3)
if(APPLE)
  list(APPEND fuse3_names libosxfuse3.dylib)
endif()

find_library(FUSE3_LIBRARIES
    NAMES ${fuse3_names}
    PATHS /usr/local
    PATH_SUFFIXES lib lib64
    HINTS ${PC_FUSE3_LIBDIR} ${PC_FUSE3_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    FUSE3 DEFAULT_MSG
    FUSE3_INCLUDE_DIRS FUSE3_LIBRARIES)

if(FUSE3_FOUND AND NOT TARGET FUSE3)
  add_library(FUSE3 UNKNOWN IMPORTED)
  set_target_properties(FUSE3 PROPERTIES
      IMPORTED_LOCATION "${FUSE3_LIBRARIES}"
      INTERFACE_COMPILE_OPTIONS "${PC_FUSE3_CFLAGS_OTHER}"
      INTERFACE_COMPILE_DEFINITIONS "_FILE_OFFSET_BITS=64"
      INTERFACE_INCLUDE_DIRECTORIES "${FUSE3_INCLUDE_DIRS}")
endif()

mark_as_advanced(
    FUSE3_INCLUDE_DIRS FUSE3_LIBRARIES)

