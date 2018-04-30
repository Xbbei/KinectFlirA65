cmake_minimum_required(VERSION 2.8)
project(QTFlirKinect)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")
#set(CMAKE_BUILD_TYPE Debug)

find_package(Qt4 REQUIRED)
find_package(OpenNI2 REQUIRED)
find_package(Ebus REQUIRED)
find_package(OpenCV REQUIRED)
find_package(LibUSB REQUIRED)
find_package(Boost COMPONENTS thread REQUIRED)
find_package(Boost COMPONENTS filesystem REQUIRED)
find_package(Boost COMPONENTS system REQUIRED)
find_package(Boost COMPONENTS date_time REQUIRED)

include(${QT_USE_FILE})
 
qt4_wrap_cpp(main_moc_SRCS
             main.h)

include_directories(${OPENNI2_INCLUDE_DIR})
include_directories(${Ebus_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${LibUSB_INCLUDE_DIRS})

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(BOOST_ALL_DYN_LINK ON)   # force dynamic linking for all libraries

set(srcs main.cpp
	 Logger.cpp
         FlirKinect/EbusFlirInterface.cpp
	 FlirKinect/OpenNI2Interface.cpp
         )

add_executable(QTFlirKinect ${srcs} ${main_moc_SRCS})

target_link_libraries(QTFlirKinect           
                      ${Boost_LIBRARIES}
                      ${OpenCV_LIBS} 
                      boost_system
                      boost_filesystem
                      boost_thread
                      ${Ebus_LIBRARIES}
		      ${OPENNI2_LIBRARY}
                      ${QT_LIBRARIES}
		      ${LibUSB_LIBRARY})

