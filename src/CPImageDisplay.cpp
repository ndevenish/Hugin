// -*- c-basic-offset: 4 -*-

/** @file CPImageDisplay.cpp
 *
 *  @brief implementation of CPImageDisplay Class
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

#include <qpixmap.h>
#include <qpainter.h>

#include "CPImageDisplay.h"

using namespace std;

CPImageDisplay::CPImageDisplay(QWidget* parent, const char* name, WFlags fl)
    : QWidget(parent, name, fl),
      pixmap(0), 
      drawNewPoint(false),
      editState(NO_SELECTION),
      scaleFactor(1.0)
{
    // red, green, blue, cyan, magenta, darkRed, darkGreen, darkBlue, darkCyan
    pointColors.push_back(red);
    pointColors.push_back(green);
    pointColors.push_back(blue);
    pointColors.push_back(cyan);
    pointColors.push_back(magenta);
    pointColors.push_back(darkRed);
    pointColors.push_back(darkGreen);
    pointColors.push_back(darkBlue);
    pointColors.push_back(darkCyan);
}

CPImageDisplay::~CPImageDisplay()
{
}

void CPImageDisplay::setPixmap (const QPixmap * img)
{
    pixmap = img;
    if (pixmap) {
//        setFixedSize(pixmap->size());
        setMinimumSize(pixmap->size());
//        setMaximumSize(pixmap->size());
        resize(pixmap->size());
    }
    updateGeometry();
//    update();
}

QSizePolicy CPImageDisplay::sizePolicy() const
{
    return QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

QSize CPImageDisplay::minimumSizeHint () const
{
    if (pixmap) {
        return QSize(scale(pixmap->size().width()), scale(pixmap->size().height()));
    } else {
        return QSize();
    }
}

QSize CPImageDisplay::sizeHint () const
{
    if (pixmap) {
        return QSize(scale(pixmap->size().width()), scale(pixmap->size().height()));
    } else {
        return QSize();
    }
}



void CPImageDisplay::paintEvent( QPaintEvent * event)
{
    QPainter p( this );
    if (pixmap) {
        p.save();
        if (scaleFactor != 1.0) {
            p.scale(scaleFactor, scaleFactor);
        }
        // draw image here (FIXME, redraw only visible regions.)
        p.drawPixmap(0, 0, *pixmap);
        p.restore();
    } else {
        p.eraseRect(event->rect());
    }
    // draw known points.
    int i=0;
    vector<QPoint>::const_iterator it;
    for (it = points.begin(); it != points.end(); ++it) {
        p.setPen(QPen(black,1));
        p.setBrush(QBrush(pointColors[i%pointColors.size()]));
        p.drawEllipse( scale(it->x()) - 4,
                       scale(it->y()) - 4,
                       9, 9 );
        i++;
    }
    
    if (drawNewPoint) {
        qDebug("Drawing create_point");
        p.setBrush(QBrush(yellow));
        p.drawEllipse( scale(newPoint.x()) -4, scale(newPoint.y())-4, 9,9 );
    }
    
    switch(editState) {
    case SELECT_REGION:
        p.setRasterOp(XorROP);
        p.setPen(QPen(white,1));
        p.setBrush(QBrush());
        p.drawRect(scale(region.left()),
                   scale(region.top()),
                   scale(region.width()),
                   scale(region.height()));
        break;
    case NEW_POINT_SELECTED:
    case KNOWN_POINT_SELECTED:
    case NO_SELECTION:
        break;
    }
}


void CPImageDisplay::setCtrlPoints(const std::vector<QPoint> & cps)
{
    points = cps;
    // update view
    update();
}


void CPImageDisplay::mouseMoveEvent(QMouseEvent *mouse)
{
    QPoint mpos;
    mpos.setX(invScale(mouse->x()));
    mpos.setY(invScale(mouse->y()));
    EditorState oldstate = editState;
    if (mouse->state() & LeftButton) {
        switch(editState) {
        case NO_SELECTION:
            Q_ASSERT(0);
            break;
        case KNOWN_POINT_SELECTED:
            if (mpos.x() >= 0 && mpos.x() <= pixmap->width()){
                points[selectedPointNr].setX(mpos.x());
            } else if (mpos.x() < 0) {
                points[selectedPointNr].setX(0);
            } else if (mpos.x() > pixmap->width()) {
                points[selectedPointNr].setX(pixmap->width());
            }
            
            if (mpos.y() >= 0 && mpos.y() <= pixmap->height()){
                points[selectedPointNr].setY(mpos.y());
            } else if (mpos.y() < 0) {
                points[selectedPointNr].setY(0);
            } else if (mpos.y() > pixmap->height()) {
                points[selectedPointNr].setY(pixmap->height());
            }
            emit(pointMoved(selectedPointNr, points[selectedPointNr]));
            // do more intelligent updating here?
            update();
            break;
        case NEW_POINT_SELECTED:
            newPoint = mpos;
            emit(newPointMoved(newPoint));
            update();
            break;
        case SELECT_REGION:
            region.setRight(mpos.x());
            region.setBottom(mpos.y());
            // do more intelligent updating here?
            update();
        }
        qDebug("ImageDisplay: mouse move, state change: %d -> %d",oldstate, editState);
    }
}


void CPImageDisplay::mousePressEvent(QMouseEvent *mouse)
{
    QPoint mpos;
    mpos.setX(invScale(mouse->x()));
    mpos.setY(invScale(mouse->y()));
    EditorState oldstate = editState;
    unsigned int selPointNr = 0;
    EditorState clickState = isOccupied(mpos, selPointNr);
    if (mouse->button() == LeftButton) {
        // we can always select a new point
        if (clickState == KNOWN_POINT_SELECTED) {
            if (selectedPointNr != selPointNr) {
                selectedPointNr = selPointNr;
                point = points[selectedPointNr];
                editState = clickState;
                emit(pointSelected(selectedPointNr));
            }
        } else if (clickState == NEW_POINT_SELECTED) {
            editState = NEW_POINT_SELECTED;
        } else if (clickState == SELECT_REGION) {
            editState = SELECT_REGION;
            region.setLeft(mpos.x());
            region.setTop(mpos.y());
        }
        qDebug("ImageDisplay: mouse down, state change: %d -> %d",oldstate, editState);
    }
}


void CPImageDisplay::mouseReleaseEvent(QMouseEvent *mouse)
{
    QPoint mpos;
    mpos.setX(invScale(mouse->x()));
    mpos.setY(invScale(mouse->y()));
    EditorState oldState = editState;
    if (mouse->button() == LeftButton) {
        switch(editState) {
        case NO_SELECTION:
            Q_ASSERT(0);
            break;
        case KNOWN_POINT_SELECTED:
            if (point != points[selectedPointNr]) {
                emit(pointChanged(selectedPointNr, points[selectedPointNr]));
            }
            break;
        case NEW_POINT_SELECTED:
            Q_ASSERT(drawNewPoint);
            emit(newPointChanged(newPoint));
            break;
        case SELECT_REGION:
            if (region.topLeft() == mpos) {
                // create a new point.
                drawNewPoint = true;
                editState = KNOWN_POINT_SELECTED;
                newPoint = mpos;
                emit(newPointChanged(newPoint));
                update();
            } else {
                editState = NO_SELECTION;
                emit(regionSelected(region));
                update();
            }
        }
        qDebug("ImageDisplay: mouse release, state change: %d -> %d",oldState, editState);
    }
}


void CPImageDisplay::clearNewPoint()
{
    drawNewPoint = false;
}



CPImageDisplay::EditorState CPImageDisplay::isOccupied(const QPoint &mpos, unsigned int & pointNr) const
{
    // check if mouse is over a known point
    EditorState ret;
    vector<QPoint>::const_iterator it;
    for (it = points.begin(); it != points.end(); ++it) {
        if (mpos.x() < it->x() + 4 &&
            mpos.x() > it->x() - 4 &&
            mpos.y() < it->y() + 4 &&
            mpos.y() > it->y() - 4
            )
        {
            pointNr = it - points.begin();
            return KNOWN_POINT_SELECTED;
            break;
        }
    }

    if (drawNewPoint &&
        mpos.x() < newPoint.x() + 4 &&
        mpos.x() > newPoint.x() - 4 &&
        mpos.y() < newPoint.y() + 4 &&
        mpos.y() > newPoint.y() - 4)
    {
        return NEW_POINT_SELECTED;
    } else {
        return SELECT_REGION;
    }
}
    
    

void CPImageDisplay::selectPoint(unsigned int nr)
{
    Q_ASSERT(nr < points.size());
    
    selectedPointNr = nr;
}
