cmake_minimum_required (VERSION 4.3.0 FATAL_ERROR)

set (CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set (CMAKE_SUPPRESS_REGENERATION ON)
set (CMAKE_CONFIGURATION_TYPES Debug Release)

set (CMAKE_C_FLAGS_RELEASE "/O2 /Ob2 /Oi /Ot /Oy /GF /GS- /Gy /DNDEBUG")
set (CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
set (CMAKE_SHARED_LINKER_FLAGS_RELEASE "/INCREMENTAL:NO /OPT:REF /OPT:ICF /nodefaultlib:msvcrt")

project (ultraMods LANGUAGES CXX C)

include (FetchContent)

set (CMAKE_CXX_STANDARD 23)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_C_STANDARD 17)
set (CMAKE_C_STANDARD_REQUIRED ON)

FetchContent_Declare (minhook
					GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
					GIT_TAG v1.3.4
					GIT_SHALLOW TRUE
					SOURCE_SUBDIR nonexistent)
FetchContent_MakeAvailable (minhook)

# Every built mod lands in mods/<game>/<mod> with what its zip holds; build.sh zips and deploys from there
set (mods_dir ${CMAKE_BINARY_DIR}/mods)

include (common/spatial_audio_fix/spatial_audio_fix.cmake)

add_subdirectory (ultraPatcher)

# One folder per game, one per mod inside it; a mod with a CMakeLists.txt is built here
file (GLOB mod_lists CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*/*/CMakeLists.txt")
foreach (mod_list ${mod_lists})
	get_filename_component (mod_source "${mod_list}" DIRECTORY)
	if (NOT mod_source MATCHES "/common/")
		add_subdirectory ("${mod_source}")
	endif ()
endforeach ()
