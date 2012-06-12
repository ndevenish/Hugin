// -*- c-basic-offset: 4 -*-
/** @file hugin1/PT/PanoCommand.h
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

#ifndef _Hgn1_PANOCOMMAND_H
#define _Hgn1_PANOCOMMAND_H


#include <huginapp/PanoCommand.h>

//#include "PanoImage.h"
#include "Panorama.h"
//#include "PanoToolsInterface.h"
#include <panodata/StandardImageVariableGroups.h>
#include <panotools/PanoToolsUtils.h>


namespace PT {

    /** default panorama cmd, provides undo with mementos
     */
    class PanoCommand : public HuginBase::PanoCommand<std::string>
    {
        public:
            PanoCommand(Panorama& p)
              : HuginBase::PanoCommand<std::string>(p), o_pano(p)
            {
            };

            virtual ~PanoCommand() {};
            
            virtual bool processPanorama(HuginBase::ManagedPanoramaData& panoramaData)
            {
                return processPanorama(o_pano);
            }
            
            virtual bool processPanorama(Panorama& pano) { return true; };
            
        protected:
            Panorama& o_pano;
    };
    
    //=========================================================================
    //=========================================================================
    
    /** PanoCommand to combine other PanoCommands.
        Use to get one Undo step from what would normally be several
        PanoCommands.
     */
    class CombinedPanoCommand : public PanoCommand
    {
        public:
            /** Constructor.
             *  @param commands List of pointers to commands. The applied from
                                beginning to end. CombinedPanoCommand deletes
                                the commands when it is itself deleted.
             */
            CombinedPanoCommand(Panorama & pano, std::vector<PanoCommand*> & commands)
                : PanoCommand(pano), commands(commands)
                {};
            ~CombinedPanoCommand()
                {
                    for (std::vector<PanoCommand*>::iterator it = commands.begin();
                         it != commands.end();
                         it++)
                    {
                        delete *it;
                    }
                }
            virtual bool processPanorama(Panorama & pano)
                {
                    bool result = true;
                    for (std::vector<PanoCommand*>::iterator it = commands.begin();
                         it != commands.end();
                         it++)
                    {
                        result &= (**it).processPanorama(pano);
                    }
                    /// @todo Should I revert if processing fails?
                    return result;
                };
            
            virtual std::string getName() const
                {
                    return "multiple commmands";
                };
        private:
            std::vector<PanoCommand*> commands;
    };


    //=========================================================================
    //=========================================================================
    

    /** reset the panorama */
    class NewPanoCmd : public PanoCommand
    {
    public:
        NewPanoCmd(Panorama & pano)
            : PanoCommand(pano)
            { m_clearDirty=true; };

        virtual bool processPanorama(Panorama& pano)
            {
                pano.reset();

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
        AddImagesCmd(Panorama & pano, const std::vector<SrcPanoImage> & images)
            : PanoCommand(pano), imgs(images)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                std::vector<SrcPanoImage>::const_iterator it;
                for (it = imgs.begin(); it != imgs.end(); ++it) {
                    pano.addImage(*it);
                }

                return true;
            }
        
        virtual std::string getName() const
            {
                return "add images";
            }

    private:
        std::vector<SrcPanoImage> imgs;
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
        UpdateCPsCmd(Panorama & p, const CPVector & cps, bool doUpdateCPError=true)
            : PanoCommand(p),
              cps(cps), updateCPError(doUpdateCPError)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                CPVector::const_iterator it;
                unsigned int i=0;
                for (it = cps.begin(); it != cps.end(); ++it, i++) {
                    pano.changeControlPoint(i, *it);
                }
                if(updateCPError)
                {
                    HuginBase::PTools::calcCtrlPointErrors(pano);
                };

                return true;
            }
        
        virtual std::string getName() const
            {
                return "update control points";
            }

    private:
        CPVector cps;
        bool updateCPError;
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
                HuginBase::PTools::calcCtrlPointErrors(pano);

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

    /** updates the optimize vector, aka all variables which should be optimized */
    class UpdateOptimizeVectorCmd : public PanoCommand
    {
    public:
        UpdateOptimizeVectorCmd(Panorama &p, OptimizeVector optvec)
            : PanoCommand(p),
              m_optvec(optvec)
        { };

    virtual bool processPanorama(Panorama & pano)
        {
            pano.setOptimizeVector(m_optvec);
            return true;
        }

    virtual std::string getName() const
        {
            return "update optimize vector";
        }

    private:
        OptimizeVector m_optvec;
        int mode;

    };


    //=========================================================================
    //=========================================================================

    /** update the optimizer master switch */
    class UpdateOptimizerSwitchCmd : public PanoCommand
    {
    public:
        UpdateOptimizerSwitchCmd(Panorama &p, int mode)
            : PanoCommand(p),
              m_mode(mode)
        { };

    virtual bool processPanorama(Panorama & pano)
        {
            pano.setOptimizerSwitch(m_mode);
            return true;
        }

    virtual std::string getName() const
        {
            return "update optimizer master switch";
        }

    private:
        int m_mode;
    };

    //=========================================================================
    //=========================================================================

    /** update the photometric optimizer master switch */
    class UpdatePhotometricOptimizerSwitchCmd : public PanoCommand
    {
    public:
        UpdatePhotometricOptimizerSwitchCmd(Panorama &p, int mode)
            : PanoCommand(p),
              m_mode(mode)
        { };

    virtual bool processPanorama(Panorama & pano)
        {
            pano.setPhotometricOptimizerSwitch(m_mode);
            return true;
        }

    virtual std::string getName() const
        {
            return "update photometric optimizer master switch";
        }

    private:
        int m_mode;
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
                HuginBase::PTools::calcCtrlPointErrors(pano);

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
                // adjust canvas size
                PanoramaOptions opts=pano.getOptions();
                double hfov, height;
                pano.fitPano(hfov, height);
                opts.setHFOV(hfov);
                opts.setHeight(hugin_utils::roundi(height));
                pano.setOptions(opts);

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


    /** straighten panorama horizontically */
    class StraightenPanoCmd : public PanoCommand
    {
    public:
        StraightenPanoCmd(Panorama & p )
            : PanoCommand(p)
            { }

        virtual bool processPanorama(Panorama& pano)
            {
                pano.straighten();
                PanoramaOptions opts=pano.getOptions();
                if(opts.getHFOV()<360)
                {
                    // center non 360 deg panos
                    pano.centerHorizontically();
                };
                // adjust canvas size
                double hfov, height;
                pano.fitPano(hfov, height);
                opts.setHFOV(hfov);
                opts.setHeight(hugin_utils::roundi(height));
                pano.setOptions(opts);

                return true;
            }
        
        virtual std::string getName() const
            {
                return "straighten panorama";
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
                HuginBase::PTools::calcCtrlPointErrors(pano);

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
                HuginBase::PTools::calcCtrlPointErrors(pano);

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
                HuginBase::PTools::calcCtrlPointErrors(pano);

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
				for (unsigned int i = 0; i < pano.getNrOfImages(); i++) {
                    if (set_contains(m_active, i)) {
						pano.activateImage(i, true);
					} else {
						pano.activateImage(i, false);
					}
                }

                return true;
            }
        
        virtual std::string getName() const
            {
                return "change active images";
            }

    private:
        UIntSet m_active;
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

                return true;
            }
        
        virtual std::string getName() const
            {
                return "swap images";
            }

    private:
        unsigned int m_i1;
        unsigned int m_i2;
    };

    /** move image from position1 to position2 */
    class MoveImageCmd : public PanoCommand
    {
    public:
        MoveImageCmd(Panorama & p, size_t i1, size_t i2)
            : PanoCommand(p), m_i1(i1), m_i2(i2)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                pano.moveImage(m_i1, m_i2);

                return true;
            }
        
        virtual std::string getName() const
            {
                return "move images";
            }

    private:
        unsigned int m_i1;
        unsigned int m_i2;
    };

    /** merge two project files */
    class MergePanoCmd : public PanoCommand
    {
    public:
        MergePanoCmd(Panorama & p, Panorama & p2)
            : PanoCommand(p), newPano(p2)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                pano.mergePanorama(newPano);
                HuginBase::PTools::calcCtrlPointErrors(pano);

                return true;
            }
        
        virtual std::string getName() const
            {
                return "merge panorama";
            }

    private:
        Panorama newPano;
    };

    //=========================================================================
    //=========================================================================

    /** update source image
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

                return true;
            }
            
        virtual std::string getName() const
            {
                return "update source image";
            }
    
    private:
            SrcPanoImage img;
            unsigned imgNr;
    };

    //=========================================================================
    //=========================================================================

    /** update source images
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
                HuginBase::PTools::calcCtrlPointErrors(pano);

                return true;
            }
            
        virtual std::string getName() const
            {
                return "update source images";
            }
    
    private:
            std::vector<SrcPanoImage> imgs;
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

                return true;
            }
        
        virtual std::string getName() const
            {
                return "set panorama options";
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
        LoadPTProjectCmd(Panorama & p, const std::string  & filename, const std::string & prefix = "")
            : PanoCommand(p),
            filename(filename),
            prefix(prefix)
            { m_clearDirty=true; }

        virtual bool processPanorama(Panorama& pano)
            {
                std::ifstream in(filename.c_str());
#ifndef _Hgn1_PANORAMA_H
                PanoramaMemento newPano;
                if (newPano.loadPTScript(in,prefix)) {
                    pano.setMemento(newPano);
                } else {
                    DEBUG_ERROR("could not load panotools script");
                }
#else
                AppBase::DocumentData::ReadWriteError err = pano.readData(in);
                if (err != AppBase::DocumentData::SUCCESSFUL) {
                    DEBUG_ERROR("could not load panotools script");
                    return false;
                }
#endif
                
                in.close();

                return true;
            }
        
        virtual std::string getName() const
            {
                return "load project";
            }

    private:
        const std::string &filename;
        const std::string &prefix;
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

                return true;
            }
        
        virtual std::string getName() const
            {
                return "rotate panorama";
            }

    private:
        double y,p,r;
    };

    //=========================================================================
    //=========================================================================

    /** Translate the panorama
     */
    class TranslatePanoCmd : public PanoCommand
    {
    public:
        TranslatePanoCmd(Panorama & p, double TrX, double TrY, double TrZ)
            : PanoCommand(p), X(TrX), Y(TrY), Z(TrZ)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                pano.translate(X, Y, Z);

                return true;
            }

        virtual std::string getName() const
            {
                return "translate panorama";
            }

     private:
       double X,Y,Z;
     };

    //=========================================================================
    //=========================================================================



    /** Update the focal length 
     */
    class UpdateFocalLengthCmd : public PanoCommand
    {
    public:
        UpdateFocalLengthCmd(Panorama & p, UIntSet imgs, double newFocalLength)
            : PanoCommand(p), imgNrs(imgs), m_focalLength(newFocalLength)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                pano.UpdateFocalLength(imgNrs,m_focalLength);
                return true;
            }
        
        virtual std::string getName() const
            {
                return "update focal length";
            }

    private:
        UIntSet imgNrs;
        double m_focalLength;
    };

    //=========================================================================
    //=========================================================================


    /** Update the crop factor 
     */
    class UpdateCropFactorCmd : public PanoCommand
    {
    public:
        UpdateCropFactorCmd(Panorama & p, UIntSet imgs, double newCropFactor)
            : PanoCommand(p), imgNrs(imgs), m_cropFactor(newCropFactor)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                //search all image with the same lens, otherwise the crop factor is updated via links, 
                //but not the hfov if the image is not the given UIntSet
                UIntSet allImgWithSameLens;
                UIntSet testedLens;
                HuginBase::StandardImageVariableGroups variable_groups(pano);
                HuginBase::ImageVariableGroup & lenses = variable_groups.getLenses();
                for(UIntSet::const_iterator it=imgNrs.begin();it!=imgNrs.end();it++)
                {
                    allImgWithSameLens.insert(*it);
                    unsigned int lensNr=lenses.getPartNumber(*it);
                    if(set_contains(testedLens,lensNr))
                        continue;
                    testedLens.insert(lensNr);
                    for(unsigned int i=0;i<pano.getNrOfImages();i++)
                    {
                        if(lenses.getPartNumber(i)==lensNr)
                        {
                            allImgWithSameLens.insert(i);
                        };
                    };
                };
                pano.UpdateCropFactor(allImgWithSameLens, m_cropFactor);
                return true;
            }
        
        virtual std::string getName() const
            {
                return "update crop factor";
            }

    private:
        UIntSet imgNrs;
        double m_cropFactor;
    };

    //=========================================================================
    //=========================================================================


    /** Switch the part number of an image
     */
    class ChangePartNumberCmd : public PanoCommand
    {
    public:
        ChangePartNumberCmd(Panorama & p,
                            UIntSet image_numbers,
                            std::size_t new_part_number,
                            std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> variables
                            )
            :   PanoCommand(p),
                image_numbers(image_numbers),
                new_part_number(new_part_number),
                variables(variables)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                // it might change as we are setting them
                std::size_t new_new_part_number = new_part_number;
                HuginBase::ImageVariableGroup group(variables, pano);
                for (UIntSet::iterator it = image_numbers.begin();
                     it != image_numbers.end(); it++)
                {
                    group.switchParts(*it, new_new_part_number);
                    // update the lens number if it changes.
                    new_new_part_number = group.getPartNumber(*it);
                }
                return true;
            }
        
        virtual std::string getName() const
            {
                return "Change part number";
            }

    private:
        UIntSet image_numbers;
        std::size_t new_part_number;
        std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> variables;
    };
    
    //=========================================================================
    //=========================================================================
    
    
    /** Change the linking of some variables across parts of an
     * ImageVariableGroup containing some specified images.
     */
    class ChangePartImagesLinkingCmd : public PanoCommand
    {
    public:
        /** Constructor.
         * @param p the panorama this affects
         * @param image_numbers the set of image numbers that are contained
         * within the parts you would like to link or unlink.
         * @param changeVariables the set of variables you would like to change
         * the linking of across those parts.
         * @param new_linked_state true to link variables, false to unlink them.
         * @param groupVariables the variables that make the ImageVariableGroup
         * that will define which images belong to which parts.
         */
        ChangePartImagesLinkingCmd(Panorama & p,
                    UIntSet image_numbers,
                    std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> changeVariables,
                    bool new_linked_state,
                    std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> groupVariables)
            :   PanoCommand(p),
                image_numbers(image_numbers),
                changeVariables(changeVariables),
                new_linked_state(new_linked_state),
                groupVariables(groupVariables)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                HuginBase::ImageVariableGroup group(groupVariables, pano);
                if (new_linked_state)
                {
                    for (UIntSet::iterator imageIt = image_numbers.begin();
                        imageIt != image_numbers.end(); imageIt++)
                    {
                        // link the variables
                        for (std::set<HuginBase::ImageVariableGroup::ImageVariableEnum>::iterator variableIt = changeVariables.begin();
                             variableIt != changeVariables.end(); variableIt++)
                        {
                            group.linkVariableImage(*variableIt, *imageIt);
                        }
                    }
                } else {
                    for (UIntSet::iterator imageIt = image_numbers.begin();
                        imageIt != image_numbers.end(); imageIt++)
                    {
                        // unlink the variable
                        for (std::set<HuginBase::ImageVariableGroup::ImageVariableEnum>::iterator variableIt = changeVariables.begin();
                             variableIt != changeVariables.end(); variableIt++)
                        {
                            group.unlinkVariableImage(*variableIt, *imageIt);
                            group.updatePartNumbers();
                        }
                    }
                }
                return true;
            }
        
        virtual std::string getName() const
            {
                return "Change image variable links";
            }

    private:
        UIntSet image_numbers;
        std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> changeVariables;
        bool new_linked_state;
        std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> groupVariables;
    };
    
    
    //=========================================================================
    //=========================================================================
    
    
    /** Link a set of lens variables for some lens.
     */
    class LinkLensVarsCmd : public PanoCommand
    {
    public:
        LinkLensVarsCmd(Panorama & p,
                    std::size_t lens_number,
                    std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> variables
                    )
            :   PanoCommand(p),
                lens_number(lens_number),
                variables(variables)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                HuginBase::StandardImageVariableGroups variable_groups(pano);
                HuginBase::ImageVariableGroup & lenses = variable_groups.getLenses();
                std::set<HuginBase::ImageVariableGroup::ImageVariableEnum>::iterator it;
                for (it = variables.begin(); it != variables.end(); it++)
                {
                    lenses.linkVariablePart(*it, lens_number);
                }
                return true;
            }
        
        virtual std::string getName() const
            {
                return "Link lens variables";
            }

    private:
        std::size_t lens_number;
        std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> variables;
    };
    //=========================================================================
    //=========================================================================
    
