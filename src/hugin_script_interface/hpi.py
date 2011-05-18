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

from __future__ import print_function

import hsi
import os

# the dispatcher is currently not very sophisticated. All it does
# is take the string passed to it in the 'plugin' parameter and
# tries either to find a file of that name and execute it or
# to find a module of that name and import it. The file/module
# is expected to define a function 'entry'.
# If loading succeeds, an attempt is made to call the function
# entry() with the parameters passed to the dispatcher after
# the plugin name

def hpi_dispatch ( plugin_name , *args ) :
    print("dispatcher: loading plugin %s" % plugin_name)
    try :
        if os.path.exists ( plugin_name ) :
            # there is a file by that name. let's execute it
            print("dispatcher: found file %s" % plugin_name)
            # we make an empty dictionary as globals for the plugin
            plugin = dict()
            # and execute the plugin there
            exec(compile(open( plugin_name ).read(), plugin_name, 'exec'), plugin)
            # the plugin should have put a function entry()
            # into it's global namespace, we call it
            success = plugin [ 'entry' ] ( *args )
            # then we get rid of the dictionary again to free
            # all resources the plugin may have anchored there
            del plugin
        else :
	    # no such file. could be a module.
            print("dispatcher: no file %s, trying import" % plugin_name)
            plugin = __import__ ( plugin_name )
            success = plugin.entry ( *args )
            del plugin
    except ImportError :
        print("%s: import of plugin failed" % plugin_name)
        success = -10
    except :
        print("%s: plugin failed with an exception" % plugin_name)
        success = -11
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

    # this means hpi.py has been called standalone rather than
    # having been imported as a module. In this case we want to
    # look at the argument vector:
    
    import sys
    if len ( sys.argv ) < 3 :
        print("use: hpi panorama.pto plugin.py [plugin args]")
        sys.exit ( -12 )

    # okay, there are some arguments, let's go ahead and try
    # opening the first as a panorama.
    
    p = hsi.Panorama()
    ifs = hsi.ifstream ( sys.argv[1] )
    if not ifs.good() :
        print("failed to open %s" % sys.argv[1])
        sys.exit ( -13 )
    
    if p.readData ( ifs ) != hsi.DocumentData.SUCCESSFUL :
        print("failed to read panorama data from %s" % sys.argv[1])
        sys.exit ( -14 )

    del ifs

    # if we're here, the load succeeded and we can finally call
    # the plugin
    
    success = hpi_dispatch ( sys.argv[2] , p , *(sys.argv[3:]) )

    # currently the data aren't written back, but you could do
    # so by setting writeback to True:

    writeback = False

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
