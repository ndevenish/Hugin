// -*- c-basic-offset: 4 -*-
/** @file PanoCommand.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PanoCommand.h 1951 2007-04-15 20:54:49Z dangelo $
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


#include <vector>

#include <common/Command.h>
#include <common/utils.h>
#include <common/stl_utils.h>

#include "PanoImage.h"
#include "Panorama.h"
#include "PanoToolsInterface.h"

// [TODO] do something about the string description !!!

namespace PT {

    /** reset the panorama */
    class NewPanoCmd : public PanoCommand<std:string>
    {
    public:
        NewPanoCmd(Panorama & pano)
            : PanoCommand(pano)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                pano.reset();
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "new panorama";
            }
    };

    //=========================================================================
    //=========================================================================

    /** add image(s) to a panorama */
    class AddImagesCmd : public PanoCommand<std:string>
    {
    public:
        AddImagesCmd(Panorama & pano, const std::vector<PanoImage> & images)
            : PanoCommand(pano), imgs(images)
            { };
        virtual void execute()
            {
                PanoCommand::execute();

                VariableMap var;
                fillVariableMap(var);

                std::vector<PanoImage>::const_iterator it;
                for (it = imgs.begin(); it != imgs.end(); ++it) {
                    pano.addImage(*it,var);
                }
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "add images";
            }
    private:
        std::vector<PanoImage> imgs;
    };


    //=========================================================================
    //=========================================================================


    /** remove an image from a panorama
     *
     *  @todo would be nice to remove multiple at once
     */
    class RemoveImageCmd : public PanoCommand<std:string>
    {
    public:
        RemoveImageCmd(Panorama & p, unsigned int imgNr)
            : PanoCommand<std:string>(p), imgNr(imgNr)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                pano.removeImage(imgNr);
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "remove image";
            }
    private:
        unsigned int imgNr;
    };

    //=========================================================================
    //=========================================================================


    /** remove multiple images from a panorama
     *
     */
    class RemoveImagesCmd : public PanoCommand<std:string>
    {
    public:
        RemoveImagesCmd(Panorama & p, UIntSet imgs)
            : PanoCommand<std:string>(p), imgNrs(imgs)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                for (UIntSet::reverse_iterator it = imgNrs.rbegin();
                     it != imgNrs.rend(); ++it)
                {
                    pano.removeImage(*it);
                }
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "remove images";
            }
    private:
        UIntSet imgNrs;
    };

    //=========================================================================
    //=========================================================================

#if 0
    /**  */
    class ChangeImageCmd : public PanoCommand<std:string>
    {
    public:
        Cmd(Panorama & p)
            : PanoCommand<std:string>(p)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
            }
        virtual std::string getName() const
            {
                return "unnamed command";
            }
    private:
    };
