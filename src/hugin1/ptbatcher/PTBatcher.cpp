// -*- c-basic-offset: 4 -*-

/** @file PTBatcher.cpp
 *
 *  @brief Batch processor for Hugin
 *
 *  @author Marko Kuder <marko.kuder@gmail.com>
 *
 *  $Id: PTBatcher.cpp 3322 2008-08-18 1:10:07Z mkuder $
 *
 *  This program is free software; you can redistribute it and/or
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

#include <string>
#include <hugin_version.h>

#if (!defined __APPLE__)
#include "getopt.h"
#endif

#ifdef WIN32
#include <getopt.h>
#else
#include <unistd.h>
#endif

#include "PTBatcher.h"

using namespace std;

//Prints out help text
void usage()
{
    cout << "PTBatcher: panotools batch stitcher" << endl
         << "Version " << DISPLAY_VERSION << endl << endl
         << "Usage:  PTBatcher [options] input.pto output" << endl
         << "Options:" << endl
         << "  -a file         add project to queue. Unless -o option is present, the default prefix is appointed." << endl
         << "  -o              output prefix (together with -a option)" << endl
         << "  -l              list all projects in batch queue" << endl
         << "  -c              clear all projects from batch queue" << endl
         << "  -x id           remove project with specified id" << endl
         << "  -r              reset all projects to waiting status" << endl
         << "  -b              start batch process" << endl
         << "  -p              use parallel processing (together with -b option)" << endl
         << "  -d              delete .pto files when complete (together with -b option)" << endl
         << "  -s              shutdown computer when complete (together with -b option)" << endl
         << "  -v              verbose mode" << endl
         << "  -h              print this help" << endl
         << "Arguments:" << endl
         << "  input.pto      Path to project file." << endl
         << endl;
}

#ifdef __WXMSW__	//windows version needs int main to enable console (and an OnInit function definition)
bool HostApp::OnInit()
{
    return true;
};

int main(int argc, char* argv[])
#else				//linux version needs main as OnInit for wxWidgets to work
bool HostApp::OnInit()
#endif
{
    //a character that defines the type of options
    //encountered, so only logical combinations are used
    char type = 'n';

    bool optionError = false;
    bool prefix = false;

    //create application and batch objects
#ifdef __WXMSW__	//windows version needs to create a wxApp object
    HostApp app;
    app.InitBatch(wxString::FromAscii(argv[0]));
    Batch* batch = app.batch;
    char** newArgv = argv;
#else	//linux version needs to convert argv
    InitBatch(argv[0]);

    char** newArgv = (char**)malloc(sizeof(char*)*argc);
    for(int i=0; i<argc; i++)
    {
        newArgv[i]= (char*)malloc(sizeof(char)*wxString(argv[i]).Length());
        strcpy(newArgv[i],wxString(argv[i]).char_str());
    }
#endif
    //parse arguments
    const char* optstring = "haolxrbpdsvc";
    char c;
    while ((c = getopt (argc, newArgv, optstring)) != -1)
    {
        switch(c)
        {
            case 'a':
                if(type != 'n' && type != 'a')
                {
                    optionError = true;
                }
                type = 'a';
                break;
            case 'l':
                if(type != 'n' && type != 'l')
                {
                    optionError = true;
                }
                type = 'l';
                break;
            case 'c':
                if(type != 'n' && type != 'c')
                {
                    optionError = true;
                }
                type = 'c';
                break;
            case 'x':
                if(type != 'n' && type != 'x')
                {
                    optionError = true;
                }
                type = 'x';
                break;
            case 'r':
                if(type != 'n' && type != 'r')
                {
                    optionError = true;
                }
                type = 'r';
                break;
            case 'b':
                if(type != 'n' && type != 'b')
                {
                    optionError = true;
                }
                type = 'b';
                break;
            case 'o':
                prefix = true;
                break;
            case 'p':
                batch->parallel = true;
                break;
            case 'd':
                batch->deleteFiles = true;
                break;
            case 's':
                batch->shutdown = true;
                break;
            case 'v':
                batch->verbose = true;
                break;
            case 'h':
                usage();
                return 0;
            default:
                usage();
                return 0;
        }
    }

    //check for other errors in option definition
    if(optionError || type == 'n' ||
            (type == 'a' && (batch->parallel || batch->deleteFiles || batch->shutdown)) ||
            (type == 'l' && (prefix || batch->parallel || batch->deleteFiles || batch->shutdown)) ||
            (type == 'c' && (prefix || batch->parallel || batch->deleteFiles || batch->shutdown)) ||
            (type == 'x' && (prefix || batch->parallel || batch->deleteFiles || batch->shutdown)) ||
            (type == 'r' && (prefix || batch->parallel || batch->deleteFiles || batch->shutdown)) ||
            (type == 'b' && prefix))
    {
        usage();
        //this->OnExit();
        return 1;
    }
    string input = "";
    string output = "";
    if(type == 'a')
    {
        if(prefix && (optind+2 == argc))
        {
            input = newArgv[optind];
            output = newArgv[optind+1];
        }
        else if(!prefix && (optind+1 == argc))
        {
            input = newArgv[optind];
        }
        else
        {
            usage();
            return 1;
        }
        batch->LoadTemp();
        batch->AddProjectToBatch(wxString(input.c_str(), wxConvLocal),wxString(output.c_str(), wxConvLocal));
        if(output != "")
        {
            cout << "Added project " << input << " with output " << output << endl;
        }
        else
        {
            cout << "Added project " << input << endl;
        }
        batch->SaveTemp();
    }
    else if(type == 'l')
    {
        if(optind!=argc)
        {
            usage();
            return 1;
        }
        batch->LoadTemp();
        batch->ListBatch();
        //this->OnExit();
        return 0;
    }
    else if(type == 'c')
    {
        if(optind!=argc)
        {
            usage();
            return 1;
        }
        batch->LoadTemp();
        batch->ClearBatch();
        batch->SaveTemp();
        return 0;
    }
    else if(type == 'x')
    {
        if(optind+1==argc && atoi(newArgv[optind])!=0)
        {
            batch->LoadTemp();
            batch->RemoveProject(atoi(newArgv[optind]));
            batch->SaveTemp();
            return 0;
        }
        else
        {
            usage();
            return 1;
        }
    }
    else if(type == 'r')
    {
        if(optind!=argc)
        {
            usage();
            return 1;
        }
        batch->LoadTemp();
        for(int i=0; i<batch->GetProjectCount(); i++)
        {
            batch->SetStatus(i,Project::WAITING);
        }
        batch->SaveTemp();
        return 0;
    }
    else if(type == 'b')
    {
        batch->LoadTemp();
        batch->RunBatch();
        while(!batch->AllDone())	//wait for all projects to complete to save temp
        {
#if defined __WXMSW__
            wxSleep(1);
#else
            sleep(1000);
#endif
        }
        batch->SaveTemp();
    }
    //this->OnExit();
    return EXIT_SUCCESS;
}
