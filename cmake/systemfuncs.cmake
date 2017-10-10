###############################################################################
# extract_include_filename(<line> <output variable> [<optional output variable of quote type>])
#
# This macro will take a #include line and extract the filename.
###############################################################################
macro(extract_include_filename INCLUDE FILENAME)
  # Strip the leading and trailing spaces from the include line
  STRING(STRIP ${INCLUDE} INCLUDE_STRIPPED)
  # Make sure to only do the following if there is a string
  if(INCLUDE_STRIPPED STREQUAL "")
    set(FILE "")
  else(INCLUDE_STRIPPED STREQUAL "")
    # Extract the filename including the quotes or angle brackets
    string(REGEX REPLACE "^.*([\"<].*[\">]).*$" "\\1" FILE "${INCLUDE_STRIPPED}")
    # If an optional 3rd argument is given, we'll store if the quote style was quoted or angle bracketed
    if(${ARGC} GREATER 2)
      string(SUBSTRING ${FILE} 0 1 QUOTE)
      if(QUOTE STREQUAL "<")
        set(${ARGV2} "angle brackets")
      else(QUOTE STREQUAL "<")
        set(${ARGV2} "quotes")
      endif(QUOTE STREQUAL "<")
    endif(${ARGC} GREATER 2)
    # Now remove the quotes or angle brackets
    string(REGEX REPLACE "^[\"<](.*)[\">]$" "\\1" FILE "${FILE}")
  endif(INCLUDE_STRIPPED STREQUAL "")
  # Set the filename to the the given variable
  set(${FILENAME} "${FILE}")
endmacro(extract_include_filename)

###############################################################################
# find_includes(<source filename> <output variable>)
#
# This macro will search through a file for #include lines, regardless of
#   whitespace, but only returns the lines that are valid for the current
#   platform CMake is running on.
###############################################################################
macro(find_includes SRC INCLUDES)
  # Read all lines from the file that start with #, regardless of whitespace before the #
  file(STRINGS ${SRC} LINES REGEX "^[ \t]*#.*$")
  # Set that any #include lines found are valid, and create temporary variables for the last found #ifdef/#ifndef
  set(VALID_LINE TRUE)
  set(LAST_DEF)
  set(LAST_CHECK)
  # Create an empty include list
  set(INCLUDES_LIST)
  # Iterate through all the # lines
  foreach(LINE ${LINES})
    # Search for #ifdef, #ifndef, #else, #endif, and #include
    string(REGEX MATCH "^[ \t]*#[ \t]*ifdef[ \t]*.*$" FOUND_IFDEF ${LINE})
    string(REGEX MATCH "^[ \t]*#[ \t]*ifndef[ \t]*.*$" FOUND_IFNDEF ${LINE})
    string(REGEX MATCH "^[ \t]*#[ \t]*else.*$" FOUND_ELSE ${LINE})
    string(REGEX MATCH "^[ \t]*#[ \t]*endif.*$" FOUND_ENDIF ${LINE})
    string(REGEX MATCH "^[ \t]*#[ \t]*include[ \t]*[\"<].*[\">][\ t]*.*$" FOUND_INCLUDE ${LINE})
    # If we found a #ifdef on the line, extract the data after the #ifdef and set if the lines after it are valid based on the variables in CMake
    if(FOUND_IFDEF)
      # Extract the define
      string(REGEX REPLACE "^[ \t]*#[ \t]*ifdef[ \t]*(.*)$" "\\1" DEFINE ${LINE})
      # Replace _WIN32 with WIN32, so we can check if the WIN32 variable of CMake is set instead of _WIN32
      if(DEFINE STREQUAL "_WIN32")
        set(DEFINE WIN32)
      endif(DEFINE STREQUAL "_WIN32")
      # Set the last define to this one, and set the last check to true, so when #else is encountered, we can do an opposing check
      set(LAST_DEF ${DEFINE})
      set(LAST_CHECK TRUE)
      # If the define is true (it either exists or is a non-false result), the lines following will be checked, otherwise they will be skipped
      if(${DEFINE})
        set(VALID_LINE TRUE)
      else(${DEFINE})
        set(VALID_LINE FALSE)
      endif(${DEFINE})
    else(FOUND_IFDEF)
      # If we found a #ifndef on the line, the same thing as #ifdef is done, except with the checks in the opposite direction
      if(FOUND_IFNDEF)
        # Extract the define
        string(REGEX REPLACE "^[ \t]*#[ \t]*ifndef[ \t]*(.*)$" "\\1" DEFINE ${LINE})
        # Replace _WIN32 with WIN32, so we can check if the WIN32 variable of CMake is set instead of _WIN32
        if(DEFINE STREQUAL "_WIN32")
          set(DEFINE WIN32)
        endif(DEFINE STREQUAL "_WIN32")
        # Set the last define to this one, and set the last check to false, so when #else is encountered, we can do an opposing check
        set(LAST_DEF ${DEFINE})
        set(LAST_CHECK FALSE)
        # If the define is not true (it either doesn't exists or is a false result), the lines following will be checked, otherwise they will be skipped
        if(${DEFINE})
          set(VALID_LINE FALSE)
        else(${DEFINE})
          set(VALUE_LINE TRUE)
        endif(${DEFINE})
      else(FOUND_IFNDEF)
        # If we found a #else on the line, we check the last define in the opposite direction
        if(FOUND_ELSE)
          # When LAST_CHECK is true, we were inside a #ifdef, now act as if we are entering a #ifndef section by doing an opposing check
          if(LAST_CHECK)
            if(${LAST_DEF})
              set(VALID_LINE FALSE)
            else(${LAST_DEF})
              set(VALID_LINE TRUE)
            endif(${LAST_DEF})
          # When LAST_CHECK is false, we were inside a #ifndef, now act as if we are entering a #ifdef section by doing an opposing check
          else(LAST_CHECK)
            if(${LAST_DEF})
              set(VALID_LINE TRUE)
            else(${LAST_DEF})
              set(VALID_LINE FALSE)
            endif(${LAST_DEF})
          endif(LAST_CHECK)
        else(FOUND_ELSE)
          # If we found a #endif on the line, we'll assume everything following the line is valid until we meet another one of the above lines
          if(FOUND_ENDIF)
            set(VALID_LINE TRUE)
          else(FOUND_ENDIF)
            # If we found a #include on the line, add the entire line to the list of includes unless the line isn't valid
            if(FOUND_INCLUDE)
              if(VALID_LINE)
                list(APPEND INCLUDES_LIST "${LINE}")
              endif(VALID_LINE)
            endif(FOUND_INCLUDE)
          endif(FOUND_ENDIF)
        endif(FOUND_ELSE)
      endif(FOUND_IFNDEF)
    endif(FOUND_IFDEF)
  endforeach(LINE)
  set(${INCLUDES} ${INCLUDES_LIST})
