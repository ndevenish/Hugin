// -*- c-basic-offset: 4 -*-
/** @file InputImages.cpp
 *
 *  @brief implementation of InputImages Class
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

#include <qfiledialog.h>
#include <qlabel.h>
#include <qpixmap.h>

#include "Panorama/Panorama.h"
#include "Panorama/PanoImage.h"
#include "Panorama/PanoCommand.h"
#include "CommandHistory.h"

#include "LensDialog.h"
#include "InputImages.h"

using namespace std;

PanoImageLVItem::PanoImageLVItem(QListView * parent, PT::Panorama &pano, unsigned int nr)
    : QListViewItem(parent), pano(pano),
      imgNr(nr)
{
}

PanoImageLVItem::~PanoImageLVItem()
{
}

// return text for specific column
QString PanoImageLVItem::text(int column) const
{
    const PT::ImageVariables & vars = pano.getVariable(imgNr);
    switch(column) {
    case 0:
        return QString::number(imgNr);
        break;
    case 1:
    {
        // XXXX needs to be changed for windows.
        string filename = pano.getImage(imgNr).getFilename();
        return filename.substr(filename.rfind('/'), string::npos ).c_str();
        break;
    }
    case 2:
        {
        QString ret;
        ret.sprintf("%dx%d", pano.getImage(imgNr).getWidth(), pano.getImage(imgNr).getHeight());
        return ret;
        }
        break;
    case 3:
        {
            double fov = vars.HFOV.getValue();
            if (fov == 0.0) {
                return QString("invalid");
            }
            return QString::number(fov);
        }
        break;
    case 4:
        return QString::number(vars.yaw.getValue());
        break;
    case 5:
        return QString::number(vars.pitch.getValue());
        break;
    case 6:
        return QString::number(vars.roll.getValue());
        break;
    case 7:
        {
            QString ret;
            ret.sprintf("%4.3f, %4.3f, %4.3f",
                        vars.a.getValue(),
                        vars.b.getValue(),
                        vars.c.getValue());
            return ret;
        }
        break;
    case 8:
        {
            QString ret;
            ret.sprintf("%4.3f, %4.3f", vars.d.getValue(), vars.e.getValue());
            return ret;
        }
        break;
    default:
        return QString::null;
    }
}


//=============================================================================
//=============================================================================

// my try at an image preview for QFileDialog, but it didn't work..
class Preview : public QLabel, public QFilePreview
{
public:
    Preview( QWidget *parent=0 ) : QLabel( parent ) {}

    void previewUrl( const QUrl &u )
        {
            QString path = u.path();
            qDebug("preview, url:%s, path: %s", u.toString().ascii(), path.ascii());
            QPixmap pix( path );
            if ( pix.isNull() )
                setText( "This is not a pixmap" );
            else
                setPixmap( pix );
        }
};


//=============================================================================
//=============================================================================


InputImages::InputImages(PT::Panorama & p, QWidget* parent,
                         const char* name, WFlags fl)
    : InputImagesBase(parent, name, fl),
      pano(p)
{

}


InputImages::~InputImages()
{
}


void InputImages::removeImage()
{
    QListViewItem * item = imagesListView->selectedItem();
    if (item !=0) {
        PanoImageLVItem * pitem = static_cast<PanoImageLVItem*>(item);
        // remove from panorama
        GlobalCmdHist::getInstance().addCommand(
            new PT::RemoveImageCmd(pano,pitem->getNr())
            );
    }
}


void InputImages::addImage()
{
    cerr << "mainwin add image" << endl;

//    Preview* p = new Preview;

    QFileDialog* fd = new QFileDialog( this );
    fd->setFilters(QString("Images (*.png *.jpg *.tiff *.PNG *.JPG *.TIFF);;All(*)"));
    fd->setMode(QFileDialog::ExistingFiles);
//    fd->setContentsPreviewEnabled( TRUE );
//    fd->setContentsPreview( p, p );
//    fd->setPreviewMode( QFileDialog::Contents );
    if ( fd->exec() == QDialog::Accepted ) {
        QStringList files = fd->selectedFiles();
        vector<string> filesv;
        for(unsigned int i = 0; i < files.size(); i++) {
            filesv.push_back(files[i].ascii());
        }
        if (files.size() > 0) {
            GlobalCmdHist::getInstance().addCommand(
                new PT::AddImagesCmd(pano,filesv)
                );
        }
    }
}


void InputImages::editLens()
{
    if (pano.getNrOfLenses() > 0) {

        LensDialog dialog(pano.getLens(0),this,"Lens Settings",true);
        if (dialog.exec() == QDialog::Accepted) {
            GlobalCmdHist::getInstance().addCommand(
                new PT::ChangeLensCmd(pano,0,dialog.getLens())
                );
        }
    }
}

void InputImages::updateView()
{
    DEBUG_DEBUG("InputImages::updateView()")
    // we don't know exaclty what has been changed..
    // remove all listview items and readd them.
    imagesListView->clear();
    unsigned int n = pano.getNrOfImages();
    for (unsigned int i = 0; i < n; i++) {
        new PanoImageLVItem(imagesListView,pano,i);
    }
    imagesListView->triggerUpdate();
}


