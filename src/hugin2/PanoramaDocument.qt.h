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


namespace HuginQt {

    
class PanoramaDocumentTemplate : public QADocumentTemplate
{
public:
    PanoramaDocumentTemplate(QObject* parent = NULL);
    virtual ~PanoramaDocumentTemplate() {};
    
public:
    virtual QADocument* makeNewDocument(QString filePath, QAFiletype fileType, QObject* parent = NULL) =0;
    
public:
    virtual QAFiletype defaultFiletype() const;    // PanoToolsOptimizerScriptFiletype
    virtual QAFiletypeSet readableTypes() const;    // PanoToolsOptimizerScriptFiletype
    virtual QAFiletypeSet importableTypes() const;    // {}
    virtual bool isEditable() const;    // true
    
public:
    virtual QString displayName();  // "Hugin Project (PTOptimiser Script)"
    virtual QString documentTemplateID(); // "HuginQt::PanoramaDocument"
};
    

class PanoramaDocument : public HuginDocument, public HuginBase::PanoramaObserver
{
    Q_OBJECT
    
public:
    PanoramaDocument(QADocumentController* parent = NULL);
    PanoramaDocument(QString filePath, QADocumentController* parent = NULL);
    PanoramaDocument(QString filePath, QAFiletype fileType, QADocumentController* parent = NULL);
    virtual ~PanoramaDocument() {}
protected:
    bool setContentsToFile(const QString& filePath, const QAFiletype& fileType);
    
public:
    virtual QADocumentTemplate* documentTemplate();
private:
    static PanoramaDocumentTemplate m_documentTemplate;

protected:
    virtual bool saveToPath(QString path, QAFiletype filetype);
    virtual bool revertDocumentToFile(QString path);
    
public:
    virtual AppBase::DocumentData* documentData();
    virtual HuginBase::PanoramaData* panoramaData();
protected:
    HuginBase::Panorama o_panorama;
  
public:
    virtual void panoramaChanged(PanoramaData &pano);
    virtual void panoramaImagesChanged(PanoramaData& pano, const HuginBase::UIntSet& changed);
public signal:
    virtual void documentDataChanged();
    virtual void panoramaImageChanged(const HuginBase::UIntSet& changed);
        
};


} //namespace