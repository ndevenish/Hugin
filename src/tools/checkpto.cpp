// -*- c-basic-offset: 4 -*-

/** @file checkpto.cpp
 *
 *  @brief helper program for assistant makefile
 *
 *
 *  @author Thomas Modes
 *
 */

/*  This program is free software; you can redistribute it and/or
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <hugin_config.h>

#include <fstream>
#include <sstream>
#include <getopt.h>

#include <panodata/Panorama.h>
#include "algorithms/optimizer/ImageGraph.h"
#include "hugin_base/panotools/PanoToolsUtils.h"
#include "hugin_base/panodata/StandardImageVariableGroups.h"
#include "algorithms/basic/CalculateCPStatistics.h"
#include "algorithms/basic/LayerStacks.h"

static void usage(const char* name)
{
    std::cout << name << ": report the number of image groups in a project" << std::endl
         << name << " version " << hugin_utils::GetHuginVersion() << std::endl
         << std::endl
         << "Usage:  " << name << " input.pto" << std::endl
         << std::endl
         << name << " examines the connections between images in a project and" << std::endl
         << "reports back the number of parts or image groups in that project" << std::endl
         << std::endl
         << "Further switches:" << std::endl
         << "  --print-output-info     Print more information about the output" << std::endl
         << "  --print-lens-info       Print more information about lenses" << std::endl
         << "  --print-stack-info      Print more information about assigned stacks" << std::endl
         << "                          spaceholders will be replaced with real values" << std::endl
         << std::endl
         << name << " is used by the assistant and by the stitching makefiles" << std::endl
         << std::endl;
}

void printImageGroup(const std::vector<HuginBase::UIntSet>& imageGroup, const std::string& prefix=std::string())
{
    for (size_t i=0; i < imageGroup.size(); i++)
    {
        if (!prefix.empty())
        {
            std::cout << prefix << " " << i << ": ";
        };
        std::cout << "[";
        size_t c=0;
        for (HuginBase::UIntSet::const_iterator it = imageGroup[i].begin(); it != imageGroup[i].end(); ++it)
        {
            std::cout << (*it);
            if (c+1 != imageGroup[i].size())
            {
                std::cout << ", ";
            }
            ++c;
        }
        std::cout << "]";
        if (prefix.empty() && (i+1 != imageGroup.size()))
        {
            std::cout << ", ";
        }
        std::cout << std::endl;
    }
};

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "h";
    enum
    {
        PRINT_OUTPUT_INFO=1000,
        PRINT_LENS_INFO=1004,
        PRINT_STACK_INFO=1005,
    };
    static struct option longOptions[] =
    {
        { "print-output-info", no_argument, NULL, PRINT_OUTPUT_INFO },
        { "print-lens-info", no_argument, NULL, PRINT_LENS_INFO },
        { "print-stack-info", no_argument, NULL, PRINT_STACK_INFO },
        { "help", no_argument, NULL, 'h' },
        0
    };

    int c;
    bool printOutputInfo=false;
    bool printLensInfo = false;
    bool printStackInfo = false;
    int optionIndex = 0;
    while ((c = getopt_long (argc, argv, optstring, longOptions,&optionIndex)) != -1)
    {
        switch (c)
        {
            case 'h':
                usage(hugin_utils::stripPath(argv[0]).c_str());
                return 0;
            case PRINT_OUTPUT_INFO:
                printOutputInfo=true;
                break;
            case PRINT_LENS_INFO:
                printLensInfo = true;
                break;
            case PRINT_STACK_INFO:
                printStackInfo = true;
                break;
            case '?':
                break;
            default:
                abort ();
        }
    }

    if (argc - optind != 1)
    {
        usage(hugin_utils::stripPath(argv[0]).c_str());
        return -1;
    };

    std::string input=argv[optind];

    HuginBase::Panorama pano;
    std::ifstream prjfile(input.c_str());
    if (!prjfile.good())
    {
        std::cerr << "could not open script : " << input << std::endl;
        return -1;
    }
    pano.setFilePrefix(hugin_utils::getPathPrefix(input));
    AppBase::DocumentData::ReadWriteError err = pano.readData(prjfile);
    if (err != AppBase::DocumentData::SUCCESSFUL)
    {
        std::cerr << "error while parsing panos tool script: " << input << std::endl
            << "DocumentData::ReadWriteError code: " << err << std::endl;
        return -1;
    }

    HuginBase::ConstStandardImageVariableGroups variable_groups(pano);
    std::cout << std::endl
              << "Opened project " << input << std::endl << std::endl
              << "Project contains" << std::endl
              << pano.getNrOfImages() << " images" << std::endl
              << variable_groups.getLenses().getNumberOfParts() << " lenses" << std::endl
              << variable_groups.getStacks().getNumberOfParts() << " stacks" << std::endl
              << pano.getNrOfCtrlPoints() << " control points" << std::endl << std::endl;
    //cp statistics
    if(pano.getNrOfCtrlPoints()>0)
    {
        double min;
        double max;
        double mean;
        double var;
        HuginBase::PTools::calcCtrlPointErrors(pano);
        HuginBase::CalculateCPStatisticsError::calcCtrlPntsErrorStats(pano, min, max, mean, var);
        if(max>0)
        {
            std::cout << "Control points statistics" << std::endl
                      << std::fixed << std::setprecision(2)
                      << "\tMean error        : " << mean << std::endl
                      << "\tStandard deviation: " << sqrt(var) << std::endl
                      << "\tMinimum           : " << min << std::endl
                      << "\tMaximum           : " << max << std::endl;
        };
    };
    HuginGraph::ImageGraph graph(pano);
    const HuginGraph::ImageGraph::Components comps = graph.GetComponents();
    int returnValue=0;
    if(comps.size()==1)
    {
        std::cout << "All images are connected." << std::endl;
        // return value must be 0, otherwise the assistant does not continue
        returnValue=0;
    }
    else
    {
        std::cout << "Found unconnected images!" << std::endl
                  << "There are " << comps.size() << " image groups." << std::endl;

        std::cout << "Image groups: " << std::endl;
        for (size_t i=0; i < comps.size(); i++)
        {
            std::cout << "[";
            HuginGraph::ImageGraph::Components::value_type::const_iterator it;
            size_t c=0;
            for (it = comps[i].begin(); it != comps[i].end(); ++it)
            {
                std::cout << (*it);
                if (c+1 != comps[i].size())
                {
                    std::cout << ", ";
                }
                c++;
            }
            std::cout << "]";
            if (i+1 != comps.size())
            {
                std::cout << ", " << std::endl;
            }
        }
        returnValue=comps.size();
    };
    std::cout << std::endl;
    if (printLensInfo)
    {
        std::cout << std::endl << "Lenses:" << std::endl;
        printImageGroup(variable_groups.getLenses().getPartsSet(), "Lens");
    };
    if (printStackInfo)
    {
        std::cout << std::endl << "Stacks:" << std::endl;
        printImageGroup(variable_groups.getStacks().getPartsSet(), "Stack");
    };
    if (printOutputInfo)
    {
        HuginBase::UIntSet outputImages=HuginBase::getImagesinROI(pano, pano.getActiveImages());
        std::vector<HuginBase::UIntSet> stacks=HuginBase::getHDRStacks(pano, outputImages, pano.getOptions());
        std::cout << std::endl << "Output contains" << std::endl
             << stacks.size() << " images stacks:" << std::endl;
        printImageGroup(stacks);
        std::vector<HuginBase::UIntSet> layers=HuginBase::getExposureLayers(pano, outputImages, pano.getOptions());
        std::cout << std::endl << std::endl << "and " << layers.size() << " exposure layers:" << std::endl;
        printImageGroup(layers);
    };
    return returnValue;
}
