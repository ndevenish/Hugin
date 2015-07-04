/** @file hpi_classes.h
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

#ifdef MAC_SELF_CONTAINED_BUNDLE
  #include <Python27/Python.h>
#else
  #include <Python.h>            // always first
#endif // MAC_SELF_CONTAINED_BUNDLE
#include "swigpyrun.h"         // contains SWIG access
#include <panodata/Panorama.h>

namespace hpi
{
/** a class which encapsulates the python interface. It will only function if it's activate()
 *  method is called. Note there is only a single instance of this class;
 *  it lives in hpi.cpp
 *  the python interface relies on two modules:
 *  - hsi, containing the SWIG-generated wrap of hugin functionality
 *  - hpi, the module to dispatch plugin calls to individual plugins
 */
class python_interface
{
private :
    /** flag, true if activated */
    bool activated;
    /** pointer to loaded hsi module */
    PyObject* hsi_module;
    /** pointer to loaded hpi module */
    PyObject* hpi_module;
    /** general module-loading function */
    PyObject* load_module ( const char* name );

public:
    /** constructor */
    python_interface() : activated(false), hsi_module(NULL), hpi_module(NULL) {};
    /* destructor, does cleanup */
    ~python_interface();

    /** @brief: loads the necessary modules hsi and hpi.
     *  this will only succeed if Python can find them.
     *  Currently this will only succeed if the modules are
     *  in PYTHONPATH, so you might either keep them in
     *  Python's module directory or set PYTHONPATH to where
     *  you keep them
     */
    bool activate();

    /** call a routine in the hsi module with a bunch
     *  of parameters. The parameters are passed in as a python tuple
     *  which is constructed with the python_arglist class below.
     *  note that currently the only function in hpi is hpi_dispatch()
     *  but since this may change it isn't hardcoded.
     */
    int call_hpi ( const char* hpi_func ,
                   PyObject* pArgs );
};

/** helper class to generated PyObject from C/C++/hugin classes */
class python_arglist
{
    // the class members are private
private :
    PyObject* pArgs;
    int argc;
    int have;

public:
    /** @brief general function to make a Python object from a hugin object.
     *
     *  @param hsi_type type of the passed object, must be passed either as the SWIG mangled name
     *  or as the human-readable name, so for a char*, "char*" or "_p_char" will work.
     *  @param hugin_value pointer to object, this general case receives a void * and can make any type
     *  defined in the SWIG interface.
     */
    PyObject* make_hsi_object ( const char* hsi_type ,
                                void* hugin_value );

    /** the constructor is called with the number of arguments
     *  the argument list is meant to contain. Note that you
     *  must add precisely this amount of arguments.
     */
    explicit python_arglist ( int _argc );
    /** destructor, does cleanup */
    ~python_arglist();

public :
    /** @brief add a python object to the argument list. 
     *
     *  This will only succeed if there is still space in the tuple. 
     *  Note that you must have the Python object already to use this method.
     */
    bool add ( PyObject* python_arg );
    /** add a string to the argument list after converting it
     *  to an acceptable Python object. This function can easily
     *  be copied and modified to apply to other data types,
     *  just pick the proper PyXXX_From_YYY Python API function.
     */
    bool add ( const char* str );
    /** returns the generated PyObject
     *  Note that this will only succeed if the argument count is correct.
     */
    PyObject* yield();
};

}; //namespace