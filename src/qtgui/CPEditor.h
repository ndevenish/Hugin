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
#include <set>
#include <qvbox.h>

class QTabBar;
class QVBox;
class QScrollView;
class CPImageDisplay;
class CPListView;
class QComboBox;
class QLineEdit;
class QLabel;
class QProgressBar;

namespace PT {
    class Panorama;
    class ControlPoint;
}

/*
template<class _Pair>
struct select1st : public std::unary_function<_Pair,
  typename _Pair::first_type> {
  typename _Pair::first_type& operator()(_Pair& __x) const {
    return __x.first;
  }
  const typename _Pair::first_type& operator()(const _Pair& __x) const {
    return __x.first;
  }
};
*/

/** Control Point Editor Widget
 *
 *  This widget allows the user to edit the control points of a panorama.
 *
 *  use cases:
 *    - add point (clicking on both images)
 *    - remove point (selected point)
 *    - edit point (position, properties)
 *    -
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
    CPEditor(PT::Panorama & pano, QProgressBar & progBar, QWidget* parent, const char* name = 0, WFlags fl = 0);

    /** dtor.
     */
    virtual ~CPEditor();

public slots:

    // slots to be called from the view
    void createNewPointFirst(QPoint);
    void createNewPointSecond(QPoint);
    void findRegionFromFirst(QRect);
    void findRegionFromSecond(QRect);
    /** select a point through a global nr */
    void selectGlobalPoint(unsigned int globalnr);

    void setFirstImage(unsigned int img);
    void setSecondImage(unsigned int img);

    // slots for the model
    void updateView();

protected:
    /** called when a image or control points are changed, updates
     *  control points in the subwidgets.
     */
    void updateDialog();

    bool globalPNr2LocalPNr(unsigned int & localNr, unsigned int globalNr) const;


protected slots:
    void setFirstImageFromTab(int tabId);
    void setSecondImageFromTab(int tabId);

    // the point nr is an index into currentPoints, not the global ctrl point nr.
    void moveFirstPoint(unsigned int nr, QPoint);
    void moveSecondPoint(unsigned int nr, QPoint);

    /// select a point through the local point nr.
    void selectLocalPoint(unsigned int);

    /// update point from x1,y1,x2,y2 and type edit widgets.
    void applyEditedPoint();
    
    /// remove a selected point
    void removePoint();

private:

    PT::Panorama & pano;
    QProgressBar & progBar;

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

    CPListView * ctrlPointLV;
    QLineEdit * x1Edit;
    QLineEdit * y1Edit;
    QLineEdit * x2Edit;
    QLineEdit * y2Edit;

    QComboBox * alignCombo;
    QComboBox * imageProcCombo;
    QLineEdit * normEdit;

    /*
    QScrollView * srcWindow;
    QLabel * srcLabel;

    QScrollView * templWindow;
    QLabel * templLabel;

    QScrollView * resultWindow;
    QLabel * resultLabel;
    */
    
    typedef std::pair<unsigned int, PT::ControlPoint> CPoint;
    std::vector<CPoint> currentPoints;
    // this set contains all points that are switched, in local point
    // numbers
    std::set<unsigned int> mirroredPoints;
};


#endif // _CPEDITOR_H
