// -*- c-basic-offset: 4 -*-
/** @file InputImages.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _INPUTIMAGES_H
#define _INPUTIMAGES_H

#include <qlistview.h>

#include "inputimagesbase.h"


namespace PT {
    class Panorama;
    class PanoImage;
}

/// display a listview item..
class PanoImageLVItem : public QListViewItem
{
public:
    PanoImageLVItem(QListView * parent, PT::PanoImage &img);
    virtual ~PanoImageLVItem();

    // return right text
    QString text(int column) const ;

    PT::PanoImage & getImage()
        { return image; };

    unsigned int getNr() const;
private:
    PT::PanoImage & image;
};

//=============================================================================
//=============================================================================


/** Widget to edit input images.
 *
 */
class InputImages : public InputImagesBase
{
    Q_OBJECT
public:

    /** ctor.
     */
    InputImages(PT::Panorama & pano, QWidget* parent = 0, const char* name = 0, WFlags fl = 0);

    /** dtor.
     */
    virtual ~InputImages();

public slots:
    // actions, initiated by the user
    void addImage();
    void removeImage();
    void editLens();
    void setCommonLens(bool common);

    // receive notifications
    void imageRemoved(unsigned int img);
    void imageAdded(unsigned int img);
private:
    PT::Panorama & pano;
};



#endif // _INPUTIMAGES_H
