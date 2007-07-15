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

#ifndef _Hgn1_PANOCOMMAND_H
#define _Hgn1_PANOCOMMAND_H


#include <huginapp/PanoCommand.h>

//#include "PanoImage.h"
#include "Panorama.h"
//#include "PanoToolsInterface.h"



namespace PT {

    /** default panorama cmd, provides undo with mementos
     */
    class PanoCommand : public HuginBase::PanoCommand<std::string>
    {
        public:
            PanoCommand(Panorama& p)
              : HuginBase::PanoCommand<std::string>(p)
            {};

            virtual ~PanoCommand() {};
            
            
            virtual bool processPanorama(HuginBase::ManagedPanoramaData& pano)
            {
                try {
                    return processPanorama(dynamic_cast<Panorama&>(pano));
                } catch(std::bad_cast e) {
                    DEBUG_WARN("Can't handle Non- PT::Panorama instance.")
                    return false;
                }
            }
    };

    
    //=========================================================================
    //=========================================================================
    

    /** reset the panorama */
    class NewPanoCmd : public PanoCommand
    {
    public:
        NewPanoCmd(Panorama & pano)
            : PanoCommand(pano)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                pano.reset();
                pano.changeFinished();

                return true;
            }
        
        virtual std::string getName() const
            {
                return "new panorama";
            }
    };

    //=========================================================================
    //=========================================================================

    /** add image(s) to a panorama */
    class AddImagesCmd : public PanoCommand
    {
    public:
        AddImagesCmd(Panorama & pano, const std::vector<PanoImage> & images)
            : PanoCommand(pano), imgs(images)
            { };

        virtual bool processPanorama(Panorama& pano)
            {

                VariableMap var;
                fillVariableMap(var);

                std::vector<PanoImage>::const_iterator it;
                for (it = imgs.begin(); it != imgs.end(); ++it) {
                    pano.addImage(*it,var);
                }
                pano.changeFinished();

                return true;
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
    class RemoveImageCmd : public PanoCommand
    {
    public:
        RemoveImageCmd(Panorama & p, unsigned int imgNr)
            : PanoCommand(p), imgNr(imgNr)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                pano.removeImage(imgNr);
                pano.changeFinished();

                return true;
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
    class RemoveImagesCmd : public PanoCommand
    {
    public:
        RemoveImagesCmd(Panorama & p, UIntSet imgs)
            : PanoCommand(p), imgNrs(imgs)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                for (UIntSet::reverse_iterator it = imgNrs.rbegin();
                     it != imgNrs.rend(); ++it)
                {
                    pano.removeImage(*it);
                }
                pano.changeFinished();

                return true;
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
    class ChangeImageCmd : public PanoCommand
    {
    public:
        Cmd(Panorama & p)
            : PanoCommand(p)
            { };

        virtual bool processPanorama(Panorama& pano)
            {

                return true;
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
    class UpdateVariablesCmd : public PanoCommand
    {
    public:
        UpdateVariablesCmd(Panorama & p, const VariableMapVector & vars)
            : PanoCommand(p),
              vars(vars)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                pano.updateVariables(vars);
                pano.changeFinished();

                return true;
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
    class UpdateCPsCmd : public PanoCommand
    {
    public:
        UpdateCPsCmd(Panorama & p, const CPVector & cps)
            : PanoCommand(p),
              cps(cps)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                CPVector::const_iterator it;
                unsigned int i=0;
                for (it = cps.begin(); it != cps.end(); ++it, i++) {
                    pano.changeControlPoint(i, *it);
                }
                pano.changeFinished();

                return true;
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
    class UpdateVariablesCPCmd : public PanoCommand
    {
    public:
        UpdateVariablesCPCmd(Panorama & p, const VariableMapVector & vars,
                             const CPVector & cps)
            : PanoCommand(p),
              vars(vars), cps(cps)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                pano.updateVariables(vars);
                pano.updateCtrlPointErrors(cps);
                pano.changeFinished();

                return true;
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
    class UpdateVariablesCPSetCmd : public PanoCommand
    {
    public:
        UpdateVariablesCPSetCmd(Panorama & p, UIntSet imgs, const VariableMapVector & vars,
                                const CPVector & cps)
            : PanoCommand(p),
              m_imgs(imgs),
              vars(vars), cps(cps)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                pano.updateVariables(m_imgs, vars);
                pano.updateCtrlPointErrors(m_imgs, cps);
                pano.markAsOptimized();
                pano.changeFinished();

                return true;
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
    class UpdateImageVariablesCmd : public PanoCommand
    {
    public:
        UpdateImageVariablesCmd(Panorama & p, unsigned int ImgNr, const VariableMap & vars)
            : PanoCommand(p), imgNr(ImgNr),
              vars(vars)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                pano.updateVariables(imgNr, vars);
                pano.changeFinished();

                return true;
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
    class UpdateImagesVariablesCmd : public PanoCommand
    {
    public:
        UpdateImagesVariablesCmd(Panorama & p, const UIntSet & change, const VariableMapVector & vars)
            : PanoCommand(p), change(change),
              vars(vars)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                UIntSet::iterator it;
                VariableMapVector::const_iterator v_it = vars.begin();
                for (it = change.begin(); it != change.end(); ++it) {
                    pano.updateVariables(*it, *v_it);
                    ++v_it;
                }
                pano.changeFinished();

                return true;
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
    class SetVariableCmd : public PanoCommand
    {
    public:
        SetVariableCmd(Panorama & p, const UIntSet & images, const Variable & var)
            : PanoCommand(p), images(images),
              var(var)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                UIntSet::iterator it;
                for (it = images.begin(); it != images.end(); ++it) {
                    pano.updateVariable(*it, var);
                }
                pano.changeFinished();

                return true;
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
    class SetLensVariableCmd : public PanoCommand
    {
    public:
        SetLensVariableCmd(Panorama & p, int lens, const LensVarMap & var)
            : PanoCommand(p), lensNr(lens), vars(var)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                LensVarMap::const_iterator it;
                for (it = vars.begin(); it != vars.end(); ++it) {
                    pano.updateLensVariable(lensNr, it->second);
                }
                pano.changeFinished();

                return true;
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
    class SetLensVariablesCmd : public PanoCommand
    {
        public:
            SetLensVariablesCmd(Panorama & p, UIntSet lenses, const std::vector<LensVarMap> & var)
            : PanoCommand(p), lensNrs(lenses), vars(var)
            { };

            virtual bool processPanorama(Panorama& pano)
            {
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

                return true;
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
    class SetImageLensCmd : public PanoCommand
    {
    public:
        SetImageLensCmd(Panorama & p, const UIntSet & imgNrs, int lensNr)
            : PanoCommand(p),
              imgNrs(imgNrs), lensNr(lensNr)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                for (UIntSet::iterator it = imgNrs.begin();
                     it != imgNrs.end(); ++it)
                {
                    pano.setLens(*it, lensNr);
                }
                pano.changeFinished();

                return true;
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
    class CenterPanoCmd : public PanoCommand
    {
    public:
        CenterPanoCmd(Panorama & p )
            : PanoCommand(p)
            { }

        virtual bool processPanorama(Panorama& pano)
            {
                pano.centerHorizontically();
                pano.changeFinished();

                return true;
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
    class StraightenPanoCmd : public PanoCommand
    {
    public:
        StraightenPanoCmd(Panorama & p )
            : PanoCommand(p)
            { }

        virtual bool processPanorama(Panorama& pano)
            {
                pano.straighten();
                pano.changeFinished();

                return true;
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
    class AddCtrlPointCmd : public PanoCommand
    {
    public:
        AddCtrlPointCmd(Panorama & p, const ControlPoint & cpoint)
            : PanoCommand(p), point(cpoint)
            { }

        virtual bool processPanorama(Panorama& pano)
            {
                pano.addCtrlPoint(point);
                pano.changeFinished();

                return true;
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
    class AddCtrlPointsCmd : public PanoCommand
    {
    public:
        AddCtrlPointsCmd(Panorama & p, const CPVector & cpoints)
            : PanoCommand(p), cps(cpoints)
            { }

        virtual bool processPanorama(Panorama& pano)
            {
                for (CPVector::iterator it = cps.begin();
                     it != cps.end(); ++it)
                {
                    pano.addCtrlPoint(*it);
                }
                pano.changeFinished();

                return true;
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
    class RemoveCtrlPointCmd : public PanoCommand
    {
    public:
        RemoveCtrlPointCmd(Panorama & p, unsigned int cpNr)
            : PanoCommand(p), pointNr(cpNr)
            { }

        virtual bool processPanorama(Panorama& pano)
            {
                pano.removeCtrlPoint(pointNr);
                pano.changeFinished();

                return true;
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
    class RemoveCtrlPointsCmd : public PanoCommand
    {
    public:
        RemoveCtrlPointsCmd(Panorama & p, const UIntSet & points )
            : PanoCommand(p), m_points(points)
            { }

        virtual bool processPanorama(Panorama& pano)
            {
                for(UIntSet::reverse_iterator it = m_points.rbegin();
                    it != m_points.rend(); ++it)
                {
                    pano.removeCtrlPoint(*it);
                }
                pano.changeFinished();

                return true;
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
    class ChangeCtrlPointCmd : public PanoCommand
    {
    public:
        ChangeCtrlPointCmd(Panorama & p, unsigned int nr, ControlPoint point)
            : PanoCommand(p), pNr(nr), point(point)
            { }

        virtual bool processPanorama(Panorama& pano)
            {
                pano.changeControlPoint(pNr, point);
                pano.changeFinished();

                return true;
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
    class AddLensCmd : public PanoCommand
    {
    public:
        AddLensCmd(Panorama & p, const Lens & lens)
            : PanoCommand(p), lens(lens)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                pano.addLens(lens);
                pano.changeFinished();

                return true;
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
    class AddNewLensToImagesCmd : public PanoCommand
    {
    public:
        AddNewLensToImagesCmd(Panorama & p, const Lens & lens, const UIntSet & imgs)
            : PanoCommand(p), lens(lens), imgNrs(imgs)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                unsigned int lnr = pano.addLens(lens);
                for (UIntSet::iterator it = imgNrs.begin();
                     it != imgNrs.end(); ++it)
                {
                    pano.setLens(*it, lnr);
                }
                pano.changeFinished();

                return true;
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
    class ChangeLensCmd : public PanoCommand
    {
    public:
        ChangeLensCmd(Panorama & p, unsigned int lensNr, const Lens & lens)
            : PanoCommand(p), lensNr(lensNr), newLens(lens)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                pano.updateLens(lensNr, newLens);
                pano.changeFinished();

                return true;
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
    class ChangeLensesCmd : public PanoCommand
    {
    public:
        ChangeLensesCmd(Panorama & p, UIntSet & lNr, const LensVector & lenses)
            : PanoCommand(p), change(lNr), vect(lenses)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                UIntSet::iterator it;
                LensVector::const_iterator v_it = vect.begin();
                for (it = change.begin(); it != change.end(); ++it) {
                    pano.updateLens(*it, *v_it);
                    v_it++;
                }
                pano.changeFinished();

                return true;
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
    class SetActiveImagesCmd : public PanoCommand
    {
    public:
        SetActiveImagesCmd(Panorama & p, UIntSet & active)
            : PanoCommand(p), m_active(active)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
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

                return true;
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
    class SwapImagesCmd : public PanoCommand
    {
    public:
        SwapImagesCmd(Panorama & p, unsigned int i1, unsigned int i2)
            : PanoCommand(p), m_i1(i1), m_i2(i2)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                pano.swapImages(m_i1, m_i2);
                pano.changeFinished();

                return true;
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
    class SetImageOptionsCmd : public PanoCommand
    {
    public:
        SetImageOptionsCmd(Panorama & p, ImageOptions opts, UIntSet imgs)
            : PanoCommand(p), options(opts), imgNrs(imgs)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                for (UIntSet::iterator it = imgNrs.begin();
                     it != imgNrs.end(); ++it)
                {
                    pano.setImageOptions(*it, options);
                }
                pano.changeFinished();

                return true;
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
    class UpdateSrcImageCmd : public PanoCommand
    {
        public:
            UpdateSrcImageCmd(Panorama & p, unsigned i, SrcPanoImage img)
            : PanoCommand(p), img(img), imgNr(i)
            { };

            virtual bool processPanorama(Panorama& pano)
            {
                pano.setSrcImage(imgNr, img);
                pano.changeFinished();

                return true;
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
    class UpdateSrcImagesCmd : public PanoCommand
    {
        public:
            UpdateSrcImagesCmd(Panorama & p, UIntSet i, std::vector<SrcPanoImage> imgs)
            : PanoCommand(p), imgs(imgs), imgNrs(i)
            { };

            virtual bool processPanorama(Panorama& pano)
            {
                int i = 0;
                for (UIntSet::iterator it = imgNrs.begin();
                     it != imgNrs.end(); ++it)
                {
                    pano.setSrcImage(*it, imgs[i]);
                    i++;
                }
                pano.changeFinished();

                return true;
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
    class UpdateImageOptionsCmd : public PanoCommand
    {
        public:
            UpdateImageOptionsCmd(Panorama & p, std::vector<ImageOptions> opts, UIntSet imgs)
            : PanoCommand(p), options(opts), imgNrs(imgs)
            {
                assert(opts.size() == imgs.size());
            };

            virtual bool processPanorama(Panorama& pano)
            {
                int i=0;
                for (UIntSet::iterator it = imgNrs.begin();
                     it != imgNrs.end(); ++it)
                {
                    pano.setImageOptions(*it, options[i]);
                    i++;
                }
                pano.changeFinished();

                return true;
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
    class SetPanoOptionsCmd : public PanoCommand
    {
    public:
        SetPanoOptionsCmd(Panorama & p, const PanoramaOptions & opts)
            : PanoCommand(p), options(opts)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                pano.setOptions(options);
                pano.changeFinished();

                return true;
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
    class LoadPTProjectCmd : public PanoCommand
    {
    public:
        LoadPTProjectCmd(Panorama & p, std::istream & i, const std::string & prefix = "")
            : PanoCommand(p),
              in(i),
	      prefix(prefix)
            { }

        virtual bool processPanorama(Panorama& pano)
            {
                PanoramaMemento newPano;
                if (newPano.loadPTScript(in,prefix)) {
                    pano.setMemento(newPano);
                } else {
                    DEBUG_ERROR("could not load panotools script");
                }
                pano.changeFinished();

                return true;
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
    class SetVigCorrCmd : public PanoCommand
    {
        public:
            SetVigCorrCmd(Panorama & p, unsigned int lensNr,
                          unsigned int vigCorrMode, std::vector<double> & coeff,
                          const std::string & flatfield)
            : PanoCommand(p), m_lensNr(lensNr),
              m_mode(vigCorrMode), m_coeff(coeff), m_flat(flatfield)
            { };

            virtual bool processPanorama(Panorama& pano)
            {
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

                return true;
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
    class RotatePanoCmd : public PanoCommand
    {
    public:
        RotatePanoCmd(Panorama & p, double yaw, double pitch, double roll)
            : PanoCommand(p), y(yaw), p(pitch), r(roll)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                pano.rotate(y, p, r);
                pano.changeFinished();

                return true;
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
