project(nf_network_playground_pool VERSION 1.0.0 LANGUAGES CXX)

file(GLOB_RECURSE public **.hpp)
file(GLOB_RECURSE private **.cpp)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${public} ${private})
#add_executable(${PROJECT_NAME} WIN32 ${public} ${private})

#set(nf_os_windows_DIR "C:/install/share/nf_os_windows")
#set(nf_logging_DIR "C:/install/share/nf_logging")
#set(nf_buffer_ringbuffer_DIR "C:/install/share/nf_buffer_ringbuffer")
#find_package(nf_os_windows CONFIG REQUIRED)
#find_package(nf_logging CONFIG REQUIRED)
#find_package(nf_buffer_ringbuffer CONFIG REQUIRED)

add_executable(${PROJECT_NAME} ${public} ${private})

#target_compile_options(${PROJECT_NAME} PRIVATE /WX /Wall /utf-8)
target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
target_compile_features(${PROJECT_NAME}	PRIVATE cxx_std_17)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)


target_link_libraries(${PROJECT_NAME}
	PRIVATE
	#lib::nf_logging
	#lib::nf_buffer_ringbuffer
	#lib::nf_os_windows
	nf_network
)

set(CMAKE_INSTALL_PREFIX "C:/install")


target_include_directories(${PROJECT_NAME}
	PUBLIC
	    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
		$<BUILD_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
)
