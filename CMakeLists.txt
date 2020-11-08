##########################################
## base cmake file ##
##########################################

# required for c++ 20
# /W3 warning in msvc fixed with 3.15
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

project(mkpoker
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "mkpoker")

#    message(STATUS "init compiler flags: ${CMAKE_CXX_FLAGS}")
#    message(STATUS "init compiler flags debug: ${CMAKE_CXX_FLAGS_DEBUG}")
#    message(STATUS "init compiler flags release: ${CMAKE_CXX_FLAGS_RELEASE}")

# guard
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()

# configuration for dev, only executes if this is the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    # compile flags for clang
    #if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    #    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -stdlib=libc++ -Wall -Wextra")
    #    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -stdlib=libc++ -xWall -xWextra")
    #elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        # check the status of compiler flagss
#        string(REGEX REPLACE "/W[0-9]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
#        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive- /W4 /wd4514 /wd4710 /wd5045 /wd4668")
#        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /permissive- /wd4514 /wd4710 /wd5045 /wd4668")
#        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /permissive- /wd4514 /wd4710 /wd5045 /wd4668")
#    endif ()

#    message(STATUS "init compiler flags: ${CMAKE_CXX_FLAGS}")
#    message(STATUS "init compiler flags debug: ${CMAKE_CXX_FLAGS_DEBUG}")
#    message(STATUS "init compiler flags release: ${CMAKE_CXX_FLAGS_RELEASE}")

    # maybe change CMAKE_CXX_STANDARD etc.
    # ...
endif()


# base library header only
add_library(mkpoker-base INTERFACE)
add_library(mkpoker::base ALIAS mkpoker-base)
target_include_directories(mkpoker-base INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>)
target_include_directories(mkpoker-base SYSTEM INTERFACE $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)
target_compile_features(mkpoker-base INTERFACE cxx_std_20)
target_compile_options(mkpoker-base INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive- /std:c++latest>)
target_compile_options(mkpoker-base INTERFACE $<$<CXX_COMPILER_ID:Clang>:-std=c++20 -stdlib=libc++ -Wall -Wextra>)
target_compile_options(mkpoker-base INTERFACE $<$<CXX_COMPILER_ID:GNU>:-std=c++20 -Wall -Wextra>)



# additional stuff, execute only if this is the main project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    # build tests
    message(STATUS "Testing enabled")
    enable_testing()
    add_subdirectory(test)

    # Docs only available if this is the main app
    find_package(Doxygen)
    if(Doxygen_FOUND)
	#add_subdirectory(docs)
	message(STATUS "Doxygen found, building docs")
    else()
	message(STATUS "Doxygen not found, not building docs")
    endif()
endif()




