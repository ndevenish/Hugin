// -*- c-basic-offset: 4 -*-
/** @file Panorama.h
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

#ifndef PANORAMA_H
#define PANORAMA_H

#include <vector>
#include <functional>

#include <qobject.h>
#include <qstring.h>
#include <qprocess.h>
#include <qsettings.h>
#include <qdom.h>

#include "PanoImage.h"

namespace PT {

    QString getAttrib(QDomNamedNodeMap map, QString name);

    //=========================================================================
    //=========================================================================

    // represents a control point
    struct ControlPoint
    {
        enum OptimizeMode {
            X_Y = 0,  /// evaluate x,y
            X,        /// evaluate x, points are on a vertical line
            Y         /// evaluate y, points are on a horizontal line
        };

          ControlPoint()
              : image1(0), image2(0),
                x1(0),y1(0),
                x2(0),y2(0),
                error(0), mode(X_Y)
              { };

        ControlPoint(Panorama & pano, const QDomNode & node);
        ControlPoint(PanoImage * s, double sX, double sY,
                     PanoImage * d, double dX, double dY,
                     OptimizeMode mode = X_Y)
            : image1(s), image2(d),
              x1(sX),y1(sY),
              x2(dX),y2(dY),
              error(0), mode(mode)
            { };


        void printScriptLine(std::ostream & o) const;

        QDomNode toXML(QDomDocument & doc) const;

        void setFromXML(const QDomNode & elem, Panorama & pano);

        PanoImage * image1;
        PanoImage * image2;
        double x1,y1;
        double x2,y2;
        double error;
        OptimizeMode mode;
    };

    typedef std::vector<ControlPoint*> CPVector;

    //=========================================================================
    //=========================================================================


    struct ControlPointInImage : public std::binary_function<const ControlPoint &, unsigned int, bool>{
        bool operator()(const ControlPoint & point, const PanoImage * img) const
            {
                return ((point.image1 == img) || (point.image2 == img));
            }
    };


    //=========================================================================
    //=========================================================================


    class PanoramaOptions
    {
    public:

        PanoramaOptions()
            : projectionFormat(EQUIRECTANGULAR),
              HFOV(360),
              width(0), height(0),
              outfile("panorama.jpg"),outputFormat("JPEG"),
              quality(80),progressive(false),
              colorCorrection(NONE), colorReferenceImage(0),
              gamma(1.0), interpolator(POLY_3)
            {};

        virtual ~PanoramaOptions() {};

        enum ProjectionFormat { RECTILINEAR = 0,
                                CYLINDRICAL = 1,
                                EQUIRECTANGULAR = 2 };

        enum ColorCorrection { NONE = 0,
                               BRIGHTNESS_COLOR,
                               BRIGHTNESS,
                               COLOR };

        enum Interpolator {
            POLY_3 = 0,
            SPLINE_16,
            SPLINE_36,
            SINC_256,
            SPLINE_64,
            BILINEAR,
            NEAREST_NEIGHBOUR,
            SINC_1024
        };

        QDomNode toXML(QDomDocument & doc) const;

        void setFromXML(const QDomNode & elem);

        void printScriptLine(std::ostream & o) const;

        // they are public, because they need to be set through
        // get/setOptions in Panorama.

        ProjectionFormat projectionFormat;
        double HFOV;
        unsigned int width;
        unsigned int height;
        QString outfile;
        QString outputFormat;
        // jpeg options
        int quality;
        bool progressive;
        ColorCorrection colorCorrection;
        unsigned int colorReferenceImage;

        // misc options
        double gamma;
        Interpolator interpolator;

    };

    
    //=========================================================================
    //=========================================================================


    /** The central Panorama object.
     *
     *  It contains the model (see Model View Controller Pattern) of
     *  a Panorama.
     *
     *  It also provides easy access to the operations that can be done
     *  on a Panorama (with the Panorama Tools).
     *
     *  Operations are implemented using the Command pattern, with
     *  PanoramaCommand as the base class for Panoramas.
     *
     * @todo how to implementet more complicated commmands, that
     *       requires a decision that cant be made automatically, and
     *       needs user interaction with the commands, like
     *       optimize). Split it into multiple commands, and save the
     *       state information inside the class. or create commands
     *       that open dialogs etc?
     *
     *
     * */

    class Panorama : public QObject
    {
        Q_OBJECT
    public:
        Panorama();
        virtual ~Panorama();

        QDomElement toXML(QDomDocument & doc);

        void setFromXML(const QDomNode & elem);

        /** run PTOptimizer to optimize the panorama.
         *
         * reads the optimization results when finished */
        void optimize();

        /** create panorama */
        void stitch(const PanoramaOptions & target);

        /** read settings.
         */
        void readSettings();
        void writeSettings();

        //
        // functions needed for the commands
        //

        /** add an Image to the panorama
         *  @return image number
         */
        PanoImage * addImage(const QString & filename);

        /** remove an Image.
         *
         *  deletes all associated control points
         */
        void removeImage(PanoImage * img);

        /** get an image
         */
        PanoImage * getImage(unsigned int nr)
            {
                Q_ASSERT(nr < images.size());
                return images[nr];
            };

        /** get # of images */
        unsigned int getNrImages()
            { return images.size(); }

        ControlPoint * addControlPoint(const ControlPoint & point);
        void removeControlPoint(ControlPoint * point);
        // ignores the image pointers.
        void changeControlPoint(unsigned int ctrlPointNr, ControlPoint point);

        ControlPoint * getControlPoint(unsigned int nr)
            {
                Q_ASSERT(nr < controlPoints.size());
                return controlPoints[nr];
            };

        void setOptions(const PanoramaOptions & opt);
        const PanoramaOptions & getOptions() const
            {
                return options;
            }

        //
        // functions to get panorama information.
        //


        /// set PTStitcher executable
        void setPTStitcherFileName(QString app)
            { stitcherExe = app; }
        /// set PTOptimizer executable
        void setPTOptimizerFileName(QString app)
            { optimizerExe = app; }

    public:
        /// create an optimizer script
        void printOptimizerScript(std::ostream & o);

        /// create the stitcher script
        void printStitcherScript(std::ostream & o);
        /// read after optimization, fills in control point errors.
        void readOptimizerScript(std::istream & i);

        /** return all control points.
         *
         *  ugly. exposes details...
         */
        std::vector<ControlPoint*>  getCtrlPoints()
            { return controlPoints; };

        /** return control points for a given image */
        std::vector<ControlPoint *> getCtrlPointsForImage(PanoImage * img) const;

        /** return all images
         *
         *  ugly, exposes internals...
         */
        std::vector<PanoImage*> & getImages()
            { return images; };

        bool hasCommonLens() const
            { return commonLens; }

        virtual const char * isA() const { return "Panorama"; };

        void setCommonLens(bool);
        void updateLens(const LensSettings & l, PanoImage * img);

        // report changes
        /// call if
        void reportChange();
        void reportChangedImage(unsigned int img);
        void reportAddedImage(unsigned int img);
        void reportRemovedImage(unsigned int img);

        void reportAddedCtrlPoint(unsigned int point);
        void reportRemovedCtrlPoint(unsigned int point);
        
        bool isDirty()
            { return dirty; };
        void setDirty(bool d)
            { dirty = d; };
    public slots:
        void processExited();

    signals:
        /// some internal state changed. this includes
        /// everything inside the panorama, images as well as
        /// panorama settings.
        void stateChanged();

        /// a specific image has been changed
        void imageChanged(unsigned int nr);
        /// an image has been added
        void imageAdded(unsigned int nr);
        /// an image has been removed
        void imageRemoved(unsigned int nr);

        // signals about control points
        void ctrlPointAdded(unsigned int nr);
        void ctrlPointRemoved(unsigned int nr);

    private:
        
        enum ProcessType { NO_PROCESS, OPTIMIZER, STITCHER };
        /// save the panorama into a file suitable for PTOptimizer
        std::vector<PanoImage *> images;
        std::vector<QPixmap> imageData;
        PanoramaOptions options;
        std::vector<ControlPoint *> controlPoints;

        // same lens
        bool commonLens;

        // to run stitcher & optimizer
        QProcess process;
        ProcessType currentProcess;
        QString optimizerExe;
        QString stitcherExe;
        QString PTScriptFile;
        
        bool dirty;

        QSettings settings;
    };
}

#endif // PANORAMA_H
