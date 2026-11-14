# A Mass Effect Legendary Edition merge mod: its checked-in .m3m and moddesc.ini laid out as ME3Tweaks Mod Manager
# expects them (package/), its readme and the licence beside. Called from <game>/<mod>/CMakeLists.txt as merge_mod (<m3m name>)
function (merge_mod m3m)
	get_filename_component (mod "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
	get_filename_component (game_dir "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
	get_filename_component (game "${game_dir}" NAME)

	set (out "${mods_dir}/${game}/${mod}")

	configure_file (moddesc.ini "${out}/package/moddesc.ini" @ONLY NEWLINE_STYLE CRLF)
	configure_file (${m3m}.m3m "${out}/package/MergeMods/${m3m}.m3m" COPYONLY)
	configure_file (readme.txt "${out}/readme.txt" @ONLY NEWLINE_STYLE CRLF)
	configure_file (${CMAKE_SOURCE_DIR}/LICENSE "${out}/LICENSE.txt" @ONLY NEWLINE_STYLE CRLF)
endfunction ()
