#!/usr/bin/env python

from __future__ import print_function

#    shooting_pattern.py - apply a 6-1-1 shooting pattern to a panorama
#    Copyright (C) 2011  Kay F. Jahnke

#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.

#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.

#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

# this script is an archetypal plugin for hpi, the hugin plugin interface.
# the plugin defines an entry routine, which does the actual work of
# the plugin. this routine will take parameters passed on to it from
# the dispatcher in hpi.py.

# If you modify this script to derive your plugin from it, this is the
# routine you first start tinkering with.

# Notice that you won't see any output from print statements if you
# haven't called hugin from the command line.

# The script will apply a shooting pattern to an eight-image panorama,
# assuming the pattern was six around, 1 up 60 degrees and one down 60.

# the position() subroutine sets yaw, pitch and roll for an image

# If you want to integrate your script into the menu, you will need to
# edit the following self-explanatory lines:

# @category initial distribution
# @name     6-1-1 Shooting Pattern
# @description apply a shooting pattern (6 around, 1 up 1 down)
# @sys win nix mac
# @api-max 2016.1
# @api-min 2011.1

# the position routine will apply yaw, pitch to an image

def position ( pano , number , yaw , pitch , roll ) :
    print ( "%d: y %d p %d r %d" % ( number , yaw , pitch , roll ) )

    # we make sure we only access images that exist.
    images = pano.getNrOfImages()
    if number < 0 or number >= images :
        print ( "no image %d" % number )
    else :
        # fine, let's do it.
        img=pano.getSrcImage(number)
        img.setYaw(yaw)
        img.setPitch(pitch)
        img.setRoll(roll)
        pano.setSrcImage(number,img)

# the entry routine is what is called from hugin.
# notice that it takes an arbitrary number of arguments after the
# first one which must be a panorama.

def entry ( pano ) :

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

# This script won't work standalone.

if __name__ == "__main__":

    print ( 'this script only runs as a hugin plugin.' )
