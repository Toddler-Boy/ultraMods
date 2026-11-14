# A Baldur's Gate 3 mod: its stats, English texts and Script Extender script packed into one .pak with LSLib, laid out
# for the zip. Called from <game>/<mod>/CMakeLists.txt as larian_mod (<folder> <uuid> <version> <status> <description>)
FetchContent_Declare (lslib
					URL https://github.com/Norbyte/lslib/releases/download/v1.20.4/ExportTool-v1.20.4.zip
					URL_HASH SHA256=5e02368fb8acafda9b45acba37a3f3bf507fc3d65a083a159abbeab06337190e
					SOURCE_SUBDIR nonexistent)
FetchContent_MakeAvailable (lslib)

function (larian_mod folder uuid version status description)
	get_filename_component (mod "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
	get_filename_component (game_dir "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
	get_filename_component (game "${game_dir}" NAME)
	string (MAKE_C_IDENTIFIER "${game}_${mod}" target)

	set (out "${mods_dir}/${game}/${mod}")
	set (stage "${CMAKE_CURRENT_BINARY_DIR}/pak")
	set (source ${CMAKE_SOURCE_DIR}/common/larian_mod)
	set (divine "${lslib_SOURCE_DIR}/Tools/Divine.exe")

	# Version64 holds major, minor, revision and build in 9, 8, 16 and 31 bits
	string (REPLACE "." ";" parts "${version}")
	list (GET parts 0 major)
	list (GET parts 1 minor)
	list (GET parts 2 revision)
	math (EXPR VERSION64 "(${major} << 55) | (${minor} << 47) | (${revision} << 31)")

	set (NAME "${mod}")
	set (FOLDER "${folder}")
	set (UUID "${uuid}")
	set (STATUS "${status}")
	set (DESCRIPTION "${description}")

	set (staged
		 "${stage}/Mods/${folder}/meta.lsx"
		 "${stage}/Mods/${folder}/ScriptExtender/Config.json"
		 "${stage}/Mods/${folder}/ScriptExtender/Lua/BootstrapServer.lua"
		 "${stage}/Public/${folder}/Stats/Generated/Data/${folder}.txt")
	configure_file (${source}/meta.lsx "${stage}/Mods/${folder}/meta.lsx" @ONLY NEWLINE_STYLE CRLF)
	configure_file (${source}/Config.json "${stage}/Mods/${folder}/ScriptExtender/Config.json" @ONLY NEWLINE_STYLE CRLF)
	configure_file (${source}/BootstrapServer.lua "${stage}/Mods/${folder}/ScriptExtender/Lua/BootstrapServer.lua" @ONLY NEWLINE_STYLE CRLF)
	configure_file (${folder}.txt "${stage}/Public/${folder}/Stats/Generated/Data/${folder}.txt" @ONLY NEWLINE_STYLE CRLF)
	configure_file (readme.txt "${out}/readme.txt" @ONLY NEWLINE_STYLE CRLF)
	configure_file (${CMAKE_SOURCE_DIR}/LICENSE "${out}/LICENSE.txt" @ONLY NEWLINE_STYLE CRLF)

	set (loca "${stage}/Localization/English/${folder}.loca")
	set (pak "${out}/package/${folder}.pak")
	add_custom_command (OUTPUT "${loca}"
						COMMAND "${divine}" -g bg3 -a convert-loca -s "${CMAKE_CURRENT_SOURCE_DIR}/English.xml" -d "${loca}"
						DEPENDS English.xml
						VERBATIM)
	add_custom_command (OUTPUT "${pak}"
						COMMAND "${divine}" -g bg3 -a create-package -s "${stage}" -d "${pak}"
						DEPENDS "${loca}" ${staged}
						VERBATIM)
	add_custom_target (${target} ALL DEPENDS "${pak}")
	set_target_properties (${target} PROPERTIES FOLDER "${game}")
endfunction ()
