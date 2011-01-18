# this script is an archetypal plugin for hpi, the hugin plugin interface.
# It demonstrates the good practise of allowing to run the plugin from
# the command line or as a plugin - this makes it much easier to go
# through the modify-test-cycle.

# the plugin defines an entry routine, which does the actual work of
# the plugin. this routine will take parameters passed from hugin or
# their equivalent derived from command line arguments
# If you modify this script to derive your plugin from it, this is the
# routine you first start tinkering with.
# Notice that you won't see any effect if you haven't called hugin
# from the command line, since all the script does is print out how
# many images are in the panorama.

def entry ( pano ) :

    # EAFP (Easier to Ask Forgiveness than Permission)
    # so we'll just go ahead and try access the pano
    # if anything's wrong, we'll catch the exception.

    images = pano.getNrOfImages()
    print "found %d images in panorama" % images

# now the top level of the script. It checks out if it's been called
# from the command line or not and acts accordingly:

try :

    if __name__ == "__main__" : # called from the command line

        import sys
    
        if len ( sys.argv ) < 2 :
            print "use: %s <pto file>"
        else :
	    # if we're on our own, we need to import hsi
            import hsi
            # now we can open the pano and call entry()
            pano = hsi.pano_open ( sys.argv[1] )
            entry ( pano )

    else : # we've been called as a plugin, so call entry()

        entry ( *plugin_args  )

except :

    # for now, our exception handling is quite unsophisticated...
    print 'ooops...'
    
