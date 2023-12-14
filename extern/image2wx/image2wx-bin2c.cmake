function(bin2c base_filename symbol image_path template_path c_path)

	# slurp in file as a hex string
	file(READ ${image_path} content HEX)

	# Separate into individual bytes.
	string(REGEX MATCHALL "([A-Fa-f0-9][A-Fa-f0-9])" hex_raw_list ${content})

	# see tabbing of image2wx::image::${symbol}::bytes in image.cpp.in (aka ${template_path})
	set(hex_bytes_list "\t\t\t\t")

	# break up into lines with 16 hex values each
	set(counter 0)
	foreach (hex IN LISTS hex_raw_list)
		string(APPEND hex_bytes_list "0x${hex},")
		MATH(EXPR counter "${counter}+1")
		if (counter GREATER 16)
			string(APPEND hex_bytes_list "\n\t\t\t\t")
			set(counter 0)
		endif ()
	endforeach ()

	# generate cpp file from template
	configure_file(${template_path} ${c_path})

endfunction()

bin2c("${BASE_FILENAME}" "${SYMBOL}" "${IMAGE_PATH}" "${TEMPLATE_PATH}" "${C_PATH}")
