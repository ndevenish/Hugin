// -*- c-basic-offset: 4 -*-
/** @file MainWindow.cpp
 *
 *  @brief implementation of MainWindow Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <iostream>
#include <qcheckbox.h>
#include <qdragobject.h>
#include <qstringlist.h>
#include <qtabwidget.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qmessagebox.h>
#include <qstatusbar.h>
#include <qprogressbar.h>

#include "MainWindow.h"
#include "InputImages.h"
#include "PanoOptionsWidget.h"
#include "CPEditor.h"
#include "OptimizerVarWidget.h"
#include "Panorama/PanoCommand.h"
#include "CommandHistory.h"

using namespace std;
using namespace PT;

MainWindow::MainWindow()
{
    cerr << "MainWindow created" << endl;

    pano.setObserver(this);
    tabWidget = new QTabWidget(this, "tabWidget");
    setCentralWidget( tabWidget );

    inputImages = new InputImages(pano, this, "inputImages1" );
    tabWidget->insertTab( inputImages, tr("Input Images") );

    // statusbar
    progressbar = new QProgressBar(this);
    progressbar->setTotalSteps(100);
    statusBar()->addWidget(progressbar);

    // update widgets if the pano object is changed
    connect( this, SIGNAL(modelChanged()), inputImages, SLOT(updateView()));

//    tab_2 = new QWidget( tabWidget, "tab_2" );
    cpEditor = new CPEditor(pano,*progressbar,tabWidget);
    tabWidget->insertTab( cpEditor, tr("Control Points") );
    connect( this, SIGNAL(modelChanged()), cpEditor, SLOT(updateView()));

    optimizerWidget = new OptimizerVarWidget( pano, tabWidget, "optimizerVarWidget1" );
    tabWidget->insertTab( optimizerWidget, tr("Optimizer") );
    connect(this, SIGNAL(modelChanged()), optimizerWidget, SLOT(updateView()));


    panoOptionsWidget = new PanoOptionsWidget(pano, this, "pano options");
    tabWidget->insertTab( panoOptionsWidget, tr("Panorama"));
    connect(this, SIGNAL(modelChanged()), panoOptionsWidget, SLOT(updateView()));


}

MainWindow::~MainWindow()
{
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    event->accept( QUriDrag::canDecode(event) );
}


void MainWindow::dropEvent(QDropEvent* event)
{
    QStringList files;
    if ( QUriDrag::decodeLocalFiles(event, files) ) {
        if (files.size() > 0) {
            vector<string> filesv;
            for(unsigned int i = 0; i < files.size(); i++) {
                filesv.push_back(files[i].ascii());
            }
            GlobalCmdHist::getInstance().addCommand(
                new PT::AddImagesCmd(pano,filesv)
                );
        }
    } else {
        qWarning("MainWindow: unknown dropEvent");
    }
}

void MainWindow::saveProject(QString file)
{
    qDebug("FIXME save not implemnted: %s",file.ascii());
    /*
    QDomDocument doc( "Hugin_Project" );
    QDomElement root = doc.createElement( "Project" );
    doc.appendChild( root );

    QDomElement p = pano.toXML(doc);
    root.appendChild(p);

    QString xml = doc.toString();

    std::ofstream of(file);
    of << xml << std::endl;
    */
}

void MainWindow::fileOpen()
{
#if 0
    QString filename;
    filename = QFileDialog::getOpenFileName (
        QString::null,
        "PA project (*.pap)",
        this,
        "openProjectDialog",
        "Open Project");
    QDomDocument doc( "mydocument" );
    QFile file( filename );
    if ( !file.open( IO_ReadOnly ) ) {
        qWarning("Could not open file %s", filename.ascii());
        return;
    }
    QString errorMsg;
    int errorLine, errorColumn;
    if ( !doc.setContent( &file,&errorMsg, &errorLine, &errorColumn ) ) {
        qWarning("XML parsing failed...");
        QMessageBox::warning(this,"XML error while reading project file",
                             QString("File: ") + filename + " at line " +
                             QString::number(errorLine) + " Column: " +
                             QString::number(errorColumn) + ":\r\n" +
                             errorMsg,
                             QMessageBox::Ok,
                             QMessageBox::NoButton);
        return;
    }
    file.close();
    QDomNodeList panoNodes = doc.elementsByTagName("panorama");
    if (panoNodes.length() != 1) {
        qWarning("invalid xml file, no or multiple <panorama> tags found");
        return;
    }
    pano.setFromXML(panoNodes.item(0));
    projectFile = filename;
#endif
}


void MainWindow::fileNew()
{
    DEBUG_DEBUG("file new not implemented");
}


void MainWindow::panoramaChanged()
{
    emit(modelChanged());
}
