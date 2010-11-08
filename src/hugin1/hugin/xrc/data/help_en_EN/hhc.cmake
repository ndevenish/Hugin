# html help compiler is returning non zero retun value
# so using execute_process to discard the return value
EXECUTE_PROCESS(COMMAND ${HTML_HELP_COMPILER} hugin_help_en_EN.hhp ERROR_QUIET)