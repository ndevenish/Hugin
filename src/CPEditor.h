// -*- c-basic-offset: 4 -*-
/** @file CPEditor.h
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

#ifndef _CPEDITOR_H
#define _CPEDITOR_H

#include <vector>
#include <qvbox.h>

class QTabBar;
class QVBox;
class QScrollView;
class CPImageDisplay;
class CPListView;

namespace PT {
    class Panorama;
    class ControlPoint;
}

/** Control Point Editor Widget
 *
 *  This widget allows the user to edit the control points of a panorama.
 *
 *  use cases:
 *    - add point (clicking on both images)
 *    - remove point (selected point)
 *    - edit point (position, properties)
 *
 *  subwidgets:
 *    - It will contains two tab widgets with a number of CPImageDisplay's
 *    - control point list (only cp for the current 2 images).
 *    - widgets for control point properties edit.
 *
 */
class CPEditor : public QVBox
{
    Q_OBJECT
public:

    /** ctor.
     */
    CPEditor(PT::Panorama & pano, QWidget* parent, const char* name = 0, WFlags fl = 0);

    /** dtor.
     */
    virtual ~CPEditor();

public slots:

    // slots to be called from the view
    void moveFirstPoint(unsigned int, QPoint);
    void moveSecondPoint(unsigned int, QPoint);
    void createNewPointFirst(QPoint);
    void createNewPointSecond(QPoint);
    void findRegionFromFirst(QRect);
    void findRegionFromSecond(QRect);
    void selectPoint(unsigned int);

    // slots for the model
    void addImage(unsigned int img);
    void removeImage(unsigned int img);

    void addCtrlPoint(unsigned int point);
    void removeCtrlPoint(unsigned int point);

    void setFirstImage(unsigned int img);
    void setSecondImage(unsigned int img);

protected:
    /** called when a image or control points are changed, updates
     *  control points in the subwidgets.
     */
    void updateDialog();

protected slots:
    void setFirstImageFromTab(int tabId);
    void setSecondImageFromTab(int tabId);

private:

    PT::Panorama & pano;

    // the current images
    unsigned int firstImage;
    unsigned int secondImage;

    QPoint newPoint;

    enum CPCreationState { NO_POINT, FIRST_POINT, SECOND_POINT};
    CPCreationState cpCreationState;

    QTabBar * firstTab;
    QScrollView* lsv;
    CPImageDisplay * firstImgDisplay;

    QTabBar * secondTab;
    QScrollView* rsv;
    CPImageDisplay * secondImgDisplay;

    std::vector<PT::ControlPoint*> currentPoints;
    CPListView * ctrlPointLV;
};


#endif // _CPEDITOR_H
