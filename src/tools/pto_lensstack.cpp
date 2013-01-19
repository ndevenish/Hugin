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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <hugin_version.h>

#include <fstream>
#include <sstream>
#include <getopt.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <panodata/Panorama.h>
#include <panodata/StandardImageVariableGroups.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace HuginBase;
using namespace AppBase;

struct ParsedImg
{
    int imgNr;
    int lensStackNr;
};

typedef std::vector<ParsedImg> ParseImgVec;

// parse a single variable and put result in struct ParseVar
void ParseSingleImage(ParseImgVec& varVec, std::string s, std::string regExpression)
{
    boost::regex reg(regExpression);
    boost::smatch matches;
    if(boost::regex_match(s, matches, reg))
    {
        bool valid=true;
        if(matches.size()>1)
        {
            ParsedImg var;
            // parse image number
            std::string temp(matches[1].first, matches[1].second);
            try
            {
                var.imgNr=boost::lexical_cast<int>(temp);
            }
            catch (boost::bad_lexical_cast)
            {
                var.imgNr=-1;
            };
                // read lens/stack number
            if(matches.size()>2)
            {
                temp=std::string(matches[2].first, matches[2].second);
                try
                {
                    var.lensStackNr=boost::lexical_cast<int>(temp);
                }
                catch (boost::bad_lexical_cast)
                {
                    valid=false;
                };
            };
            if(valid)
            {
                varVec.push_back(var);
            };
        };
    };
};

//parse complete variables string
void ParseImageLensStackString(ParseImgVec& parseVec, std::string input, std::string regExpression)
{
    std::vector<std::string> splitResult;
    boost::algorithm::split(splitResult, input, boost::algorithm::is_any_of(", "), boost::algorithm::token_compress_on);
    for(size_t i=0; i<splitResult.size();i++)
    {
        ParseSingleImage(parseVec, splitResult[i], regExpression);
    };
};

// assign a new lens/stack
void NewPart(Panorama& pano, std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> vars, unsigned int imgNr)
{
    for (std::set<HuginBase::ImageVariableGroup::ImageVariableEnum>::iterator it = vars.begin();  it != vars.end(); it++)
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
    cout << name << ": modify assigned lenses and stack in pto files" << endl
         << name << " version " << DISPLAY_VERSION << endl
         << endl
         << "Usage:  " << name << " [options] --switches imglist input.pto" << endl
         << endl
         << "     -o, --output=file.pto   Output Hugin PTO file. Default: <filename>_lens.pto" << endl
         << "     -h, --help              Shows this help" << endl
         << endl
         << "     --new-lens imglist      Assign to given images a new lens number" << endl
         << "     --new-stack imglist     Assign to given images a new stack number" << endl
         << "                               Examples:" << endl
         << "           --new-lens i2          Image 2 gets a new lens" << endl
         << "           --new-stack i4,i5      Images 4 and 5 get a new stack" << endl    
         << endl
         << "     --change-lens imglist   Assign to given images a new lens number" << endl
         << "     --change-stack imglist  Assign to given images a new stack number" << endl
         << "                               Examples:" << endl
         << "           --change-lens i2=0      Image 2 is assigned lens number 0" << endl
         << "           --change-stack i4=0,i5=1   Image 4 is assigned to stack 0," << endl
         << "                                      image 5 to stack number 1" << endl    
         << endl;
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
    string output;
    while ((c = getopt_long (argc, argv, optstring, longOptions,&optionIndex)) != -1)
    {
        switch (c)
        {
            case 'o':
                output = optarg;
                break;
            case 'h':
                usage(argv[0]);
                return 0;
            case SWITCH_NEW_LENS:
                ParseImageLensStackString(newLensImgs, std::string(optarg), "i(\\d+?)");
                break;
            case SWITCH_NEW_STACK:
                ParseImageLensStackString(newStackImgs, std::string(optarg), "i(\\d+?)");
                break;
            case SWITCH_CHANGE_LENS:
                ParseImageLensStackString(changeLensImgs, std::string(optarg), "i(\\d+?)=(\\d+?)");
                break;
            case SWITCH_CHANGE_STACK:
                ParseImageLensStackString(changeStackImgs, std::string(optarg), "i(\\d+?)=(\\d+?)");
                break;
            case ':':
                cerr <<"Option " << longOptions[optionIndex].name << " requires a parameter" << endl;
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
        cout << "Error: " << argv[0] << " needs at least one project file." << endl;
        return 1;
    };
    if (argc - optind != 1)
    {
        cout << "Error: " << argv[0] << " can only work on one project file at one time" << endl;
        return 1;
    };

    if(newLensImgs.size() + newStackImgs.size() + changeLensImgs.size() + changeStackImgs.size()==0)
    {
        cerr << "Error: no images/lens/stacks to modify given" << endl;
        return 1;
    };

    string input=argv[optind];
    // read panorama
    Panorama pano;
    ifstream prjfile(input.c_str());
    if (!prjfile.good())
    {
        cerr << "could not open script : " << input << endl;
        return 1;
    }
    pano.setFilePrefix(hugin_utils::getPathPrefix(input));
    DocumentData::ReadWriteError err = pano.readData(prjfile);
    if (err != DocumentData::SUCCESSFUL)
    {
        cerr << "error while parsing panos tool script: " << input << endl;
        cerr << "DocumentData::ReadWriteError code: " << err << endl;
        return 1;
    }

    if(pano.getNrOfImages()==0)
    {
        cerr << "error: project file does not contains any image" << endl;
        cerr << "aborting processing" << endl;
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
            cout << "Warning: Pto project contains already for each image an own lens" << endl
                 << "         Nothing to do." << endl;
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
            cout << "Warning: Pto project contains already for each image an own stack" << endl
                 << "         Nothing to do." << endl;
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
                ImageVariableGroup group(StandardImageVariableGroups::getLensVariables(), pano);
                group.switchParts(changeLensImgs[i].imgNr, changeLensImgs[i].lensStackNr);
            };
        }
        else
        {
            cout << "Warning: Pto project contains only one lens." << endl
                 << "         Therefor the lens can not be changed. Use --new-lens instead." << endl;
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
                ImageVariableGroup group(StandardImageVariableGroups::getStackVariables(), pano);
                group.switchParts(changeStackImgs[i].imgNr, changeStackImgs[i].lensStackNr);
            };
        }
        else
        {
            cout << "Warning: Pto project contains only one stack." << endl
                 << "         Therefore the stack can not be changed. Use --new-stack instead." << endl;
        };
    };

    //write output
    UIntSet imgs;
    fill_set(imgs,0, pano.getNrOfImages()-1);
    // Set output .pto filename if not given
    if (output=="")
    {
        output=input.substr(0,input.length()-4).append("_lens.pto");
    }
    ofstream of(output.c_str());
    pano.printPanoramaScript(of, pano.getOptimizeVector(), pano.getOptions(), imgs, false, hugin_utils::getPathPrefix(input));

    cout << endl << "Written output to " << output << endl;
    return 0;
}
