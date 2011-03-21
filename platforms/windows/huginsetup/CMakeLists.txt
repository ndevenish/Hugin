# Windows installer settings
IF(WIN32)
  # Set install directory
  IF(CMAKE_SIZEOF_VOID_P EQUAL 4) #32bit
    SET(WINDOWS_INSTALL_DIRECTORY "$PROGRAMFILES\\Hugin")
	SET(WINDOWS_ARCH_TYPE "32")
  ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
  IF(CMAKE_SIZEOF_VOID_P EQUAL 8) #64bit
    SET(WINDOWS_INSTALL_DIRECTORY "$PROGRAMFILES64\\Hugin")
	SET(WINDOWS_ARCH_TYPE "64")
  ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 8)
  
  # Toggle Development Version Warning
  IF(HUGIN_DEVELOPMENT_VERSION EQUAL 1)
    SET(DEV_WARNING_TOGGLE "$(TEXT_PreReleaseWelcomePage)")
  ELSE(HUGIN_DEVELOPMENT_VERSION EQUAL 1)
    SET(DEV_WARNING_TOGGLE "$(TEXT_WelcomePage)")
  ENDIF(HUGIN_DEVELOPMENT_VERSION EQUAL 1)
  
  # Configure Files
  CONFIGURE_FILE(HuginSetup.nsi ${CMAKE_INSTALL_PREFIX}/../HuginSetup.nsi)
  
  # Install Files
  INSTALL(FILES HuginSetup_common.nsh
    DESTINATION ${CMAKE_INSTALL_PREFIX}/../)
  add_subdirectory(CPGenerators)
  add_subdirectory(Functions)
  add_subdirectory(Graphics)
  add_subdirectory(Languages)
  add_subdirectory(Licenses)
  add_subdirectory(Plugins)
ENDIF(WIN32)