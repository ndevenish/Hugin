/** @file hpi_classes.h
 *
 *  @brief core classes of the hpi interface, not for user code
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
 *  KFJ 2011-01-18
 * 
 */
// hpi_classes.h defines the Python interface. It's not meant to
// be included by hugin code except for hpi.cpp, which holds
// the single instance of class python_interface and provides
// a function to call hpi plugins.
// the classes here define more functionality than is actually
// used currently, but this does no harm.

#include <Python.h>            // always first
#include "swigpyrun.h"         // contains SWIG access
#include <panodata/Panorama.h> // maybe need more hugin headers?

// the Python interface has a class of it's own which encapsulates
// the interface. It will only function if it's activate()
// method is called. Note there is only a single instance of this class;
// it lives in hpi.cpp

class python_interface
{
private :
  bool activated ;        // flag, true if activated
  PyObject * hsi_module ; // pointer to loaded hsi module
  PyObject * hpi_module ; // pointer to loaded hpi module

  // the python interface relies on two modules:
  // - hsi, containing the SWIG-generated wrap of hugin functionality
  // - hpi, the module to dispatch plugin calls to individual plugins
  // load_module is a general module-loading function used for both:
  
  PyObject * load_module ( const char * name )
  {

#ifdef HPI_VERBOSE

    fprintf ( stderr , "HPI: loading module %s\n" , name ) ;

#endif

    PyObject * pModule = PyImport_ImportModule ( name ) ;

    if ( ! pModule )
    {
      PyErr_Print () ;
      fprintf ( stderr , "HPI ERROR: Failed to load module %s\n" , name );
      return NULL ;
    }

    return pModule ;
  }
  
public:

  // Ctor and Dtor
  
  python_interface() : activated(false) {} ;

  ~python_interface()
  {
    if ( activated ) // the interface was used, do cleanup
    {

#ifdef HPI_VERBOSE

      fprintf ( stderr , "HPI: finalizing Python\n" ) ;

#endif

      // cleaning up
      Py_XDECREF ( hsi_module ) ;
      Py_XDECREF ( hpi_module ) ;
      Py_Finalize();
    }
  }

  // activate loads the necessary modules hsi and hpi.
  // this will only succeed if Python can find them.
  // Currently this will only succeed if the modules are
  // in PYTHONPATH, so you might either keep them in
  // Python's module directory or set PYTHONPATH to where
  // you keep them, like 'export PYTHONPATH=.'
  
  bool activate()
  {

    if ( activated )
      return true ;

#ifdef HPI_VERBOSE

    fprintf ( stderr , "HPI: initializing Python\n" ) ;

#endif    
    
    Py_Initialize();

    // load hsi module
    hsi_module = load_module ( "hsi" ) ;

    if ( hsi_module )
    {
      // load hpi module
      hpi_module = load_module ("hpi");
      if ( hpi_module )
        activated = true ;
      else
	Py_DECREF ( hsi_module ) ;
    }
    if ( ! activated )
      Py_Finalize() ;
    return activated ;
  }

  // call_hsi will call a routine in the hsi module with a bunch
  // of parameters. The parameters are passed in as a python tuple
  // which is constructed with the python_arglist class below.
  // note that currently the only function in hpi is hpi_dispatch()
  // but since this may change it isn't hardcoded.
  
  int call_hpi ( const char * hpi_func ,
		 PyObject * pArgs )
  {
    int result = -1 ;

#ifdef HPI_VERBOSE

    fprintf ( stderr , "HPI: calling %s\n" , hpi_func ) ;

#endif    

    // look up the desired function in the hpi module's namespace
    PyObject * pFunc = PyObject_GetAttrString ( hpi_module , hpi_func ) ;
    // if it's found and can be called, go ahead call it
    if ( pFunc && PyCallable_Check ( pFunc ) )
    {
      PyObject * pValue = PyObject_CallObject ( pFunc , pArgs ) ;
      if ( pValue != NULL )
      {
	// the function should have returned an integer
	result = PyInt_AsLong ( pValue ) ;
        Py_DECREF(pValue);
      }
    }
    Py_XDECREF ( pFunc ) ;
    
#ifdef HPI_VERBOSE

    fprintf ( stderr , "HPI: call returns: %d\n" , result );

#endif
    
    return result ;
  }
} ;

class python_arglist
{
// the class members are private

private :

  PyObject *pArgs ;
  int argc ;
  int have ;

public:
  
// general function to make a Python object from a hugin object.
// this general case receives a void * and can make any type
// defined in the SWIG interface.
// hsi_type must be passed either as the SWIG mangled name
// or as the human-readable name, so for a char*,
// "char*" or "_p_char" will work.

  PyObject * make_hsi_object ( const char * hsi_type ,
			       void * hugin_value )
  {
    SWIGRUNTIME swig_type_info *swigtype =
    SWIG_Python_TypeQuery ( hsi_type ) ;

#ifdef HPI_VERBOSE

    fprintf ( stderr ,
	      "HPI: making a %s from %p\n" ,
	      hsi_type ,
	      hugin_value ) ;
#endif
	      
    if ( ! swigtype )
        {
           fprintf ( stderr,
		      "HPI ERROR: can't find SWIG type for %s\n" ,
	              hsi_type ) ;
            return NULL ;
        }

   // we have managed to gain the SWIG type information,
   // se we can generate the Python object
   return SWIG_NewPointerObj ( hugin_value , swigtype , 0 ) ;
  }

  // the constructor is called with the number of arguments
  // the argument list is meant to contain. Note that you
  // must add precisely this amount of arguments.
  
  python_arglist ( int _argc ) : argc ( _argc ) , have ( 0 )
  {
    pArgs = PyTuple_New ( argc ) ;
  }

  ~python_arglist()
  {
    Py_DECREF ( pArgs ) ;
  }

public :

  // add a python object to the argument list. This will only
  // succeed if there is still space in the tuple. Note that
  // you must have the Python object already to use this method.

  bool add ( PyObject * python_arg )
  {     
    if ( ( ! python_arg ) || ( have >= argc ) )
      return false ;
    PyTuple_SetItem ( pArgs , have++ , python_arg ) ;
    return true ;
  }

  // add a string to the argument list after converting it
  // to an acceptable Python object. This function can easily
  // be copied and modified to apply to other data types,
  // just pick the proper PyXXX_From_YYY Python API function.
  
  bool add ( const char * str )
  {

#ifdef HPI_VERBOSE

    fprintf ( stderr ,
	      "HPI: making a PyString from '%s'\n" , str ) ;
	      
#endif

#if PY_MAJOR_VERSION>=3
    return add ( PyUnicode_FromString(str));
#else
    return add ( PyString_FromString ( str ) ) ;
#endif
  }

  // once we're done filling the tuple with arguments, we call
  // yield to get it and pass it to the Python function.
  // Note that this will only succeed if the argument count
  // is correct.
  
  PyObject * yield()
  {
    if ( argc != have )
      return NULL ;
    return pArgs ;
  }
} ;
