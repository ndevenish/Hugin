# Replace the wxNotebook and associated XRC wx elements from the passed file
MESSAGE("Patching XRC elements for OSX")
FILE(READ ${file} _contents)
STRING(REPLACE wxNotebook wxChoice _contents "${_contents}")
STRING(REPLACE cp_editor_left_tab cp_editor_left_choice _contents "${_contents}")
STRING(REPLACE cp_editor_right_tab cp_editor_right_choice _contents "${_contents}")
FILE(WRITE ${file} "${_contents}")
