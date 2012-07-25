#!/usr/bin/env python

# plugin_skeleton.py - a skeleton for a hugin plugin
# Copyright (C) 2011  Kay F. Jahnke

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# @category Examples
# @name     Skeleton Plugin
# @api-min  2011.1
# @api-max  2012.0

# we use modern type python print statements:

from __future__ import print_function

# the entry routine is called by hugin. This is the workhorse routine
# where you actually do stuff - or don't ;-)
# it receives a panorama object from hugin as it's only parameter.

def entry ( pano ) :

    print ( '********************' )
    print ( 'doing nothing at all with:' )
    print ( '%s' % pano )
    print ( '********************' )

    return 0

# The script determines whether it's been called as a plugin or from
# the command line by looking at the global variable __name__, which
# is set to '__main__' when it's called standalone. Otherwise, the
# entry() routine is called from hugin.

if __name__ == "__main__":

    print ( 'this script only runs as a hugin plugin.' )
