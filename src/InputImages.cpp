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

#include "LensDialog.h"
#include "InputImages.h"

using namespace std;

PanoImageLVItem::PanoImageLVItem(QListView * parent, PT::PanoImage &img)
    : QListViewItem(parent), image(img)
{
    qDebug("LVItem ctor, img: %s",img.getFilename().ascii());
}

PanoImageLVItem::~PanoImageLVItem()
{
    qDebug("LVItem dtor, img: %s",image.getFilename().ascii());
}


unsigned int PanoImageLVItem::getNr() const
{
    return image.getNr();
}

// return text for specific column
QString PanoImageLVItem::text(int column) const
{
    switch(column) {
    case 0:
        return QString::number(image.getNr());
        break;
    case 1:
        // XXXX needs to be changed for windows.
        return image.getFilename().section( '/', -1 );
        break;
    case 2:
        {
            QString ret;
            ret.sprintf("%dx%d", image.getWidth(), image.getHeight());
            return ret;
        }
        break;

    case 3:
        {
            double fov = image.getLens().HFOV;
            if (fov == 0.0) {
                return QString("invalid");
            }
            return QString::number(fov);
        }
        break;
    case 4:
        return QString::number(image.getPosition().yaw);
        break;
    case 5:
        return QString::number(image.getPosition().pitch);
        break;
    case 6:
        return QString::number(image.getPosition().roll);
        break;
    case 7:
        {
            QString ret;
            ret.sprintf("%4.3f, %4.3f, %4.3f",
                        image.getLens().a,
                        image.getLens().b,
                        image.getLens().c);
            return ret;
        }
        break;
    case 8:
        {
            QString ret;
            ret.sprintf("%4.3f, %4.3f", image.getLens().d, image.getLens().e);
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
    connect(&pano,SIGNAL(imageAdded(unsigned int)), this, SLOT(imageAdded(unsigned int)));
    connect(&pano,SIGNAL(imageRemoved(unsigned int)), this, SLOT(imageRemoved(unsigned int)));

    // update widgets if the pano object is changed
    connect( &pano, SIGNAL( imageChanged(unsigned int) ), imagesListView, SLOT(triggerUpdate()));
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
        if (files.size() > 0) {
            GlobalCmdHist::getInstance().addCommand(
                new PT::AddImagesCmd(pano,files)
                );
        }
    }
}


void InputImages::editLens()
{
    if (pano.hasCommonLens() && pano.getImages().size() > 0) {
        LensDialog dialog(pano.getImages()[0]->getLens(),this,"Lens Settings",true);
        if (dialog.exec() == QDialog::Accepted) {
            GlobalCmdHist::getInstance().addCommand(
                new PT::ChangeLensCmd(pano,0,dialog.getLens())
                );
        }
    } else {
        QListViewItem * item = imagesListView->selectedItem();
        PanoImageLVItem * img = dynamic_cast<PanoImageLVItem *>(item);
        if (img) {
            LensDialog dialog(img->getImage().getLens(),this,"Lens Settings",true);
            if (dialog.exec() == QDialog::Accepted) {
                GlobalCmdHist::getInstance().addCommand(
                    new PT::ChangeLensCmd(pano, img->getNr(), dialog.getLens())
                    );
            }
        }
    }
}

void InputImages::setCommonLens(bool common)
{
    GlobalCmdHist::getInstance().addCommand(
        new PT::SetCommonLensCmd(pano, common)
        );
}

void InputImages::imageAdded(unsigned int img)
{
    new PanoImageLVItem(imagesListView, *pano.getImage(img));
}


void InputImages::imageRemoved(unsigned int img)
{
    qDebug("InputImages::imageRemoved(%d), %d items in listview",img, imagesListView->childCount());
    QListViewItemIterator it( imagesListView );
    Q_ASSERT(it.current());
    for( ; it.current(); ++it) {
        PanoImageLVItem * pitem = static_cast<PanoImageLVItem*>(it.current());
        Q_ASSERT(pitem);
        if (pitem->getNr() ==  img) {
            delete pitem;
        }
    }
    imagesListView->triggerUpdate();
}


