cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

if(WIN32)
	# from output of `vcpkg install wxwidgets`...
	#	Applied user-wide integration for this vcpkg root.
	#	CMake projects should use: "-DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
	#	All MSBuild C++ projects can now #include any installed libraries. Linking will be handled automatically. Installing new libraries will make them instantly available.

	set(CMAKE_TOOLCHAIN_FILE ../vcpkg/scripts/buildsystems/vcpkg.cmake)
endif()

project(SensorCal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ensure find_package(image2wx) can find our version
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/extern/image2wx)

add_subdirectory(extern)
add_subdirectory(src)
