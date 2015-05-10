// -*- c-basic-offset: 4 -*-
/** @file hugin1/base_wx/PanoCommand.h
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
*  License along with this software. If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#ifndef _Hgn1_PANOCOMMAND_H
#define _Hgn1_PANOCOMMAND_H


#include "Command.h"

#include "panodata/Panorama.h"
#include <panodata/StandardImageVariableGroups.h>

namespace PanoCommand
{
    /** PanoCommand to combine other PanoCommands.
    Use to get one Undo step from what would normally be several
    PanoCommands.
    */
    class WXIMPEX CombinedPanoCommand : public PanoCommand
    {
    public:
        /** Constructor.
        *  @param commands List of pointers to commands. They applied from
        *  beginning to end. CombinedPanoCommand deletes the commands when it is itself deleted.
        */
        CombinedPanoCommand(HuginBase::Panorama & pano, std::vector<PanoCommand*> & commands)
            : PanoCommand(pano), commands(commands) {};
        ~CombinedPanoCommand();
        virtual bool processPanorama(HuginBase::Panorama & pano);
        virtual std::string getName() const { return "multiple commmands"; };
    private:
        std::vector<PanoCommand*> commands;
    };

    /** reset the panorama */
    class WXIMPEX NewPanoCmd : public PanoCommand
    {
    public:
        NewPanoCmd(HuginBase::Panorama & pano) : PanoCommand(pano) { m_clearDirty = true; };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "new panorama"; };
    };

    /** add image(s) to a panorama */
    class WXIMPEX AddImagesCmd : public PanoCommand
    {
    public:
        AddImagesCmd(HuginBase::Panorama & pano, const std::vector<HuginBase::SrcPanoImage> & images)
            : PanoCommand(pano), imgs(images) { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "add images"; };
    private:
        std::vector<HuginBase::SrcPanoImage> imgs;
    };

    /** remove an image from a panorama
    *
    *  @todo would be nice to remove multiple at once
    */
    class WXIMPEX RemoveImageCmd : public PanoCommand
    {
    public:
        RemoveImageCmd(HuginBase::Panorama & p, unsigned int imgNr)
            : PanoCommand(p), imgNr(imgNr)
        { };

        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "remove image"; };
    private:
        unsigned int imgNr;
    };

    /** remove multiple images from a panorama
    */
    class WXIMPEX RemoveImagesCmd : public PanoCommand
    {
    public:
        RemoveImagesCmd(HuginBase::Panorama & p, HuginBase::UIntSet imgs) : PanoCommand(p), imgNrs(imgs) {};
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "remove images"; };
    private:
        HuginBase::UIntSet imgNrs;
    };

    /** update all variables */
    class WXIMPEX UpdateVariablesCmd : public PanoCommand
    {
    public:
        UpdateVariablesCmd(HuginBase::Panorama & p, const HuginBase::VariableMapVector & vars)
            : PanoCommand(p), vars(vars) {};
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "update variables"; };
    private:
        HuginBase::VariableMapVector vars;
    };

    /** update all control points*/
    class WXIMPEX UpdateCPsCmd : public PanoCommand
    {
    public:
        UpdateCPsCmd(HuginBase::Panorama & p, const HuginBase::CPVector & cps, bool doUpdateCPError = true)
            : PanoCommand(p), cps(cps), updateCPError(doUpdateCPError)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "update control points"; };
    private:
        HuginBase::CPVector cps;
        bool updateCPError;
    };

    /** update all variables & control points*/
    class WXIMPEX UpdateVariablesCPCmd : public PanoCommand
    {
    public:
        UpdateVariablesCPCmd(HuginBase::Panorama & p, const HuginBase::VariableMapVector & vars, const HuginBase::CPVector& cps)
            : PanoCommand(p), vars(vars), cps(cps)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "update variables and control points"; };
    private:
        HuginBase::VariableMapVector vars;
        HuginBase::CPVector cps;
    };

    /** update all variables & control points*/
    class WXIMPEX UpdateVariablesCPSetCmd : public PanoCommand
    {
    public:
        UpdateVariablesCPSetCmd(HuginBase::Panorama & p, HuginBase::UIntSet imgs, const HuginBase::VariableMapVector & vars, const HuginBase::CPVector & cps)
            : PanoCommand(p), m_imgs(imgs), vars(vars), cps(cps)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "update variables and control points"; };
    private:
        HuginBase::UIntSet m_imgs;
        HuginBase::VariableMapVector vars;
        HuginBase::CPVector cps;
    };

    /** update variables of a single image */
    class WXIMPEX UpdateImageVariablesCmd : public PanoCommand
    {
    public:
        UpdateImageVariablesCmd(HuginBase::Panorama & p, unsigned int ImgNr, const HuginBase::VariableMap & vars)
            : PanoCommand(p), imgNr(ImgNr), vars(vars)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "update image variables"; };
    private:
        unsigned int imgNr;
        HuginBase::VariableMap vars;
    };

    /** update variables of a group of images */
    class WXIMPEX UpdateImagesVariablesCmd : public PanoCommand
    {
    public:
        UpdateImagesVariablesCmd(HuginBase::Panorama & p, const HuginBase::UIntSet & change, const HuginBase::VariableMapVector & vars)
            : PanoCommand(p), change(change), vars(vars)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "update image variables"; }
    private:
        HuginBase::UIntSet change;
        HuginBase::VariableMapVector vars;
    };

    /** updates the optimize vector, aka all variables which should be optimized */
    class WXIMPEX UpdateOptimizeVectorCmd : public PanoCommand
    {
    public:
        UpdateOptimizeVectorCmd(HuginBase::Panorama &p, HuginBase::OptimizeVector optvec)
            : PanoCommand(p), m_optvec(optvec)
        { };
        virtual bool processPanorama(HuginBase::Panorama & pano);
        virtual std::string getName() const { return "update optimize vector"; }
    private:
        HuginBase::OptimizeVector m_optvec;
    };

    /** update the optimizer master switch */
    class WXIMPEX UpdateOptimizerSwitchCmd : public PanoCommand
    {
    public:
        UpdateOptimizerSwitchCmd(HuginBase::Panorama &p, int mode) : PanoCommand(p), m_mode(mode) { };
        virtual bool processPanorama(HuginBase::Panorama & pano);
        virtual std::string getName() const { return "update optimizer master switch"; }
    private:
        int m_mode;
    };

    /** update the photometric optimizer master switch */
    class WXIMPEX UpdatePhotometricOptimizerSwitchCmd : public PanoCommand
    {
    public:
        UpdatePhotometricOptimizerSwitchCmd(HuginBase::Panorama &p, int mode) : PanoCommand(p), m_mode(mode) { };
        virtual bool processPanorama(HuginBase::Panorama & pano);
        virtual std::string getName() const { return "update photometric optimizer master switch"; }
    private:
        int m_mode;
    };

    /** update a single variable, possibly for a group of images */
    class WXIMPEX SetVariableCmd : public PanoCommand
    {
    public:
        SetVariableCmd(HuginBase::Panorama & p, const HuginBase::UIntSet & images, const HuginBase::Variable & var)
            : PanoCommand(p), images(images), var(var)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "set image variable"; }
    private:
        HuginBase::UIntSet images;
        HuginBase::Variable var;
    };

    /** center panorama horizontically */
    class WXIMPEX CenterPanoCmd : public PanoCommand
    {
    public:
        CenterPanoCmd(HuginBase::Panorama & p) : PanoCommand(p) { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "center panorama"; }
    };

    /** straighten panorama horizontically */
    class WXIMPEX StraightenPanoCmd : public PanoCommand
    {
    public:
        StraightenPanoCmd(HuginBase::Panorama & p) : PanoCommand(p) { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "straighten panorama"; };
    };

    /** add a control point */
    class WXIMPEX AddCtrlPointCmd : public PanoCommand
    {
    public:
        AddCtrlPointCmd(HuginBase::Panorama & p, const HuginBase::ControlPoint & cpoint)
            : PanoCommand(p), point(cpoint)
        {};
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "add control point"; };
    private:
        HuginBase::ControlPoint point;
    };

    /** add multiple control points */
    class WXIMPEX AddCtrlPointsCmd : public PanoCommand
    {
    public:
        AddCtrlPointsCmd(HuginBase::Panorama & p, const HuginBase::CPVector & cpoints)
            : PanoCommand(p), cps(cpoints)
        { }
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "add control points"; };
    private:
        HuginBase::CPVector cps;
    };

    /** remove a control point */
    class WXIMPEX RemoveCtrlPointCmd : public PanoCommand
    {
    public:
        RemoveCtrlPointCmd(HuginBase::Panorama & p, unsigned int cpNr)
            : PanoCommand(p), pointNr(cpNr)
        { }
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "remove control point"; };
    private:
        unsigned int pointNr;
    };

    /** remove several  control points */
    class WXIMPEX RemoveCtrlPointsCmd : public PanoCommand
    {
    public:
        RemoveCtrlPointsCmd(HuginBase::Panorama & p, const HuginBase::UIntSet & points)
            : PanoCommand(p), m_points(points)
        { }
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "remove control points"; };
    private:
        HuginBase::UIntSet m_points;
    };

    /** change a control point */
    class WXIMPEX ChangeCtrlPointCmd : public PanoCommand
    {
    public:
        ChangeCtrlPointCmd(HuginBase::Panorama & p, unsigned int nr, HuginBase::ControlPoint point)
            : PanoCommand(p), pNr(nr), point(point)
        {};
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "change control point"; };
    private:
        unsigned int pNr;
        HuginBase::ControlPoint point;
    };

    /** set active images */
    class WXIMPEX SetActiveImagesCmd : public PanoCommand
    {
    public:
        SetActiveImagesCmd(HuginBase::Panorama & p, HuginBase::UIntSet & active)
            : PanoCommand(p), m_active(active)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "change active images"; };
    private:
        HuginBase::UIntSet m_active;
    };

    /** swap two images */
    class WXIMPEX SwapImagesCmd : public PanoCommand
    {
    public:
        SwapImagesCmd(HuginBase::Panorama & p, unsigned int i1, unsigned int i2)
            : PanoCommand(p), m_i1(i1), m_i2(i2)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "swap images"; };
    private:
        unsigned int m_i1;
        unsigned int m_i2;
    };

    /** move image from position1 to position2 */
    class WXIMPEX MoveImageCmd : public PanoCommand
    {
    public:
        MoveImageCmd(HuginBase::Panorama & p, size_t i1, size_t i2)
            : PanoCommand(p), m_i1(i1), m_i2(i2)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "move images"; };
    private:
        unsigned int m_i1;
        unsigned int m_i2;
    };

    /** merge two project files */
    class WXIMPEX MergePanoCmd : public PanoCommand
    {
    public:
        MergePanoCmd(HuginBase::Panorama & p, HuginBase::Panorama & p2)
            : PanoCommand(p), newPano(p2)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "merge panorama"; };
    private:
        HuginBase::Panorama newPano;
    };

    /** update source image
    */
    class WXIMPEX UpdateSrcImageCmd : public PanoCommand
    {
    public:
        UpdateSrcImageCmd(HuginBase::Panorama & p, unsigned i, HuginBase::SrcPanoImage img)
            : PanoCommand(p), img(img), imgNr(i)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const{ return "update source image"; };
    private:
        HuginBase::SrcPanoImage img;
        unsigned imgNr;
    };

    /** update source images
    */
    class WXIMPEX UpdateSrcImagesCmd : public PanoCommand
    {
    public:
        UpdateSrcImagesCmd(HuginBase::Panorama & p, HuginBase::UIntSet i, std::vector<HuginBase::SrcPanoImage> imgs)
            : PanoCommand(p), imgs(imgs), imgNrs(i)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "update source images"; };
    private:
        std::vector<HuginBase::SrcPanoImage> imgs;
        HuginBase::UIntSet imgNrs;
    };

    /** set the panorama options */
    class WXIMPEX SetPanoOptionsCmd : public PanoCommand
    {
    public:
        SetPanoOptionsCmd(HuginBase::Panorama & p, const HuginBase::PanoramaOptions & opts)
            : PanoCommand(p), options(opts)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "set panorama options"; };
    private:
        HuginBase::PanoramaOptions options;
    };

    /** dump the current project and load a new one.
    *
    *  Use this for  style projects.
    *
    */
    class WXIMPEX LoadPTProjectCmd : public PanoCommand
    {
    public:
        LoadPTProjectCmd(HuginBase::Panorama & p, const std::string  & filename, const std::string & prefix = "");
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "load project"; };
    private:
        const std::string &filename;
        const std::string &prefix;
    };

    /** Rotate the panorama
    */
    class WXIMPEX RotatePanoCmd : public PanoCommand
    {
    public:
        RotatePanoCmd(HuginBase::Panorama & p, double yaw, double pitch, double roll)
            : PanoCommand(p), y(yaw), p(pitch), r(roll)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "rotate panorama"; };
    private:
        double y, p, r;
    };

    /** Translate the panorama
    */
    class WXIMPEX TranslatePanoCmd : public PanoCommand
    {
    public:
        TranslatePanoCmd(HuginBase::Panorama & p, double TrX, double TrY, double TrZ)
            : PanoCommand(p), X(TrX), Y(TrY), Z(TrZ)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const{ return "translate panorama"; };
    private:
        double X, Y, Z;
    };

    /** Update the focal length
    */
    class WXIMPEX UpdateFocalLengthCmd : public PanoCommand
    {
    public:
        UpdateFocalLengthCmd(HuginBase::Panorama & p, HuginBase::UIntSet imgs, double newFocalLength)
            : PanoCommand(p), imgNrs(imgs), m_focalLength(newFocalLength)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "update focal length"; };
    private:
        HuginBase::UIntSet imgNrs;
        double m_focalLength;
    };

    /** Update the crop factor
    */
    class WXIMPEX UpdateCropFactorCmd : public PanoCommand
    {
    public:
        UpdateCropFactorCmd(HuginBase::Panorama & p, HuginBase::UIntSet imgs, double newCropFactor)
            : PanoCommand(p), imgNrs(imgs), m_cropFactor(newCropFactor)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "update crop factor"; };
    private:
        HuginBase::UIntSet imgNrs;
        double m_cropFactor;
    };

    /** Switch the part number of an image
    */
    class WXIMPEX ChangePartNumberCmd : public PanoCommand
    {
    public:
        ChangePartNumberCmd(HuginBase::Panorama & p, HuginBase::UIntSet image_numbers, std::size_t new_part_number,
            std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> variables)
            : PanoCommand(p), image_numbers(image_numbers), new_part_number(new_part_number),variables(variables)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "Change part number"; };
    private:
        HuginBase::UIntSet image_numbers;
        std::size_t new_part_number;
        std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> variables;
    };

    /** Change the linking of some variables across parts of an
    * ImageVariableGroup containing some specified images.
    */
    class WXIMPEX ChangePartImagesLinkingCmd : public PanoCommand
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
        ChangePartImagesLinkingCmd(HuginBase::Panorama & p,
            HuginBase::UIntSet image_numbers, std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> changeVariables,
            bool new_linked_state, std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> groupVariables)
            : PanoCommand(p), image_numbers(image_numbers), changeVariables(changeVariables),
            new_linked_state(new_linked_state), groupVariables(groupVariables)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "Change image variable links"; };
    private:
        HuginBase::UIntSet image_numbers;
        std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> changeVariables;
        bool new_linked_state;
        std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> groupVariables;
    };

    /** Link a set of lens variables for some lens.
    */
    class WXIMPEX LinkLensVarsCmd : public PanoCommand
    {
    public:
        LinkLensVarsCmd(HuginBase::Panorama & p, std::size_t lens_number, std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> variables)
            : PanoCommand(p), lens_number(lens_number), variables(variables)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "Link lens variables"; };
    private:
        std::size_t lens_number;
        std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> variables;
    };

    /// @todo avoid copying image data in processPanorama
