#
# Defines fc library target.

CMAKE_MINIMUM_REQUIRED( VERSION 3.2 FATAL_ERROR )
PROJECT( fc )

set( CMAKE_CXX_STANDARD 14 )
SET( CMAKE_CXX_STANDARD_REQUIRED ON )

if( "${CMAKE_CXX_COMPILER_ID}" MATCHES "^(Apple)?Clang|GNU$" )
  set( CMAKE_CXX_EXTENSIONS ON ) # for __int128 support
else( "${CMAKE_CXX_COMPILER_ID}" MATCHES "^(Apple)?Clang|GNU$" )
  set( CMAKE_CXX_EXTENSIONS OFF )
endif( "${CMAKE_CXX_COMPILER_ID}" MATCHES "^(Apple)?Clang|GNU$" )

MESSAGE(STATUS "Configuring project fc located in: ${CMAKE_CURRENT_SOURCE_DIR}")
SET( CMAKE_AUTOMOC OFF )

# Setup module path to make visible used CMake extensions
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/GitVersionGen")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

INCLUDE(GetPrerequisites)
INCLUDE(GetGitRevisionDescription)
INCLUDE(CheckLibraryExists)

get_git_head_revision(GIT_REFSPEC FC_GIT_REVISION_SHA)
get_git_unix_timestamp(FC_GIT_REVISION_UNIX_TIMESTAMP)

SET( DEFAULT_HEADER_INSTALL_DIR include/\${target} )
SET( DEFAULT_LIBRARY_INSTALL_DIR lib/ )
SET( DEFAULT_EXECUTABLE_INSTALL_DIR bin/ )
SET( CMAKE_DEBUG_POSTFIX _debug )
SET( BUILD_SHARED_LIBS NO )

set(platformBitness 32)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(platformBitness 64)
endif()

SET(BOOST_COMPONENTS)
LIST(APPEND BOOST_COMPONENTS coroutine thread date_time filesystem system program_options chrono unit_test_framework context iostreams regex)
# boost::endian is also required, but FindBoost can't handle header-only libs
SET( Boost_USE_STATIC_LIBS ON CACHE STRING "ON or OFF" )

