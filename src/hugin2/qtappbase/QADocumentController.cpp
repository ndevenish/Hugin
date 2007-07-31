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


namespace QtAppBase {
    

QADocumentController::QADocumentController(QObject* parent);

QADocumentController* QADocumentController::sharedDocumentController();


void QADocumentController::QADocumentController::newDocument();
void QADocumentController::openDocumentWithDialog();
void QADocumentController::importDocumentWithDialog();
void QADocumentController::saveAllDocuments();
bool QADocumentController::askAndCloseAll(bool& cancellable const);

void QADocumentController::closeAll();
bool QADocumentController::hasEditedDocuments();
QADocument::ReadWriteError QADocumentController::openUntitledDocument();
QADocument::ReadWriteError QADocumentController::openDocumentWithPath(QString& filepath const);

QADocument* QADocumentController::newUntitledDocumentOfType(QAFiletype& type const, QADocument::ReadWriteError* error = NULL);
QADocument* QADocumentController::newDocumentFromFile(QFileInfo& fileinfo const, QAFiletype& type const, QADocument::ReadWriteError* error = NULL);
QADocument* QADocumentController::newDocumentFromFile(QString& filepath const, QAFiletype& type const, QADocument::ReadWriteError* error = NULL);


QFileInfoList QADocumentController::recentDocuments();
int QADocumentController::maximumRecentDocumentCount();
void QADocumentController::clearRecentDocuments();


QAFiletype QADocumentController::defaultType() const;
void QADocumentController::setDefaultType(const QAFiletype& filetype);
void QADocumentController::addDocumentTemplate(const QADocumentTemplate& docTemplate);
QList<QADocumentTemplate> QADocumentController::documentTemplates(); const
QADocumentTemplate QADocumentController::documentTemplateForFiletype(const QAFiletype& filetype) const;
QADocumentTemplate QADocumentController::documentTemplateForFile(const QFileInfo& fileinfo) const;
QADocumentTemplate QADocumentController::documentTemplateForFile(const QString& filepath) const;


    
} //namespace
