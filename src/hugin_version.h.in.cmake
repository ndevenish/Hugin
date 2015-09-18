#ifndef __HUGIN_VERSION_H__

#define VERSION_MAJOR ${V_MAJOR}
#define VERSION_MINOR ${V_MINOR}
#define VERSION_PATCH ${V_PATCH}
#define HUGIN_WC_REVISION ${HUGIN_WC_REVISION}
#define HUGIN_API_VERSION "${V_MAJOR}.${V_MINOR}"

#if defined _WIN32 || defined __APPLE__
#define PACKAGE_VERSION "${HUGIN_PACKAGE_VERSION} built by ${HUGIN_BUILDER}"
#define DISPLAY_VERSION "${DISPLAY_VERSION} built by ${HUGIN_BUILDER}"
#else
#define PACKAGE_VERSION "${HUGIN_PACKAGE_VERSION}"
#define DISPLAY_VERSION "${DISPLAY_VERSION}"
#endif

/* this is a hg checkout, tag is as such
 * all builds from HG will be considered development versions
 */
#cmakedefine HUGIN_DEVELOPMENT_VERSION

#endif
