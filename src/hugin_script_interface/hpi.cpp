/** @file hpi.h
 *
 *  @brief C++ call interface to hpi
 *
 *  @author Kay F. Jahnke
 *
 *  hpi.cpp is the code for the part of hpi that has to be
 *  linked to any application using the hsi types to enable
 *  it to call python plugins. This separates the implementation
 *  of the plugin interface, which is mainly in hpi.h and this file,
 *  from the user interface, which is in hpi.h and only declares
 *  callhpi(). Inclusion of hpi.h therefore has no side-effects
 *  and hpi can be modified without triggering recompiles in
 *  code using it.
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
 *  KFJ 2011-01-18
 *
 */

// uncomment this if you want diagnostics:
// #define HPI_VERBOSE

// first pull in the Python interface classes
#include "hugin_script_interface/hpi_classes.h"

// pull in the declaration of callhpi() to ensure it's consistent
// with the definition below
#include "hugin_script_interface/hpi.h"

// callhpi() is a variadic function, so we need stdarg.h
// This is old-fashioned and may become deprecated, but for
// now it's very convenient.
#include <stdarg.h>

namespace hpi
{
/** this is where we keep the single instance of class python_interface */
static python_interface hpi_instance;

int callhpi ( const char* plugin_name ,
              int argc ,
              ... )
{
    va_list arglist ;
    va_start ( arglist , argc ) ;

    // activate the python interface if it's not on already
    if ( ! hpi_instance.activate() )
    {
        // trouble
        fprintf ( stderr , "HPI ERROR: failed to activate Python interface\n" ) ;
        va_end ( arglist ) ;
        return -20 ;
    }

    // we want to build a python argument list with argc + 1 arguments,
    // + 1 because the first argument is the plugin name:
    python_arglist pyarglist ( argc + 1 ) ;
    pyarglist.add ( plugin_name ) ;

    for ( int argno = 0 ; argno < argc ; argno++ )
    {
        // extract the type name
        const char* type_name = va_arg ( arglist , const char* ) ;
        // extract the pointer to argument object
        void* pvalue = va_arg ( arglist , void* ) ;
        // make a python object from this information
        PyObject* pyarg = pyarglist.make_hsi_object ( type_name , pvalue ) ;
        // try add it to the arglist
        if ( ! pyarglist.add ( pyarg ) )
        {
            // oops... this went wrong
            va_end ( arglist ) ;
            return -21 ;
        }
    }
    va_end ( arglist ) ; // we're done with that

    // seems we're okay. get the argument tuple out
    PyObject* arg_tuple = pyarglist.yield() ;
    if ( ! arg_tuple )
    {
        // oops... this went wrong
        return -22 ;
    }

    // now let's call the thing. This is done by passing the argument
    // tuple to hpi_dispatch, a Python function defined in hpi.py
    // this function interprets the first argument as a plugin name
    // and calls that plugin with the remaining parameters.
    return hpi_instance.call_hpi ( "hpi_dispatch" , arg_tuple );
}

} // namespace hsi

// When I didn't yet have an entry point from hugin to call hpi,
// I used the next bit as my main() code. For now this is left
// inside, it may be handy later on.

#ifdef standalone

#include <fstream>

using namespace std;
using namespace HuginBase;
using namespace hugin_utils;
using namespace AppBase;

// we use this function to make a panorama from a pto file
// so we have something to pass to the hsi module. Later on,
// this function will disappear and the calls will generate
// from hugin

HuginBase::Panorama* pano_open ( const char* infile )
{
    string input ( infile ) ;

    ifstream prjfile(input.c_str());

    if (!prjfile.good())
    {
        fprintf ( stderr ,
                  "could not open script %s\n" ,
                  input.c_str() ) ;
        return NULL;
    }

    HuginBase::Panorama* pano = new HuginBase::Panorama ;

    pano->setFilePrefix ( getPathPrefix ( input ) );
    AppBase::DocumentData::ReadWriteError err = pano->readData(prjfile);

    if (err != DocumentData::SUCCESSFUL)
    {
        fprintf ( stderr ,
                  "%s %s\n%s %d\n" ,
                  "error while parsing panos tool script:" ,
                  input.c_str() ,
                  "DocumentData::ReadWriteError code:" ,
                  err ) ;
        delete pano ;
        return NULL ;
    }
    fprintf ( stderr , "opened pano: %p\n" , pano ) ;
    return pano ;
}

// demo-main that opens the pto named in argv[1] and passes
// the Panorama pointer to a function named by argv[2]
// in the hsi module

int main ( int argc , char* argv[] )
{
    HuginBase::Panorama* pano = pano_open ( argv[1] ) ;
    if ( ! pano )
    {
        fprintf(stderr, "Failed to open pano\n");
        return 1;
    }

    int success = hsi::callhpi ( argv[2] , 1 , "HuginBase::Panorama*" , pano ) ;

    return success ;
}
#endif // standalone
