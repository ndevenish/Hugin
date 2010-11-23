IF (WIN32)
   SET (HUGIN_BUILDER "" CACHE STRING
      "set Builder Name."
   )
   IF(NOT HUGIN_BUILDER) 
      MESSAGE(FATAL_ERROR "Need to set builder (HUGIN_BUILDER) manually")
   ENDIF(NOT HUGIN_BUILDER)

   SET (HUGIN_BUILDDATE "" CACHE STRING
      "set Build Date."
   )
   IF(NOT HUGIN_BUILDDATE) 
      MESSAGE(FATAL_ERROR "Need to set build date (HUGIN_BUILDDATE) manually")
   ENDIF(NOT HUGIN_BUILDDATE)
   
   #depreciated
   
   # InnoSetup installer files
   # using @VAR@ for substituion because ${VAR} might interfere with InnoSetup's own syntax
   #CONFIGURE_FILE(hugin_release.iss ${CMAKE_INSTALL_PREFIX}/../hugin_release.iss)
   #CONFIGURE_FILE(hugin_prerelease.iss ${CMAKE_INSTALL_PREFIX}/../hugin_prerelease.iss)
   #CONFIGURE_FILE(PreRelease_Notes.txt ${CMAKE_INSTALL_PREFIX}/../PreRelease_Notes.txt)
   #CONFIGURE_FILE(modpath.iss ${CMAKE_INSTALL_PREFIX}/../modpath.iss @ONLY)

   # Images for installer
   # images were corrupted, let's hope this works better
   #CONFIGURE_FILE(smallimage.bmp ${CMAKE_INSTALL_PREFIX}/../smallimage.bmp COPYONLY)
   #CONFIGURE_FILE(wizardimage.bmp ${CMAKE_INSTALL_PREFIX}/../wizardimage.bmp COPYONLY)
   #CONFIGURE_FILE(hugin.ico ${CMAKE_INSTALL_PREFIX}/../hugin.ico COPYONLY)

   # Text Files for installer
   # using @only for consistency
   #CONFIGURE_FILE(installer_license.txt ${CMAKE_INSTALL_PREFIX}/../installer_license.txt @ONLY)
   #CONFIGURE_FILE(Release_Notes.txt ${CMAKE_INSTALL_PREFIX}/../Release_Notes.txt @ONLY)
   #CONFIGURE_FILE(url.txt ${CMAKE_INSTALL_PREFIX}/../url.txt @ONLY)

ENDIF (WIN32)

