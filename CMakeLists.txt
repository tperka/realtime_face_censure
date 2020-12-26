cmake_minimum_required(VERSION 3.10)

project(FaceDetection LANGUAGES CXX)



#add_executable(b.out src/main_B.cpp src/FaceDetector.cpp src/BlurDrawer.cpp include/BlurDrawer.h)
#add_executable(c.out src/main_C.cpp src/FaceDetector.cpp src/BlurDrawer.cpp include/BlurDrawer.h)
#add_executable(d.out src/main_D.cpp include/names.hpp)
#add_executable(d.out src/main_A.cpp include/names.hpp)


# OpenCV setup
find_package(OpenCV REQUIRED)
#target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

find_package( Boost REQUIRED COMPONENTS program_options regex )
include_directories( ${Boost_INCLUDE_DIRS} )

message(STATUS "OpenCV library status:")
message(STATUS "    config: ${OpenCV_DIR}")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
#target_link_libraries(${PROJECT_NAME} Threads::Threads)

#target_link_libraries(${PROJECT_NAME}  rt)

# Introduce preprocessor variables to keep paths of asset files
set(FACE_DETECTION_CONFIGURATION
        "${PROJECT_SOURCE_DIR}/assets/deploy.prototxt")

set(FACE_DETECTION_WEIGHTS
        "${PROJECT_SOURCE_DIR}/assets/res10_300x300_ssd_iter_140000_fp16.caffemodel")


set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")

file( GLOB APP_SOURCES ${SRC_DIR}/*.cpp )
foreach( sourcefile ${APP_SOURCES} )
        file(RELATIVE_PATH filename ${SRC_DIR} ${sourcefile})
        string( REPLACE "main_" "" file ${filename} )
        string( REPLACE ".cpp" ".out" execfile ${file} )
        add_executable( ${execfile} ${sourcefile})
        target_compile_features(${execfile} PUBLIC cxx_std_14)
        target_include_directories(${execfile} PRIVATE include)
        target_link_libraries(${execfile} ${OpenCV_LIBS})
        target_link_libraries(${execfile} Threads::Threads)
        target_link_libraries(${execfile}  rt)
        target_compile_definitions(${execfile} PRIVATE
                FACE_DETECTION_CONFIGURATION="${FACE_DETECTION_CONFIGURATION}")
        target_compile_definitions(${execfile} PRIVATE
                FACE_DETECTION_WEIGHTS="${FACE_DETECTION_WEIGHTS}")
endforeach(sourcefile ${APP_SOURCES})
