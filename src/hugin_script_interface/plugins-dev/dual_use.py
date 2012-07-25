#!/usr/bin/env python

from __future__ import print_function

gpl = r"""
    dual_use.py - a skeleton for a plugin that can also run standalone
    
    Copyright (C) 2011  Kay F. Jahnke

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

# @category Examples
# @name     Dual Use Plugin
# @api-min  2011.1
# @api-max  2012.0

# dual_use will function as a hugin plugin and as a standalone
# Python script.

# first is the entry routine which is called when this script is used
# as a plugin from hugin. This is the workhorse routine where you
# actually do stuff - or don't ;-)

def entry ( pano ) :

    print ( '********************' )
    print ( 'doing nothing at all with:' )
    print ( '%s' % pano )
    print ( '********************' )

    return 0

# the standalone routine is what's called if this script has been called
# from the command line. In this case we do the CLI thing. In this demo
# we expect a panorama as the only parameter which we open and pass to
# the workhorse routine. No checking is done.

def standalone() :

    import sys                      # to look at the argument vector
    import hsi                      # to use hugin's scripting interface

    if len ( sys.argv ) < 2 :
        print ( 'usage: %s <pto file>' % sys.argv[0] )
        sys.exit ( -1 )
  
    panofile = sys.argv[1]          # the only parameter: a pto file
    ifs = hsi.ifstream ( panofile ) # create a C++ ifstream from it
    pano = hsi.Panorama()           # and a Panorama object
    success = pano.readData ( ifs ) # feed the ifstream to the pano object
    
    # check if all went well
    if success != hsi.DocumentData.SUCCESSFUL :
        # if it failed, complain
        print ( 'input file %s contains invalid data' % panofile )
        success = -1
    else :
        # if it succeeded, call the workhorse
        entry ( pano )
        success = 0

    # and that's it
    return success

# The script determines whether it's been called as a plugin or from
# the command line by looking at the global variable __name__, which
# is set to '__main__' when it's called standalone. Otherwise, the
# entry() routine is called from hugin.

if __name__ == "__main__":

    standalone()
