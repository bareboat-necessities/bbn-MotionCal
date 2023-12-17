project(libcalib)
set(CMAKE_STATIC_LIBRARY_PREFIX "")

add_library(libcalib STATIC)

target_sources(libcalib
    PRIVATE
		include/libcalib.h
		include/libcalib_common.h
		include/libcalib_mahony.h
		include/libcalib_nxp.h
		include/libcalib_quality.h
        src/libcalib.cpp
		src/magcal.cpp
		src/mahony.cpp
		src/matrix.cpp
		src/matrix.h
        src/nxp.cpp
		src/quality.cpp
		src/rawdata.cpp
)

target_include_directories(libcalib PUBLIC ${PROJECT_SOURCE_DIR}/include)