cmake_minimum_required(VERSION 2.6.0)

project(libefusion)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(Pangolin 0.1 REQUIRED)
find_package(CUDA REQUIRED)
find_package(SuiteSparse REQUIRED)

set(efusion_SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Shaders" CACHE PATH "Where the shaders live")

include_directories(${Pangolin_INCLUDE_DIRS})
include_directories(${CUDA_INCLUDE_DIRS})
include_directories(${EIGEN_INCLUDE_DIRS})
include_directories(${SUITESPARSE_INCLUDE_DIRS})

file(GLOB srcs *.cpp)
file(GLOB utils_srcs Utils/*.cpp)
file(GLOB shader_srcs Shaders/*.cpp)
file(GLOB cuda Cuda/*.cu)
file(GLOB containers Cuda/containers/*.cpp)

if(WIN32)
  file(GLOB hdrs *.h)
  file(GLOB utils_hdrs Utils/*.h)
  file(GLOB shader_hdrs Shaders/*.h)
  file(GLOB cuda_hdrs Cuda/*.cuh)
  file(GLOB containers_hdrs Cuda/containers/*.hpp)
endif()

set(CUDA_ARCH_BIN "61 62" CACHE STRING "Specify 'real' GPU arch to build binaries for, BIN(PTX) format is supported. Example: 1.3 2.1(1.3) or 13 21(13)")
set(CUDA_ARCH_PTX "" CACHE STRING "Specify 'virtual' PTX arch to build PTX intermediate code for. Example: 1.0 1.2 or 10 12")              

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
include(CudaComputeTargetFlags.cmake)                  
APPEND_TARGET_ARCH_FLAGS()

set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS}  "-Xcompiler;-fPIC;")           
set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS} "--ftz=true;--prec-div=false;--prec-sqrt=false") 

CUDA_COMPILE(cuda_objs ${cuda})

if(WIN32)
  set(ADDITIONAL_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
endif()

set(CMAKE_AARCH_FLAGS "-march=native -mtune=cortex-a57 -mcpu=cortex-a57")

set(CMAKE_OPT_FLAGS "-fno-defer-pop -ffp-contract=fast -finline-limit=100 -fmerge-all-constants -fmodulo-sched -fmodulo-sched-allow-regmoves -fgcse-sm -fgcse-las -fgcse-after-reload -fdeclone-ctor-dtor -flive-range-shrinkage -fira-hoist-pressure -fira-loop-pressure -fsched-pressure -fsched-spec-load -fsched-spec-load-dangerous -fsched-stalled-insns=0 -fsched-stalled-insns-dep=1 -fsched2-use-superblocks -freschedule-modulo-scheduled-loops -fselective-scheduling2 -fsel-sched-pipelining -fsel-sched-pipelining-outer-loops -fipa-pta -floop-nest-optimize -floop-parallelize-all -ftree-vectorize -fvect-cost-model=dynamic -fsimd-cost-model=dynamic -fvariable-expansion-in-unroller -ffast-math -fno-math-errno -funsafe-math-optimizations -fassociative-math -freciprocal-math -ffinite-math-only -fbranch-probabilities -fprofile-arcs -fprofile-values -fprofile-reorder-functions -fvpt -funroll-loops -ftracer -frename-registers -funswitch-loops")
#--param l1-cache-size=?? =--param l2-cache-size=??
#NOTE ffast-math does not guarantee IEEE or ISO rules/specs for maths
#NOTE same for no-math-errno
#NOTE same for unsafe-math-optimizations
#NOTE same for associative-math
#NOTE reciprocal-math might do same thing
#NOTE finite-math-only
#NOTE unroll-loops increases code size so could be worse
#NOTE rename-registers could not give benefit
#NOTE That's it that jumps out at me but tbh it is not exhaustive so who knowsss


set(CMAKE_CXX_FLAGS ${ADDITIONAL_CMAKE_CXX_FLAGS} "-Ofast -Wall -std=c++11 -DSHADER_DIR=${efusion_SHADER_DIR} ${CMAKE_AARCH_FLAGS} ${CMAKE_OPT_FLAGS}")
#set(CMAKE_CXX_FLAGS ${ADDITIONAL_CMAKE_CXX_FLAGS} "-O3 -msse2 -msse3 -Wall -std=c++11 -DSHADER_DIR=${efusion_SHADER_DIR}")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++11 -DSHADER_DIR=${efusion_SHADER_DIR}")
  
if(WIN32)
  add_definitions(-DWIN32_LEAN_AND_MEAN)
  add_definitions(-DNOMINMAX)
  set (EXTRA_WINDOWS_LIBS ${EXTRA_WINDOWS_LIBS} ws2_32)
endif()

add_library(efusion SHARED 
            ${srcs}
            ${utils_srcs}
            ${shader_srcs}
            ${cuda} 
            ${cuda_objs} 
            ${containers}
            ${hdrs}
            ${utils_hdrs}
            ${shader_hdrs}
            ${cuda_hdrs} 
            ${containers_hdrs}
)

target_link_libraries(efusion
                      ${Eigen_LIBRARIES}
                      ${Pangolin_LIBRARIES}
                      ${CUDA_LIBRARIES}
                      ${SUITESPARSE_LIBRARIES}
					  ${EXTRA_WINDOWS_LIBS}
)

INSTALL(TARGETS efusion
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
)
