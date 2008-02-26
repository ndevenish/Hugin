#ifndef __HUGIN_VERSION_H__

#define VERSION_MAJOR ${V_MAJOR}
#define VERSION_MINOR ${V_MINOR}
#define VERSION_PATCH ${V_PATCH}
#define HUGIN_WC_REVISION ${HUGIN_WC_REVISION}

#define PACKAGE_VERSION "${HUGIN_PACKAGE_VERSION} built by ${HUGIN_BUILDER}"

/* this is an svn checkout, tag is as such
 * all builds from SVN will be considered development versions
 */
#cmakedefine HUGIN_DEVELOPMENT_VERSION

#endif
