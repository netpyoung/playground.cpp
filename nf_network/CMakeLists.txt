cmake_minimum_required(VERSION 3.16.2)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}\\scripts\\buildsystems\\vcpkg.cmake" CACHE STRING "")
endif()

project(nf_network VERSION 1.0.0)

option(BUILD_INSTALL	"Install header and library"	ON)
option(BUILD_TEST		"Build own tests."				ON)
option(BUILD_PLAYGORUND	"Build playground."				ON)
option(BUILD_DUMMY		"Build dummy client."			ON)

add_subdirectory(src)

if (BUILD_TEST)
	enable_testing()
	add_subdirectory(test)
endif (BUILD_TEST)

if (BUILD_PLAYGORUND)
	add_subdirectory(playground_lan)
	add_subdirectory(playground_net)
	add_subdirectory(playground_pool)
	add_subdirectory(playground_pool_tls)
endif (BUILD_PLAYGORUND)

if (BUILD_DUMMY)
	add_subdirectory(dummy_client)
endif (BUILD_DUMMY)