set(SRCS gui.cpp portlist.cpp serialdata.cpp visualize.cpp)
set(PNGS checkempty.png checkemptygray.png checkgreen.png)

find_package(OpenGL REQUIRED)

find_package(wxWidgets REQUIRED COMPONENTS core base gl)
include(${wxWidgets_USE_FILE})

find_package(image2wx REQUIRED)
include(${image2wx_USE_FILE})
include_directories(${image2wx_SOURCE_DIR} ${image2wx_INCLUDE_DIRS})
generate_image2wx_sources(PNG_SRCS "${PNGS}")
list(APPEND SRCS ${PNG_SRCS})

add_executable(SensorCal WIN32 MACOSX_BUNDLE ${SRCS})

if(APPLE)
	message("[SensorCal] building for mac")
    # Set a custom plist file for the app bundle - needed for Mac OS Retina display

    set_target_properties(SensorCal
		PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
		MACOSX_BUNDLE_ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/icon.icns
		)
	
	target_compile_definitions(SensorCal PUBLIC "MACOSX")
	target_compile_definitions(SensorCal PUBLIC "GL_SILENCE_DEPRECATION")
elseif(WIN32)
	message("[SensorCal] building for windows")
    target_sources(SensorCal PRIVATE main.exe.manifest)
	target_compile_definitions(SensorCal PUBLIC "WINDOWS")
else()
	target_compile_definitions(SensorCal PUBLIC "LINUX")
endif()

target_link_libraries(SensorCal PUBLIC ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARY} libcalib)
