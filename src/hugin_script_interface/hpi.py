#! /usr/bin/env python

from __future__ import print_function

# hpi.py - dispatcher for hugin plugins
#    
# Copyright (C) 2011  Kay F. Jahnke
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os

# the dispatcher is currently not very sophisticated. All it does
# is take the string passed to it in the 'plugin' parameter and
# tries to find a file of that name and execute it. The file is
# expected to define a function 'entry'.
# If loading succeeds, an attempt is made to call the function
# entry() with the parameters passed to the dispatcher after
# the plugin name; currently this is always a pointer to the
# current panorama and nothing else.

def hpi_dispatch ( plugin_name , *args ) :
    print("hpi: accessing plugin file %s" % plugin_name)
    try :
        if os.path.exists ( plugin_name ) :
            # there is a file by that name. let's execute it
            # we make an empty dictionary as globals for the plugin
            plugin = dict()
            # and execute the plugin there
            exec(compile(open( plugin_name ).read(), plugin_name, 'exec'), plugin)
            # the plugin should have put a function entry()
            # into it's global namespace, we call it
            success = plugin [ 'entry' ] ( *args )
            # then we get rid of the dictionary again to free
            # all resources the plugin may have anchored there
            print ( 'hpi: plugin terminated with exit code %d' % success )
            del plugin
        else :
            # KFJ 2011-06-16 loading plugins as modules is no
            # longer supported (wasn't used anyway)
            # missing plugin file is an error:
            print("hpi: can't find plugin file")
            success = -10

    # KFJ 2011-06-19 added better exception handling
    # we catch any exception which may have occured in the plugin:
    
    except Exception:

        # to deal with the exception, we import two more modules:

        import sys
        import traceback
        
        # first we retrieve the details of the exception:

        exc_type, exc_value, exc_traceback = sys.exc_info()

        # emit a message with the type and text of the exception:
        
        print('hpi: plugin %s failed with %s: "%s"'
              % (os.path.basename(plugin_name),
                 exc_type.__name__,
                 exc_value) )

        # and, for good measure, a stack traceback to point
        # to the precise location and call history of the error:
        
        print ( 'hpi: stack traceback:' )
        traceback.print_tb ( exc_traceback )

        success = -11

    print ( 'hpi: terminating with exit code %d' % success )
    return success

# The remainder of the file is not needed for calling from hugin.
# To allow testing of plugins without starting hugin, hpi.py
# can be run independently. In that case it takes at least
# two parameters: the panorama to open and the plugin to run.
# Additional parameters are passed on to the plugin.
# This is no more than an ad-hoc solution for testing; if
# behaviour like this is considered permanently useful, the
# mechanism should be polished some.

if __name__ == "__main__":

    import sys
    import hsi

    # hpi.py has been called standalone rather than having been
    # mported as a module. In this case we want to look at the
    # argument vector:

    if len ( sys.argv ) < 3 :
        print("use: %s panorama.pto plugin.py [plugin args]"
              % sys.argv[0])
        sys.exit ( -12 )

    # okay, there are some arguments, let's go ahead and try
    # opening the first one as a panorama.
    
    p = hsi.Panorama()
    ifs = hsi.ifstream ( sys.argv[1] )
    if not ifs.good() :
        print("failed to open %s" % sys.argv[1])
        sys.exit ( -13 )

    # next, make the panorama object read the file

    if p.readData ( ifs ) != hsi.DocumentData.SUCCESSFUL :
        print("failed to read panorama data from %s" % sys.argv[1])
        sys.exit ( -14 )

    del ifs

    # if we're here, the load succeeded and we can finally call
    # the plugin
    
    success = hpi_dispatch ( sys.argv[2] , p , *(sys.argv[3:]) )

    # KFJ 2011-06-19 changed writeback to True, so that hpi.py
    # can be used to execute hugin plugins on the command line
    # and the result is written back to the pto file.

    writeback = True

    if writeback :

        ofs = hsi.ofstream ( sys.argv[1] )

        if not ofs.good() :
            print("failed to open %s" % sys.argv[1])
            sys.exit ( -15 )
    
        if p.writeData ( ofs ) != hsi.DocumentData.SUCCESSFUL :
            print("failed to write panorama data to %s" % sys.argv[1])
            sys.exit ( -16 )

        del ofs
    
    sys.exit ( success )
