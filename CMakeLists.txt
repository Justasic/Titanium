cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

# Force external build
if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR} AND NOT WIN32)
    message(FATAL_ERROR "You can not use CMake to build from the root of it's source tree! Remove the CMakeCache.txt file from this directory, then create a separate directory (either below this directory or elsewhere), and then re-run CMake from there.")
endif(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR} AND NOT WIN32)

# Project version
SET(PROJECT_MAJOR_VERSION 1)
SET(PROJECT_MINOR_VERSION 0)
SET(PROJECT_PATCH_LEVEL 0)

# Cmake module path (if exists)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
# include our function checks
include(systemfuncs)

# Set our version strings
GetGitRevision()
SET(VERSION_SIMPLE "${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION}.${PROJECT_PATCH_LEVEL}")
SET(VERSION_FULL "${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION}.${PROJECT_PATCH_LEVEL}-${GIT_REVISION_SHORT}")

# Find clang (if the system has it)
find_program(CLANG clang)
find_program(CLANGXX clang++)

# Set the default build type to "Release" if not already set.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

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
project(Titanium CXX)
enable_language(C)

# Check for platform-specific things we need
include (CheckTypeSize)
include (CheckIncludeFile)
include (CheckLibraryExists)
include (CheckFunctionExists)
include (CheckCXXSourceCompiles)
include (CheckCXXCompilerFlag)
include (CheckCCompilerFlag)

message(STATUS "Building \"${CMAKE_BUILD_TYPE}\" version of ${PROJECT_NAME}-${VERSION_FULL}")

# Make sure the compiler accepts C11
#check_cxx_compiler_flag(-std=c++11 HAVE_C11_FLAG)
list(APPEND CFLAGS -Werror=implicit-function-declaration -Wall -Wextra -Wno-unused-parameter)
if (${CMAKE_BUILD_TYPE} MATCHES "Release")
    set(LINKFLAGS "-s")
endif (${CMAKE_BUILD_TYPE} MATCHES "Release")

if (NOT NO_CLANG)
	# Because of C++17 being somewhat incomplete for clang/g++ we
	# have to make clang use libc++ instead of glibc++
	find_library(LIBCPP libc++ REQUIRED)
    LIST(APPEND CFLAGS -stdlib=libc++)
    set(LINKFLAGS "${LINKFLAGS} -stdlib=libc++")
endif (NOT NO_CLANG)

# Add our compile definitions.
# XXX: This is a global definition change!
add_compile_options(${CFLAGS})

# Add some feature test macro definitions.
# _GNU_SOURCE is used because we use strndupa and alloca, remove references to alloca/strndupa and
# you can remove this feature test macro for _GNU_SOURCE
#add_definitions(-D_POSIX_SOURCE=1 -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE=1)

if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    # Add some freebsd-specific macros
    add_definitions(-D_BSD_SOURCE)
endif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")

check_type_size(int8_t HAVE_INT8_T)
check_type_size(uint8_t HAVE_UINT8_T)
check_type_size(int16_t HAVE_INT16_T)
check_type_size(uint16_t HAVE_UINT16_T)
check_type_size(uint32_t HAVE_UINT32_T)
check_type_size(int64_t HAVE_INT64_T)
check_type_size(uint64_t HAVE_UINT64_T)
check_type_size("long long" HAVE_LONG_LONG)
check_type_size("unsigned long long" HAVE_UNSIGNED_LONG_LONG)
check_type_size(size_t HAVE_SIZE_T)
check_type_size(time_t HAVE_TIME_T)
check_type_size("long double" HAVE_LONG_DOUBLE)

check_type_size(u_int8_t HAVE_U_INT8_T)
check_type_size(u_int16_t HAVE_U_INT16_T)
check_type_size(u_int32_t HAVE_U_INT32_T)
check_type_size(u_int64_t HAVE_U_INT64_T)

check_function_exists(strnlen HAVE_STRNLEN)
check_function_exists(kqueue HAVE_KQUEUE)
check_function_exists(select HAVE_SELECT)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(poll HAVE_POLL)
check_function_exists(wcsftime HAVE_WCSFTIME)
check_function_exists(stat HAVE_STAT)
check_function_exists(realpath HAVE_REALPATH)
check_function_exists(readlink HAVE_READLINK)
check_function_exists(dirname HAVE_DIRNAME)
check_function_exists(basename HAVE_BASENAME)
check_function_exists(dlopen HAVE_DLOPEN)
check_function_exists(dlclose HAVE_DLCLOSE)
check_function_exists(dlsym HAVE_DLSYM)
check_function_exists(dlerror HAVE_DLERROR)
check_function_exists(signal HAVE_SIGNAL)
check_function_exists(uname HAVE_UNAME)
check_function_exists(backtrace HAVE_BACKTRACE)
check_function_exists(backtrace_symbols HAVE_BACKTRACE_SYMBOLS)
check_function_exists(setjmp HAVE_SETJMP)
check_function_exists(longjmp HAVE_LONGJMP)
check_function_exists(inet_ntop HAVE_INET_NTOP)
check_function_exists(inet_pton HAVE_INET_PTON)
check_function_exists(ntohs HAVE_NTOHS)
check_function_exists(ntohl HAVE_NTOHL)
check_function_exists(bind HAVE_BIND)
check_function_exists(accept HAVE_ACCEPT)
check_function_exists(connect HAVE_CONNECT)
check_function_exists(close HAVE_CLOSE)
check_function_exists(listen HAVE_LISTEN)
check_function_exists(send HAVE_SEND)
check_function_exists(recv HAVE_RECV)
check_function_exists(getsockopt HAVE_GETSOCKOPT)
check_function_exists(localtime_r HAVE_LOCALTIME_R)
check_function_exists(wcstombs HAVE_WCSTOMBS)
check_function_exists(wcslen HAVE_WCSLEN)

