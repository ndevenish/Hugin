/** @file hpi_classes.cpp
 *
 *  @brief core classes of the hpi interface, not for user code
 *
 *  @author Kay F. Jahnke
 *
 *  hpi_classes.h defines the Python interface. It's not meant to
 *  be included by hugin code except for hpi.cpp, which holds
 *  the single instance of class python_interface and provides
 *  a function to call hpi plugins.
 *  the classes here define more functionality than is actually
 *  used currently, but this does no harm.
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

#include "hpi_classes.h"

namespace hpi
{

PyObject* python_interface::load_module ( const char* name )
{
#ifdef HPI_VERBOSE
    fprintf ( stdout , "HPI: loading module %s\n" , name ) ;
#endif
    PyObject* pModule = PyImport_ImportModule ( name );
    if ( ! pModule )
    {
        PyErr_Print () ;
        fprintf ( stderr , "HPI ERROR: Failed to load module %s\n" , name );
        return NULL ;
    }
    return pModule;
}

python_interface::~python_interface()
{
    if ( activated ) // the interface was used, do cleanup
    {
#ifdef HPI_VERBOSE
        fprintf ( stdout , "HPI: finalizing Python\n" ) ;
#endif
        // cleaning up
        Py_XDECREF ( hsi_module ) ;
        Py_XDECREF ( hpi_module ) ;
        Py_Finalize();
    }
}

bool python_interface::activate()
{
    if ( activated )
    {
        return true ;
    }
#ifdef HPI_VERBOSE
    fprintf ( stdout , "HPI: initializing Python\n" ) ;
#endif
    Py_Initialize();
    // load hsi module
    hsi_module = load_module ( "hsi" ) ;
    if ( hsi_module )
    {
        // load hpi module
        hpi_module = load_module ("hpi");
        if ( hpi_module )
        {
            activated = true ;
        }
        else
        {
            Py_DECREF ( hsi_module ) ;
        }
    }
    if ( ! activated )
    {
        Py_Finalize() ;
    }
    return activated ;
}

int python_interface::call_hpi ( const char* hpi_func ,
                PyObject* pArgs )
{
    int result = -1 ;
#ifdef HPI_VERBOSE
    fprintf ( stdout , "HPI: calling %s\n" , hpi_func ) ;
#endif
    // look up the desired function in the hpi module's namespace
    PyObject* pFunc = PyObject_GetAttrString ( hpi_module , hpi_func ) ;
    // if it's found and can be called, go ahead call it
    if ( pFunc && PyCallable_Check ( pFunc ) )
    {
        PyObject* pValue = PyObject_CallObject ( pFunc , pArgs ) ;
        if ( pValue != NULL )
        {
            // the function should have returned an integer
            result = PyInt_AsLong ( pValue ) ;
            Py_DECREF(pValue);
        }
    }
    Py_XDECREF ( pFunc ) ;

#ifdef HPI_VERBOSE
    fprintf ( stdout , "HPI: call returns: %d\n" , result );
#endif
    return result ;
};

PyObject* python_arglist::make_hsi_object ( const char* hsi_type ,
                            void* hugin_value )
{
    swig_type_info* swigtype = SWIG_Python_TypeQuery ( hsi_type );
#ifdef HPI_VERBOSE
    fprintf ( stdout ,
                "HPI: making a %s from %p\n" ,
                hsi_type ,
                hugin_value );
#endif
    if ( ! swigtype )
    {
        fprintf ( stderr,
                    "HPI ERROR: can't find SWIG type for %s\n" ,
                    hsi_type );
        return NULL;
    }

    // we have managed to gain the SWIG type information,
    // se we can generate the Python object
    return SWIG_NewPointerObj ( hugin_value , swigtype , 0 );
};

python_arglist::python_arglist ( int _argc ) : argc ( _argc ) , have ( 0 )
{
    pArgs = PyTuple_New ( argc );
}

python_arglist::~python_arglist()
{
    Py_DECREF ( pArgs ) ;
}

bool python_arglist::add ( PyObject* python_arg )
{
    if ( ( ! python_arg ) || ( have >= argc ) )
    {
        return false ;
    }
    PyTuple_SetItem ( pArgs , have++ , python_arg ) ;
    return true ;
}

bool python_arglist::add ( const char* str )
{
#ifdef HPI_VERBOSE
    fprintf ( stdout , "HPI: making a PyString from '%s'\n" , str ) ;
#endif

#if PY_MAJOR_VERSION>=3
    return add ( PyUnicode_FromString(str));
#else
    return add ( PyString_FromString(str));
#endif
}

PyObject* python_arglist::yield()
{
    if ( argc != have )
    {
        return NULL ;
    }
    return pArgs ;
}

} //namespace
