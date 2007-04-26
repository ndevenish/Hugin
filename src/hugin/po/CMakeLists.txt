
# automatically include all po files in the directory
FILE(GLOB PO_FILES *.po)

ADD_TRANSLATIONS(hugin ${PO_FILES})
