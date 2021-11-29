project(nf_network_test VERSION 1.0.0 LANGUAGES CXX)

file(GLOB_RECURSE public **.hpp)
file(GLOB_RECURSE private **.cpp)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${public} ${private})


set(GTest_ROOT "$ENV{VCPKG_ROOT}\\packages\\gtest_x64-windows\\share\\gtest")	
find_package(GTest CONFIG REQUIRED)

#set(nf_os_windows_DIR "C:/install/share/nf_os_windows")
#set(nf_logging_DIR "C:/install/share/nf_logging")
#set(nf_buffer_ringbuffer_DIR "C:/install/share/nf_buffer_ringbuffer")
#find_package(nf_os_windows CONFIG REQUIRED)
#find_package(nf_logging CONFIG REQUIRED)
#find_package(nf_buffer_ringbuffer CONFIG REQUIRED)

add_executable(${PROJECT_NAME} ${public} ${private})
target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
target_compile_features(${PROJECT_NAME}	PRIVATE cxx_std_17)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)

set(CMAKE_INSTALL_PREFIX "C:/install")

target_include_directories(${PROJECT_NAME}
	PUBLIC
		$<INSTALL_INTERFACE:include>
	    $<BUILD_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
	#lib::nf_os_windows
	#lib::nf_logging
	#lib::nf_buffer_ringbuffer
	nf_network
	GTest::gtest
	GTest::gtest_main
	GTest::gmock
	GTest::gmock_main
)

add_test(
	NAME	${PROJECT_NAME}
	COMMAND	${PROJECT_NAME}
)