// -*- c-basic-offset: 4 -*-
/** @file CPImageDisplay.h
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

#ifndef _CPIMAGEDISPLAY_H
#define _CPIMAGEDISPLAY_H

#include <vector>
#include <qwidget.h>
#include <qpoint.h>
#include <qpixmap.h>

namespace PT {
    class ControlPoint;
}

/** Display images and their control points
 *
 *  This class is used to display an Image and
 *  control points.
 *
 *  features (unfinished are preceeded with FIXME :
 *    - display an Image (FIXME)
 *       the loaded images might also be needed by other parts of the
 *       program, so they are not owned by the Display. (the image itself is
 *       part of the model) (which should use smartpoints for it, so that
 *       big panoramas can be edited without wasting a lot of memory).
 *
 *    - display control points in different colors
 *    - display of a selected point.
 *    - manipulation of control points (drag them around)
 *    - slowdown key for the mouse movement (FIXME)
 *    - report clicks on the image.
 *    - ability to select regions.
 *    - show a magnification glass (FIXME)
 *    - display image at multiple zoom settings. (FIXME)
 *    - select mode for some positions, maybe useful when a (FIXME)
 *      semi-automatic  matcher finds similar areas, the user can
 *      then choose between the most common ones.
 *
 *
 */
class CPImageDisplay : public QWidget
{
  Q_OBJECT
public:

    /** ctor.
     */
    CPImageDisplay(QWidget* parent = 0, const char* name = 0, WFlags fl = 0);
    CPImageDisplay();

    /** dtor.
     */
    virtual ~CPImageDisplay();

    // FIXME use a lazy evaluation QPixmap here?
    void setPixmap (const QPixmap & img);

    /// control point inside this image
    void setCtrlPoints(const std::vector<QPoint> & points);

    virtual QSizePolicy sizePolicy() const;
    virtual QSize sizeHint () const;
    virtual QSize minimumSizeHint () const;

public slots:
    /// clear new point
    void clearNewPoint();

    void selectPoint(unsigned int);


signals:
    /// emited whenever a point is selected
    void pointSelected(unsigned int);

    /// when a point is moved (mouse button released)
    void pointChanged(unsigned int, QPoint);

    /// during the point movement, can be used to track
    void pointMoved(unsigned int, QPoint);

    /// whenever a possible new point was created or moved(mouse button released)
    void newPointChanged(QPoint);

    /// track the new points movement
    void newPointMoved(QPoint);

    /// whenever a region was selected
    void regionSelected(QRect);
    // emited whenever the window has been scrolled..
    //void signalScrolled


protected:


    void paintEvent(QPaintEvent *);
    void mouseMoveEvent (QMouseEvent *);
    void mousePressEvent (QMouseEvent *);
    void mouseReleaseEvent (QMouseEvent *);

    // draw a control point
    void drawPoint(QPainter & p, const QPoint & point, const QColor & color) const;

private:

    QPixmap pixmap;
    std::vector<QPoint> points;

    // this is only valid during MOVE_POINT
    unsigned int selectedPointNr;
    // valid during MOVE_POINT and CREATE_POINT
    QPoint point;
    bool drawNewPoint;
    QPoint newPoint;

    // only valid during SELECT_REGION
    QRect region;
    // state of widget (selection modes etc)
    // select region can also be used to just click...

    /**  state machine for selection process:
     *
     *   states:
     *    - NO_SELECTION nothing selected
     *        - KNOWN_POINT_SELECTED
     *          - mouse down on known point
     *          - set from outside
     *        - NEW_POINT_SELECTED
     *          - mouse down on new point (if one exists)
     *        - REGION
     *          - mouse down on unidentifed field
     *
     *    - KNOWN_POINT_SELECTED,
     *       a known point is selected and can be moved around.
     *       clients are notified about the movement, points can also
     *       be switched.
     *
     *        - NEW_POINT_SELECTED
     *          - mouse down on new point (if one exists)
     *        - REGION
     *          - mouse down on unidentifed field
     *
     *    - REGION the user can draw a bounding box.
     *        - NEW_POINT_SELECTED
     *          - mouse up on same point as mouse down.
     *        - NO_SELECTION
     *          - mouse up on different point, report selection
     *
     *    - NEW_POINT_SELECTED (can move new point), mouse up reports change,
     *           movement can be tracked
     *        - KNOWN_POINT_SELECTED
     *          - mouse down on known point
     *          - programatic change
     *        - REGION
     *          - mouse down on free space
     *
     */
    enum EditorState {NO_SELECTION, KNOWN_POINT_SELECTED, NEW_POINT_SELECTED, SELECT_REGION};
    EditorState editState;

    // colors for the different points
    std::vector<QColor> pointColors;
    double scaleFactor;

    /// check if p is over a known point, if it is, pointNr contains
    /// the point
    EditorState isOccupied(const QPoint &p, unsigned int & pointNr) const;

};



#endif // _CPIMAGEDISPLAY_H
