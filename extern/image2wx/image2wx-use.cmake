
function(generate_image2wx_sources output_var image_file_list)

	# NOTE bruceo: it's tempting to use add_library() to make a target here
	#	and then add sources to it (ala FileEmbed.cmake). but when using both
	#	normal and OBJECT libraries, i found that the generated sources were
	#	built for *both* the library and the calling executable. super weird.
	#	thus, we choose to instead build a list of the generated sources and
	#	pass them back to the caller for inclusion in the exectuable.

	set(source_list ${image2wx_SOURCE_DIR}/image2wx.cpp)
	
	foreach (image_file IN LISTS image_file_list)
		
		get_filename_component(base_filename ${image_file} NAME) # no directory, but include extension
    	string(MAKE_C_IDENTIFIER ${base_filename} symbol)

		set(image_path ${CMAKE_CURRENT_SOURCE_DIR}/${image_file})

		set(c_filename "${symbol}.cpp")
    	set(c_path ${CMAKE_CURRENT_BINARY_DIR}/image2wx/${c_filename})

		add_custom_command(
			OUTPUT ${c_path}
			COMMAND ${CMAKE_COMMAND}
			-DBASE_FILENAME="${base_filename}"
			-DSYMBOL=${symbol}
			-DIMAGE_PATH="${image_path}"
			-DTEMPLATE_PATH="${image2wx_IMAGE_C_TEMPLATE_FILE}"
			-DC_PATH="${c_path}"
			-P "${image2wx_BIN2C_FILE}"
			MAIN_DEPENDENCY ${image_path}
			DEPENDS
			${image2wx_USE_FILE}
			${image2wx_BIN2C_FILE}
			${image2wx_IMAGE_C_TEMPLATE_FILE}
			)

		list(APPEND source_list ${c_path})

	endforeach()

	set(${output_var} ${source_list} PARENT_SCOPE)

endfunction()