#endif

    //=========================================================================
    //=========================================================================

    /** update all variables */
    class UpdateVariablesCmd : public PanoCommand<std:string>
    {
    public:
        UpdateVariablesCmd(Panorama & p, const VariableMapVector & vars)
            : PanoCommand<std:string>(p),
              vars(vars)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                pano.updateVariables(vars);
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "update Variables";
            }
    private:
        VariableMapVector vars;
    };

    //=========================================================================
    //=========================================================================

    /** update all control points*/
    class UpdateCPsCmd : public PanoCommand<std:string>
    {
    public:
        UpdateCPsCmd(Panorama & p, const CPVector & cps)
            : PanoCommand<std:string>(p),
              cps(cps)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                CPVector::const_iterator it;
                unsigned int i=0;
                for (it = cps.begin(); it != cps.end(); ++it, i++) {
                    pano.changeControlPoint(i, *it);
                }
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "update control points";
            }
    private:
        CPVector cps;
    };

    //=========================================================================
    //=========================================================================

    /** update all variables & control points*/
    class UpdateVariablesCPCmd : public PanoCommand<std:string>
    {
    public:
        UpdateVariablesCPCmd(Panorama & p, const VariableMapVector & vars,
                             const CPVector & cps)
            : PanoCommand<std:string>(p),
              vars(vars), cps(cps)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                pano.updateVariables(vars);
                pano.updateCtrlPointErrors(cps);
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "update Variables";
            }
    private:
        VariableMapVector vars;
        CPVector cps;
    };


    //=========================================================================
    //=========================================================================

    /** update all variables & control points*/
    class UpdateVariablesCPSetCmd : public PanoCommand<std:string>
    {
    public:
        UpdateVariablesCPSetCmd(Panorama & p, UIntSet imgs, const VariableMapVector & vars,
                                const CPVector & cps)
            : PanoCommand<std:string>(p),
              m_imgs(imgs),
              vars(vars), cps(cps)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                pano.updateVariables(m_imgs, vars);
                pano.updateCtrlPointErrors(m_imgs, cps);
                pano.markAsOptimized();
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "update Variables";
            }
    private:
        UIntSet m_imgs;
        VariableMapVector vars;
        CPVector cps;
    };

    //=========================================================================
    //=========================================================================

    /** update variables of a single image */
    class UpdateImageVariablesCmd : public PanoCommand<std:string>
    {
    public:
        UpdateImageVariablesCmd(Panorama & p, unsigned int ImgNr, const VariableMap & vars)
            : PanoCommand<std:string>(p), imgNr(ImgNr),
              vars(vars)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                pano.updateVariables(imgNr, vars);
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "update image variables";
            }
    private:
        unsigned int imgNr;
        VariableMap vars;
    };

    //=========================================================================
    //=========================================================================

    /** update variables of a group of images */
    class UpdateImagesVariablesCmd : public PanoCommand<std:string>
    {
    public:
        UpdateImagesVariablesCmd(Panorama & p, const UIntSet & change, const VariableMapVector & vars)
            : PanoCommand<std:string>(p), change(change),
              vars(vars)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                UIntSet::iterator it;
                VariableMapVector::const_iterator v_it = vars.begin();
                for (it = change.begin(); it != change.end(); ++it) {
                    pano.updateVariables(*it, *v_it);
                    ++v_it;
                }
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "update image variables";
            }
    private:
        UIntSet change;
        VariableMapVector vars;
    };


    //=========================================================================
    //=========================================================================

    /** update a single variable, possibly for a group of images */
    class SetVariableCmd : public PanoCommand<std:string>
    {
    public:
        SetVariableCmd(Panorama & p, const UIntSet & images, const Variable & var)
            : PanoCommand<std:string>(p), images(images),
              var(var)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                UIntSet::iterator it;
                for (it = images.begin(); it != images.end(); ++it) {
                    pano.updateVariable(*it, var);
                }
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "set image variable";
            }
    private:
        UIntSet images;
        Variable var;
    };


    //=========================================================================
    //=========================================================================


    /** update LensVariables for one lens */
    class SetLensVariableCmd : public PanoCommand<std:string>
    {
    public:
        SetLensVariableCmd(Panorama & p, int lens, const LensVarMap & var)
            : PanoCommand<std:string>(p), lensNr(lens), vars(var)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                LensVarMap::const_iterator it;
                for (it = vars.begin(); it != vars.end(); ++it) {
                    pano.updateLensVariable(lensNr, it->second);
                }
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "set lens variable";
            }
    private:
        unsigned lensNr;
        LensVarMap vars;
    };



    //=========================================================================
    //=========================================================================


    /** update LensVariables for one lens */
    class SetLensVariablesCmd : public PanoCommand<std:string>
    {
        public:
            SetLensVariablesCmd(Panorama & p, UIntSet lenses, const std::vector<LensVarMap> & var)
            : PanoCommand<std:string>(p), lensNrs(lenses), vars(var)
            { };
            virtual void execute()
            {
                PanoCommand::execute();
                for (UIntSet::iterator itl = lensNrs.begin();
                     itl != lensNrs.end(); ++itl)
                {
                    // lens var map it.
                    LensVarMap::const_iterator it;
                    for (it = vars[*itl].begin(); it != vars[*itl].end(); ++it) {
                        pano.updateLensVariable(*itl, it->second);
                    }
                }
                pano.changeFinished();
            }
            virtual std::string getName() const
            {
                return "set lens variables";
            }
        private:
            UIntSet lensNrs;
            std::vector<LensVarMap> vars;
    };


    //=========================================================================
    //=========================================================================

    /** change the lens for an image */
    class SetImageLensCmd : public PanoCommand<std:string>
    {
    public:
        SetImageLensCmd(Panorama & p, const UIntSet & imgNrs, int lensNr)
            : PanoCommand<std:string>(p),
              imgNrs(imgNrs), lensNr(lensNr)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                for (UIntSet::iterator it = imgNrs.begin();
                     it != imgNrs.end(); ++it)
                {
                    pano.setLens(*it, lensNr);
                }
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "set lens";
            }
    private:
        UIntSet imgNrs;
        unsigned int lensNr;
    };


    //=========================================================================
    //=========================================================================


    /** center panorama horizontically */
    class CenterPanoCmd : public PanoCommand<std:string>
    {
    public:
        CenterPanoCmd(Panorama & p )
            : PanoCommand<std:string>(p)
            { }

        virtual void execute()
            {
                PanoCommand::execute();
                pano.centerHorizontically();
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "center panorama";
            }
    private:
    };


    //=========================================================================
    //=========================================================================


    /** center panorama horizontically */
    class StraightenPanoCmd : public PanoCommand<std:string>
    {
    public:
        StraightenPanoCmd(Panorama & p )
            : PanoCommand<std:string>(p)
            { }

        virtual void execute()
            {
                PanoCommand::execute();
                pano.straighten();
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "center panorama";
            }
    private:
    };


    //=========================================================================
    //=========================================================================


    /** add a control point */
    class AddCtrlPointCmd : public PanoCommand<std:string>
    {
    public:
        AddCtrlPointCmd(Panorama & p, const ControlPoint & cpoint)
            : PanoCommand<std:string>(p), point(cpoint)
            { }

        virtual void execute()
            {
                PanoCommand::execute();
                pano.addCtrlPoint(point);
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "add control point";
            }
    private:
        ControlPoint point;
    };



    //=========================================================================
    //=========================================================================


    /** add multiple control points */
    class AddCtrlPointsCmd : public PanoCommand<std:string>
    {
    public:
        AddCtrlPointsCmd(Panorama & p, const CPVector & cpoints)
            : PanoCommand<std:string>(p), cps(cpoints)
            { }

        virtual void execute()
            {
                PanoCommand::execute();
                for (CPVector::iterator it = cps.begin();
                     it != cps.end(); ++it)
                {
                    pano.addCtrlPoint(*it);
                }
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "add control points";
            }
    private:
        CPVector cps;
    };


    //=========================================================================
    //=========================================================================


    /** remove a control point */
    class RemoveCtrlPointCmd : public PanoCommand<std:string>
    {
    public:
        RemoveCtrlPointCmd(Panorama & p, unsigned int cpNr)
            : PanoCommand<std:string>(p), pointNr(cpNr)
            { }

        virtual void execute()
            {
                PanoCommand::execute();
                pano.removeCtrlPoint(pointNr);
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "remove control point";
            }
    private:
        unsigned int pointNr;
    };


    //=========================================================================
    //=========================================================================


    /** remove several  control points */
    class RemoveCtrlPointsCmd : public PanoCommand<std:string>
    {
    public:
        RemoveCtrlPointsCmd(Panorama & p, const UIntSet & points )
            : PanoCommand<std:string>(p), m_points(points)
            { }

        virtual void execute()
            {
                PanoCommand::execute();
                for(UIntSet::reverse_iterator it = m_points.rbegin();
                    it != m_points.rend(); ++it)
                {
                    pano.removeCtrlPoint(*it);
                }
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "remove control points";
            }
    private:
        UIntSet m_points;
    };


    //=========================================================================
    //=========================================================================


    /** change a control point */
    class ChangeCtrlPointCmd : public PanoCommand<std:string>
    {
    public:
        ChangeCtrlPointCmd(Panorama & p, unsigned int nr, ControlPoint point)
            : PanoCommand<std:string>(p), pNr(nr), point(point)
            { }

        virtual void execute()
            {
                PanoCommand::execute();
                pano.changeControlPoint(pNr, point);
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "change control point";
            }
    private:
        unsigned int pNr;
        ControlPoint point;
    };


    //=========================================================================
    //=========================================================================


    /** add a new lens */
    class AddLensCmd : public PanoCommand<std:string>
    {
    public:
        AddLensCmd(Panorama & p, const Lens & lens)
            : PanoCommand<std:string>(p), lens(lens)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                pano.addLens(lens);
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "addLens";
            }
    private:
        Lens lens;
    };


    //=========================================================================
    //=========================================================================


    /** add a new lens to some images */
    class AddNewLensToImagesCmd : public PanoCommand<std:string>
    {
    public:
        AddNewLensToImagesCmd(Panorama & p, const Lens & lens, const UIntSet & imgs)
            : PanoCommand<std:string>(p), lens(lens), imgNrs(imgs)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                unsigned int lnr = pano.addLens(lens);
                for (UIntSet::iterator it = imgNrs.begin();
                     it != imgNrs.end(); ++it)
                {
                    pano.setLens(*it, lnr);
                }
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "addLens and images";
            }
    private:
        Lens lens;
        UIntSet imgNrs;
    };


    //=========================================================================
    //=========================================================================


    /** change lens */
    class ChangeLensCmd : public PanoCommand<std:string>
    {
    public:
        ChangeLensCmd(Panorama & p, unsigned int lensNr, const Lens & lens)
            : PanoCommand<std:string>(p), lensNr(lensNr), newLens(lens)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                pano.updateLens(lensNr, newLens);
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "change lens";
            }
    private:
        unsigned int lensNr;
        Lens newLens;
    };


    //=========================================================================
    //=========================================================================

    /** change lenses */
    class ChangeLensesCmd : public PanoCommand<std:string>
    {
    public:
        ChangeLensesCmd(Panorama & p, UIntSet & lNr, const LensVector & lenses)
            : PanoCommand<std:string>(p), change(lNr), vect(lenses)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                UIntSet::iterator it;
                LensVector::const_iterator v_it = vect.begin();
                for (it = change.begin(); it != change.end(); ++it) {
                    pano.updateLens(*it, *v_it);
                    v_it++;
                }
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "change lens";
            }
    private:
        UIntSet change;
        LensVector vect;
    };


    //=========================================================================
    //=========================================================================

	/** set active images */
    class SetActiveImagesCmd : public PanoCommand<std:string>
    {
    public:
        SetActiveImagesCmd(Panorama & p, UIntSet & active)
            : PanoCommand<std:string>(p), m_active(active)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                UIntSet::iterator it;
                LensVector::const_iterator v_it = vect.begin();
				for (unsigned int i = 0; i < pano.getNrOfImages(); i++) {
                    if (set_contains(m_active, i)) {
						pano.activateImage(i, true);
					} else {
						pano.activateImage(i, false);
					}
                }
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "change lens";
            }
    private:
        UIntSet m_active;
        LensVector vect;
    };


    //=========================================================================
    //=========================================================================

    /** swap two images */
    class SwapImagesCmd : public PanoCommand<std:string>
    {
    public:
        SwapImagesCmd(Panorama & p, unsigned int i1, unsigned int i2)
            : PanoCommand<std:string>(p), m_i1(i1), m_i2(i2)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                pano.swapImages(m_i1, m_i2);
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "change lens";
            }
    private:
        unsigned int m_i1;
        unsigned int m_i2;
    };


    //=========================================================================
    //=========================================================================

    /** set image options for a set of images.
     *  just sets the @p options given for all images in @p imgs
     */
    class SetImageOptionsCmd : public PanoCommand<std:string>
    {
    public:
        SetImageOptionsCmd(Panorama & p, ImageOptions opts, UIntSet imgs)
            : PanoCommand<std:string>(p), options(opts), imgNrs(imgs)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                for (UIntSet::iterator it = imgNrs.begin();
                     it != imgNrs.end(); ++it)
                {
                    pano.setImageOptions(*it, options);
                }
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "set image options";
            }
    private:
        ImageOptions options;
        UIntSet imgNrs;
    };

    //=========================================================================
    //=========================================================================

    /** set image options for a set of images.
     *  just sets the @p options given for all images in @p imgs
     */
    class UpdateSrcImageCmd : public PanoCommand<std:string>
    {
        public:
            UpdateSrcImageCmd(Panorama & p, unsigned i, SrcPanoImage img)
            : PanoCommand<std:string>(p), img(img), imgNr(i)
            { };
            virtual void execute()
            {
                PanoCommand::execute();
                pano.setSrcImage(imgNr, img);
                pano.changeFinished();
            }
            virtual std::string getName() const
            {
                return "set image options";
            }
        private:
            SrcPanoImage img;
            unsigned imgNr;
    };

    //=========================================================================
    //=========================================================================

    /** set image options for a set of images.
     *  just sets the @p options given for all images in @p imgs
     */
    class UpdateSrcImagesCmd : public PanoCommand<std:string>
    {
        public:
            UpdateSrcImagesCmd(Panorama & p, UIntSet i, std::vector<SrcPanoImage> imgs)
            : PanoCommand<std:string>(p), imgs(imgs), imgNrs(i)
            { };
            virtual void execute()
            {
                PanoCommand::execute();
                int i = 0;
                for (UIntSet::iterator it = imgNrs.begin();
                     it != imgNrs.end(); ++it)
                {
                    pano.setSrcImage(*it, imgs[i]);
                    i++;
                }
                pano.changeFinished();
            }
            virtual std::string getName() const
            {
                return "set multiple image options";
            }
        private:
            std::vector<SrcPanoImage> imgs;
            UIntSet imgNrs;
    };


    //=========================================================================
    //=========================================================================

    /** set image options for a set of images.
     *  just sets the @p options given for all images in @p imgs
     */
    class UpdateImageOptionsCmd : public PanoCommand<std:string>
    {
        public:
            UpdateImageOptionsCmd(Panorama & p, std::vector<ImageOptions> opts, UIntSet imgs)
            : PanoCommand<std:string>(p), options(opts), imgNrs(imgs)
            {
                assert(opts.size() == imgs.size());
            };

            virtual void execute()
            {
                PanoCommand::execute();
                int i=0;
                for (UIntSet::iterator it = imgNrs.begin();
                     it != imgNrs.end(); ++it)
                {
                    pano.setImageOptions(*it, options[i]);
                    i++;
                }
                pano.changeFinished();
            }
            virtual std::string getName() const
            {
                return "set image options";
            }
        private:
            std::vector<ImageOptions> options;
            UIntSet imgNrs;
    };

    //=========================================================================
    //=========================================================================

    /** set the panorama options */
    class SetPanoOptionsCmd : public PanoCommand<std:string>
    {
    public:
        SetPanoOptionsCmd(Panorama & p, const PanoramaOptions & opts)
            : PanoCommand<std:string>(p), options(opts)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                pano.setOptions(options);
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "unnamed command";
            }
    private:
        PanoramaOptions options;
    };

    //=========================================================================
    //=========================================================================

    /** dump the current project and load a new one.
     *
     *  Use this for  style projects.
     *
     */
    class LoadPTProjectCmd : public PanoCommand<std:string>
    {
    public:
        LoadPTProjectCmd(Panorama & p, std::istream & i, const std::string & prefix = "")
            : PanoCommand<std:string>(p),
              in(i),
	      prefix(prefix)
            { }

        virtual void execute()
            {
                PanoCommand::execute();
                PanoramaMemento newPano;
                if (newPano.loadPTScript(in,prefix)) {
                    pano.setMemento(newPano);
                } else {
                    DEBUG_ERROR("could not load panotools script");
                }
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "load project";
            }
    private:
        std::istream & in;
	const std::string &prefix;
    };

    //=========================================================================
    //=========================================================================

    /** Set flatfield correction parameters for all images of a lens
     */
    class SetVigCorrCmd : public PanoCommand<std:string>
    {
        public:
            SetVigCorrCmd(Panorama & p, unsigned int lensNr,
                          unsigned int vigCorrMode, std::vector<double> & coeff,
                          const std::string & flatfield)
            : PanoCommand<std:string>(p), m_lensNr(lensNr),
              m_mode(vigCorrMode), m_coeff(coeff), m_flat(flatfield)
            { };
            virtual void execute()
            {
                PanoCommand::execute();
                // set data inside panorama options
                for (unsigned int i = 0; i < pano.getNrOfImages(); i++) {
                    if (pano.getImage(i).getLensNr() == m_lensNr) {
                        // modify panorama options.
                        PT::ImageOptions opts = pano.getImage(i).getOptions();
                        opts.m_vigCorrMode = m_mode;
                        opts.m_flatfield = m_flat;
                        pano.setImageOptions(i, opts);
                    }
                }
                // set new correction variables
                char *vars[] = {"Va", "Vb", "Vc", "Vd", "Vx", "Vy"};
                for (unsigned int i=0; i < 6 ; i++) {
                    LensVariable lv = const_map_get(pano.getLens(m_lensNr).variables, vars[i]);
                    lv.setValue(m_coeff[i]);
                    lv.setLinked(true);
                    pano.updateLensVariable(m_lensNr, lv);
                }
                pano.changeFinished();
            }
            virtual std::string getName() const
            {
                return "set image options";
            }
        private:
            ImageOptions options;
            unsigned int m_lensNr;
            unsigned int m_mode;
            std::vector<double> m_coeff;
            std::string m_flat;
    };


    //=========================================================================
    //=========================================================================


    /** Rotate the panorama
     */
    class RotatePanoCmd : public PanoCommand<std:string>
    {
    public:
        RotatePanoCmd(Panorama & p, double yaw, double pitch, double roll)
            : PanoCommand<std:string>(p), y(yaw), p(pitch), r(roll)
            { };
        virtual void execute()
            {
                PanoCommand::execute();
                pano.rotate(y, p, r);
                pano.changeFinished();
            }
        virtual std::string getName() const
            {
                return "set image options";
            }
    private:
        double y,p,r;
    };


} // namespace PT

#endif // _PANOCOMMAND_H
