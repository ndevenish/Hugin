import hsi
import os

# the dispatcher is curently not very sophisticated. All it does
# is take the string passed to it in the plugin parameter and
# tries to load it as a module, assuming it contains a plugin.

def hpi_dispatch ( plugin_name , *args ) :
    print("dispatcher: use plugin %s" % plugin_name)
    # deposit arguments in global namespace
    plugin_args = args
    try :
        if os.path.exists ( plugin_name ) :
            # there is a file by that name. let's execute it
            print("dispatcher found file %s" % plugin_name)
            exec(compile(open( plugin_name ).read(), plugin_name, 'exec'))
            success = 0
        else :
	    # no such file. could be a module
            print("no file %s, trying module import" % plugin_name)
            plugin = __import__ ( plugin_name )
            success = plugin.entry ( *args )
    except ImportError :
        print("%s: import failed" % plugin_name)
        success = -2
    except :
        print("%s: call failed" % plugin_name)
        success = -1
    return success

# the remainder is a tiny unit test :-)

if __name__ == "__main__":

    import sys
    if len ( sys.argv ) < 2 :
        print("use: hpi panorama.pto")
        sys.exit()
    p = hsi.pano_open ( sys.argv[1] )
    hpi_dispatch ( 'pano_print' , p )
