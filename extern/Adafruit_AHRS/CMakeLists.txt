project(Adafruit_AHRS)

add_library(Adafruit_AHRS STATIC)

target_sources(Adafruit_AHRS
	PUBLIC
		src/Adafruit_AHRS.h
    PRIVATE
		src/Adafruit_AHRS_FusionInterface.h
		src/Adafruit_AHRS_Madgwick.cpp
		src/Adafruit_AHRS_Madgwick.h
		src/Adafruit_AHRS_Mahony.cpp
		src/Adafruit_AHRS_Mahony.h
		src/Adafruit_AHRS_NXPFusion.cpp
		src/Adafruit_AHRS_NXPFusion.h
		src/Adafruit_AHRS_NXPmatrix.c
)

target_include_directories(Adafruit_AHRS PUBLIC ${PROJECT_SOURCE_DIR}/src)