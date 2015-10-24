// -*- c-basic-offset: 4 -*-

/** @file pto_lensstack.cpp
 *
 *  @brief program to manipulate lens and stack assignment in pto files
 *
 *  @author T. Modes
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

#include <fstream>
#include <sstream>
#include <getopt.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <panodata/Panorama.h>
#include <panodata/StandardImageVariableGroups.h>
#include "hugin_utils/utils.h"

struct ParsedImg
{
    int imgNr;
    int lensStackNr;
    ParsedImg(): imgNr(-1), lensStackNr(-1) {};
};

typedef std::vector<ParsedImg> ParseImgVec;

// parse a single variable and put result in struct ParseVar
void ParseSingleImage(ParseImgVec& varVec, const std::string& s)
{
    std::string text(s);
    if (text[0] == 'i')
    {
        text.erase(0, 1);
        ParsedImg var;
        // search =
        const std::size_t pos = text.find_first_of("=", 0);
        if (pos == std::string::npos)
        {
            // no =, try to convert to number
            if (!hugin_utils::stringToInt(text, var.imgNr))
            {
                return;
            }
        }
        else
        {
            if (pos > 0 && pos < text.length() - 1)
            {
                std::string tempString(text.substr(0, pos));
                if (!hugin_utils::stringToInt(tempString, var.imgNr))
                {
                    return;
                };
                tempString = text.substr(pos + 1, text.length() - pos - 1);
                if(!hugin_utils::stringToInt(tempString, var.lensStackNr))
                {
                    return;
                };
            }
            else
            {
                // = at first or last position of string
                return;
            };
        };
        varVec.push_back(var);
    };
};

//parse complete variables string
void ParseImageLensStackString(ParseImgVec& parseVec, std::string input)
{
    std::vector<std::string> splitResult = hugin_utils::SplitString(input, ", ");
    for(size_t i=0; i<splitResult.size(); i++)
    {
        ParseSingleImage(parseVec, splitResult[i]);
    };
};

// assign a new lens/stack
void NewPart(HuginBase::Panorama& pano, std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> vars, unsigned int imgNr)
{
    for (std::set<HuginBase::ImageVariableGroup::ImageVariableEnum>::iterator it = vars.begin();  it != vars.end(); ++it)
    {
        switch (*it)
        {
#define image_variable( name, type, default_value )\
case HuginBase::ImageVariableGroup::IVE_##name:\
    pano.unlinkImageVariable##name(imgNr);\
    break;
#include <panodata/image_variables.h>
#undef image_variable
        }
    }
};

static void usage(const char* name)
{
    std::cout << name << ": modify assigned lenses and stack in pto files" << std::endl
         << name << " version " << hugin_utils::GetHuginVersion() << std::endl
         << std::endl
         << "Usage:  " << name << " [options] --switches imglist input.pto" << std::endl
         << std::endl
         << "     -o, --output=file.pto   Output Hugin PTO file. Default: <filename>_lens.pto" << std::endl
         << "     -h, --help              Shows this help" << std::endl
         << std::endl
         << "     --new-lens imglist      Assign to given images a new lens number" << std::endl
         << "     --new-stack imglist     Assign to given images a new stack number" << std::endl
         << "                               Examples:" << std::endl
         << "           --new-lens i2          Image 2 gets a new lens" << std::endl
         << "           --new-stack i4,i5      Images 4 and 5 get a new stack" << std::endl
         << std::endl
         << "     --change-lens imglist   Assign to given images a new lens number" << std::endl
         << "     --change-stack imglist  Assign to given images a new stack number" << std::endl
         << "                               Examples:" << std::endl
         << "           --change-lens i2=0      Image 2 is assigned lens number 0" << std::endl
         << "           --change-stack i4=0,i5=1   Image 4 is assigned to stack 0," << std::endl
         << "                                      image 5 to stack number 1" << std::endl
         << std::endl;
}

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "o:h";

    enum
    {
        SWITCH_NEW_LENS=1000,
        SWITCH_NEW_STACK,
        SWITCH_CHANGE_LENS,
        SWITCH_CHANGE_STACK
    };
    static struct option longOptions[] =
    {
        {"output", required_argument, NULL, 'o' },
        {"new-lens", required_argument, NULL, SWITCH_NEW_LENS },
        {"new-stack", required_argument, NULL, SWITCH_NEW_STACK },
        {"change-lens", required_argument, NULL, SWITCH_CHANGE_LENS },
        {"change-stack", required_argument, NULL, SWITCH_CHANGE_STACK },
        {"help", no_argument, NULL, 'h' },
        0
    };

    ParseImgVec newLensImgs;
    ParseImgVec newStackImgs;
    ParseImgVec changeLensImgs;
    ParseImgVec changeStackImgs;
    int c;
    int optionIndex = 0;
    std::string output;
    while ((c = getopt_long (argc, argv, optstring, longOptions,&optionIndex)) != -1)
    {
        switch (c)
        {
            case 'o':
                output = optarg;
                break;
            case 'h':
                usage(hugin_utils::stripPath(argv[0]).c_str());
                return 0;
            case SWITCH_NEW_LENS:
                ParseImageLensStackString(newLensImgs, std::string(optarg));
                break;
            case SWITCH_NEW_STACK:
                ParseImageLensStackString(newStackImgs, std::string(optarg));
                break;
            case SWITCH_CHANGE_LENS:
                ParseImageLensStackString(changeLensImgs, std::string(optarg));
                break;
            case SWITCH_CHANGE_STACK:
                ParseImageLensStackString(changeStackImgs, std::string(optarg));
                break;
            case ':':
                std::cerr <<"Option " << longOptions[optionIndex].name << " requires a parameter" << std::endl;
                return 1;
                break;
            case '?':
                break;
            default:
                abort ();
        }
    }

    if (argc - optind == 0)
    {
        std::cout << "Error: " << argv[0] << " needs at least one project file." << std::endl;
        return 1;
    };
    if (argc - optind != 1)
    {
        std::cout << "Error: " << argv[0] << " can only work on one project file at one time" << std::endl;
        return 1;
    };

    if(newLensImgs.size() + newStackImgs.size() + changeLensImgs.size() + changeStackImgs.size()==0)
    {
        std::cerr << "Error: no images/lens/stacks to modify given" << std::endl;
        return 1;
    };

    std::string input=argv[optind];
    // read panorama
    HuginBase::Panorama pano;
    std::ifstream prjfile(input.c_str());
    if (!prjfile.good())
    {
        std::cerr << "could not open script : " << input << std::endl;
        return 1;
    }
    pano.setFilePrefix(hugin_utils::getPathPrefix(input));
    AppBase::DocumentData::ReadWriteError err = pano.readData(prjfile);
    if (err != AppBase::DocumentData::SUCCESSFUL)
    {
        std::cerr << "error while parsing panos tool script: " << input << std::endl;
        std::cerr << "AppBase::DocumentData::ReadWriteError code: " << err << std::endl;
        return 1;
    }

    if(pano.getNrOfImages()==0)
    {
        std::cerr << "error: project file does not contains any image" << std::endl;
        std::cerr << "aborting processing" << std::endl;
        return 1;
    };

    // new lenses
    if(newLensImgs.size()>0)
    {
        HuginBase::StandardImageVariableGroups variable_groups(pano);
        if(variable_groups.getLenses().getNumberOfParts()<pano.getNrOfImages())
        {
            for(size_t i=0; i<newLensImgs.size(); i++)
            {
                //skip invalid image numbers
                if(newLensImgs[i].imgNr<0 || newLensImgs[i].imgNr>=(int)pano.getNrOfImages())
                {
                    continue;
                };
                NewPart(pano, HuginBase::StandardImageVariableGroups::getLensVariables(), newLensImgs[i].imgNr);
            };
        }
        else
        {
            std::cout << "Warning: Pto project contains already for each image an own lens" << std::endl
                 << "         Nothing to do." << std::endl;
        };
    };

    // new stacks
    if(newStackImgs.size()>0)
    {
        HuginBase::StandardImageVariableGroups variable_groups(pano);
        if(variable_groups.getStacks().getNumberOfParts()<pano.getNrOfImages())
        {
            for(size_t i=0; i<newStackImgs.size(); i++)
            {
                //skip invalid image numbers
                if(newStackImgs[i].imgNr<0 || newStackImgs[i].imgNr>=(int)pano.getNrOfImages())
                {
                    continue;
                };
                NewPart(pano, HuginBase::StandardImageVariableGroups::getStackVariables(), newStackImgs[i].imgNr);
            };
        }
        else
        {
            std::cout << "Warning: Pto project contains already for each image an own stack" << std::endl
                 << "         Nothing to do." << std::endl;
        };
    };

    // change lenses
    if(changeLensImgs.size()>0)
    {
        HuginBase::StandardImageVariableGroups variable_groups(pano);
        size_t lensCount=variable_groups.getLenses().getNumberOfParts();
        if(lensCount>1)
        {
            for(size_t i=0; i<changeLensImgs.size(); i++)
            {
                //skip invalid image numbers
                if(changeLensImgs[i].imgNr<0 || changeLensImgs[i].imgNr>=(int)pano.getNrOfImages())
                {
                    continue;
                };
                if(changeLensImgs[i].lensStackNr<0 || changeLensImgs[i].lensStackNr>=lensCount)
                {
                    continue;
                };
                HuginBase::ImageVariableGroup group(HuginBase::StandardImageVariableGroups::getLensVariables(), pano);
                group.switchParts(changeLensImgs[i].imgNr, changeLensImgs[i].lensStackNr);
            };
        }
        else
        {
            std::cout << "Warning: Pto project contains only one lens." << std::endl
                 << "         Therefor the lens can not be changed. Use --new-lens instead." << std::endl;
        };
    };

    // change stacks
    if(changeStackImgs.size()>0)
    {
        HuginBase::StandardImageVariableGroups variable_groups(pano);
        size_t stackCount=variable_groups.getStacks().getNumberOfParts();
        if(stackCount>1)
        {
            for(size_t i=0; i<changeStackImgs.size(); i++)
            {
                //skip invalid image numbers
                if(changeStackImgs[i].imgNr<0 || changeStackImgs[i].imgNr>=(int)pano.getNrOfImages())
                {
                    continue;
                };
                if(changeStackImgs[i].lensStackNr<0 || changeStackImgs[i].lensStackNr>=stackCount)
                {
                    continue;
                };
                HuginBase::ImageVariableGroup group(HuginBase::StandardImageVariableGroups::getStackVariables(), pano);
                group.switchParts(changeStackImgs[i].imgNr, changeStackImgs[i].lensStackNr);
            };
        }
        else
        {
            std::cout << "Warning: Pto project contains only one stack." << std::endl
                 << "         Therefore the stack can not be changed. Use --new-stack instead." << std::endl;
        };
    };

    //write output
    HuginBase::UIntSet imgs;
    fill_set(imgs,0, pano.getNrOfImages()-1);
    // Set output .pto filename if not given
    if (output=="")
    {
        output=input.substr(0,input.length()-4).append("_lens.pto");
    }
    std::ofstream of(output.c_str());
    pano.printPanoramaScript(of, pano.getOptimizeVector(), pano.getOptions(), imgs, false, hugin_utils::getPathPrefix(input));

    std::cout << std::endl << "Written output to " << output << std::endl;
    return 0;
}
