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

class QADocument : public QObject
{
    Q_OBJECT
    
public:
    QADocument(QADocumentController* parent = NULL);
    QADocument(QString filePath, QADocumentController* parent = NULL);
    QADocument(QString filePath, QAFiletype fileType, QADocumentController* parent = NULL);
    virtual ~QADocument() {}
    
public:
    virtual QADocumentTemplate* documentTemplate();
    
public:
    virtual void setDocumentController(QADocumentController* controller);
    virtual QADocumentController* documentController();
private:
    QADocumentController* m_documentController;
    
public:
    virtual QString filePath() const;
    virtual void setFilePath(QString newPath);
    virtual bool fileName() const;
    virtual bool fileExsists() const;
private:
    QFileInfo m_fileInfo;

public:
    virtual QAFiletype filetype() const;
    virtual void setFileType(QAFiletype filetype);
private:
    QAFiletype m_fileType;
    
public:
    virtual QWidget* mainDocumentWindow() =0;
    
public slots:
    virtual bool save();
    virtual bool saveAs();
protected:
    virtual bool saveToPath(QString path, QAFiletype filetype);

public slots:
    virtual bool revert();
protected:
    virtual bool revertDocumentToFile(QString path);

public slots:
    virtual bool askAndClose(bool cancellable = true);
public:
    virtual void close();

public:
    virtual AppBase::DocumentData* documentData() =0;
    virtual QUndoStack* undoStack();
};

} //namespace






// ----------------------------------
virtual void QADocument::setDocumentController(QADocumentController* controller)
{
    m_documentController = controller;
}

virtual QADocumentController* QADocument::documentController()
{
    return m_documentController;
}

virtual QString QADocument::filePath() const
{
    return o_fileInfo.absoluteFilePath();
}

virtual void QADocument::setFilePath(QString newPath)
{
    o_fileInfo.setFile(newPath);
    o_fileInfo.makeAbsolute();
}

virtual bool QADocument::fileName() const
{
    return o_fileInfo.fileName();
}

virtual bool QADocument::fileExsists() const
{
    return o_fileInfo.exists();
}