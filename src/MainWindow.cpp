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

#include "MainWindow.h"
#include "InputImages.h"
#include "PanoOptionsWidget.h"
#include "CPEditor.h"
#include "OptimizerVarWidget.h"
#include "Panorama/PanoCommand.h"

using namespace std;
using namespace PT;

MainWindow::MainWindow()
{
    cerr << "MainWindow created" << endl;

    tabWidget = new QTabWidget(this, "tabWidget");
    setCentralWidget( tabWidget );

    inputImages = new InputImages(pano, this, "inputImages1" );
    tabWidget->insertTab( inputImages, tr("Input Images") );

//    tab_2 = new QWidget( tabWidget, "tab_2" );
    cpEditor = new CPEditor(pano,tabWidget);
    tabWidget->insertTab( cpEditor, tr("Control Points") );

    optimizerWidget = new OptimizerVarWidget( pano, tabWidget, "optimizerVarWidget1" );
    tabWidget->insertTab( optimizerWidget, tr("Optimizer") );

    panoOptionsWidget = new PanoOptionsWidget(pano, this, "pano options");
    tabWidget->insertTab( panoOptionsWidget, tr("Panorama"));
    
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
            GlobalCmdHist::getInstance().addCommand(
                new PT::AddImagesCmd(pano,files)
                );
        }
    } else {
        qWarning("MainWindow: unknown dropEvent");
    }
}

void MainWindow::saveProject(QString file)
{
    QDomDocument doc( "Hugin_Project" );
    QDomElement root = doc.createElement( "Project" );
    doc.appendChild( root );

    QDomElement p = pano.toXML(doc);
    root.appendChild(p);

    QString xml = doc.toString();

    std::ofstream of(file);
    of << xml << std::endl;
}

void MainWindow::fileOpen()
{
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
}


void MainWindow::fileNew()
{
    GlobalCmdHist::getInstance().addCommand(
        new PT::AddCtrlPointCmd(pano,ControlPoint(pano.getImage(0),10,11,
                                                  pano.getImage(0),20,21,
                                                  ControlPoint::X)
            )
        );
    std::cerr << "file new not implemented" << std::endl;
}
