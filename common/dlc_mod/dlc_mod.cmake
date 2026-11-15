# A Mass Effect Legendary Edition DLC mod: its DLC folder and moddesc.ini laid out as ME3Tweaks Mod Manager expects them
# (package/), its readme and the licence beside. Called from <game>/<mod>/CMakeLists.txt as dlc_mod (<DLC folder>)
function (dlc_mod dlc)
	get_filename_component (mod "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
	get_filename_component (game_dir "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
	get_filename_component (game "${game_dir}" NAME)

	set (out "${mods_dir}/${game}/${mod}")

	configure_file (moddesc.ini "${out}/package/moddesc.ini" @ONLY NEWLINE_STYLE CRLF)
	file (GLOB_RECURSE dlc_files CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/${dlc}/*")
	foreach (dlc_file ${dlc_files})
		configure_file ("${dlc_file}" "${out}/package/${dlc_file}" COPYONLY)
	endforeach ()
	configure_file (readme.txt "${out}/readme.txt" @ONLY NEWLINE_STYLE CRLF)
	configure_file (${CMAKE_SOURCE_DIR}/LICENSE "${out}/LICENSE.txt" @ONLY NEWLINE_STYLE CRLF)
endfunction ()
