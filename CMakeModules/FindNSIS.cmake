# - Find NSIS (makensis.exe)
# 2010 Matthew Petroff
# based on FindPANO13 by TKSharpless
# defines cache vars
#  MAKENSIS_EXECUTABLE, NSIS script compiler
#  NSIS_FOUND, yes if found

FIND_PROGRAM(MAKENSIS_EXECUTABLE
	NAMES makensis makensis.exe
	PATHS C:/progra~1/NSIS/
	)

IF(MAKENSIS_EXECUTABLE)
  SET(NSIS_FOUND "YES" )
ENDIF(MAKENSIS_EXECUTABLE)

