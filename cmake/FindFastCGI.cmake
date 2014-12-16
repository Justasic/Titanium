# - Find libfcgi-dev
#
# -*- cmake -*-
#
# Find the native FastCGI includes and library
#
#  FastCGI_INCLUDE_DIR - where to find fcgiapp.h, etc.
#  FastCGI_LIBRARIES   - List of libraries when using libfcgi-dev.
#  FastCGI_FOUND       - True if FastCGI found.

IF (FastCGI_INCLUDE_DIR AND FastCGI_LIBRARIES)
  # Already in cache, be silent
  SET(FastCGI_FIND_QUIETLY TRUE)
ENDIF (FastCGI_INCLUDE_DIR AND FastCGI_LIBRARIES)

# Include dir
SET(FastCGI_INCLUDE_FILES fastcgi.h fcgi_config.h fcgi_stdio.h fcgiapp.h fcgimisc.h fcgio.h fcgios.h)
FIND_PATH(FastCGI_INCLUDE_DIR
  NAMES ${FastCGI_INCLUDE_FILES}
)

# Library, because FIND_LIBARAY doesnt work for multiple libs and we still need FastCGI_LIBRARY to be undefined,
# we do this shitty work around. - Justasic
FIND_LIBRARY(FastCGI_fcgi_LIB
  NAMES fcgi
  PATHS /usr/lib /usr/local/lib /usr/lib64/
)
FIND_LIBRARY(FastCGI_fcgipp_LIB
  NAMES fcgi++
  PATHS /usr/lib /usr/local/lib /usr/lib64/
)

if (FastCGI_fcgipp_LIB AND FastCGI_fcgi_LIB)
  SET(FastCGI_LIBRARY ${FastCGI_fcgipp_LIB} ${FastCGI_fcgi_LIB})
  message(STATUS "Found FastCGI Libraries: ${FastCGI_LIBRARY}")
endif(FastCGI_fcgipp_LIB AND FastCGI_fcgi_LIB)


IF (FastCGI_INCLUDE_DIR AND FastCGI_LIBRARY)
  SET(FastCGI_FOUND TRUE)
  SET( FastCGI_LIBRARIES ${FastCGI_LIBRARY} )
ELSE (FastCGI_INCLUDE_DIR AND FastCGI_LIBRARY)
  SET(FastCGI_FOUND FALSE)
  SET( FastCGI_LIBRARIES )
ENDIF (FastCGI_INCLUDE_DIR AND FastCGI_LIBRARY)


# handle the QUIETLY and REQUIRED arguments and set FastCGI_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FastCGI DEFAULT_MSG FastCGI_LIBRARY FastCGI_INCLUDE_DIR)

IF(FastCGI_FOUND)
  SET( FastCGI_LIBRARIES ${FastCGI_LIBRARY} )
ELSE(FastCGI_FOUND)
  SET( FastCGI_LIBRARIES )
ENDIF(FastCGI_FOUND)

MARK_AS_ADVANCED(
  FastCGI_LIBRARY
  FastCGI_INCLUDE_DIR
  )

