TEMPLATE	=app
#CONFIG	+= qt warn_on release thread rtti warn
CONFIG	+= qt warn_on debug thread rtti warn
DEFINES	+= QT_CLEAN_NAMESPACE
LANGUAGE	= C++


SOURCES	+= main.cpp \
	MainWindow.cpp \
	LensDialog.cpp \
	OptimizerVarWidget.cpp \
	InputImages.cpp \
	CommandHistory.cpp \
	PanoOptionsWidget.cpp \
	CPListView.cpp \
        CPImageDisplay.cpp \
        CPEditor.cpp \
        QTImageCache.cpp \
        ImageTransforms.cpp

HEADERS	+= MainWindow.h \
	LensDialog.h \
	OptimizerVarWidget.h \
	InputImages.h \
	Command.h \
	CommandHistory.h \
	PanoOptionsWidget.h \
	CPListView.h \
        CPImageDisplay.h \
        CPEditor.h \
        utils.h \
        stl_utils.h \
        QTImageCache.h \
        ImageTransforms.h

# Panorama subdir sources.
SOURCES   += Panorama/Panorama.cpp \
             Panorama/PanoImage.cpp \
             Panorama/Process.cpp 

HEADERS   += Panorama/Panorama.h \
             Panorama/PanoImage.h \
             Panorama/Process.h \
             Panorama/PanoCommand.h 


unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
  DEFINES += UNIX
  LIBS	+= -lexif -ljpeg
  LIBS += `Magick++-config --ldflags --libs`
  QMAKE_CXXFLAGS_DEBUG += -O2
#  CFLAGS += -O2 `Magick++-config --cppflags --cxxflags`

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
