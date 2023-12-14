project(libmotioncal)
set(CMAKE_STATIC_LIBRARY_PREFIX "")

add_library(libmotioncal STATIC)

target_sources(libmotioncal
    PRIVATE
        src/fusion.cpp
        src/libmotioncal.cpp
		src/magcal.cpp
		src/mahony.cpp
		src/matrix.cpp
		src/quality.cpp
		src/rawdata.cpp
)

target_include_directories(libmotioncal PUBLIC ${PROJECT_SOURCE_DIR}/include)