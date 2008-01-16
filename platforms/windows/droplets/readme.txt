Goal:
-----

simplify the usage of enfuse command line program on windows.

This package contrains the following batch files:
-------------------------------------------------

enfuse_droplet.bat 
enfuse_droplet_360.bat

Both versions are identical except that in the _360 version the -w
parameter is set in order to fusion around the 360° boundary. For
details on parameters and current state of development see
http://wiki.panotools.org/Enfuse

Copyright:
----------

This descrition and the batch files are
Copyright (C) 2008 Erik Krause.

They are free software; you can redistribute them and/or modify
them under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Requirements:
-------------

enfuse must be properly installed (copied to the hard drive).
At least windows 2000 necessary. Currently tested on windows 2000, XP
and Vista.

Installation:
-------------

Please unzip to the folder where enfuse resides. Create a shortcut to
one or both of the batch files wherever you want (you can even place a
shortcut in the SendTo folder). Please see windows help on how to
create a shortcut.

Usage:
------

There are two ways to use the droplets:
1. drag and drop some images (a bracketed series) on the droplet to
   fusion them. It's your responsibility to choose only compatible
   images.
   The result will be a .tif file named like the image of the series
   you last clicked on (for dragging - possibly marked with a dotted
   frame) plus _fusioned added to the file name.
2. drag and drop a folder on the droplet. You will be asked to specify
   how many images are in each bracketed series or whether you want to
   fusion all images in the folder. Only files with extension .jpg and
   .tif are processed.
   The result will be .tif files named like the first image of each
   series plus _fusioned added to the file name.

Customisation:
--------------

You can create more versions of the batch file by copying to a
different name and changing the additional parameters in line 4. For
enfusing a focus stack f.e. use
 --wExposure=0 __wSaturation=0 --wContrast=1 --HardMask

If you want to move the result images to a different location, add a
line
 move "*_fusioned.tif" "<Your Path>"
to the very end of the file. Of course replace <YourPath> with the
path you want the images moved to.

Bug reporting:
--------------

Please report all bugs and malfunctions to erik.krause@gmx.de. Thank
you for helping to improve the scripts!

Version history:
----------------

v 0.1 - 05. Jan 2008 Initial version

v 0.2 - 06. Jan 2008 Fixed some bugs related to blanks in path names.
v 0.2.1 - 15. Jan 2008 Updated readme, minor beautifications

best regards
Erik Krause

