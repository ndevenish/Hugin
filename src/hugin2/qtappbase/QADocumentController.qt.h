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
    

class QADocumentController : public QObject
{
    
public:
    QADocumentController(QObject* parent = NULL);
    virtual ~QADocumentController() {};
    
public:
    static QADocumentController* sharedDocumentController();
    
public slot:
    void newDocument();
    void openDocumentWithDialog();
    void importDocumentWithDialog();
    void saveAllDocuments();
    bool askAndCloseAll(bool& cancellable const);
public:
    void closeAll();
    bool hasEditedDocuments();
    QADocument::ReadWriteError openUntitledDocument();
    QADocument::ReadWriteError openDocumentWithPath(QString& filepath const);
protected:
    QADocument* newUntitledDocumentOfType(QAFiletype& type const, QADocument::ReadWriteError* error = NULL);
    QADocument* newDocumentFromFile(QFileInfo& fileinfo const, QAFiletype& type const, QADocument::ReadWriteError* error = NULL);
    QADocument* newDocumentFromFile(QString& filepath const, QAFiletype& type const, QADocument::ReadWriteError* error = NULL);
    
public:
    QFileInfoList recentDocuments();
    int maximumRecentDocumentCount();
public slot:
    void clearRecentDocuments();
public signal:
    void updateRecentDocuments();

public:
    QAFiletype defaultType() const;
    void setDefaultType(const QAFiletype& filetype);
    void addDocumentTemplate(const QADocumentTemplate& docTemplate);
    QList<QADocumentTemplate> documentTemplates(); const
    QADocumentTemplate documentTemplateForFiletype(const QAFiletype& filetype) const;
    QADocumentTemplate documentTemplateForFile(const QFileInfo& fileinfo) const;
    QADocumentTemplate documentTemplateForFile(const QString& filepath) const;
};

    
} //namespace
