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
#cmakedefine HAVE_LOG2 1

/* Define if you have log1p function */
#cmakedefine HAVE_LOG1P 1

/* locate of the xrc files, as defined during configuration */
#define INSTALL_LOCALE_DIR "${INSTALL_LOCALE_DIR}"

/* Location for data, as defined during configuration*/
#define INSTALL_DATA_DIR "${INSTALL_DATA_DIR}/"

/* Location for XRC files and other data, as defined during configuration*/
#define INSTALL_XRC_DIR "${INSTALL_XRC_DIR}/"

/* if FFTW library is available */
#cmakedefine HAVE_FFTW 1

/* Build a fully self contained OSX bundle (with embedded ressources) */
#cmakedefine MAC_SELF_CONTAINED_BUNDLE

/* contains directory of HuginStitchProject.app, if MAC_SELF_CONTAINED_BUNDLE 
   is not set. */
#define INSTALL_OSX_BUNDLE_DIR "${INSTALL_OSX_BUNDLE_DIR}"

/* if compiler supports OpenMP */
#cmakedefine HAVE_OPENMP 1

/* if we are compiling in C++11 mode */
#cmakedefine HAVE_CXX11

/* if compiler supports C++11 threads */
#cmakedefine USE_CXX11_THREAD

/* if we have C++17 <filesystem> header */
#cmakedefine HAVE_STD_FILESYSTEM

#endif
