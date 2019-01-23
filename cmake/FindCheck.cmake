# - Find check
# Find the native check includes and libraries
#
#  CHECK_INCLUDE_DIRS - where to find check.h, etc.
#  CHECK_LIBRARIES    - List of libraries when using check.
#  CHECK_FOUND        - True if check found.

if (CHECK_INCLUDE_DIR)
	# Already in cache, be silent
	set(CHECK_FIND_QUIETLY TRUE)
endif ()

find_package (PkgConfig QUIET)
pkg_check_modules (PC_CHECK QUIET check)

message("PC_CHECK_STATIC_LIBRARIES: ${PC_CHECK_STATIC_LIBRARIES}")
message("PC_CHECK_STATIC_LIBRARY_DIRS: ${PC_CHECK_STATIC_LIBRARY_DIRS}")
message("PC_CHECK_STATIC_CFLAGS ${PC_CHECK_STATIC_CFLAGS}")

set (CHECK_VERSION ${PC_CHECK_VERSION})

find_path (CHECK_INCLUDE_DIR check.h
	HINTS
		${PC_OGG_INCLUDE_DIRS}
		${CHECK_ROOT}
	PATH_PREFIXES include
	)

find_library (CHECK_LIBRARY
	NAMES check
	HINTS
		${PC_CHECK_LIBRARY_DIRS}
		${CHECK_ROOT}
	)
# Handle the QUIETLY and REQUIRED arguments and set CHECK_FOUND
# to TRUE if all listed variables are TRUE.
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (Check
	REQUIRED_VARS
		CHECK_LIBRARY
		CHECK_INCLUDE_DIR
	VERSION_VAR
		CHECK_VERSION
	)

if (CHECK_FOUND)
	set (CHECK_LIBRARIES ${CHECK_LIBRARY} ${PC_CHECK_STATIC_LIBRARIES})
	set (CHECK_INCLUDE_DIRS ${CHECK_INCLUDE_DIR})
	set (CHECK_OPTIONS ${PC_CHECK_STATIC_CFLAGS})
	
	if(NOT TARGET check)
	add_library(check UNKNOWN IMPORTED)
		set_target_properties(check PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${CHECK_INCLUDE_DIRS}"
			IMPORTED_LOCATION "${CHECK_LIBRARIES}"
		)
  endif ()
endif ()

mark_as_advanced (CHECK_INCLUDE_DIR CHECK_LIBRARY)
