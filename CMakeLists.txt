cmake_minimum_required(VERSION 2.8.7)
enable_testing()

if (NOT DEFINED CMAKE_MACOSX_RPATH)
	set(CMAKE_MACOSX_RPATH true)
endif()

# pkg-config requirements from SBT.
include(${DEPENDENCIES_FILE})

# Optional dependencies.
find_path(HAVE_GLOG glog/logging.h)
if(NOT ${HAVE_GLOG} MATCHES HAVE_GLOG-NOTFOUND)
	message(STATUS "Found glog: ${HAVE_GLOG}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_GLOG")
	link_libraries(glog)
else()
	message(STATUS "Did not find glog; logging is disabled")
endif()

find_path(HAVE_EV ev.h)
if(NOT ${HAVE_EV} MATCHES HAVE_EV-NOTFOUND)
	message(STATUS "Found ev: ${HAVE_EV}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_EV")
	link_libraries(ev)
else()
	message(FATAL_ERROR "Did not find ev. This is required for networking")
endif()

# Include directory for our config.h.
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src/main/cpp)

subdirs(src/main/cpp)
subdirs(src/test/cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -include cpp14compat.h")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
