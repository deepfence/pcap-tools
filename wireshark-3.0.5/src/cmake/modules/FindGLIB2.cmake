#
# - Try to find the GLIB2 libraries
# Once done this will define
#
#  GLIB2_FOUND        - system has glib2
#  GLIB2_INCLUDE_DIRS - the glib2 include directory
#  GLIB2_LIBRARIES    - glib2 library
#  GLIB2_DLL_DIR      - (Windows) Path to required GLib2 DLLs.
#  GLIB2_DLLS         - (Windows) List of required GLib2 DLLs.

# Copyright (c) 2008 Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if( GLIB2_MAIN_INCLUDE_DIR AND GLIB2_LIBRARIES )
	# Already in cache, be silent
	set( GLIB2_FIND_QUIETLY TRUE )
endif()

include( FindWSWinLibs )
FindWSWinLibs( "glib2-*" "GLIB2_HINTS" )

if (NOT WIN32)
	find_package(PkgConfig)
	pkg_search_module( PC_GLIB2 glib-2.0 )
endif()

find_path( GLIB2_MAIN_INCLUDE_DIR
	NAMES
		glib.h
	HINTS
		"${PC_GLIB2_INCLUDEDIR}"
		"${GLIB2_HINTS}/include"
	PATH_SUFFIXES
		glib-2.0
		glib-2.0/include
	PATHS
		/opt/gnome/include
		/opt/local/include
		/sw/include
		/usr/include
		/usr/local/include
)

find_library( GLIB2_LIBRARY
	NAMES
		glib-2.0
		libglib-2.0
	HINTS
		"${PC_GLIB2_LIBDIR}"
		"${GLIB2_HINTS}/lib"
	PATHS
		/opt/gnome/lib64
		/opt/gnome/lib
		/opt/lib/
		/opt/local/lib
		/sw/lib/
		/usr/lib64
		/usr/lib
)

# search the glibconfig.h include dir under the same root where the library is found
get_filename_component( glib2LibDir "${GLIB2_LIBRARY}" PATH)

find_path( GLIB2_INTERNAL_INCLUDE_DIR
	NAMES
		glibconfig.h
	HINTS
		"${GLIB2_INCLUDEDIR}"
		"${glib2LibDir}"
		${CMAKE_SYSTEM_LIBRARY_PATH}
	PATH_SUFFIXES
		glib-2.0/include
	PATHS
		${GLIB2_LIBRARY}

)

if(PC_GLIB2_VERSION)
	set(GLIB2_VERSION ${PC_GLIB2_VERSION})
elseif(GLIB2_INTERNAL_INCLUDE_DIR)
	# On systems without pkg-config (e.g. Windows), search its header
	# (available since the initial commit of GLib).
	file(STRINGS ${GLIB2_INTERNAL_INCLUDE_DIR}/glibconfig.h GLIB_MAJOR_VERSION
		REGEX "#define[ ]+GLIB_MAJOR_VERSION[ ]+[0-9]+")
	string(REGEX MATCH "[0-9]+" GLIB_MAJOR_VERSION ${GLIB_MAJOR_VERSION})
	file(STRINGS ${GLIB2_INTERNAL_INCLUDE_DIR}/glibconfig.h GLIB_MINOR_VERSION
		REGEX "#define[ ]+GLIB_MINOR_VERSION[ ]+[0-9]+")
	string(REGEX MATCH "[0-9]+" GLIB_MINOR_VERSION ${GLIB_MINOR_VERSION})
	file(STRINGS ${GLIB2_INTERNAL_INCLUDE_DIR}/glibconfig.h GLIB_MICRO_VERSION
		REGEX "#define[ ]+GLIB_MICRO_VERSION[ ]+[0-9]+")
	string(REGEX MATCH "[0-9]+" GLIB_MICRO_VERSION ${GLIB_MICRO_VERSION})
	set(GLIB2_VERSION ${GLIB_MAJOR_VERSION}.${GLIB_MINOR_VERSION}.${GLIB_MICRO_VERSION})
else()
	set(GLIB2_VERSION "")
endif()

include( FindPackageHandleStandardArgs )
find_package_handle_standard_args( GLIB2
	REQUIRED_VARS   GLIB2_LIBRARY GLIB2_MAIN_INCLUDE_DIR GLIB2_INTERNAL_INCLUDE_DIR
	VERSION_VAR     GLIB2_VERSION
)

if( GLIB2_FOUND )
	set( GLIB2_LIBRARIES ${GLIB2_LIBRARY} )
	# Include transitive dependencies for static linking.
	if(UNIX AND CMAKE_FIND_LIBRARY_SUFFIXES STREQUAL ".a")
		find_library(PCRE_LIBRARY pcre)
		list(APPEND GLIB2_LIBRARIES -pthread ${PCRE_LIBRARY})
	endif()
	set( GLIB2_INCLUDE_DIRS ${GLIB2_MAIN_INCLUDE_DIR} ${GLIB2_INTERNAL_INCLUDE_DIR} )
	if ( WIN32 AND GLIB2_FOUND )
		set ( GLIB2_DLL_DIR "${GLIB2_HINTS}/bin"
			CACHE PATH "Path to GLib 2 DLLs"
		)
		# XXX Are GIO and GObject really necessary?
		# libglib and libgio in glib2-2.52.2-1.34-win32ws depend on
		# libgcc_s_sjlj-1.dll, now included with gnutls-3.6.3-1-win32ws.
		# (64-bit GLib does not depend on libgcc_s).
		file( GLOB _glib2_dlls RELATIVE "${GLIB2_DLL_DIR}"
			"${GLIB2_DLL_DIR}/libglib-*.dll"
			"${GLIB2_DLL_DIR}/libgio-*.dll"
			"${GLIB2_DLL_DIR}/libgmodule-*.dll"
			"${GLIB2_DLL_DIR}/libgobject-*.dll"
			"${GLIB2_DLL_DIR}/libintl-*.dll"
			#"${GLIB2_DLL_DIR}/libgcc_s_*.dll"
		)
		set ( GLIB2_DLLS ${_glib2_dlls}
			# We're storing filenames only. Should we use STRING instead?
			CACHE FILEPATH "GLib 2 DLL list"
		)
		mark_as_advanced( GLIB2_DLL_DIR GLIB2_DLLS )
	endif()
elseif( GLIB2_FIND_REQUIRED )
	message( SEND_ERROR "Package required but not found" )
else()
	set( GLIB2_LIBRARIES )
	set( GLIB2_MAIN_INCLUDE_DIRS )
	set( GLIB2_DLL_DIR )
	set( GLIB2_DLLS )
endif()

mark_as_advanced( GLIB2_INCLUDE_DIRS GLIB2_LIBRARIES )
