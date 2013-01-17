// -*- c-basic-offset: 4 -*-

/** @file pto_var.cpp
 *
 *  @brief program to manipulate variables for scripting
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
#include <panodata/ImageVariableTranslate.h>
#include <panodata/ImageVariableGroup.h>
#include <panodata/StandardImageVariableGroups.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace HuginBase;
using namespace AppBase;

struct ParseVar
{
    std::string varname;
    int imgNr;
    double val;
    ParseVar(): varname(""), imgNr(-1), val(0.0) {};
};

typedef std::vector<ParseVar> ParseVarVec;

// parse a single variable and put result in struct ParseVar
void ParseSingleVar(ParseVarVec& varVec, std::string s, std::string regExpression)
{
    boost::regex reg(regExpression);
    boost::smatch matches;
    if(boost::regex_match(s, matches, reg))
    {
        bool valid=true;
        if(matches.size()>1)
        {
            ParseVar var;
            std::string temp(matches[1].first, matches[1].second);
            //check if variable name is valid
            bool validVarname=false;
#define image_variable( name, type, default_value ) \
            if (HuginBase::PTOVariableConverterFor##name::checkApplicability(temp))\
            {\
                validVarname=true;\
            };
#include "panodata/image_variables.h"
#undef image_variable
            if(!validVarname)
            {
                return;
            };
            var.varname=temp;
            // now parse (sometimes optional) image number
            if(matches.size()>2)
            {
                temp=std::string(matches[2].first, matches[2].second);
                if(temp.length()>0)
                {
                    try
                    {
                        var.imgNr=boost::lexical_cast<int>(temp);
                    }
                    catch (boost::bad_lexical_cast)
                    {
                        var.imgNr=-1;
                    };
                };
                // read variable value
                if(matches.size()>3)
                {
                    temp=std::string(matches[3].first, matches[3].second);
                    try
                    {
                        var.val=boost::lexical_cast<double>(temp);
                    }
                    catch (boost::bad_lexical_cast)
                    {
                        valid=false;
                        var.val=0.0;
                    };
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
void ParseVariableString(ParseVarVec& parseVec, std::string input, std::string regExpression)
{
    std::vector<std::string> splitResult;
    boost::algorithm::split(splitResult, input, boost::algorithm::is_any_of(", "), boost::algorithm::token_compress_on);
    for(size_t i=0; i<splitResult.size();i++)
    {
        ParseSingleVar(parseVec, splitResult[i], regExpression);
    };
};

// adds given varname to optVec
// does some additional checking:
//   1. don't add y,p,r for anchor image
//   2. handle vignetting and EMoR parameters as group
void AddToOptVec(HuginBase::OptimizeVector& optVec, std::string varname, size_t imgNr, 
    std::set<size_t> refImgs, bool linkRefImgsYaw, bool linkRefImgsPitch, bool linkRefImgsRoll)
{
    if(varname=="y")
    {
        if(!set_contains(refImgs, imgNr) || linkRefImgsYaw)
        {
            optVec[imgNr].insert(varname);
        };
    }
    else
    {
        if(varname=="p")
        {
            if(!set_contains(refImgs, imgNr) || linkRefImgsPitch)
            {
                optVec[imgNr].insert(varname);
            };
        }
        else
        {
            if(varname=="r")
            {
                if(!set_contains(refImgs, imgNr) || linkRefImgsRoll)
                {
                    optVec[imgNr].insert(varname);
                };
            }
            else
            {
                if(varname=="TrX" || varname=="TrY" || varname=="TrZ")
                {
                    if(!set_contains(refImgs, imgNr))
                    {
                        optVec[imgNr].insert(varname);
                    };
                }
                else
                {
                    if(varname=="Vb" || varname=="Vc" || varname=="Vd")
                    {
                        optVec[imgNr].insert("Vb");
                        optVec[imgNr].insert("Vc");
                        optVec[imgNr].insert("Vd");
                    }
                    else
                    {
                        if(varname=="Vx" || varname=="Vy")
                        {
                            optVec[imgNr].insert("Vx");
                            optVec[imgNr].insert("Vy");
                        }
                        else
                        {
                            if(varname=="Ra" || varname=="Rb" || varname=="Rc" || varname=="Rd" || varname=="Re")
                            {
                                optVec[imgNr].insert("Ra");
                                optVec[imgNr].insert("Rb");
                                optVec[imgNr].insert("Rc");
                                optVec[imgNr].insert("Rd");
                                optVec[imgNr].insert("Re");
                            }
                            else
                            {
                                optVec[imgNr].insert(varname);
                            };
                        };
                    };
                };
            };
        };
    };
};

// link or unlink the parsed image variables
void UnLinkVars(Panorama& pano, ParseVarVec parseVec, bool link)
{
    for(size_t i=0; i<parseVec.size(); i++)
    {
        //skip invalid image numbers
        if(parseVec[i].imgNr<0 || parseVec[i].imgNr>=(int)pano.getNrOfImages())
        {
            continue;
        };
        
        //convert to ImageVariableGroup::IVE_name format
        std::set<HuginBase::ImageVariableGroup::ImageVariableEnum> variables;
#define image_variable( name, type, default_value ) \
        if (HuginBase::PTOVariableConverterFor##name::checkApplicability(parseVec[i].varname))\
        {\
            variables.insert(HuginBase::ImageVariableGroup::IVE_##name);\
        };
#include "panodata/image_variables.h"
#undef image_variable
        
        if(variables.size()>0)
        {
            //lens variable
            if(set_contains(HuginBase::StandardImageVariableGroups::getLensVariables(), *variables.begin()))
            {
                HuginBase::ImageVariableGroup group(HuginBase::StandardImageVariableGroups::getLensVariables(), pano);
                if (link)
                {
                    group.linkVariableImage(*variables.begin(), parseVec[i].imgNr);
                }
                else
                {
                    group.unlinkVariableImage(*variables.begin(), parseVec[i].imgNr);
                    group.updatePartNumbers();
                }
            }
            else
            {
                //stack variables
                // handle yaw, pitch, roll, TrX, TrY and TrZ always together
                if(set_contains(HuginBase::StandardImageVariableGroups::getStackVariables(), *variables.begin()))
                {
                    HuginBase::ImageVariableGroup group(HuginBase::StandardImageVariableGroups::getStackVariables(), pano);
                    if (link)
                    {
                        group.linkVariableImage(HuginBase::ImageVariableGroup::IVE_Yaw, parseVec[i].imgNr);
                        group.linkVariableImage(HuginBase::ImageVariableGroup::IVE_Pitch, parseVec[i].imgNr);
                        group.linkVariableImage(HuginBase::ImageVariableGroup::IVE_Roll, parseVec[i].imgNr);
                        group.linkVariableImage(HuginBase::ImageVariableGroup::IVE_X, parseVec[i].imgNr);
                        group.linkVariableImage(HuginBase::ImageVariableGroup::IVE_Y, parseVec[i].imgNr);
                        group.linkVariableImage(HuginBase::ImageVariableGroup::IVE_Z, parseVec[i].imgNr);
                    }
                    else
                    {
                        group.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_Yaw, parseVec[i].imgNr);
                        group.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_Pitch, parseVec[i].imgNr);
                        group.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_Roll, parseVec[i].imgNr);
                        group.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_X, parseVec[i].imgNr);
                        group.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_Y, parseVec[i].imgNr);
                        group.unlinkVariableImage(HuginBase::ImageVariableGroup::IVE_Z, parseVec[i].imgNr);
                        group.updatePartNumbers();
                    }
                }
                else
                {
                    cerr << "Warning: " << parseVec[i].varname << " is not a valid linkable variable." << endl;
                };
            };
        };
    };
};

static void usage(const char* name)
{
    cout << name << ": change image variables inside pto files" << endl
         << name << " version " << DISPLAY_VERSION << endl
         << endl
         << "Usage:  " << name << " [options] --opt|--link|--unlink|--set varlist input.pto" << endl
         << endl
         << "     -o, --output=file.pto  Output Hugin PTO file. Default: <filename>_var.pto" << endl
         << "     -h, --help             Shows this help" << endl
         << endl
         << "     --opt varlist          Change optimizer variables" << endl
         << "                            Examples:" << endl
         << "           --opt y,p,r        Optimize yaw, pitch and roll of all images" << endl
         << "           --opt v0,b2        Optimize hfov of image 0 and barrel distortion" << endl    
         << "                              of image 2" << endl
         << endl
         << "     --link varlist         Link given variables" << endl
         << "                            Example:" << endl
         << "           --link v3          Link hfov of image 3" << endl
         << "           --link a1,b1,c1    Link distortions parameter for image 1" << endl
         << endl
         << "     --unlink varlist       Unlink given variables" << endl
         << "                            Examples:" << endl
         << "           --unlink v5        Unlink hfov for image 5" << endl
         << "           --unlink a2,b2,c2  Unlink distortions parameters for image 2" << endl
         << endl
         << "     --set varlist          Sets variable to new value" << endl
         << "                            Examples:" << endl
         << "           --set y0=0,r0=0,p0=0  Resets position of image 0" << endl
         << "           --set Vx4=-10,Vy4=10  Sets vignetting offset for image 4" << endl
         << endl;
}

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "o:h";

    enum
    {
        SWITCH_OPT=1000,
        SWITCH_LINK,
        SWITCH_UNLINK,
        SWITCH_SET
    };
    static struct option longOptions[] =
    {
        {"output", required_argument, NULL, 'o' },
        {"opt", required_argument, NULL, SWITCH_OPT },
        {"link", required_argument, NULL, SWITCH_LINK },
        {"unlink", required_argument, NULL, SWITCH_UNLINK },
        {"set", required_argument, NULL, SWITCH_SET },
        {"help", no_argument, NULL, 'h' },
        0
    };

    ParseVarVec optVars;
    ParseVarVec linkVars;
    ParseVarVec unlinkVars;
    ParseVarVec setVars;
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
            case SWITCH_OPT:
                ParseVariableString(optVars, std::string(optarg), "([a-zA-Z]{1,3})(\\d*?)");
                break;
            case SWITCH_LINK:
                ParseVariableString(linkVars, std::string(optarg), "([a-zA-Z]{1,3})(\\d+?)");
                break;
            case SWITCH_UNLINK:
                ParseVariableString(unlinkVars, std::string(optarg), "([a-zA-Z]{1,3})(\\d+?)");
                break;
            case SWITCH_SET:
                ParseVariableString(setVars, std::string(optarg), "([a-zA-Z]{1,3})(\\d+?)=([-+]?\\d+?\\.?\\d*?)");
                break;
            case ':':
                cerr <<"Option " << longOptions[optionIndex].name << " requires a number" << endl;
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

    if(optVars.size() + linkVars.size() + unlinkVars.size() + setVars.size()==0)
    {
        cerr << "Error: no variables to modify given" << endl;
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

    //link/unlink variables
    if(linkVars.size()>0)
    {
        UnLinkVars(pano, linkVars, true);
    };

    if(unlinkVars.size()>0)
    {
        UnLinkVars(pano, unlinkVars, false);
    };

    // set variables to new value
    if(setVars.size()>0)
    {
        for(size_t i=0; i<setVars.size(); i++)
        {
            //skip invalid image numbers
            if(setVars[i].imgNr<0 || setVars[i].imgNr>=(int)pano.getNrOfImages())
            {
                continue;
            };
            HuginBase::Variable var(setVars[i].varname, setVars[i].val);
            pano.updateVariable(setVars[i].imgNr, var);
        };
    };

    // update optimzer vector
    if(optVars.size()>0)
    {
        std::set<size_t> refImgs=pano.getRefImages();
        bool linkRefImgsYaw=false;
        bool linkRefImgsPitch=false;
        bool linkRefImgsRoll=false;
        pano.checkRefOptStatus(linkRefImgsYaw, linkRefImgsPitch, linkRefImgsRoll);

        HuginBase::OptimizeVector optVec(pano.getNrOfImages());
        for(size_t i=0; i<optVars.size(); i++)
        {
            //skip invalid image numbers
            if(optVars[i].imgNr>=(int)pano.getNrOfImages())
            {
                continue;
            };
            if(optVars[i].imgNr==-1)
            {
                for(size_t imgNr=0; imgNr<pano.getNrOfImages(); imgNr++)
                {
                    AddToOptVec(optVec, optVars[i].varname, imgNr, refImgs, linkRefImgsYaw, linkRefImgsPitch, linkRefImgsRoll);
                };
            }
            else
            {
                AddToOptVec(optVec, optVars[i].varname, optVars[i].imgNr, refImgs, true, true, true);
            };
        };
        pano.setOptimizerSwitch(0);
        pano.setPhotometricOptimizerSwitch(0);
        pano.setOptimizeVector(optVec);
    };

    //write output
    UIntSet imgs;
    fill_set(imgs,0, pano.getNrOfImages()-1);
    // Set output .pto filename if not given
    if (output=="")
    {
        output=input.substr(0,input.length()-4).append("_var.pto");
    }
    ofstream of(output.c_str());
    pano.printPanoramaScript(of, pano.getOptimizeVector(), pano.getOptions(), imgs, false, hugin_utils::getPathPrefix(input));

    cout << endl << "Written output to " << output << endl;
    return 0;
}