#define image_variable( name, type, default_value )\
    class WXIMPEX ChangeImage##name##Cmd : public PanoCommand\
                {\
    public:\
        ChangeImage##name##Cmd(HuginBase::Panorama & p,\
                               HuginBase::UIntSet image_numbers,\
                               type value)\
            :   PanoCommand(p),\
                image_numbers(image_numbers),\
                value(value)\
                        { };\
        virtual bool processPanorama(HuginBase::Panorama& pano);\
        virtual std::string getName() const { return "Change image's " #name; };\
    private:\
        HuginBase::UIntSet image_numbers;\
        type value;\
        };
#include <panodata/image_variables.h>
#undef image_variable

    /** Make a new part in a ImageVariableGroup for a set of images, given the
    * variables that make up the group.
    */
    class WXIMPEX NewPartCmd : public PanoCommand
    {
    public:
        /** Constructor.
        * @param p HuginBase::Panorama to act up
        * @param image_numbers A set of images which should all be in a single
        * new group.
        * @param vars The set of image variables that make up the group. Should
        * be got from StandardVariableGroups.
        */
        NewPartCmd(HuginBase::Panorama & p, HuginBase::UIntSet image_numbers, std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> vars)
            : PanoCommand(p), image_numbers(image_numbers), vars(vars)
        { }
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "New Part"; };
    private:
        HuginBase::UIntSet image_numbers;
        std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> vars;
    };

    /** update mask for given image */
    class WXIMPEX UpdateMaskForImgCmd : public PanoCommand
    {
    public:
        UpdateMaskForImgCmd(HuginBase::Panorama & p, unsigned int img, const HuginBase::MaskPolygonVector & mask)
            : PanoCommand(p), m_img(img), m_mask(mask)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "update mask"; };
    private:
        unsigned int m_img;
        HuginBase::MaskPolygonVector m_mask;
    };

    /** update global white balance */
    class WXIMPEX UpdateWhiteBalance : public PanoCommand
    {
    public:
        UpdateWhiteBalance(HuginBase::Panorama & p, double redFactor, double blueFactor)
            : PanoCommand(p), m_red(redFactor), m_blue(blueFactor)
        { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "update global white balance"; };
    private:
        double m_red;
        double m_blue;
    };

    /** reset output exposure to mean exposure of all images */
    class WXIMPEX ResetToMeanExposure : public PanoCommand
    {
    public:
        ResetToMeanExposure(HuginBase::Panorama & p) : PanoCommand(p) { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "set exposure to mean exposure"; };
    };

    /** distributes all images above the sphere, for the assistant */
    class WXIMPEX DistributeImagesCmd : public PanoCommand
    {
    public:
        DistributeImagesCmd(HuginBase::Panorama & p) : PanoCommand(p) { };
        virtual bool processPanorama(HuginBase::Panorama& pano);
        virtual std::string getName() const { return "distribute images"; };
    };


} // namespace PanoCommand

#endif // _PANOCOMMAND_H
