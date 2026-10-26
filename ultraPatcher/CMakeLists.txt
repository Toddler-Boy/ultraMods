# The generic patcher, without patches; make_patcher.py appends a game's patches to a copy of it
add_executable (ultraPatcher WIN32 ultraPatcher.cpp ultraPatcher.manifest)
target_compile_definitions (ultraPatcher PRIVATE UNICODE NOMINMAX)
target_link_libraries (ultraPatcher PRIVATE comctl32)
target_link_options (ultraPatcher PRIVATE /MANIFESTUAC:NO $<$<CONFIG:Release>:/OPT:REF /OPT:ICF>)
set_target_properties (ultraPatcher PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/ultraPatcher)
