# - Find 7ZA (7zip command line)
# 2010 Matthew Petroff
# based on FindPANO13 by TKSharpless
# reads cache variable
#  SOURCE_BASE_DIR -- directory that contains hugin source root
# defines cache vars
#  7ZA, 7zip command line executable
#  7ZIP_FOUND, yes if found

FIND_PROGRAM(7ZA
	NAMES 7za 7za.exe
	PATHS ${SOURCE_BASE_DIR}/7zip
	)

IF(7ZA)
  SET(7ZIP_FOUND "YES" )
ENDIF(7ZA)

