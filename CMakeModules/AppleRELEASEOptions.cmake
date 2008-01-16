
# --- If we are on OS X copy all the embedded libraries to the app bundle
IF (APPLE)
   MESSAGE ( "Building Standalone release" )
#  IF ( NOT DEP_QT_LIBS )
#    SET (DEP_QT_LIBS "QtCore QtGui" )
#
#  ENDIF ( NOT DEP_QT_LIBS )
#    
#   IF ( NOT QT_PREFIX )
#    SET (QT_PREFIX "")
#  ENDIF ( NOT QT_PREFIX )
#
#  IF ( NOT QT_SUFFIX )
#    SET (QT_SUFFIX "framework")
#  ENDIF ( NOT QT_SUFFIX )
#
#  IF ( NOT QT_VERSION )
#    SET (QT_VERSION "4.2.3")
#  ENDIF ( NOT QT_VERSION )
#
#
  # -- This script will take care of files that the application is dependent on
  CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/CMakeModules/PackageMacAppBundleLibs.sh.in
                 ${hugin_BINARY_DIR}/${BINDIR}/PackageMacAppBundleLibs.sh @ONLY IMMEDIATE)

#  CONFIGURE_FILE(${CMAKE_PROJECT_DIR}/CMakeModules/PackageQt4ForOSXAppBundle.sh.in
#                 ${BINDIR}/PackageQt4ForOSXAppBundle.sh @ONLY IMMEDIATE)

  GET_TARGET_PROPERTY(EXE_LOC ${PROGNAME} LOCATION)

  ADD_CUSTOM_COMMAND(
    TARGET ${PROGNAME}
    POST_BUILD
    COMMAND "/bin/chmod"
    ARGS ugo+x ${hugin_BINARY_DIR}/${BINDIR}/PackageMacAppBundleLibs.sh
    )

  #-- Copy and adjust install_names on the Supporting Libraries
  #
  ADD_CUSTOM_COMMAND (
    TARGET  ${PROGNAME}
    POST_BUILD
    COMMAND ${hugin_BINARY_DIR}/${BINDIR}/PackageMacAppBundleLibs.sh
    ARGS
  )
  
#  ADD_CUSTOM_COMMAND(
#     TARGET ${OS_X_APP_NAME}
#     POST_BUILD
#     COMMAND "/bin/chmod"
#     ARGS ugo+x ${BINDIR}/PackageQt4ForOSXAppBundle.sh
#     )
        
  #-- Use the generated Shell script to copy and correct the QtLibs
#  ADD_CUSTOM_COMMAND (
#    TARGET  ${OS_X_APP_NAME}
#    POST_BUILD
#    COMMAND ${BINDIR}/PackageQt4ForOSXAppBundle.sh
#    ARGS
#  )
  

ENDIF (APPLE)

