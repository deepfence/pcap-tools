#
# - Find systemd libraries
#
#  SYSTEMD_INCLUDE_DIRS - where to find systemd/sd-journal.h, etc.
#  SYSTEMD_LIBRARIES    - List of libraries when using libsystemd.
#  SYSTEMD_FOUND        - True if libsystemd is found.

pkg_search_module(PC_SYSTEMD QUIET libsystemd)

find_path(SYSTEMD_INCLUDE_DIR
  NAMES
    systemd/sd-journal.h
  HINTS
    ${PC_SYSTEMD_INCLUDE_DIRS}
)

find_library(SYSTEMD_LIBRARY
  NAMES
    systemd
  HINTS
    ${PC_SYSTEMD_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SYSTEMD
  REQUIRED_VARS   SYSTEMD_LIBRARY SYSTEMD_INCLUDE_DIR
  VERSION_VAR     PC_SYSTEMD_VERSION)

if(SYSTEMD_FOUND)
  set(SYSTEMD_LIBRARIES ${SYSTEMD_LIBRARY})
  set(SYSTEMD_INCLUDE_DIRS ${SYSTEMD_INCLUDE_DIR})
else()
  set(SYSTEMD_LIBRARIES)
  set(SYSTEMD_INCLUDE_DIRS)
endif()

mark_as_advanced(SYSTEMD_LIBRARIES SYSTEMD_INCLUDE_DIRS)
