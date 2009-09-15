#ifndef __CONFIG_H__

/* various libraries. For compatability with the old source code
 * most cmake variables are defined under a second name as well */

/* Define if you have JPEG library */
#cmakedefine JPEG_FOUND 1
/* Define if you have JPEG library (old style) */
#ifdef JPEG_FOUND
#define HasJPEG 1
#endif

/* Define if you have PNG library */
#cmakedefine PNG_FOUND 1
#ifdef PNG_FOUND
#define HasPNG 1
#endif

/* Define if you have TIFF library */
#cmakedefine TIFF_FOUND 1
#ifdef TIFF_FOUND
#define HasTIFF 1
#endif

/* Define if you have OpenEXR library */
#cmakedefine OPENEXR_FOUND 1
#ifdef OPENEXR_FOUND
#define HasEXR 1
#endif

/* Define if you have Panotools library (pano13) */
#cmakedefine TLALLI_FOUND 1
#cmakedefine PANO13_FOUND 1

#ifdef TLALLI_FOUND
#define HasTLALLI 1
#elif defined PANO13_FOUND
#define HasPANO13 1
#endif

/* Define if you have log2 function */
#cmakedefine HAVE_LOG2 0

/* locate of the xrc files, as defined during configuration */
#define INSTALL_LOCALE_DIR "${INSTALL_LOCALE_DIR}"

/* Location for data, as defined during configuration*/
#define INSTALL_DATA_DIR "${INSTALL_DATA_DIR}/"

/* Location for XRC files and other data, as defined during configuration*/
#define INSTALL_XRC_DIR "${INSTALL_XRC_DIR}/"

/* Use exiv2, if found */
#cmakedefine EXIV2_FOUND 1
#ifdef EXIV2_FOUND
#define HUGIN_USE_EXIV2 1
#endif

/* Build a fully self contained OSX bundle (with embedded ressources) */
#cmakedefine MAC_SELF_CONTAINED_BUNDLE

/* contains directory of HuginStitchProject.app, if MAC_SELF_CONTAINED_BUNDLE 
   is not set. */
#define INSTALL_OSX_BUNDLE_DIR "${INSTALL_OSX_BUNDLE_DIR}"

#endif
