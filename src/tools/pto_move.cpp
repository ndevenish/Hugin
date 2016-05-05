// -*- c-basic-offset: 4 -*-

/** @file pto_move.cpp
 *
 *  @brief helper program for moving project with all images at once
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

#include "hugin_config.h"
#include <iostream>
#include <string>
#include <fstream>
#ifdef HAVE_STD_FILESYSTEM
#include <filesystem>
namespace fs = std::tr2::sys;
#define OVERWRITE_EXISTING std::tr2::sys::copy_options::overwrite_existing
#else
#define BOOST_FILESYSTEM_VERSION 3
#ifdef __GNUC__
  #include "hugin_config.h"
  #include <boost/version.hpp>
  #if BOOST_VERSION<105700
    #if BOOST_VERSION>=105100
      // workaround a bug in boost filesystem
      // boost filesystem is probably compiled with C++03
      // but Hugin is compiled with C++11, this results in
      // conflicts in boost::filesystems at a enum
      // see https://svn.boost.org/trac/boost/ticket/6779
      #define BOOST_NO_CXX11_SCOPED_ENUMS
    #else
      #define BOOST_NO_SCOPED_ENUMS
    #endif
  #endif
#endif
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#define OVERWRITE_EXISTING boost::filesystem::copy_option::overwrite_if_exists
#endif
#include <getopt.h>
#include <panodata/Panorama.h>
#include <hugin_utils/stl_utils.h>

std::string IncludeTrailingDelimiter(std::string path)
{
    std::string s=path;
#ifdef _WIN32
    if(s.compare(s.length()-1,1,"\\")!=0 && s.compare(s.length()-1,1,"/")!=0)
    {
        s.append("\\");
    };
#else
    if(s.compare(s.length()-1,1,"/")!=0)
    {
        s.append("/");
    };
#endif
    return s;
};

// rebase a filename from relative to srcPath to relative to destPath and return absolute new dest path
bool RebaseFilename(fs::path srcFile, fs::path& destFile, std::string srcPath, std::string destPath)
{
    fs::path input=fs::absolute(srcFile);
    std::string fullInputPath=input.string();
    std::string srcPathWithTrailingDelimiter=IncludeTrailingDelimiter(srcPath);
    if(fullInputPath.compare(0, srcPathWithTrailingDelimiter.length(), srcPathWithTrailingDelimiter)!=0)
    {
        return false;
    };
    fullInputPath.replace(0, srcPathWithTrailingDelimiter.length(), IncludeTrailingDelimiter(destPath));
    destFile=fs::path(fullInputPath);
    return true;
};

bool checkDestinationDirectory(std::string dir, fs::path& pathTo)
{
    pathTo=fs::path(dir);
    try
    {
        // check if a destination directory is given
        if(pathTo.extension().string().length()>0)
        {
            std::cerr << "ERROR: Your destination is a file. Copy/Move several files to " << std::endl
                      << "a single file is not allowed." << std::endl
                      << "Canceled operation." << std::endl;
            return false;
        };
        // create destination directory if not exists
        if(!fs::exists(pathTo))
        {
            if(!fs::create_directories(pathTo))
            {
                std::cerr << "ERROR: Could not create destination directory: " << pathTo.string() << std::endl
                          << "Maybe you have not sufficient rights to create this directory." << std::endl;
                return false;
            };
        };
    }
    catch (const fs::filesystem_error& ex)
    {
        std::cout << ex.what() << std::endl;
        return false;
    }
    pathTo=fs::absolute(pathTo);
    return true;
};

typedef std::vector<fs::path> pathVec;

bool PTOCopyMove(bool movingFile, fs::path src, fs::path dest, bool overwriteAllFiles)
{
    fs::path destFile(hugin_utils::GetAbsoluteFilename(dest.string()));
    std::cout << (movingFile ? "Moving project file  " : "Copying project file ") << src.filename() << std::endl
              << "  from " << src.parent_path() << std::endl
              << "  to " << destFile.parent_path() << std::endl;
    // open project file
    HuginBase::Panorama pano;
    std::string input=src.string();
    std::ifstream prjfile(input.c_str());
    if (!prjfile.good())
    {
        std::cerr << "ERROR: Could not open script: " << src.string() << std::endl;
        return false;
    }
    std::string inputPathPrefix=hugin_utils::getPathPrefix(input);
    std::string outputPathPrefix=hugin_utils::getPathPrefix(destFile.string());
    pano.setFilePrefix(inputPathPrefix);
    AppBase::DocumentData::ReadWriteError err = pano.readData(prjfile);
    if (err != AppBase::DocumentData::SUCCESSFUL)
    {
        std::cerr << "ERROR: error while parsing panos tool script: " << input << std::endl
                  << "AppBase::DocumentData::ReadWriteError code: " << err << std::endl;
        return false;
    };
    prjfile.close();
    if(pano.getNrOfImages()==0)
    {
        std::cerr << "ERROR: project " << input << " does not contain any images." << std::endl;
        return false;
    };
    pathVec imagesFrom, imagesTo;
    // check if all images exists
    for(size_t i=0; i<pano.getNrOfImages(); i++)
    {
        fs::path p(pano.getImage(i).getFilename());
        imagesFrom.push_back(p);
        if(!fs::exists(p) || !fs::is_regular_file(p))
        {
            std::cerr << "ERROR: image " << p.string() << " not found on disc." << std::endl
                      << "Skipping project " << input << std::endl;
            return false;
        };
        p=fs::absolute(p);
        // now build now image filename
        fs::path newFilename;
        if(RebaseFilename(p, newFilename, inputPathPrefix, outputPathPrefix))
        {
            pano.setImageFilename(i, newFilename.string());
            imagesTo.push_back(newFilename);
        };
    };
    if(imagesFrom.size()>0)
    {
        if(imagesFrom.size()==imagesTo.size())
        {
            fs::path targetDir(destFile);
            targetDir.remove_filename();
            if(!checkDestinationDirectory(targetDir.string(), targetDir))
            {
                return false;
            };
            if(fs::exists(destFile) && !overwriteAllFiles)
            {
                std::cout << "Project file " << destFile << " does already exists." << std::endl
                          << "  Overwrite this file? [Y|N] ";
                std::string userAnswer;
                while(userAnswer.length()==0)
                {
                    std::cin >> userAnswer;
                };
                userAnswer=hugin_utils::toupper(userAnswer);
                if(userAnswer!="YES" && userAnswer!="Y")
                {
                    std::cout << std::endl << "Moving/Copying of project file " << input << " canceled." << std::endl << std::endl;
                    return false;
                }
            };
            //copy/moving images
            for(size_t i=0; i<imagesFrom.size(); i++)
            {
                // check if target directory already exists
                targetDir=fs::path(imagesTo[i]);
                targetDir.remove_filename();
                if(!checkDestinationDirectory(targetDir.string(), targetDir))
                {
                    return false;
                };
                //check if target image file already exists
                if(fs::exists(imagesTo[i]) && !overwriteAllFiles)
                {
                    std::cout << "Images file " << imagesTo[i] << " does already exists." << std::endl
                              << "  Overwrite this file? [Y|N] ";
                    std::string userAnswer;
                    while(userAnswer.length()==0)
                    {
                        std::cin >> userAnswer;
                    };
                    userAnswer=hugin_utils::toupper(userAnswer);
                    if(userAnswer!="YES" && userAnswer!="Y")
                    {
                        std::cout << std::endl << "Moving/Copying of project file " << input << " canceled." << std::endl << std::endl;
                        return false;
                    }
                };
                if(movingFile)
                {
                    try
                    {
                        fs::rename(imagesFrom[i], imagesTo[i]);
                    }
                    catch (const fs::filesystem_error& ex)
                    {
                        std::cout << ex.what() << std::endl;
                        return false;
                    }
                }
                else
                {
                    try
                    {
                        fs::copy_file(imagesFrom[i], imagesTo[i], OVERWRITE_EXISTING);
                    }
                    catch (const fs::filesystem_error& ex)
                    {
                        std::cout << ex.what() << std::endl;
                        return false;
                    }
                };
            }; // for loop for all images
            // now create pano file in new destination
            // write output
            HuginBase::UIntSet imgs;
            fill_set(imgs, 0, pano.getNrOfImages()-1);
            std::ofstream of(destFile.string().c_str());
            pano.printPanoramaScript(of, pano.getOptimizeVector(), pano.getOptions(), imgs, false, outputPathPrefix);
            of.close();
            if(movingFile)
            {
                try
                {
                    fs::remove(src);
                }
                catch (const fs::filesystem_error& ex)
                {
                    std::cout << "Could not remove original file: " << input << std::endl;
                    std::cout << ex.what() << std::endl;
                    return false;
                }
            };
        }
        else
        {
            // the images in the project file are not all relative to the same base path
            std::cout << "WARNING: Images location in project file are not consistent. " << std::endl
                      << "So don't move/copy project file " << src.string() << std::endl;
            return false;
        };
    }
    else
    {
        // now create pano file in new destination, project contains images in paths
        // not relative to base directory
        // so create only the new project file without copying/moving image files
        HuginBase::UIntSet imgs;
        fill_set(imgs, 0, pano.getNrOfImages()-1);
        std::ofstream of(destFile.string().c_str());
        pano.printPanoramaScript(of, pano.getOptimizeVector(), pano.getOptions(), imgs, false, outputPathPrefix);
        of.close();
        if(movingFile)
        {
            try
            {
                fs::remove(src);
            }
            catch (const fs::filesystem_error& ex)
            {
                std::cout << "Could not remove original file: " << input << std::endl;
                std::cout << ex.what() << std::endl;
                return false;
            }
        };
    };
    return true;
};

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
    catch(fs::filesystem_error& e)
    {
        std::cout << e.what() << std::endl;
        return false;
    }
    return true;
};

void SearchPTOFilesInDirectory(pathVec& projectFiles, std::string src, bool recursive)
{
    if(recursive)
    {
        iterateFileSystem<fs::recursive_directory_iterator>(src, projectFiles);
    }
    else
    {
        iterateFileSystem<fs::directory_iterator>(src, projectFiles);
    };
};

static void usage(const char* name)
{
    std::cout << name << ": move a project file with all images in it" << std::endl
              << name << " version " << hugin_utils::GetHuginVersion() << std::endl
              << std::endl
              << "Usage:  pto_move [options] path1/source.pto path2/dest.pto" << std::endl
              << "             Rename project file path1/source.pto to " << std::endl
              << "             path2/dest.pto. All images contained in the project will" << std::endl
              << "             be moved accordingly." << std::endl << std::endl
              << "        pto_move [options] sourceFolder destFolder" << std::endl
              << "             Moves all project files in the source folder to " << std::endl
              << "             the destination folder including all images." << std::endl
              << std::endl
              << "Options: " << std::endl
              << "  --copy       Copy project files and images instead of moving" << std::endl
              << "  --recursive  Only effective in use case 2. Go recursive in the" << std::endl
              << "               the source folder and move all project files with images" << std::endl
              << "               to destination folder by maintaining the folder structure" << std::endl
              << "               relative to source folder." << std::endl
              << "  --overwrite  Overwrite all existing files. Otherwise you will be asked" << std::endl
              << "               for each existing file." << std::endl
              << std::endl
              << std::endl;
};

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "croh";

    static struct option longOptions[] =
    {
        {"copy", no_argument, NULL, 'c' },
        {"recursive", no_argument, NULL, 'r' },
        {"overwrite", no_argument, NULL, 'o' },
        {"help", no_argument, NULL, 'h' },
        0
    };

    bool movingFiles=true; //movingFiles: false->copy, true->move
    bool recursive=false;
    bool forceOverwrite=false;
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
                movingFiles=false;
                break;
            case 'r':
                recursive=true;
                break;
            case 'o':
                forceOverwrite=true;
                break;
            case '?':
                break;
            default:
                abort ();
        }
    }

    if(argc-optind<2)
    {
        std::cout << "PTO_MOVE: You need to give at least a source and a destination project file or directory." << std::endl;
        return 1;
    };

    try
    {
        fs::path p(argv[optind]);
        if(fs::exists(p))
        {
            p=fs::absolute(p);
            if(fs::is_directory(p))
            {
                // first parameter is a directory
                fs::path pathTo;
                if(!checkDestinationDirectory(std::string(argv[argc-1]), pathTo))
                {
                    return 1;
                };
                // search all pto files in directory
                pathVec projectFiles;
                std::cout << "Searching project files in " << p << std::endl;
                SearchPTOFilesInDirectory(projectFiles, p.string(), recursive);
                if(projectFiles.size()==0)
                {
                    std::cout << "No project files found in given directory " << p.string() << std::endl;
                    return 0;
                };
                std::cout << "Found " << projectFiles.size() << " project files." << std::endl << std::endl;
                for(pathVec::const_iterator it=projectFiles.begin(); it!=projectFiles.end(); ++it)
                {
                    fs::path newPath;
                    if(RebaseFilename(*it, newPath, p.string(), pathTo.string()))
                    {
                        PTOCopyMove(movingFiles, *it, newPath, forceOverwrite);
                    };
                };
            }
            else
            {
                if(argc-optind>2)
                {
                    // several files given
                    // check if destination is a directory and create it if necessary
                    fs::path pathTo;
                    if(!checkDestinationDirectory(std::string(argv[argc-1]), pathTo))
                    {
                        return 1;
                    };
                    while(optind<argc-1)
                    {
                        p=fs::path(argv[optind]);
                        std::string ext=hugin_utils::toupper(p.extension().string());
                        // work only on pto files
                        if(ext==".PTO")
                        {
                            if(fs::exists(p) && fs::is_regular_file(p))
                            {
                                p=fs::absolute(p);
                                fs::path newPath = pathTo / p.filename();
                                PTOCopyMove(movingFiles, p, newPath, forceOverwrite);
                            }
                            else
                            {
                                std::cout << "WARNING: File " << p << " does not exists" << std::endl
                                          << "Skipping this file." << std::endl;
                            };
                        };
                        optind++;
                    };
                }
                else
                {
                    // exactly 2 files given
                    fs::path pathTo(argv[argc-1]);
                    if(pathTo.extension().string().length()>0)
                    {
                        // user has given filename with extension
                        // so simply copy/move file
                        pathTo=fs::absolute(pathTo);
                        PTOCopyMove(movingFiles, p, pathTo, forceOverwrite);
                    }
                    else
                    {
                        // target is a directory
                        if(!checkDestinationDirectory(std::string(argv[argc-1]), pathTo))
                        {
                            return 1;
                        };
                        pathTo=pathTo / p.filename();
                        if(p==pathTo)
                        {
                            std::cerr << "ERROR: Target and destination file are the same." << std::endl
                                      << "Skipping file processing." << std::endl;
                            return 1;
                        };
                        PTOCopyMove(movingFiles, p, pathTo, forceOverwrite);
                    };
                };
            };
        };
    }
    catch (const fs::filesystem_error& ex)
    {
        std::cout << ex.what() << std::endl;
    }
    return 0;
}
