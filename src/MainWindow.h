// -*- c-basic-offset: 4 -*-
/** @file MainWindow.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _MAINWINDOW_H
#define _MAINWINDOW_H

class QStringList;

#include <iostream>
#include <fstream>
#include <qfiledialog.h>
#include <qlistview.h>
#include <qdom.h>

#include "mainwindowbase.h"
//#include "InputImages.h"

class QTabWidget;
class InputImages;
class OptimizerVarWidget;
class PanoOptionsWidget;
class ControlPointListView;
class CPEditor;

/** Main Window.
 *
 *  contains the main window widgets and the logic how to control
 *  the the Panorama class.
 */
class MainWindow : public MainWindowBase
{
    Q_OBJECT
public:

    /** ctor.
     */
    MainWindow();

    /** dtor.
     */
    virtual ~MainWindow();


    void fileOpen();

    void fileSave()
        {
            if (projectFile == "") {
                fileSaveAs();
            } else {
                saveProject(projectFile);
            }
        }

    void fileSaveAs()
        {
            projectFile = QFileDialog::getSaveFileName (
                QString::null,
                "PA project (*.pap)",
                this,
                "Save project",
                "Save Project as");
            saveProject(projectFile);
        }

    void fileNew();

    void fileExit()
        {
            std::cerr << "file exit not implemented" << std::endl;
        }

    void helpIndex()
        {
            std::cerr << "help index not implemented" << std::endl;
        }

    void helpContents()
        {
            std::cerr << "help contents not implemented" << std::endl;
        }

    void helpAbout()
        {
            std::cerr << "about not implemented" << std::endl;
        }

    void optimizePanorama()
        {
            qDebug("optimize not implemented");
            pano.optimize();
        }

    void previewPanorama()
        {
            std::cerr << "previewPanorama not implemented" << std::endl;
            //pano.stitch(...);
        }

    void createPanorama()
        {
            std::cerr << "createPanorama not implemented" << std::endl;
            //pano.stitch(...);
        }

    void editLens();

    void setReferencePoint()
        {
            qDebug("MW:setReferencePoint not implemented");
        }

    // dnd stuff. accept image files.
    void dragEnterEvent(QDragEnterEvent* event);

    void dropEvent(QDropEvent* event);

    void saveProject(QString filename);

private:

    QTabWidget* tabWidget;
    InputImages * inputImages;
    CPEditor *cpEditor;
    OptimizerVarWidget * optimizerWidget;
    PanoOptionsWidget * panoOptionsWidget;

    QString projectFile;

    QWidget* tab_2;
    QWidget* tab_4;

};

#endif // _MAINWINDOW_H
