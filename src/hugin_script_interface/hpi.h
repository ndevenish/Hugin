/** @file hpi.h
 *
 *  @brief header to include hpi capability in hpi-using code
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 *  KFJ 2011-01-18
 *
 */

#if defined _WIN32 && defined Hugin_shared
#ifdef hugin_python_interface_EXPORTS
#define hsi_IMPEX __declspec(dllexport)
#else
#define hsi_IMPEX __declspec(dllimport)
#endif
#else
#define hsi_IMPEX
#endif

namespace hpi
{
/**  simplified call interface to the Python Plugin facility.
 *   @param plugin_name the name of the plugin. This must name a python file which is in PYTHONPATH, 
 *       without path and extension. 
 *   @param argc the number of arguments to the plugin, not the number of argument to this function.
 *
 *   for each argument, two more values has to be given:
 *   - a string containing the name of the type (this may need
 *     to be fully qualified, like 'HuginBase::Panorama*'
 *   - a void* to the actual hugin object
 */
hsi_IMPEX extern int callhpi ( const char* plugin_name ,
                               int argc ,
                               ... ) ;

}