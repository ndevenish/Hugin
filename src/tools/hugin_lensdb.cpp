// -*- c-basic-offset: 4 -*-

/** @file hugin_lensdb.cpp
 *
 *  @brief helper program for working with lens database
 *
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

#include <iostream>
#include <string>
#include <fstream>
#define BOOST_FILESYSTEM_VERSION 3
#include <boost/filesystem.hpp>
#include <getopt.h>
#include <panodata/Panorama.h>
#include <hugin_utils/stl_utils.h>
#include <lensdb/LensDB.h>
#include <panodata/StandardImageVariableGroups.h>
#include <hugin_base/panotools/PanoToolsUtils.h>

typedef std::vector<boost::filesystem::path> pathVec;


template <class iteratorType>
bool iterateFileSystem(std::string src, pathVec& projectFiles)
{
    try
    {
        for(iteratorType it(src); it != iteratorType(); it++)
        {
            std::string ext=hugin_utils::toupper(it->path().extension().string());
            if(ext==".PTO")
            {
                projectFiles.push_back(*it);
            };
        }
    }
    catch(boost::filesystem::filesystem_error& e)
    {
        std::cout << e.what() << std::endl;
        return false;
    }
    return true;
};

void FindPTOFiles(pathVec& projectFiles, std::string src, bool recursive)
{
    if(recursive)
    {
        iterateFileSystem<boost::filesystem::recursive_directory_iterator>(src, projectFiles);
    }
    else
    {
        iterateFileSystem<boost::filesystem::directory_iterator>(src, projectFiles);
    };
};

bool CheckProjectFile(boost::filesystem::path filename)
{
    // open project file
    HuginBase::Panorama pano;
    std::string input = filename.string();
    std::ifstream prjfile(input.c_str());
    if (!prjfile.good())
    {
        std::cerr << "ERROR: Could not open script: " << filename.string() << endl;
        return false;
    }
    std::string inputPathPrefix = hugin_utils::getPathPrefix(input);
    pano.setFilePrefix(inputPathPrefix);
    AppBase::DocumentData::ReadWriteError err = pano.readData(prjfile);
    if (err != AppBase::DocumentData::SUCCESSFUL)
    {
        std::cerr << "ERROR: error while parsing panos tool script: " << input << std::endl
                  << "DocumentData::ReadWriteError code: " << err << std::endl;
        return false;
    };
    prjfile.close();
    if (pano.getNrOfImages() == 0)
    {
        return false;
    };
    std::cout << "Checking " << filename.string();
    HuginBase::StandardImageVariableGroups lenses(pano);
    if (lenses.getLenses().getNumberOfParts()==1)
    {
        // read the EXIF data
        for (size_t i = 0; i < pano.getNrOfImages(); ++i)
        {
            HuginBase::SrcPanoImage img = pano.getSrcImage(i);
            if (!img.readEXIF())
            {
                std::cout << " Ignored (File missing or missing metadata)" << std::endl;
                return false;
            }
            pano.setSrcImage(i, img);
        };
        // update cp errors
        HuginBase::PTools::calcCtrlPointErrors(pano);
        // now save in database
        if (HuginBase::LensDB::SaveLensDataFromPano(pano))
        {
            std::cout << " Saved." << std::endl;
            return true;
        }
        else
        {
            std::cout << " Ignored." << std::endl;
            return false;
        }
    };
    std::cout << " Ignored (More than one lens)." << std::endl;
    return false;
};

static void usage(const char* name)
{
    std::cout << name << ": tool for lens database maintenance" << std::endl
              << name << " version " << hugin_utils::GetHuginVersion() << std::endl
              << std::endl
              << "Usage:  hugin_lensdb [--recursive] --populate BASEPATH " << std::endl
              << "             Populate database with information from all pto files" << std::endl
              << "             in given BASEPATH" << std::endl
              << "             With --recursive switch all subfolders will also be" << std::endl
              << "             searched." << std::endl
              << "        hugin_lensdb --compress" << std::endl
              << "             Compresses the database by replacing single values" << std::endl
              << "             with averaged values." << std::endl
              << "        hugin_lensdb --remove-lens=LENS" << std::endl
              << "             Removes given lens from the database." << std::endl
              << "        hugin_lensdb --remove-camera=MAKER|MODEL" << std::endl
              << "             Removes given camera from the database." << std::endl
              << "        hugin_lensdb --export-database=FILENAME" << std::endl
              << "             Export data from database to external file." << std::endl
              << "        hugin_lensdb --import-from-file=FILENAME" << std::endl
              << "             Import data from external file." << std::endl
              << std::endl;
};

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "crph";
    enum
    {
        REMOVE_LENS=1000,
        REMOVE_CAM=1001,
        EXPORT_DB=1002,
        IMPORT_DB=1003,
    };

    static struct option longOptions[] =
    {
        { "compress", no_argument, NULL, 'c' },
        { "recursive", no_argument, NULL, 'r' },
        { "populate", no_argument, NULL, 'p' },
        { "remove-lens", required_argument, NULL, REMOVE_LENS },
        { "remove-camera", required_argument, NULL, REMOVE_CAM },
        { "export-database", required_argument, NULL, EXPORT_DB },
        { "import-from-file", required_argument, NULL, IMPORT_DB },
        { "help", no_argument, NULL, 'h' },
        0
    };

    bool recursive=false;
    bool populate = false;
    bool compress = false;
    std::string basepath;
    std::string lensToRemove;
    std::string camToRemove;
    std::string exportDatabase;
    std::string importDatabase;
    int c;
    int optionIndex = 0;
    while ((c = getopt_long (argc, argv, optstring, longOptions,&optionIndex)) != -1)
    {
        switch (c)
        {
            case 'h':
                usage(hugin_utils::stripPath(argv[0]).c_str());
                return 0;
            case 'c':
                compress=true;
                break;
            case 'r':
                recursive=true;
                break;
            case 'p':
                populate = true;
                break;
            case REMOVE_LENS:
                lensToRemove = hugin_utils::StrTrim(std::string(optarg));
                break;
            case REMOVE_CAM:
                camToRemove = hugin_utils::StrTrim(std::string(optarg));
                break;
            case EXPORT_DB:
                exportDatabase = hugin_utils::StrTrim(std::string(optarg));
                break;
            case IMPORT_DB:
                importDatabase = hugin_utils::StrTrim(std::string(optarg));
                break;
            case '?':
                break;
            default:
                abort ();
        }
    }

    if (!exportDatabase.empty() && !importDatabase.empty())
    {
        std::cerr << "ERROR: Export and import can not be done at the same time. " << std::endl;
        return -1;
    };

    if (populate)
    {
        if (argc - optind != 1)
        {
            usage(hugin_utils::stripPath(argv[0]).c_str());
            return -1;
        };

        basepath = argv[optind];
    };

    if (!populate && !compress && lensToRemove.empty() && camToRemove.empty() && exportDatabase.empty() && importDatabase.empty())
    {
        std::cout << "Lensdatabase file: " << HuginBase::LensDB::LensDB::GetSingleton().GetDBFilename() << std::endl;
        std::cout << "Nothing to do." << std::endl;
    };

    if (!basepath.empty())
    {
        boost::filesystem::path p(basepath);
        if (boost::filesystem::exists(p))
        {
            p = boost::filesystem::absolute(p);
            if (boost::filesystem::is_directory(p))
            {
                pathVec projectFiles;
                FindPTOFiles(projectFiles, p.string(), recursive);
                if (projectFiles.empty())
                {
                    std::cerr << "ERROR: No project files found in given directory " << p.string() << std::endl;
                    return 1;
                };
                for (pathVec::const_iterator it = projectFiles.begin(); it != projectFiles.end(); ++it)
                {
                    CheckProjectFile(*it);
                };
            }
            else
            {
                std::cerr << "ERROR: " << basepath << " is not a directory." << std::endl;
                return 1;
            };
        }
        else
        {
            std::cerr << "ERROR: Path " << basepath << " does not exists." << std::endl;
            return 1;
        }
    };

    if (compress)
    {
        std::cout << "Compressing database..." << std::endl;
        if(HuginBase::LensDB::LensDB::GetSingleton().CleanUpDatabase())
        {
            std::cout << "Successful." << std::endl;
        }
        else
        {
            std::cout << "FAILED." << std::endl;
        }
    };
    // remove lens
    if (!lensToRemove.empty())
    {
        std::cout << "Removing lens \"" << lensToRemove << "\"..." << std::endl;
        if (HuginBase::LensDB::LensDB::GetSingleton().RemoveLens(lensToRemove))
        {
            std::cout << "Successful." << std::endl;
        }
        else
        {
            std::cout << "FAILED." << std::endl;
        }
    };
    // remove camera
    if (!camToRemove.empty())
    {
        std::vector<std::string> input = hugin_utils::SplitString(camToRemove, "|");
        if (input.size() == 2)
        {
            std::cout << "Removing camera \"" << input[1] << "\" (Maker: \"" << input[0] << "\")..." << std::endl;
            if (HuginBase::LensDB::LensDB::GetSingleton().RemoveCamera(input[0], input[1]))
            {
                std::cout << "Successful." << std::endl;
            }
            else
            {
                std::cout << "FAILED." << std::endl;
            }
        }
        else
        {
            std::cout << "\"" << camToRemove << "\" is not a valid string for the camera." << std::endl
                << "    Use syntax MAKER|MODEL (separate camera maker and model by |)" << std::endl;
        };
    };
    // export database
    if (!exportDatabase.empty())
    {
        std::cout << "Exporting database to \"" << exportDatabase << "\"..." << std::endl;
        if (HuginBase::LensDB::LensDB::GetSingleton().ExportToFile(exportDatabase))
        {
            std::cout << "Successful." << std::endl;
        }
        else
        {
            std::cout << "FAILED." << std::endl;
        };
    };
    // import database
    if (!importDatabase.empty())
    {
        std::cout << "Importing data from \"" << importDatabase << "\"..." << std::endl;
        if (HuginBase::LensDB::LensDB::GetSingleton().ImportFromFile(importDatabase))
        {
            std::cout << "Successful." << std::endl;
        }
        else
        {
            std::cout << "FAILED." << std::endl;
        };
    };
    HuginBase::LensDB::LensDB::Clean();
    return 0;
}
