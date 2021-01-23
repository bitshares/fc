# This overrides `find_package(Boost ... CONFIG ... )` calls
#  - calls the CMake's built-in `FindBoost.cmake` and adds `pthread` library dependency

MESSAGE(STATUS "Using custom FindBoost config")

find_package(Boost 1.58 REQUIRED COMPONENTS ${Boost_FIND_COMPONENTS})
IF("${Boost_VERSION}" GREATER_EQUAL 107000 )
    MESSAGE(FATAL_ERROR "Boost version ${Boost_VERSION} must be at least 1.58 and at most 1.69 .")
ENDIF()

#  Inject `pthread` dependency to Boost if needed
if (UNIX AND NOT CYGWIN)
    list(FIND Boost_FIND_COMPONENTS thread _using_boost_thread)
    if (_using_boost_thread GREATER -1)
        find_library(BOOST_THREAD_LIBRARY NAMES pthread DOC "The threading library used by boost-thread")
        if (BOOST_THREAD_LIBRARY)
            MESSAGE(STATUS "Adding Boost thread lib dependency: ${BOOST_THREAD_LIBRARY}")
            list(APPEND Boost_LIBRARIES ${BOOST_THREAD_LIBRARY})
        endif ()
    endif ()
endif ()
