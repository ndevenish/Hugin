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

class HuginDocumentViewManager : public QObject
{
    Q_OBJECT
    
public:
    HuginDocumentViewManager(HuginDocumentViewLayoutInstance* layout, QADocument* parent = NULL);
    virtual ~HuginDocumentViewManager() {}
    
public:
    virtual void addModuleView(QWidget* view, const QString& viewID);
    virtual void addUtilityViewToModuleView(const QString& moduleViewID, QWidget* view, const QString& utilViewID);
    virtual void layoutViews();
    virtual QWidget* viewForID(QString viewID);
    virtual QMap<QString, QWidget*> allModuleViews();
    virtual QMap<QString, QWidget*> allUtilityViewsForView(QString viewID);
        
public slot:
    virtual void raiseDocumentWindows();
    virtual void showDocumentWindows();
    virtual void closeDocumentWindows();
    virtual void activateDocumentWindows();
    virtual void raiseView(QString viewID);
    virtual void lockView(const QString& viewID);
    virtual void lockAllViews();
    virtual void unlockView(const QString& viewID);
    virtual void unlockAllViews();
    virtual void activateUtilityView(const QString& utilViewID);
    virtual void minimizeUtilityView(const QString& utilViewID);
public:
    virtual bool isLocked(const QString& viewID);
    
public:
    virtual int execDocumentModalDialog(QDialog* dialog);
    virtual int showDocumentModalDialog(QDialog* dialog);
    
};
    

} //namespace