# Configure secp256k1-zkp
if ( WIN32 )
    # autoconf won't work here, hard code the defines
    set( SECP256K1_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/secp256k1-zkp" )

    file( GLOB SECP256K1_SOURCES "${SECP256K1_DIR}/src/secp256k1.c" )
    add_library( secp256k1 ${SECP256K1_SOURCES} )

    target_include_directories( secp256k1 PRIVATE "${SECP256K1_DIR}" PUBLIC "${SECP256K1_DIR}/include" )

    set( SECP256K1_BUILD_DEFINES
        USE_FIELD_10X26
        USE_FIELD_INV_BUILTIN
        USE_NUM_NONE
        USE_SCALAR_8X32
        USE_SCALAR_INV_BUILTIN )
    set_target_properties( secp256k1 PROPERTIES COMPILE_DEFINITIONS "${SECP256K1_BUILD_DEFINES}" LINKER_LANGUAGE C )
else ( WIN32 )
    include(ExternalProject)

    ExternalProject_Add( project_secp256k1
     PREFIX ${CMAKE_CURRENT_BINARY_DIR}/vendor/secp256k1-zkp
     SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/secp256k1-zkp
     CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/vendor/secp256k1-zkp/configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/vendor/secp256k1-zkp --with-bignum=no
     BUILD_COMMAND make
     INSTALL_COMMAND true
     BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/vendor/secp256k1-zkp/src/project_secp256k1-build/.libs/libsecp256k1.a
    )

    ExternalProject_Add_Step(project_secp256k1 autogen
     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/vendor/secp256k1-zkp
     COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/vendor/secp256k1-zkp/autogen.sh
     DEPENDERS configure
    )

    ExternalProject_Get_Property(project_secp256k1 binary_dir)

    add_library(secp256k1 STATIC IMPORTED)
    set_property(TARGET secp256k1 PROPERTY IMPORTED_LOCATION ${binary_dir}/.libs/libsecp256k1${CMAKE_STATIC_LIBRARY_SUFFIX})
    set_property(TARGET secp256k1 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/vendor/secp256k1-zkp/include)
    add_dependencies(secp256k1 project_secp256k1)
    install( FILES ${binary_dir}/.libs/libsecp256k1${CMAKE_STATIC_LIBRARY_SUFFIX} DESTINATION lib/cryptonomex )
endif ( WIN32 )
# End configure secp256k1-zkp

# Configure editline
if ( NOT WIN32 )
    include(ExternalProject)

    ExternalProject_Add( project_editline
     PREFIX ${CMAKE_CURRENT_BINARY_DIR}/vendor/editline
     SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/editline
     CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/vendor/editline/configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/vendor/editline
     BUILD_COMMAND make
     INSTALL_COMMAND true
     BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/vendor/editline/src/project_editline-build/src/.libs/libeditline.a
    )

    ExternalProject_Add_Step(project_editline autogen
     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/vendor/editline
     COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/vendor/editline/autogen.sh
     DEPENDERS configure
    )
    ExternalProject_Get_Property(project_editline binary_dir)
    add_library(editline STATIC IMPORTED)
    set_property(TARGET editline PROPERTY IMPORTED_LOCATION ${binary_dir}/src/.libs/libeditline${CMAKE_STATIC_LIBRARY_SUFFIX})
    set_property(TARGET editline PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/vendor/editline/include)
    add_dependencies(editline project_editline)
    install( FILES ${binary_dir}/src/.libs/libeditline${CMAKE_STATIC_LIBRARY_SUFFIX} DESTINATION lib/cryptonomex )
endif ( NOT WIN32 )
# End configure editline

IF( WIN32 )
  MESSAGE(STATUS "Configuring fc to build on Win32")

  set( RPCRT4 rpcrt4 )

  #boost
  if ($ENV{BOOST_ROOT})
    SET(BOOST_ROOT $ENV{BOOST_ROOT})
  endif()

  set(Boost_USE_DEBUG_PYTHON ON)
  set(Boost_USE_MULTITHREADED ON)
  set(BOOST_ALL_DYN_LINK OFF) # force dynamic linking for all libraries

  LIST(APPEND PLATFORM_SPECIFIC_LIBS ws2_32 crypt32 mswsock userenv)

  # iphlpapi.lib
ENDIF(WIN32)

set(Boost_DIR "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/Boost")
FIND_PACKAGE(Boost CONFIG COMPONENTS ${BOOST_COMPONENTS} )

IF(NOT WIN32)
  MESSAGE(STATUS "Configuring fc to build on Unix/Apple")

  IF(NOT APPLE AND NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "OpenBSD")
    SET(rt_library rt )
  ENDIF(NOT APPLE AND NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "OpenBSD")
ENDIF(NOT WIN32)

IF($ENV{OPENSSL_ROOT_DIR})
  set(OPENSSL_ROOT_DIR $ENV{OPENSSL_ROOT_DIR} )
  set(OPENSSL_INCLUDE_DIR ${OPENSSL_ROOT_DIR}/include)
  message(STATUS "Setting up OpenSSL root and include vars to ${OPENSSL_ROOT_DIR}, ${OPENSSL_INCLUDE_DIR}")
ENDIF()

IF( LIBCXX_HAVE_CXX_ATOMICS_WITH_LIB )
    LIST( APPEND PLATFORM_SPECIFIC_LIBS atomic )
ENDIF( LIBCXX_HAVE_CXX_ATOMICS_WITH_LIB )

find_package(OpenSSL REQUIRED)

set( fc_sources
     src/popcount.cpp
     src/variant.cpp
     src/exception.cpp
     src/variant_object.cpp
     src/static_variant.cpp
     src/thread/thread.cpp
     src/thread/thread_specific.cpp
     src/thread/future.cpp
     src/thread/task.cpp
     src/thread/spin_lock.cpp
     src/thread/spin_yield_lock.cpp
     src/thread/mutex.cpp
     src/thread/parallel.cpp
     src/thread/non_preemptable_scope_check.cpp
     src/asio.cpp
     src/string.cpp
     src/stacktrace.cpp
     src/time.cpp
     src/utf8.cpp
     src/io/iostream.cpp
     src/io/datastream.cpp
     src/io/buffered_iostream.cpp
     src/io/fstream.cpp
     src/io/sstream.cpp
     src/io/json.cpp
     src/io/varint.cpp
     src/filesystem.cpp
     src/interprocess/signals.cpp
     src/interprocess/file_mapping.cpp
     src/rpc/cli.cpp
     src/rpc/state.cpp
     src/rpc/websocket_api.cpp
     src/log/log_message.cpp
     src/log/logger.cpp
     src/log/appender.cpp
     src/log/console_appender.cpp
     src/log/file_appender.cpp
     src/log/gelf_appender.cpp
     src/log/logger_config.cpp
     src/crypto/_digest_common.cpp
     src/crypto/openssl.cpp
     src/crypto/aes.cpp
     src/crypto/crc.cpp
     src/crypto/city.cpp
     src/crypto/base58.cpp
     src/crypto/base64.cpp
     src/crypto/bigint.cpp
     src/crypto/hex.cpp
     src/crypto/sha1.cpp
     src/crypto/ripemd160.cpp
     src/crypto/hash160.cpp
     src/crypto/sha256.cpp
     src/crypto/sha224.cpp
     src/crypto/sha512.cpp
     src/crypto/dh.cpp
     src/crypto/elliptic_common.cpp
     src/crypto/elliptic_impl_priv.cpp
     src/crypto/elliptic_secp256k1.cpp
     src/crypto/rand.cpp
     src/network/tcp_socket.cpp
     src/network/udp_socket.cpp
     src/network/http/http_connection.cpp
     src/network/http/websocket.cpp
     src/network/ip.cpp
     src/network/rate_limiting.cpp
     src/network/resolve.cpp
     src/network/url.cpp
     src/compress/zlib.cpp
     )

file( GLOB_RECURSE fc_headers ${CMAKE_CURRENT_SOURCE_DIR} *.hpp *.h )

set( sources
  ${fc_sources}
)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/git_revision.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp" @ONLY)
list(APPEND sources "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp")
list(APPEND sources ${fc_headers})

add_subdirectory( vendor/websocketpp EXCLUDE_FROM_ALL )

add_library( fc ${sources} )
install( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" DESTINATION include )

# begin editline stuff
if(WIN32)
  target_compile_definitions( fc PRIVATE _CRT_NONSTDC_NO_DEPRECATE )
else(WIN32)
  target_compile_definitions( fc PRIVATE HAVE_EDITLINE )
  set( editline_libraries editline )
endif(WIN32)
# end editline stuff

IF(WIN32)
  target_compile_definitions(fc PUBLIC WIN32 NOMINMAX _WIN32_WINNT=0x0501 _CRT_SECURE_NO_WARNINGS
    _SCL_SERCURE_NO_WARNINGS
    # Needed to disable MSVC autolinking feature (#pragma comment)
    BOOST_ALL_NO_LIB
    # The current version of websockets doesn't correctly guess what 'chrono' implementation boost::asio uses
    # on the recommended build platform of VC++12/boost_1.58.  Force it here until websocket gets their
    # autodetecting code to do the right thing.
    _WEBSOCKETPP_CPP11_CHRONO_
  )
  if( MSVC )
    # Activate C++ exception handling, assume extern C calls don't throw
    # Add /U options to be sure settings specific to dynamic boost link are ineffective
    target_compile_options(fc PUBLIC /EHsc /UBOOST_ALL_DYN_LINK /UBOOST_LINKING_PYTHON /UBOOST_DEBUG_PYTHON)
  elseif( MINGW )
     # Put MinGW specific compiler settings here
  endif()
ELSE()
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall")

  IF(APPLE)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -Wall")
  ELSE()
    if( NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
      target_compile_options(fc PUBLIC -Wall -fnon-call-exceptions)
    endif()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fnon-call-exceptions")

    if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
        if( CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.0.0 OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.0.0 )
            set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-partial-specialization" )
        endif()
    endif()
  ENDIF()
ENDIF()

IF(APPLE)
  # As of 10.10 yosemite, the OpenSSL static libraries shipped with os x have a dependency
  # on zlib, so any time you link in openssl you also need to link zlib.  .  We really want to detect whether openssl was configured with the --no-zlib
  # option or not when it was built, but that's difficult to do in practice, so we
  # just always try to link it in on mac.
  find_package( ZLIB REQUIRED )
ELSE(APPLE)
  find_package( ZLIB )
ENDIF(APPLE)

if( ZLIB_FOUND )
  MESSAGE( STATUS "zlib found" )
  target_include_directories(fc PUBLIC ${ZLIB_INCLUDE_DIRS})
  add_definitions( -DHAS_ZLIB )
else()
  MESSAGE( STATUS "zlib not found" )
  set( ZLIB_LIBRARIES "" )
endif( ZLIB_FOUND )

# This will become unnecessary once we update to websocketpp which fixes upstream issue #395
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWEBSOCKETPP_STRICT_MASKING")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_ASIO_HAS_STD_CHRONO")

OPTION( LOG_LONG_API "Log long API calls over websocket (ON OR OFF)" ON )
MESSAGE( STATUS "LOG_LONG_API: ${LOG_LONG_API}" )
if( LOG_LONG_API )
  SET( LOG_LONG_API_MAX_MS 1000 CACHE STRING "Max API execution time in ms" )
  SET( LOG_LONG_API_WARN_MS 750 CACHE STRING "API execution time in ms at which to warn" )
  MESSAGE( STATUS "     " )
  MESSAGE( STATUS "     LOGGING LONG API CALLS" )
  MESSAGE( STATUS "          MAX  MS: ${LOG_LONG_API_MAX_MS}" )
  MESSAGE( STATUS "          WARN MS: ${LOG_LONG_API_WARN_MS}" )
  MESSAGE( STATUS "     " )
  SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLOG_LONG_API -DLOG_LONG_API_MAX_MS=${LOG_LONG_API_MAX_MS} -DLOG_LONG_API_WARN_MS=${LOG_LONG_API_WARN_MS}" )
  SET( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLOG_LONG_API -DLOG_LONG_API_MAX_MS=${LOG_LONG_API_MAX_MS} -DLOG_LONG_API_WARN_MS=${LOG_LONG_API_WARN_MS}" )
endif()

target_include_directories(fc
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    "vendor/diff-match-patch-cpp-stl"
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/websocketpp

  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/secp256k1-zkp
  )

IF(NOT WIN32)
  set(LINK_USR_LOCAL_LIB -L/usr/local/lib)
ENDIF()
target_link_libraries( fc PUBLIC ${LINK_USR_LOCAL_LIB}  ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} ${Boost_LIBRARIES}
                                 ${PLATFORM_SPECIFIC_LIBS} ${RPCRT4} ${CMAKE_DL_LIBS} ${rt_library}
                                 ${editline_libraries} secp256k1 ${CMAKE_REQUIRED_LIBRARIES} )

if(WIN32 AND MSVC)
    set_source_files_properties( src/network/http/websocket.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
elseif(WIN32 AND MINGW)
    set_source_files_properties( src/network/http/websocket.cpp PROPERTIES LINK_FLAGS "-mbig-obj" )
endif()


IF(Boost_UNIT_TEST_FRAMEWORK_LIBRARY MATCHES "\\.(so|dll)$")
IF(MSVC)
add_definitions(/DBOOST_TEST_DYN_LINK)
ELSE(MSVC)
add_definitions(-DBOOST_TEST_DYN_LINK)
ENDIF(MSVC)
ENDIF()

include_directories( vendor/websocketpp )

add_subdirectory(tests)

if(MSVC)
   # add addtional import library on windows platform
   target_link_libraries( fc PUBLIC crypt32.lib)

   #Variable will hold list of .pdb files generated for libraries the 'fc' module is linked to
   set(INTERFACE_LINK_PDB_RELEASE)

   set(SHARED_LIBRARIES_RELEASE)
   foreach(boost_import_lib ${Boost_LIBRARIES_RELEASE})
      get_filename_component(import_lib_name_root ${boost_import_lib} NAME_WE)
      get_filename_component(import_lib_path ${boost_import_lib} PATH)
      set(boost_dll "${import_lib_path}/${import_lib_name_root}${CMAKE_SHARED_LIBRARY_SUFFIX}")
      set(boost_lib_pdb "${import_lib_name_root}.pdb")

      FILE(GLOB_RECURSE boost_pdb_paths "${import_lib_path}/../../bin.v2/*/${boost_lib_pdb}")

      foreach(p ${boost_pdb_paths})
        if(p MATCHES ".*/address-model-${platformBitness}/")
          GP_APPEND_UNIQUE(INTERFACE_LINK_PDB_RELEASE ${p})
        endif()
      endforeach()

      if(EXISTS "${boost_dll}")
        set(SHARED_LIBRARIES_RELEASE ${SHARED_LIBRARIES_RELEASE} "${boost_dll}")
      endif()

   endforeach()

   set(INTERFACE_LINK_PDB_DEBUG)
   set(SHARED_LIBRARIES_DEBUG)
   foreach(boost_import_lib ${Boost_LIBRARIES_DEBUG})
      get_filename_component(import_lib_name_root ${boost_import_lib} NAME_WE)
      get_filename_component(import_lib_path ${boost_import_lib} PATH)
      set(boost_dll "${import_lib_path}/${import_lib_name_root}${CMAKE_SHARED_LIBRARY_SUFFIX}")
      set(boost_lib_pdb "${import_lib_name_root}.pdb")

      FILE(GLOB_RECURSE boost_pdb_paths "${import_lib_path}/../../bin.v2/*/${boost_lib_pdb}")

      foreach(p ${boost_pdb_paths})
        if(p MATCHES ".*/address-model-${platformBitness}/")
          GP_APPEND_UNIQUE(INTERFACE_LINK_PDB_DEBUG ${p})
        endif()
      endforeach()
      if(EXISTS "${boost_dll}")
        set(SHARED_LIBRARIES_DEBUG ${SHARED_LIBRARIES_DEBUG} "${boost_dll}")
      endif()
   endforeach()

   # message(STATUS "openssl_libraries=${OPENSSL_LIBRARIES}")
   foreach(lib ${OPENSSL_LIBRARIES})
      get_filename_component(lib_name ${lib} NAME_WE)
      if (${lib_name} STREQUAL "libeay32")
         get_filename_component(lib_dir ${lib} DIRECTORY)
         get_filename_component(openssl_dir "${lib_dir}/.." REALPATH)
         set( eaydll "${openssl_dir}/bin/${lib_name}${CMAKE_SHARED_LIBRARY_SUFFIX}")
         set(eay_pdb "${openssl_dir}/bin/${lib_name}.pdb")
         message(STATUS "eay=${eaydll}")
         if(EXISTS ${eay_pdb})
           GP_APPEND_UNIQUE(INTERFACE_LINK_PDB_RELEASE ${eay_pdb})
           GP_APPEND_UNIQUE(INTERFACE_LINK_PDB_DEBUG ${eay_pdb})
         endif()

         set(SHARED_LIBRARIES_DEBUG   ${SHARED_LIBRARIES_DEBUG}   "${eaydll}")
         set(SHARED_LIBRARIES_RELEASE ${SHARED_LIBRARIES_RELEASE} "${eaydll}")
      endif()
   endforeach()

   set_property(TARGET fc PROPERTY INTERFACE_LINK_PDB_RELEASE ${INTERFACE_LINK_PDB_RELEASE})
   set_property(TARGET fc PROPERTY INTERFACE_LINK_PDB_DEBUG ${INTERFACE_LINK_PDB_DEBUG})
   set_property(TARGET fc PROPERTY SHARED_LIBRARIES_DEBUG ${SHARED_LIBRARIES_DEBUG})
   set_property(TARGET fc PROPERTY SHARED_LIBRARIES_RELEASE ${SHARED_LIBRARIES_RELEASE})

endif(MSVC)

INCLUDE(CheckLibcxxAtomic)

MESSAGE(STATUS "Finished fc module configuration...")
