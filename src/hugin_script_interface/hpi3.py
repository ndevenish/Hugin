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
        print("%s: import failed" % plugin_name)
        success = -2
    except :
        print("%s: plugin failed" % plugin_name)
        success = -1
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
        sys.exit ( -5 )

    # okay, there are some arguments, let's go ahead and try
    # opening the first as a panorama.
    
    p = hsi.Panorama()
    ifs = hsi.ifstream ( sys.argv[1] )
    if not ifs.good() :
        print("failed to open %s" % sys.argv[1])
        sys.exit ( -6 )
    
    if p.readData ( ifs ) != hsi.DocumentData.SUCCESSFUL :
        print("failed to read panorama data from %s" % sys.argv[1])
        sys.exit ( -7 )

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
            sys.exit ( -8 )
    
        if p.writeData ( ofs ) != hsi.DocumentData.SUCCESSFUL :
            print("failed to write panorama data to %s" % sys.argv[1])
            sys.exit ( -9 )

        del ofs
    
    sys.exit ( success )
