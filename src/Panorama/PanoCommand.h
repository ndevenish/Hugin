// -*- c-basic-offset: 4 -*-
/** @file PanoCommand.h
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

#ifndef _PANOCOMMAND_H
#define _PANOCOMMAND_H

#include "../Command.h"

#include "PanoImage.h"
#include "Panorama.h"

namespace PT {


    //=========================================================================
    //=========================================================================

    /** add image(s) to a panorama */
    class AddImagesCmd : public Command
    {
    public:
        AddImagesCmd(Panorama & pano, QStringList & files)
            : pano(pano), files(files)
            { };
        virtual void execute()
            {
                QStringList::const_iterator it;
                for (it = files.begin(); it != files.end(); ++it) {
                    addedImages.push_back(pano.addImage(*it));
                }
            }
        virtual void undo()
            {
                std::vector<PanoImage *>::const_iterator it;
                for (it = addedImages.begin(); it != addedImages.end(); ++it) {
                    pano.removeImage(*it);
                }
            }
        virtual QString getName() const
            {
                return "add images";
            }
    private:
        Panorama & pano;
        QStringList files;
        std::vector<PanoImage*> addedImages;
    };

    //=========================================================================
    //=========================================================================

    /** remove an image from a panorama */
    class RemoveImageCmd : public Command
    {
    public:
        RemoveImageCmd(Panorama & p, unsigned int imgNr)
            : pano(p), imgNr(imgNr)
            { };
        virtual void execute()
            {
                PanoImage * img = pano.getImage(imgNr);
                file = img->getFilename();
                pano.removeImage(img);
            }
        virtual void undo()
            {
                qWarning("FIXME Undo not implemented");
                pano.addImage(file);
            }
        virtual QString getName() const
            {
                return "remove image";
            }
    private:
        Panorama &pano;
        unsigned int imgNr;
        QString file;
    };

    //=========================================================================
    //=========================================================================

    /** add a control point */
    class AddCtrlPointCmd : public Command
    {
    public:
        AddCtrlPointCmd(Panorama & p, const ControlPoint & cpoint)
            : pano(p), point(cpoint)
            { }

        virtual void execute()
            {
                newPoint = pano.addControlPoint(point);
            }
        virtual void undo()
            {
                pano.removeControlPoint(newPoint);
            }
        virtual QString getName() const
            {
                return "add control point";
            }
    private:
        Panorama & pano;
        ControlPoint point;
        ControlPoint * newPoint;
    };

    //=========================================================================
    //=========================================================================

    /** remove a control point */
    class RemoveCtrlPointCmd : public Command
    {
    public:
        RemoveCtrlPointCmd(Panorama & p, ControlPoint * point)
            : pano(p), point(point)
            { }

        virtual void execute()
            {
                copy = *point;
                pano.removeControlPoint(point);
            }
        virtual void undo()
            {
                // XXXX adds ctrl point at the wrong position...
                pano.addControlPoint(copy);
            }
        virtual QString getName() const
            {
                return "remove control point";
            }
    private:
        Panorama & pano;
        ControlPoint * point;
        ControlPoint copy;
    };


    //=========================================================================
    //=========================================================================
    
    /** change a control point */
    class ChangeCtrlPointCmd : public Command
    {
    public:
        ChangeCtrlPointCmd(Panorama & p, unsigned int nr, ControlPoint point)
            : pano(p), pNr(nr), point(point)
            { }

        virtual void execute()
            {
                copy = *pano.getControlPoint(pNr);
                pano.changeControlPoint(pNr, point);
            }
        virtual void undo()
            {
                pano.changeControlPoint(pNr,copy);
            }
        virtual QString getName() const
            {
                return "change control point";
            }
    private:
        Panorama & pano;
        unsigned int pNr;
        ControlPoint point;
        ControlPoint copy;
    };


    //=========================================================================
    //=========================================================================


    /** change lens */
    class ChangeLensCmd : public Command
    {
    public:
        ChangeLensCmd(Panorama & p, unsigned int imageNr, const LensSettings & lens)
            : pano(p), imageNr(imageNr), newLens(lens)
            { };
        virtual void execute()
            {
                PanoImage * img = pano.getImage(imageNr);
                oldLens = img->getLens();
                img->updateLens(newLens);
            }
        virtual void undo()
            {
                pano.getImage(imageNr)->updateLens(oldLens);
            }
        virtual QString getName() const
            {
                return "change lens";
            }
    private:
        Panorama & pano;
        unsigned int imageNr;
        LensSettings newLens;
        LensSettings oldLens;
    };


    //=========================================================================
    //=========================================================================


    /** change position of image inside panorama */
    class ChangeImagePositionCmd : public Command
    {
    public:
        ChangeImagePositionCmd(Panorama & p, unsigned int imageNr,
                               const ImagePosition & prop)
            : pano(p), imageNr(imageNr), newPos(prop)
            { };

        virtual void execute()
            {
                PanoImage * img = pano.getImage(imageNr);
                oldPos = img->getPosition();
                img->setPosition(newPos);
            }
        virtual void undo()
            {
                pano.getImage(imageNr)->setPosition(oldPos);
            }
        virtual QString getName() const
            {
                return "set image position";
            }
    private:
        Panorama & pano;
        unsigned int imageNr;
        ImagePosition newPos;
        ImagePosition oldPos;
    };


    //=========================================================================
    //=========================================================================


    /** change position of image inside panorama */
    class ChangeImageOptionsCmd : public Command
    {
    public:
        ChangeImageOptionsCmd(Panorama & p, unsigned int imageNr,
                               const ImageOptions & prop)
            : pano(p), imageNr(imageNr), newOpt(prop)
            { };

        virtual void execute()
            {
                PanoImage * img = pano.getImage(imageNr);
                oldOpt = img->getOptions();
                img->setOptions(newOpt);
            }
        virtual void undo()
            {
                pano.getImage(imageNr)->setOptions(oldOpt);
            }
        virtual QString getName() const
            {
                return "set image position";
            }
    private:
        Panorama & pano;
        unsigned int imageNr;
        ImageOptions newOpt;
        ImageOptions oldOpt;
    };


    //=========================================================================
    //=========================================================================


    /** set common lens */
    class SetCommonLensCmd : public Command
    {
    public:
        SetCommonLensCmd(Panorama & p, bool commonLens)
            : pano(p), common(commonLens)
            { };
        virtual void execute()
            {
                oldCommon = pano.hasCommonLens();
                pano.setCommonLens(common);
            }
        virtual void undo()
            {
                pano.setCommonLens(oldCommon);
            }
        virtual QString getName() const
            {
                return "set common lens";
            }
    private:
        Panorama & pano;
        bool common;
        bool oldCommon;
    };


    //=========================================================================
    //=========================================================================


    /** change the panorama properties */
    class ChangePanoramaOptionsCmd : public Command
    {
    public:
        ChangePanoramaOptionsCmd(Panorama & p,
                                 const PanoramaOptions & options)
            : pano(p), imageNr(imageNr), newOptions(options)
            { };

        virtual void execute()
            {
                oldOptions = pano.getOptions();
                pano.setOptions(newOptions);
            }
        virtual void undo()
            {
                pano.setOptions(oldOptions);
            }
        virtual QString getName() const
            {
                return "set image position";
            }
    private:
        Panorama & pano;
        unsigned int imageNr;
        PanoramaOptions newOptions;
        PanoramaOptions oldOptions;
    };


    //=========================================================================
    //=========================================================================


    /// run the optimizer
    class OptimizeCmd : public Command
    {
    public:
        OptimizeCmd(Panorama & p)
            : pano(p)
            { }

        virtual void execute()
            {
                /// XXXX save all variables that could be changed
                /// XXXX by the optimizer.
                qWarning("XXXX save optimizer variables for undo");
                pano.optimize();
            }
        virtual void undo()
            {
                qWarning("XXXX restore optimizer variables.");
            }
        virtual QString getName() const
            {
                return "optimize";
            }
    private:
        Panorama & pano;
    };

    //=========================================================================
    //=========================================================================


    /// stitch the image
    class StitchCmd : public Command
    {
    public:
        StitchCmd(Panorama & p, QString filename,
                  unsigned int height=0, unsigned int width=0)
            : pano(p), file(filename), height(height), width(width)
            { }

        virtual void execute()
            {
                PanoramaOptions opt = pano.getOptions();
                if (file.length() != 0) {
                    opt.outfile = file;
                }
                if (height != 0 ) {
                    opt.height = height;
                }
                if (width != 0 ) {
                    opt.width = width;
                }
                pano.stitch(opt);
            }
        virtual void undo()
            {
            }

        virtual QString getName() const
            {
                return "stitch panorama";
            }
    private:
        Panorama & pano;
        QString file;
        unsigned int height;
        unsigned int width;
    };
}


#endif // _PANOCOMMAND_H
