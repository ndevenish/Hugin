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

#include "Process.h"
#include "common/Command.h"

#include "PanoImage.h"
#include "Panorama.h"

namespace PT {

    /** default panorama cmd, provides undo with mementos
     */
    class PanoCommand : public Command
    {
    public:
        PanoCommand(Panorama & p)
            : pano(p)
            { };

        virtual ~PanoCommand()
            {
            };

        /** save the state */
        virtual void execute()
            {
                memento = pano.getMemento();
            };
        /** set the saved state.
         *
         *  the derived class must call PanoComand::execute() in its
         *  execute() method to save the state.
         */
        virtual void undo()
            {
                pano.setMemento(memento);
                pano.changeFinished();
            }
        virtual std::string getName() const = 0;
    protected:
        Panorama & pano;
        PanoramaMemento memento;
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
    class AddImagesCmd : public PanoCommand
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
    class RemoveImageCmd : public PanoCommand
    {
    public:
        RemoveImageCmd(Panorama & p, unsigned int imgNr)
            : PanoCommand(p), imgNr(imgNr)
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

#if 0
    /**  */
    class ChangeImageCmd : public PanoCommand
    {
    public:
        Cmd(Panorama & p)
            : PanoCommand(p)
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
    class UpdateVariablesCmd : public PanoCommand
    {
    public:
        UpdateVariablesCmd(Panorama & p, const VariableMapVector & vars)
            : PanoCommand(p),
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

    /** update all variables & control points*/
    class UpdateVariablesCPCmd : public PanoCommand
    {
    public:
        UpdateVariablesCPCmd(Panorama & p, const VariableMapVector & vars,
                             const CPVector & cps)
            : PanoCommand(p),
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

    /** update variables of a single image */
    class UpdateImageVariablesCmd : public PanoCommand
    {
    public:
        UpdateImageVariablesCmd(Panorama & p, unsigned int ImgNr, const VariableMap & vars)
            : PanoCommand(p), imgNr(ImgNr),
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
    class UpdateImagesVariablesCmd : public PanoCommand
    {
    public:
        UpdateImagesVariablesCmd(Panorama & p, UIntSet & change, const VariableMapVector & vars)
            : PanoCommand(p), change(change),
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
    class SetVariableCmd : public PanoCommand
    {
    public:
        SetVariableCmd(Panorama & p, const UIntSet & images, const Variable & var)
            : PanoCommand(p), images(images),
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
    class SetLensVariableCmd : public PanoCommand
    {
    public:
        SetLensVariableCmd(Panorama & p, int lens, const LensVarMap & var)
            : PanoCommand(p), lensNr(lens), vars(var)
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

    /** change the lens for an image */
    class SetImageLensCmd : public PanoCommand
    {
    public:
        SetImageLensCmd(Panorama & p, const UIntSet & imgNrs, int lensNr)
            : PanoCommand(p),
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


    /** add a control point */
    class AddCtrlPointCmd : public PanoCommand
    {
    public:
        AddCtrlPointCmd(Panorama & p, const ControlPoint & cpoint)
            : PanoCommand(p), point(cpoint)
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


    /** remove a control point */
    class RemoveCtrlPointCmd : public PanoCommand
    {
    public:
        RemoveCtrlPointCmd(Panorama & p, unsigned int cpNr)
            : PanoCommand(p), pointNr(cpNr)
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


    /** change a control point */
    class ChangeCtrlPointCmd : public PanoCommand
    {
    public:
        ChangeCtrlPointCmd(Panorama & p, unsigned int nr, ControlPoint point)
            : PanoCommand(p), pNr(nr), point(point)
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
    class AddLensCmd : public PanoCommand
    {
    public:
        AddLensCmd(Panorama & p, const Lens & lens)
            : PanoCommand(p), lens(lens)
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


    /** change lens */
    class ChangeLensCmd : public PanoCommand
    {
    public:
        ChangeLensCmd(Panorama & p, unsigned int lensNr, const Lens & lens)
            : PanoCommand(p), lensNr(lensNr), newLens(lens)
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
    class ChangeLensesCmd : public PanoCommand
    {
    public:
        ChangeLensesCmd(Panorama & p, UIntSet & lNr, const LensVector & lenses)
            : PanoCommand(p), change(lNr), vect(lenses)
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


    /** set the panorama options */
    class SetPanoOptionsCmd : public PanoCommand
    {
    public:
        SetPanoOptionsCmd(Panorama & p, const PanoramaOptions & opts)
            : PanoCommand(p), options(opts)
            { };
        virtual void execute()
            {
                pano.setOptions(options);
                PanoCommand::execute();
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
    class LoadPTProjectCmd : public PanoCommand
    {
    public:
        LoadPTProjectCmd(Panorama & p, std::istream & i)
            : PanoCommand(p),
              in(i)
            { }

        virtual void execute()
            {
                PanoCommand::execute();
                PanoramaMemento newPano;
                if (newPano.loadPTScript(in)) {
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
    };

} // namespace PT

#endif // _PANOCOMMAND_H