check_include_file(sys/epoll.h HAVE_SYS_EPOLL_H)
check_include_file(setjmp.h HAVE_SETJMP_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(linux/limits.h HAVE_LINUX_LIMITS_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stddef.h HAVE_STDDEF_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(dlfcn.h HAVE_DLFCN_H)
check_include_file(signal.h HAVE_SIGNAL_H)
check_include_file(sys/utsname.h HAVE_SYS_UTSNAME_H)
check_include_file(execinfo.h HAVE_EXECINFO_H)
check_include_file(arpa/inet.h HAVE_ARPA_INET_H)

# Because strndupa is apparnetly not a function or some shit, we must
# make sure this program compiles.
check_cxx_source_compiles("
#define _GNU_SOURCE 1
#include <string.h>
int main() { const char *abc = \"abc\"; char *str = strndupa(abc, 5); return 0; }
" HAVE_STRNDUPA)

# Get more files
GetSources("${CMAKE_CURRENT_SOURCE_DIR}/src" SOURCE_FILES)
list(SORT SOURCE_FILES)

# Download JSON.hpp
if (NOT EXISTS ${CMAKE_BINARY_DIR}/external/json/json.hpp)
	message(STATUS "Downloading nlohmann's json.hpp library...")
	file(DOWNLOAD https://raw.githubusercontent.com/nlohmann/json/develop/src/json.hpp ${CMAKE_BINARY_DIR}/external/json/json.hpp SHOW_PROGRESS)
endif (NOT EXISTS ${CMAKE_BINARY_DIR}/external/json/json.hpp)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)
find_package(MySQL REQUIRED)
find_package(FastCGI REQUIRED)

# Check flex version to prevent yylex_destroy link errors.
if(${FLEX_VERSION} VERSION_LESS "2.5.9")
        message(FATAL_ERROR "Your flex version is ${FLEX_VERSION}, You must have flex version 2.5.9 or higher!")
endif(${FLEX_VERSION} VERSION_LESS "2.5.9")

# Generate our flex and bison targets.
FLEX_TARGET(LEXER ${CMAKE_SOURCE_DIR}/src/scanner.l ${CMAKE_BINARY_DIR}/lexer.cpp)
BISON_TARGET(PARSER ${CMAKE_SOURCE_DIR}/src/parser.yy ${CMAKE_BINARY_DIR}/parser.cpp)
ADD_FLEX_BISON_DEPENDENCY(LEXER PARSER)

# Make sure if the platform we're on requires libdl that we use it.
find_library(LIBDL dl)
# Platform-agnostically find threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Add our include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/external/json/
    ${MYSQL_INCLUDE_DIR}
    ${FastCGI_INCLUDE_DIR}
    ${CMAKE_BINARY_DIR}
)

# Configuration file and include the binary directory
configure_file(${CMAKE_SOURCE_DIR}/cmake/sysconf.h.cmake ${CMAKE_BINARY_DIR}/sysconf.h)

# Finally, tell CMake how to build the project
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${CSOURCE_FILES} ${FLEX_LEXER_OUTPUTS} ${BISON_PARSER_OUTPUTS})
set_target_properties(${PROJECT_NAME}
	PROPERTIES
		LINKER_LANGUAGE CXX
		PREFIX ""
		SUFFIX ""
		LINK_FLAGS "${LINKFLAGS}"
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
)

# We use pretty much all of C++11, a lot of C++14, and quite a bit of C++17
# so try and require as much as we can so we can have C++17 support.
target_compile_features(${PROJECT_NAME}
    PRIVATE
        cxx_thread_local
        cxx_user_literals
        cxx_variadic_macros
        cxx_variadic_templates
        cxx_rvalue_references
        cxx_right_angle_brackets
        cxx_range_for
        cxx_noexcept
        cxx_long_long_type
        cxx_lambdas
        cxx_deleted_functions
        cxx_delegating_constructors
        cxx_auto_type
        cxx_decltype
        cxx_constexpr
    PUBLIC
        cxx_nullptr
)

if (LIBDL)
	target_link_libraries(${PROJECT_NAME} dl)
endif (LIBDL)
target_link_libraries(${PROJECT_NAME} ${MYSQL_LIBRARIES} ${FastCGI_LIBRARIES} Threads::Threads)
# add_dependencies(${PROJECT_NAME} license_headers)

# Do the make install
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(FILES ${CMAKE_SOURCE_DIR}/doc/titanium.conf.example DESTINATION etc RENAME titanium.conf)
# install(FILES ${MAN_FILE} DESTINATION man)