endmacro(find_includes)

###############################################################################
# calculate_depends(<source filename> <output variable set to TRUE on fail> [<optional output variable for includes>])
#
# This macro is used in most of the src (sub)directories to calculate the
#   header file dependencies for the given source file.
###############################################################################
macro(calculate_depends SRC SKIP)
  # Temporarily set that we didn't get a 3nd argument before we actually check if we did get one or not
  set(CHECK_ANGLE_INCLUDES FALSE)
  # Check for a third argument
  if(${ARGC} GREATER 2)
    set(CHECK_ANGLE_INCLUDES TRUE)
  endif(${ARGC} GREATER 2)
  # Find all the lines in the given source file that have any form of #include on them, regardless of whitespace, but only if they are valid for the platform we are on
  find_includes(${SRC} INCLUDES)
  # Reset the list of headers to empty
  set(HEADERS)
  # Reset skip
  set(${SKIP} FALSE)
  # Iterate through the strings containing #include (if any)
  foreach(INCLUDE ${INCLUDES})
    # Extract the filename from the #include line
    extract_include_filename(${INCLUDE} FILENAME QUOTE_TYPE)
    if(QUOTE_TYPE STREQUAL "angle brackets")
	  # Ignore Nitrogen.h being an angle bracket include.
	  # This just makes modules easier and cleaner to write.
	  if (FILENAME STREQUAL "Nitrogen.h")
		  continue()
	  endif (FILENAME STREQUAL "Nitrogen.h")
      # The following checks will only be done if there was a request for angle includes to be checked
      if(CHECK_ANGLE_INCLUDES)
        # Find the path of the include file
        if(DEFAULT_INCLUDE_DIRS OR WSDK_PATH OR DEFINED $ENV{VCINSTALLDIR})
          find_path(FOUND_${FILENAME}_INCLUDE NAMES ${FILENAME} PATHS ${DEFAULT_INCLUDE_DIRS} ${WSDK_PATH}/include $ENV{VCINSTALLDIR}/include ${EXTRA_INCLUDE})
        else(DEFAULT_INCLUDE_DIRS OR WSDK_PATH OR DEFINED $ENV{VCINSTALLDIR})
          find_path(FOUND_${FILENAME}_INCLUDE NAMES ${FILENAME} ${EXTRA_INCLUDE})
        endif(DEFAULT_INCLUDE_DIRS OR WSDK_PATH OR DEFINED $ENV{VCINSTALLDIR})
        # If the include file was found, add it's path to the list of include paths, but only if it doesn't already exist and isn't in the defaults for the compiler
        if(FOUND_${FILENAME}_INCLUDE)
          list(FIND DEFAULT_INCLUDE_DIRS "${FOUND_${FILENAME}_INCLUDE}" FOUND_IN_DEFAULTS)
          if(FOUND_IN_DEFAULTS EQUAL -1)
            list(FIND ${ARGV2} "${FOUND_${FILENAME}_INCLUDE}" FOUND_IN_INCLUDES)
            if(FOUND_IN_INCLUDES EQUAL -1)
              list(APPEND ${ARGV2} "${FOUND_${FILENAME}_INCLUDE}")
            endif(FOUND_IN_INCLUDES EQUAL -1)
          endif(FOUND_IN_DEFAULTS EQUAL -1)
        else(FOUND_${FILENAME}_INCLUDE)
          set(${SKIP} TRUE)
          message("${SRC} needs header file \"${FILENAME}\" but we were unable to locate that header file! Check that the header file is within the search path of your OS.")
        endif(FOUND_${FILENAME}_INCLUDE)
      endif(CHECK_ANGLE_INCLUDES)
    endif(QUOTE_TYPE STREQUAL "angle brackets")
  endforeach(INCLUDE)
