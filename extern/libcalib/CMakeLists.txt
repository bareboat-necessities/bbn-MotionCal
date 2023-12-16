project(libcalib)
set(CMAKE_STATIC_LIBRARY_PREFIX "")

add_library(libcalib STATIC)

target_sources(libcalib
    PRIVATE
        src/libcalib.cpp
		src/magcal.cpp
		src/mahony.cpp
		src/matrix.cpp
        src/nxp.cpp
		src/quality.cpp
		src/rawdata.cpp
)

target_include_directories(libcalib PUBLIC ${PROJECT_SOURCE_DIR}/include)