FILE(READ "${PROCESS_DIR}/Accessor_preprocessed.txt" SrcPanoImage_Accessor_Preprocessed)
CONFIGURE_FILE(../hugin_base/panodata/SrcPanoImage.h ${PROCESS_DIR}/hsi_SrcPanoImage.h)

