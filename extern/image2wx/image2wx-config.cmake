get_filename_component(image2wx_SOURCE_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)

set(image2wx_USE_FILE "${image2wx_SOURCE_DIR}/image2wx-use.cmake")
set(image2wx_INCLUDE_DIRS "${image2wx_SOURCE_DIR}/include")

set(image2wx_BIN2C_FILE "${image2wx_SOURCE_DIR}/image2wx-bin2c.cmake")
set(image2wx_IMAGE_C_TEMPLATE_FILE "${image2wx_SOURCE_DIR}/image.cpp.in")