endmacro(calculate_depends)

###############################################################################
# calculate_libraries(<source filename> <output variable for linker flags> <output variable for extra depends>)
#
# This macro is used in most of the module (sub)directories to calculate the
#   library dependencies for the given source file.
###############################################################################
macro(calculate_libraries SRC SRC_LDFLAGS EXTRA_DEPENDS)
  # Set up a temporary LDFLAGS for this file
  set(THIS_LDFLAGS "${LDFLAGS}")
  # Reset extra dependencies
  set(EXTRA_DEPENDENCIES)
  # Reset library paths
  set(LIBRARY_PATHS)
  # Reset libraries
  set(LIBRARIES)
  # Check to see if there are any lines matching: /* RequiredLibraries: [something] */
  file(STRINGS ${SRC} REQUIRED_LIBRARIES REGEX "/\\*[ \t]*RequiredLibraries:[ \t]*.*[ \t]*\\*/")
  # Iterate through those lines
  foreach(REQUIRED_LIBRARY ${REQUIRED_LIBRARIES})
    # Strip off the /* RequiredLibraries: and */ from the line
    string(REGEX REPLACE "/\\*[ \t]*Required.*Libraries:[ \t]*([^ \t]*)[ \t]*\\*/" "\\1" REQUIRED_LIBRARY ${REQUIRED_LIBRARY})
    # Replace all commas with semicolons
    string(REGEX REPLACE "," ";" REQUIRED_LIBRARY ${REQUIRED_LIBRARY})
    # Iterate through the libraries given
    foreach(LIBRARY ${REQUIRED_LIBRARY})
      # Locate the library to see if it exists
      find_library(FOUND_${LIBRARY}_LIBRARY NAMES ${LIBRARY} PATHS ${EXTRA_INCLUDE} ${EXTRA_LIBS} NO_DEFAULT_PATH)
      find_library(FOUND_${LIBRARY}_LIBRARY NAMES ${LIBRARY} PATHS ${EXTRA_INCLUDE} ${EXTRA_LIBS})
      # If the library was found, we will add it to the linker flags
      if(FOUND_${LIBRARY}_LIBRARY)
        # Get the path only of the library, to add it to linker flags
        get_filename_component(LIBRARY_PATH ${FOUND_${LIBRARY}_LIBRARY} PATH)
          # For all others, add the library paths and libraries
          list(APPEND LIBRARY_PATHS "${LIBRARY_PATH}")
          list(APPEND LIBRARIES "${LIBRARY}")
          message(STATUS "Found ${REQUIRED_LIBRARY}: ${LIBRARY}")
      else(FOUND_${LIBRARY}_LIBRARY)
        # In the case of the library not being found, we fatally error so CMake stops trying to generate
        message(FATAL_ERROR "${SRC} needs library ${LIBRARY} but we were unable to locate that library! Check that the library is within the search path of your OS.")
      endif(FOUND_${LIBRARY}_LIBRARY)
    endforeach(LIBRARY)
  endforeach(REQUIRED_LIBRARY)
  # Remove duplicates from the library paths
  if(LIBRARY_PATHS)
    list(REMOVE_DUPLICATES LIBRARY_PATHS)
  endif(LIBRARY_PATHS)
  # Remove diplicates from the libraries
  if(LIBRARIES)
    list(REMOVE_DUPLICATES LIBRARIES)
  endif(LIBRARIES)
  # Iterate through library paths and add them to the linker flags
  foreach(LIBRARY_PATH ${LIBRARY_PATHS})
    list(FIND DEFAULT_LIBRARY_DIRS "${LIBRARY_PATH}" FOUND_IN_DEFAULTS)
    if(FOUND_IN_DEFAULTS EQUAL -1)
      set(THIS_LDFLAGS "${THIS_LDFLAGS} -L${LIBRARY_PATH}")
    endif(FOUND_IN_DEFAULTS EQUAL -1)
  endforeach(LIBRARY_PATH)
  # Iterate through libraries and add them to the linker flags
  foreach(LIBRARY ${LIBRARIES})
    list(APPEND EXTRA_DEPENDENCIES "${LIBRARY}")
  endforeach(LIBRARY)
  set(${SRC_LDFLAGS} "${THIS_LDFLAGS}")
  set(${EXTRA_DEPENDS} "${EXTRA_DEPENDENCIES}")
