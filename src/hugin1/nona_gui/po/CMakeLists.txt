
# automatically include all po files in the directory
FILE(GLOB PO_FILES *.po)

ADD_TRANSLATIONS(nona_gui ${PO_FILES})
