# this script is an archetypal plugin for hpi, the hugin plugin interface.
# the plugin defines an entry routine, which does the actual work of
# the plugin. this routine will take parameters passed on to it from
# the dispatcher in hpi.py.
# If you modify this script to derive your plugin from it, this is the
# routine you first start tinkering with.
# Notice that you won't see any output from print statements if you
# haven't called hugin from the command line.

# the position() subroutine sets yaw, pitch and roll for an image

def position ( pano , number , yaw , pitch , roll ) :
    print ( "%d: y %d p %d r %d" % ( number , yaw , pitch , roll ) )

    # we make sure we only access images that exist.
    images = pano.getNrOfImages()
    if number < 0 or number >= images :
        print ( "no image %d" % number )
    else :
        # fine, let's do it.
        img=pano.getImage(number)
        img.setYaw(yaw)
        img.setPitch(pitch)
        img.setRoll(roll)
        pano.setImage(number,img)

# the entry routine is what is called from the dispatcher in hpi.py
# notice that it takes an arbitrary number of arguments after the
# first one which must be a panorama.

def entry ( pano , *args ) :

    # this plugin doesn't take any extra arguments

    if args :
        print ( 'ignoring extra arguments %s' % str(args) )

    # for the purpose of this demonstration, it's assumed that
    # we have 8 images loaded: 6 around, 1 up 60 degrees, 1 down 60.
    # If there are less images in the panorama, the excessive calls
    # to 'position' have no effect. In 'real' life the plugin would
    # pass back an error message to be displayed by the GUI, but
    # this isn't yet implemented.

    position ( pano , 0 , 0 , 0 , 0 )
    position ( pano , 1 , 60 , 0 , 0 )
    position ( pano , 2 , 120 , 0 , 0 )
    position ( pano , 3 , 180 , 0 , 0 )
    position ( pano , 4 , 240 , 0 , 0 )
    position ( pano , 5 , 300 , 0 , 0 )
    position ( pano , 6 , 0 , 60 , 0 )
    position ( pano , 7 , 0 , -60 , 0 )

    return 0