endmacro(calculate_libraries)

###############################################################################
# check_functions(<source filename> <output variable set to TRUE on success>)
#
# This macro is used in most of the module (sub)directories to calculate the
#   fcuntion dependencies for the given source file.
###############################################################################
macro(check_functions SRC SUCCESS)
  # Default to true
  set(${SUCCESS} TRUE)
  # Check to see if there are any lines matching: /* RequiredFunctions: [something] */
  file(STRINGS ${SRC} REQUIRED_FUNCTIONS REGEX "/\\*[ \t]*RequiredFunctions:[ \t]*.*[ \t]*\\*/")
  # Iterate through those lines
  foreach(REQUIRED_FUNCTION ${REQUIRED_FUNCTIONS})
    # Strip off the /* RequiredFunctions: and */ from the line
    string(REGEX REPLACE "/\\*[ \t]*RequiredFunctions:[ \t]*([^ \t]*)[ \t]*\\*/" "\\1" REQUIRED_FUNCTION ${REQUIRED_FUNCTION})
    # Replace all commas with semicolons
    string(REGEX REPLACE "," ";" REQUIRED_FUNCTION ${REQUIRED_FUNCTION})
    # Iterate through the functions given
    foreach(FUNCTION ${REQUIRED_FUNCTION})
      # Check if the function exists
      check_function_exists(${REQUIRED_FUNCTION} HAVE_${REQUIRED_FUNCTION})
      # If we don't have the function warn the user and set SUCCESS to FALSE
      if(NOT HAVE_${REQUIRED_FUNCTION})
        message("${SRC} needs function ${REQUIRED_FUNCTION} but we were unable to locate that function!")
        set(${SUCCESS} FALSE)
      endif(NOT HAVE_${REQUIRED_FUNCTION})
    endforeach(FUNCTION)
  endforeach(REQUIRED_FUNCTION)
endmacro(check_functions)

###############################################################################
# compile_tmpl(<source filename> <output variable of filename>)
#
# This macro is used in most of the module (sub)directories to calculate the
#   fcuntion dependencies for the given source file.
###############################################################################

