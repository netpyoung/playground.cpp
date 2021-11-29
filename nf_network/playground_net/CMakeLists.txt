project(nf_network_playground_net VERSION 1.0.0 LANGUAGES CXX)

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
#/FS
#  /OPT:REF /OPT:ICF // https://docs.microsoft.com/ko-kr/cpp/build/reference/opt-optimizations?view=vs-2019
# ref: https://docs.microsoft.com/ko-kr/cpp/build/how-to-debug-a-release-build?view=vs-2019
target_compile_options(${PROJECT_NAME} PRIVATE
    /utf-8
	/Zo # https://docs.microsoft.com/ko-kr/cpp/build/reference/zo-enhance-optimized-debugging?view=vs-2019
	#/Zi
	#/FAcsu
	#/MT
)
target_link_options(${PROJECT_NAME} PRIVATE
	#/NODEFAULTLIB:msvcrt.lib
    /INCREMENTAL:NO
	#/DEBUG
	/MAP 
)

#target_compile_options(${PROJECT_NAME} PRIVATE
#    "$<$<CONFIG:Release>:/Zi /utf-8 /FAcsu>"
#)
#target_link_options(${PROJECT_NAME} PRIVATE
#    "$<$<CONFIG:Release>:/INCREMENTAL:NO /DEBUG /MAP /OPT:REF /OPT:ICF>"
#)

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
