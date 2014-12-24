cmake_minimum_required(VERSION 2.8)

# Force external build
if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR} AND NOT WIN32)
    message(FATAL_ERROR "You can not use CMake to build from the root of it's source tree! Remove the CMakeCache.txt file from this directory, then create a separate directory (either below this directory or elsewhere), and then re-run CMake from there.")
endif(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR} AND NOT WIN32)

# Project version
SET(PROJECT_MAJOR_VERSION 1)
SET(PROJECT_MINOR_VERSION 0)
SET(PROJECT_PATCH_LEVEL 0)

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

# Set our version strings
SET(VERSION_SIMPLE "${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION}.${PROJECT_PATCH_LEVEL}")
SET(VERSION_FULL "${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION}.${PROJECT_PATCH_LEVEL}-${VERSION_GIT}")

message(STATUS "${PROJECT_NAME} version: ${VERSION_FULL}")


# Cmake module path (if exists)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

file(GLOB SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.c")
list(SORT SOURCE_FILES)

# Find clang (if the system has it)
find_program(CLANG clang)
find_program(CLANGXX clang++)

# prefer clang instead of gcc (or whatever shit compiler they're using) This can be disabled with
# the NO_CLANG option
option(NO_CLANG "Don't prefer clang for compilation" OFF)
if (NOT NO_CLANG)
	if (CLANG)
		set(CMAKE_C_COMPILER ${CLANG})
	endif (CLANG)

	# In case we need to compile C++ in the future (which I doubt we will)
	if (CLANGXX)
		set(CMAKE_CXX_COMPILER ${CLANGXX})
	endif (CLANGXX)
endif (NOT NO_CLANG)

# Finally initialize our project
project(Titanium C)

include (CheckCCompilerFlag)

# Make sure the compiler accepts C11
check_c_compiler_flag(-std=c11 HAVE_C11_FLAG)
set(CFLAGS "${CFLAGS} -g -Werror=implicit-function-declaration")

if (NOT HAVE_C11_FLAG)
	#If the compiler doesn't have the -std=c11 flag, try and use the -std=c1x but let the user
	# know that we may not be able to compile.
	check_c_compiler_flag(-std=c1x HAVE_C1X_FLAG)

	if (NOT HAVE_C1X_FLAG)
        message(FATAL_ERROR "Your compiler (${CMAKE_C_COMPILER}) MUST support C11. Try using CC=<alternative compiler>")
	else (NOT HAVE_C1X_FLAG)
		message(WARNING "Your compiler appears to support an older version of C11, the compile may fail! Please update your compiler before reporting any bugs!")
		set(CFLAGS "${CFLAGS} -std=c1x")
	endif (NOT HAVE_C1X_FLAG)
else (NOT HAVE_C11_FLAG)
	set(CFLAGS "${CFLAGS} -std=c11")
endif (NOT HAVE_C11_FLAG)

# Add some feature test macro definitions.
# _GNU_SOURCE is used because we use strndupa and alloca, remove references to alloca/strndupa and
# you can remove this feature test macro for _GNU_SOURCE
add_definitions(-D_POSIX_SOURCE=1 -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE=1)

set(CMAKE_CXX_FLAGS ${CFLAGS})

if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    # Add some freebsd-specific macros
    add_definitions(-D_BSD_SOURCE)
endif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # Add some linux-specific macros
    add_definitions(-D_GNU_SOURCE=1)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

# Check for platform-specific things we need
include (CheckTypeSize)
include (CheckIncludeFile)
include (CheckLibraryExists)
include (CheckFunctionExists)
include (CheckCSourceCompiles)

check_type_size(int8_t HAVE_INT8_T)
check_type_size(uint8_t HAVE_UINT8_T)
check_type_size(int16_t HAVE_INT16_T)
check_type_size(uint16_t HAVE_UINT16_T)
check_type_size(uint32_t HAVE_UINT32_T)
check_type_size(int64_t HAVE_INT64_T)
check_type_size(uint64_t HAVE_UINT64_T)

check_type_size(u_int8_t HAVE_U_INT8_T)
check_type_size(u_int16_t HAVE_U_INT16_T)
check_type_size(u_int32_t HAVE_U_INT32_T)
check_type_size(u_int64_t HAVE_U_INT64_T)

#check_function_exists(strndupa HAVE_STRNDUPA)
check_function_exists(strnlen HAVE_STRNLEN)
check_function_exists(kqueue HAVE_KQUEUE)
check_function_exists(select HAVE_SELECT)
check_function_exists(poll HAVE_POLL)

check_include_file(sys/epoll.h HAVE_SYS_EPOLL_H)
check_include_file(setjmp.h HAVE_SETJMP_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stddef.h HAVE_STDDEF_H)

# Because strndupa is apparnetly not a function or some shit, we must
# make sure this program compiles.
check_c_source_compiles("
#define _GNU_SOURCE 1
#include <string.h>
int main() { const char *abc = \"abc\"; char *str = strndupa(abc, 5); return 0; }
" HAVE_STRNDUPA)

# Add our include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Configuration file and include the binary directory
configure_file(${CMAKE_SOURCE_DIR}/cmake/sysconf.h.cmake ${CMAKE_BINARY_DIR}/sysconf.h)
include_directories(${CMAKE_BINARY_DIR})

# Statically link this project
SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
SET(BUILD_SHARED_LIBRARIES OFF)
SET(BUILD_SHARED_LIBS OFF)
SET(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static")

set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)       # remove -Wl,-Bdynamic
set(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS)
set(CMAKE_SHARED_LIBRARY_C_FLAGS)         # remove -fPIC
set(CMAKE_SHARED_LIBRARY_CXX_FLAGS)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)    # remove -rdynamic
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS) 

message(STATUS "LINKFLAGS: ${CMAKE_LINKER_FLAGS}")

# Finally, tell CMake how to build the project
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# statically compile the program
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_SEARCH_END_STATIC 1)
set_target_properties(${PROJECT_NAME} PROPERTIES TARGET_SUPPORTS_SHARED_LIBS FALSE)

# Set other options
set_source_files_properties(${SOURCE_FILES} PROPERTIES LANGUAGE C COMPILE_FLAGS "${CFLAGS}")
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C PREFIX "" SUFFIX "" LINK_FLAGS "${LINKFLAGS}")


# Do the make install
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(FILES ${CMAKE_SOURCE_DIR}/doc/nbstftp.conf.example DESTINATION etc RENAME nbstftp.conf)
# install(FILES ${MAN_FILE} DESTINATION man)

