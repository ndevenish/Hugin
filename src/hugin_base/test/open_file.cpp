// -*- c-basic-offset: 4 -*-
/*
 *  open_file.cpp
 *  Hugin
 *
 *  Created by Ippei UKAI on 2007-07-09.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#include <hugin_utils/utils.h>
#include <panodata/Panorama.h>

#include <iostream>
#include <fstream>

using namespace std;
using namespace AppBase;
using namespace HuginBase;


template<typename Type>
inline void my_MSG(string myMSG, Type myARG)
{
    cout << endl << myMSG << ": [" << myARG << "]" << endl << endl;
}


int main(int argc, char *argv[])
{
    DEBUG_ASSERT(argc==2);
    string filename = string(argv[1]);
    
    ifstream ptoFile;
    Panorama ptoFileData;
    

    my_MSG("reading file", filename);
    
    ptoFile.open(filename.c_str());
    if(!ptoFile.good())
        my_MSG("file status", "Not good :(  proceeding anyway...");
    else
        my_MSG("opening file", "Good!");
    
    ptoFileData.setFilePrefix(hugin_utils::getPathPrefix(filename));
    DocumentData::ReadWriteError err = ptoFileData.readData(ptoFile);
    my_MSG("data read with errnum", err);
    
    ptoFile.close();
    
    return 0;
}