macro(compile_tmpl SRC TMPL)
  string(REGEX REPLACE "\\.tmpl$" ".cpp" TMPL_CPP ${SRC})
  message(${SRC})
  add_custom_command(
    OUTPUT ${TMPL_CPP}
    COMMAND cppcms_tmpl_cc ${SRC} -o ${TMPL_CPP} ${SRC}
    DEPENDS ${SRC})
  set(${TMPL} ${TMPL_CPP})
endmacro(compile_tmpl)

# A macro to recursively get source files, however if the source folder has
# a CMakeLists.txt file, include it as part of the build instead.
macro(GetSources SRC CPPSOURCES)
	if(NOT ${SRC} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR} AND EXISTS "${SRC}/CMakeLists.txt")
		# If we have a CMakeLists.txt file, include it instead of trying to compile
		# sources as part of main project.
		add_subdirectory("${SRC}")
        # Allow the child CMakeLists.txt to add sources to our list.
        list(APPEND ${CPPSOURCES} ${CHILD_SCOPE_SOURCES})
	else(NOT ${SRC} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR} AND EXISTS "${SRC}/CMakeLists.txt")
		# get a list of all files in the directory.
		file(GLOB SRC_FLDR "${SRC}/*")
		foreach(SOURCEFILE ${SRC_FLDR})
			# If it's a directory, recursively call this function.
			if (IS_DIRECTORY "${SOURCEFILE}")
				GetSources(${SOURCEFILE} ${CPPSOURCES})
			else (IS_DIRECTORY "${SOURCEFILE}")
				# Otherwise look for source files and append them.
				# Look for C++ files.
				string(REGEX MATCH "\\.cpp$" CPP ${SOURCEFILE})
				if (CPP)
					list(APPEND ${CPPSOURCES} ${SOURCEFILE})
				endif (CPP)

				# Look for C files.
				string(REGEX MATCH "\\.c$" C ${SOURCEFILE})
				if (C)
					list(APPEND ${CPPSOURCES} ${SOURCEFILE})
				endif (C)
			endif (IS_DIRECTORY "${SOURCEFILE}")
		endforeach(SOURCEFILE ${SRC_FLDR})
	endif(NOT ${SRC} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR} AND EXISTS "${SRC}/CMakeLists.txt")
endmacro(GetSources)

function(GetGitRevision)
# Get the git revision location for the branch we're on
    if(EXISTS "${CMAKE_SOURCE_DIR}/.git/HEAD")
            file(READ ${CMAKE_SOURCE_DIR}/.git/HEAD GIT_HEAD_LOC)
            string(LENGTH ${GIT_HEAD_LOC} HEAD_LEN)
            math(EXPR LEN "${HEAD_LEN} - 5")
            string(SUBSTRING ${GIT_HEAD_LOC} 5 ${LEN} GIT_HEAD)
            # Weird nastery to remove newlines which screw up the if statement below.
            set(GIT_SHA_PATH "${CMAKE_SOURCE_DIR}/.git/${GIT_HEAD}")
            string(REGEX REPLACE "(\r?\n)+$" "" GIT_SHA_PATH "${GIT_SHA_PATH}")
    endif(EXISTS "${CMAKE_SOURCE_DIR}/.git/HEAD")

# Get the git revision we're on for the version string
    if(EXISTS "${GIT_SHA_PATH}")
            file(READ "${GIT_SHA_PATH}" VERSION_STR)
            string(REGEX REPLACE "(\r?\n)+$" "" VERSION_STR "${VERSION_STR}")
            # Get the length of the string
            string(LENGTH ${VERSION_STR} VERSION_LEN)
            # Subtract 7 from the string's length
            math(EXPR VERSION_NUM_LEN "${VERSION_LEN} - ${VERSION_LEN} + 7")
            # Extract the value from the string
            string(SUBSTRING ${VERSION_STR} 0 ${VERSION_NUM_LEN} VERSION_GIT)
    endif(EXISTS "${GIT_SHA_PATH}")

    # Set our variables
    set(GIT_REVISION_LONG ${VERSION_STR} PARENT_SCOPE)
    set(GIT_REVISION_SHORT ${VERSION_GIT} PARENT_SCOPE)
endfunction(GetGitRevision)
