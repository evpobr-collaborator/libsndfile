# - Find FLAC
# Find the native FLAC includes and libraries
#
#  FLAC_INCLUDE_DIRS - where to find FLAC headers.
#  FLAC_LIBRARIES    - List of libraries when using libFLAC.
#  FLAC_FOUND        - True if libFLAC found.

if (FLAC_INCLUDE_DIR)
    # Already in cache, be silent
    set (FLAC_FIND_QUIETLY TRUE)
endif (FLAC_INCLUDE_DIR)

find_package (Ogg)

if (Ogg_FOUND)
	find_package (PkgConfig QUIET)
	pkg_check_modules(PC_FLAC QUIET flac)

	find_path (FLAC_INCLUDE_DIR FLAC/stream_decoder.h
		HINTS ${PC_FLAC_INCLUDEDIR} ${PC_FLAC_INCLUDE_DIRS} ${FLAC_ROOT}
		PATH_SUFFIXES include)

	# MSVC built libraries can name them *_static, which is good as it
	# distinguishes import libraries from static libraries with the same extension.
	find_library (FLAC_LIBRARY NAMES FLAC libFLAC libFLAC_dynamic libFLAC_static
		HINTS ${PC_FLAC_LIBDIR} ${PC_FLAC_LIBRARY_DIRS} ${FLAC_ROOT}
		PATH_SUFFIXES lib)

	# Handle the QUIETLY and REQUIRED arguments and set FLAC_FOUND to TRUE if
	# all listed variables are TRUE.
	include (FindPackageHandleStandardArgs)
	find_package_handle_standard_args (FLAC DEFAULT_MSG
		FLAC_INCLUDE_DIR FLAC_LIBRARY)
endif (Ogg_FOUND)

if (FLAC_FOUND)
	set (FLAC_INCLUDE_DIRS ${FLAC_INCLUDE_DIR} ${OGG_INCLUDE_DIRS})
	set (FLAC_LIBRARIES ${FLAC_LIBRARY} ${OGG_LIBRARIES})
	if (WIN32)
		find_library (WSOCK32_LIBRARY NAMES wsock32)
		set (FLAC_LIBRARIES ${FLAC_LIBRARIES} ${WSOCK32_LIBRARY})
	endif (WIN32)
endif (FLAC_FOUND)

mark_as_advanced(FLAC_INCLUDE_DIR FLAC_LIBRARY)
