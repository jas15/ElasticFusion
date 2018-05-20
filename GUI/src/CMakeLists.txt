cmake_minimum_required(VERSION 2.6.0)

project(ElasticFusion)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")

if(WIN32)
  macro(CANONIFY_BOOL var)
    if(${var})
      set(${var} TRUE)
    else()
      set(${var} FALSE)
    endif()
  endmacro()
endif()

if(WIN32)
  find_package(JPEG REQUIRED)
endif()

find_package(LAPACK REQUIRED)
find_package(BLAS REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Pangolin 0.1 REQUIRED)
find_package(CUDA REQUIRED)
find_package(OpenNI2 REQUIRED)
find_package(efusion REQUIRED)

#...

# ------------------------------------------------------------------
# Detect SUITESPARSE libraries:
# If not found automatically, set SUITESPARSE_DIR in CMake to the 
# directory where SUITESPARSE was built.
# ------------------------------------------------------------------
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}") # Add the directory where FindSUITESPARSE.cmake module can be found.

set(SUITESPARSE_USE_LAPACK_BLAS ON)
find_package(SUITESPARSE QUIET NO_MODULE)  # 1st: Try to locate the *config.cmake file.
if(NOT SUITESPARSE_FOUND)
  set(SUITESPARSE_VERBOSE ON)
  find_package(SUITESPARSE REQUIRED) # 2nd: Use FindSUITESPARSE.cmake module
  include_directories(${SUITESPARSE_INCLUDE_DIRS})
else()
  message(STATUS "Find SUITESPARSE : include(${USE_SUITESPARSE})")
  include(${USE_SUITESPARSE})
endif()

MESSAGE(STATUS "SUITESPARSE_LIBS: ${SUITESPARSE_LIBRARIES}")
# ------------------------------------------------------------------
#   End of SUITESPARSE detection
# ------------------------------------------------------------------

#...

if(WIN32)
  find_package(RealSense QUIET)
  CANONIFY_BOOL(REALSENSE_FOUND)
  message(STATUS "librealsense found: ${REALSENSE_FOUND}")
  option(WITH_REALSENSE "Build with Intel RealSense support?" ${REALSENSE_FOUND})
endif()

if(WIN32)
  include_directories(${JPEG_INCLUDE_DIR})
endif()

include_directories(${ZLIB_INCLUDE_DIR})
include_directories(${EIGEN_INCLUDE_DIRS})
include_directories(${Pangolin_INCLUDE_DIRS})
include_directories(${CUDA_INCLUDE_DIRS})
include_directories(${EFUSION_INCLUDE_DIR})
include_directories(${OPENNI2_INCLUDE_DIR})

if(WITH_REALSENSE)
  include_directories(${REALSENSE_INCLUDE_DIR})
  add_definitions(-DWITH_REALSENSE)
  set(EXTRA_LIBS ${EXTRA_LIBS} ${REALSENSE_LIBRARY})
endif()

file(GLOB srcs *.cpp)
file(GLOB tools_srcs Tools/*.cpp)

if(WIN32)
  file(GLOB hdrs *.h)
  file(GLOB tools_hdrs Tools/*.h)
endif()

if(WIN32)
  add_definitions(-Dlinux=0)
  add_definitions(-DWIN32_LEAN_AND_MEAN)
  add_definitions(-DNOMINMAX)
  set(ADDITIONAL_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  set(EXTRA_WINDOWS_LIBS ${EXTRA_WINDOWS_LIBS} ws2_32 ${JPEG_LIBRARY})
else()
  add_definitions(-Dlinux=1)
endif()

set(CMAKE_CXX_FLAGS ${ADDITIONAL_CMAKE_CXX_FLAGS} "-O3 -Wall -std=c++11")
#set(CMAKE_CXX_FLAGS ${ADDITIONAL_CMAKE_CXX_FLAGS} "-O3 -msse2 -msse3 -Wall -std=c++11")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++11")

add_executable(ElasticFusion 
               ${srcs}
               ${tools_srcs}
               ${hdrs}
               ${tools_hdrs}
)

target_link_libraries(ElasticFusion
					  ${EXTRA_WINDOWS_LIBS}
                      ${ZLIB_LIBRARY}
                      ${Pangolin_LIBRARIES}
                      ${CUDA_LIBRARIES}
                      ${EXTRA_LIBS}
                      ${EFUSION_LIBRARY}
                      ${OPENNI2_LIBRARY}
                      ${SUITESPARSE_LIBRARIES}
                      ${BLAS_LIBRARIES}
                      ${LAPACK_LIBRARIES}
)


INSTALL(TARGETS ElasticFusion
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)
