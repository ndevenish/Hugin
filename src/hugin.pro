SOURCES	+= main.cpp \
	MainWindow.cpp \
	LensDialog.cpp \
	OptimizerVarWidget.cpp \
	InputImages.cpp \
	Command.cpp \
	PanoOptionsWidget.cpp \
	CPListView.cpp \
        CPImageDisplay.cpp \
        CPEditor.cpp

HEADERS	+= MainWindow.h \
	LensDialog.h \
	OptimizerVarWidget.h \
	InputImages.h \
	Command.h \
	PanoOptionsWidget.h \
	CPListView.h \
        CPImageDisplay.h \
        CPEditor.h

# Panorama subdir sources.
SOURCES   += Panorama/Panorama.cpp \
             Panorama/PanoImage.cpp

HEADERS   += Panorama/Panorama.h \
             Panorama/PanoImage.h \
             Panorama/PanoCommand.h


unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}


FORMS	= mainwindowbase.ui \
	lensdialogbase.ui \
	optimizervarwidgetbase.ui \
	inputimagesbase.ui \
	panooptionsbase.ui
IMAGES	= images/filenew \
	images/fileopen \
	images/filesave \
	images/print \
	images/undo \
	images/redo \
	images/editcut \
	images/editcopy \
	images/editpaste \
	images/searchfind
TEMPLATE	=app
CONFIG	+= qt warn_on debug thread rtti
DEFINES	+= QT_CLEAN_NAMESPACE
unix:LIBS	+= -lexif -ljpeg
LANGUAGE	= C++
