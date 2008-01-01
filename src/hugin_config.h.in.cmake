#ifndef __CONFIG_H__

/* Define to 1 if you have the <pano12/queryfeature.h> header file. */
#define HAVE_PANO12_QUERYFEATURE_H 1

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
#cmakedefine PANO12_FOUND 1

#ifdef TLALLI_FOUND
#define HasTLALLI 1
#elif defined PANO13_FOUND
#define HasPANO13 1
#elif defined PANO12_FOUND
#define HasPANO12 1
#endif

/* locate of the xrc files, as defined during configuration */
#define INSTALL_LOCALE_DIR "${INSTALL_LOCALE_DIR}"

/* Location for XRC files and other data, as defined during configuration*/
#define INSTALL_XRC_DIR "${INSTALL_XRC_DIR}/"


#endif
