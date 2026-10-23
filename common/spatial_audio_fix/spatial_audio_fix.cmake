# The spatial audio fix for one game: an XInput stand-in under the DLL name the game imports, its readme and MinHook's
# licence, laid out for the zip. Called from <game>/<mod>/CMakeLists.txt as spatial_audio_fix (<dll> <exe> <folder in the
# game>)
function (spatial_audio_fix dll exe to)
	get_filename_component (mod "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
	get_filename_component (game_dir "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
	get_filename_component (game "${game_dir}" NAME)
	string (MAKE_C_IDENTIFIER "${game}_${mod}" target)

	set (out "${mods_dir}/${game}/${mod}")
	set (source ${CMAKE_SOURCE_DIR}/common/spatial_audio_fix)

	add_library (${target} SHARED ${source}/spatial_audio_fix.cpp ${source}/${dll}.def
				 ${minhook_SOURCE_DIR}/src/buffer.c ${minhook_SOURCE_DIR}/src/hook.c ${minhook_SOURCE_DIR}/src/trampoline.c ${minhook_SOURCE_DIR}/src/hde/hde64.c)
	target_include_directories (${target} SYSTEM PRIVATE ${minhook_SOURCE_DIR}/include)
	target_compile_definitions (${target} PRIVATE SAF_SYSTEM_DLL=L"${dll}.dll")
	set_target_properties (${target} PROPERTIES OUTPUT_NAME ${dll} FOLDER "${game}" CXX_SCAN_FOR_MODULES OFF
						   RUNTIME_OUTPUT_DIRECTORY_RELEASE "${out}" RUNTIME_OUTPUT_DIRECTORY_DEBUG "${out}/debug")

	set (GAME "${game}")
	set (DLL "${dll}.dll")
	set (EXE "${exe}")
	set (TO "${to}")
	configure_file (${source}/readme.txt "${out}/readme.txt" @ONLY NEWLINE_STYLE CRLF)
	configure_file (${minhook_SOURCE_DIR}/LICENSE.txt "${out}/LICENSE-MinHook.txt" COPYONLY)
	file (WRITE "${out}/deploy.txt" "${to}")
endfunction ()
