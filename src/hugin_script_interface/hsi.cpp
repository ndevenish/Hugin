// -*- c-basic-offset: 4 -*-

/** @file hsi.cpp
 *
 *  @brief helper functions for the hugin scripting interface
 *
 *  @author Kay F. Jahnke
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
 *  KFJ 2011-01-13
 * 
 */

#include <Python.h>
#include "swigpyrun.h"
#include <fstream>
#include <stdio.h>

// obviously we need the data types in Panorama.h

#include <panodata/Panorama.h>
#include <panotools/PanoToolsUtils.h>

using namespace std;
using namespace HuginBase;
using namespace hugin_utils;
using namespace AppBase;

// all access to a Panorama is via the Panorama class. Objects of
// this type can be created from a pto file by calling the readData()
// method of class Panorama.
// The pano_open routine does just that and returns a pointer to
// the newly created Panorama object - or NULL if it fails.
// Note that using the standard streams in this file didn't work,
// so (error) i/o is via the fprintf type functions.
// class Panorama also has a constructor that takes 

HuginBase::Panorama * pano_open ( const char * infile )
{
    string input ( infile ) ;

    ifstream prjfile(input.c_str());
    
    if (!prjfile.good()) {
        fprintf ( stderr ,
		  "could not open script %s\n" ,
		  input.c_str() ) ;
        return NULL;
    }

    HuginBase::Panorama * pano = new HuginBase::Panorama ;

    pano->setFilePrefix ( getPathPrefix ( input ) );
    AppBase::DocumentData::ReadWriteError err = pano->readData(prjfile);

    if (err != DocumentData::SUCCESSFUL) {
        fprintf ( stderr ,
		  "%s %s\n%s %d\n" ,
                  "error while parsing panos tool script:" ,
		  input.c_str() ,
		  "DocumentData::ReadWriteError code:" ,
		  err ) ;
	delete pano ;
	return NULL ;
    }

    // like when a Panorama is opened in hugin, I also call
    // calcCtrlPointErrors() to calculate the CP 'distance'
    // which is otherwise initially 0.0 for all points.
    
    HuginBase::PTools::calcCtrlPointErrors ( *pano ) ;
    return pano ;
}

// pano_close does the obvious.

void pano_close ( HuginBase::Panorama * pano )
{
    delete pano ;
}

std::istream * make_std_ifstream ( const char * charp )
{
    return (std::istream*)(new std::ifstream ( charp )) ;
}

std::ostream * make_std_ofstream ( const char * charp )
{
    return (std::ostream*)(new std::ofstream ( charp )) ;
}

void hello_python ( HuginBase::Panorama * pano )
{
  // cout << "received pano pointer" << endl ;
  fprintf ( stderr , "pano pointer is %p\n" , pano ) ;
  int img = pano->getNrOfImages() ;
  cout << "pano has " << img << " images" << endl ;
    /*
    cout << "seeking type: " << text << endl ;
    SWIGRUNTIME swig_type_info *swigtype = SWIG_TypeQuery(text) ;
    if ( ! swigtype )
        cout << "can't get swigtype"<< endl ;
    else
        cout << "found it" << endl ;
    */
}