/// @todo avoid copying image data in processPanorama
#define image_variable( name, type, default_value )\
    class ChangeImage##name##Cmd : public PanoCommand\
    {\
    public:\
        ChangeImage##name##Cmd(Panorama & p,\
                               UIntSet image_numbers,\
                               type value)\
            :   PanoCommand(p),\
                image_numbers(image_numbers),\
                value(value)\
            { };\
        \
        virtual bool processPanorama(Panorama& pano)\
            {\
                for (UIntSet::iterator it = image_numbers.begin();\
                     it != image_numbers.end(); it++)\
                {\
                    HuginBase::SrcPanoImage img = pano.getSrcImage(*it);\
                    img.set##name(value);\
                    pano.setSrcImage(*it, img);\
                }\
                return true;\
            }\
        \
        virtual std::string getName() const\
            {\
                return "Change image's " #name;\
            }\
    private:\
        UIntSet image_numbers;\
        type value;\
    };
#include <panodata/image_variables.h>
#undef image_variable
    
    
    //=========================================================================
    //=========================================================================
    
    
    /** Make a new part in a ImageVariableGroup for a set of images, given the
     * variables that make up the group.
     */
    class NewPartCmd : public PanoCommand
    {
    public:
        /** Constructor.
         * @param p Panorama to act up
         * @param image_numbers A set of images which should all be in a single
         * new group.
         * @param vars The set of image variables that make up the group. Should
         * be got from StandardVariableGroups.
         */
        NewPartCmd( Panorama & p,
                    UIntSet image_numbers,
                    std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> vars)
            :   PanoCommand(p),
                image_numbers(image_numbers),
                vars(vars)
        { }
        
        virtual bool processPanorama(Panorama& pano)
        {
            // unlink all the variables in the first image.
            DEBUG_ASSERT (image_numbers.size() > 0);
            unsigned int image_index = *image_numbers.begin();
            for (std::set<HuginBase::ImageVariableGroup::ImageVariableEnum>::iterator it = vars.begin();
                    it != vars.end(); it++)
            {
                switch (*it)
                {
#define image_variable( name, type, default_value )\
                    case HuginBase::ImageVariableGroup::IVE_##name:\
                        pano.unlinkImageVariable##name(image_index);\
                        break;
#include <panodata/image_variables.h>
#undef image_variable
                }
            }
            // now the first image should have a new part in the group.
            // we want to switch the rest of the images to the new part.
            HuginBase::ImageVariableGroup group(vars, pano);
            for (UIntSet::iterator it = ++image_numbers.begin();
                    it != image_numbers.end(); it++)
            {
                std::size_t part_number = group.getPartNumber(image_index);
                group.switchParts(*it, part_number);
            }
            return true;
        }
        
        virtual std::string getName() const
            {
                return "New Part";
            }

    private:
        UIntSet image_numbers;
        std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> vars;
    };

    //=========================================================================
    //=========================================================================

    /** update mask for given image */
    class UpdateMaskForImgCmd : public PanoCommand
    {
    public:
        UpdateMaskForImgCmd(Panorama & p, unsigned int img, const HuginBase::MaskPolygonVector & mask)
            : PanoCommand(p),
              m_img(img), m_mask(mask)
            { };

        virtual bool processPanorama(Panorama& pano)
            {
                pano.updateMasksForImage(m_img,m_mask);

                return true;
            }
        
        virtual std::string getName() const
            {
                return "update mask";
            }

    private:
        unsigned int m_img;
        HuginBase::MaskPolygonVector m_mask;
    };

    //=========================================================================
    //=========================================================================

    /** update global white balance */
    class UpdateWhiteBalance : public PanoCommand
    {
    public:
        UpdateWhiteBalance(Panorama & p, double redFactor, double blueFactor)
            : PanoCommand(p),
              m_red(redFactor), m_blue(blueFactor)
        { };

        virtual bool processPanorama(Panorama& pano)
            {
                pano.updateWhiteBalance(m_red,m_blue);

                return true;
            }
        
        virtual std::string getName() const
            {
                return "update global white balance";
            }

    private:
        double m_red;
        double m_blue;
    };

    //=========================================================================
    //=========================================================================

    /** reset output exposure to mean exposure of all images */
    class ResetToMeanExposure : public PanoCommand
    {
    public:
        ResetToMeanExposure(Panorama & p)
            : PanoCommand(p)
        { };

        virtual bool processPanorama(Panorama& pano)
            {
                HuginBase::PanoramaOptions opts=pano.getOptions();
                opts.outputExposureValue = PT::calcMeanExposure(pano);
                pano.setOptions(opts);

                return true;
            }
        
        virtual std::string getName() const
            {
                return "set exposure to mean exposure";
            }
    };

    //=========================================================================
    //=========================================================================

    /** distributes all images above the sphere, for the assistant */
    class DistributeImagesCmd : public PanoCommand
    {
    public:
        DistributeImagesCmd(Panorama & p)
            : PanoCommand(p)
        { };

        virtual bool processPanorama(Panorama& pano)
            {
                int nrImages=pano.getNrOfImages();
                if(nrImages>0)
                {
                    const SrcPanoImage& img=pano.getImage(0);
                    double hfov=img.getHFOV();
                    int imgsPerRow;
                    //distribute all images
                    //for rectilinear images calculate number of rows
                    if(img.getProjection()==HuginBase::SrcPanoImage::RECTILINEAR)
                    {
                        imgsPerRow=std::max(3, int(360/(0.8*hfov)));
                        imgsPerRow=std::min(imgsPerRow, nrImages);
                    }
                    else
                    {
                        //all other images do in one row to prevent cluttered images with fisheye images and the like
                        imgsPerRow=nrImages;
                    };
                    double offset=0.75*hfov;
                    if((imgsPerRow-1)*offset>360)
                    {
                        offset=360/(imgsPerRow-1);
                    };
                    double yaw=-(imgsPerRow-1)/2.0*offset;
                    double pitch=0;
                    if(imgsPerRow<nrImages)
                    {
                        pitch=(-(std::ceil(double(nrImages)/double(imgsPerRow))-1)/2.0*offset);
                    };
                    HuginBase::VariableMapVector varsVec=pano.getVariables();
                    size_t counter=0;
                    for(size_t i=0; i<nrImages; i++)
                    {
                        HuginBase::VariableMap::iterator it=varsVec[i].find("y");
                        if(it!=varsVec[i].end())
                        {
                            it->second.setValue(yaw);
                        };
                        it=varsVec[i].find("p");
                        if(it!=varsVec[i].end())
                        {
                            it->second.setValue(pitch);
                        };
                        yaw+=offset;
                        counter++;
                        if(counter==imgsPerRow)
                        {
                            counter=0;
                            pitch+=offset;
                            yaw=-(imgsPerRow-1)/2.0*offset;
                        };
                    };
                    pano.updateVariables(varsVec);
                };

                return true;
            }
        
        virtual std::string getName() const
            {
                return "distribute images";
            }
    };


} // namespace PT

#endif // _PANOCOMMAND_H
