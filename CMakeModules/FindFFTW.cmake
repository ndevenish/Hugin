# - Find FFTW
# Find the fftw includes and library
#
#  FFTW_INCLUDE_DIR - where to find zlib.h, etc.
#  FFTW_LIBRARIES   - List of libraries when using zlib.
#  FFTW_FOUND       - True if zlib found.

IF (FFTW_INCLUDE_DIR)
  # Already in cache, be silent
  SET(FFTW_FIND_QUIETLY TRUE)
ENDIF()

FIND_PATH(FFTW_INCLUDE_DIR 
  NAMES fftw3.h
  PATHS /usr/local/include
        /usr/include
        ${SOURCE_BASE_DIR}/fftw-3.3.4/api
        ${SOURCE_BASE_DIR}/fftw-3.3.3/api
)

include(FindLibraryWithDebug)

find_library_with_debug(FFTW_LIBRARIES
  WIN32_DEBUG_POSTFIX d
  NAMES libfftw-3.3 fftw3
  PATHS ${SYSTEM_LIB_DIRS} 
        ${SOURCE_BASE_DIR}/fftw-3.3.4/fftw-3.3-libs/x64/Release
        ${SOURCE_BASE_DIR}/fftw-3.3.4/fftw-3.3-libs/x64
        ${SOURCE_BASE_DIR}/fftw-3.3.4/fftw-3.3-libs/
        ${SOURCE_BASE_DIR}/fftw-3.3.3/fftw-3.3-libs/x64/Release
        ${SOURCE_BASE_DIR}/fftw-3.3.3/fftw-3.3-libs/x64
        ${SOURCE_BASE_DIR}/fftw-3.3.3/fftw-3.3-libs/
)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFTW DEFAULT_MSG 
                                  FFTW_INCLUDE_DIR FFTW_LIBRARIES)

MARK_AS_ADVANCED(
  FFTW_LIBRARIES
  FFTW_INCLUDE_DIR
  )
