// -*- c-basic-offset: 4 -*-
/** @file 
*
*  @author Ippei UKAI <ippei_ukai@mac.com>
*
*  $Id: $
*
*  This is free software; you can redistribute it and/or
*  modify it under the terms of the GNU General Public
*  License as published by the Free Software Foundation; either
*  version 2 of the License, or (at your option) any later version.
*
*  This software is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this software; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.
*
*  Hereby the author, Ippei UKAI, grant the license of this particular file to
*  be relaxed to the GNU Lesser General Public License as published by the Free
*  Software Foundation; either version 2 of the License, or (at your option)
*  any later version. Please note however that when the file is linked to or
*  compiled with other files in this library, the GNU General Public License as
*  mentioned above is likely to restrict the terms of use further.
*
*/

#include "QADocument.qt.h"


namespace QtAppBase {

    
QADocument::QADocument(QADocumentController* parent)
  : QObject(parent), m_documentController(parent), m_fileType(QADefaultFiletype()), m_undoStack(this)
{
}


QADocument::openFile(QString filePath, QAFiletype fileType)
{
    m_fileInfo = QFileInfo(filePath);
    m_fileInfo.makeAbsolute();
    //[TODO] check m_fileInfo.exists()
    m_fileType = fileType;
}

QADocument::show()
{
    QWidget* window = mainDocumentWindow();
    //[TODO] check window != NULL
    window->show();
    window->raise();
    window->activateWindow();
}
    
void QADocument::setDocumentController(QADocumentController* controller)
{
    setParent(controller);
    m_documentController = controller;
}

QADocumentController* QADocument::documentController()
{
    return m_documentController;
}

QString QADocument::filePath() const
{
    return o_fileInfo.absoluteFilePath();
}

void QADocument::setFilePath(QString newPath)
{
    o_fileInfo.setFile(newPath);
    o_fileInfo.makeAbsolute();
}

bool QADocument::fileName() const
{
    return o_fileInfo.fileName();
}

bool QADocument::fileExsists() const
{
    return o_fileInfo.exists();
}

QAFiletype QADocument::filetype() const
{
    return m_fileType;
}

void QADocument::setFileType(const QAFiletype& filetype)
{
    m_fileType = filetype;
}

bool QADocument::save()
{
    // if empty filepath, return saveAs
    // else if not file exisits, ask and return saveAs or continue
    // saveToPath
    // if failed, ask and return saveAs or false
}

bool QADocument::saveAs()
{
    // QFileDialog::getSaveFileName
    // saveToPath
}

bool QADocument::revert()
{
    //if isModified, ask to confirm
    //if empty filepath, reset and return
    //else if file exisits, revertDocumentToFile
    //else return false
}

bool QADocument::askAndClose(bool cancellable)
{
    //if isModified, ask save or close
    //call save or close
}

bool isModified()
{
    //documentData -> isDirty
}

bool QADocument::saveToPath(QString path, QAFiletype filetype)
{
    //open outputStream
    //documentData -> write
    //return whether succeeded
    //signal modifiedStatusChanged
}

bool QADocument::revertDocumentToFile(QString path)
{
    //reset
    //documentData -> read
    //signal modifiedStatusChanged
}

void QADocument::close()
{
    // nothing to do in this level?
}

virtual void signalDataChange()
{
    //signal documentDataChanged
}

QUndoStack* QADocument::undoStack()
{
    return &m_undoStack;
}