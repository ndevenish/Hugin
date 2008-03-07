Goal:
-----

simplify the usage of enfuse and enblend command line program on
windows.

This package contains the following batch files:
-------------------------------------------------

The main batch files:
 enfuse_auto_droplet.bat
 enfuse_droplet.bat
 enfuse_droplet_360.bat
 enfuse_align_droplet.bat
 enblend_droplet.bat
 enblend_droplet_360.bat

The helper files:
 collect_data_enfuse.bat
 collect_data_enblend.bat
 unique_filename.bat
 exiftool_enfuse_args.txt
 exiftool_enblend_args.txt

The _360 versions are identical to their same named counterparts
except that in the _360 version the -w parameter is set in order to
work around the 360° boundary. For details on enfuse parameters and
current state of development see http://wiki.panotools.org/Enfuse

The helper batch files are called by the main ones and collect image
data respectively iterate filenames until a non-existent one is found.
The two text files contain parameters for EXIFTool

Copyright:
----------

This description and the batch files are
Copyright (C) 2008 Erik Krause.

They are free software; you can redistribute them and/or modify
them under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Requirements:
-------------

enfuse and enblend must be properly installed (copied to the hard
drive). To get the EXIF copy feature working you need exiftool from
http://www.sno.phy.queensu.ca/~phil/exiftool/
The version with image alignment needs align_image_stack.exe from a
recent hugin package: http://hugin.sourceforge.net/
At least windows 2000 is necessary. Currently tested on windows 2000,
XP and Vista.

Installation:
-------------

Please unzip to the folder where enfuse and enblend reside. Create a
shortcut to either of the main batch files:
 enfuse_auto_droplet.bat
 enfuse_droplet.bat
 enfuse_droplet_360.bat
 enfuse_align_droplet.bat
 enblend_droplet.bat
 enblend_droplet_360.bat
wherever you want (you can even place a shortcut in the SendTo
folder) and give them a speaking name. Please see windows help on how
to create a shortcut.

Usage:
------
enfuse_auto_droplet.bat:

Drag and drop a folder on the droplet. The droplet will sort the
images in the folder by name and get the EV (Exposure Value computed
from exposure time, f-stop and ISO) from the first file it finds. It
will start to enfuse images until it finds an image with the same EV
as the first one. This image will be considered the start of the next
bracketed series. This will continue until all images are processed.
JPG and TIF images are treated in separate groups.

The main droplets except enfuse_auto_droplet.bat:

There are two ways to use the droplets:
1. drag and drop some images on the droplet to enfuse or enblend them.
   It's your responsibility to choose only compatible images. The
   result will be a .tif file named like the image of the series you
   last clicked on for dragging (possibly marked with a dotted frame)
   plus _enfused respectively enblended added to the file name.
2. drag and drop a folder on the droplet. You will be asked to specify
   how many images are in each series or whether you want to process
   all images in the folder. Only files with extension .jpg and
   .tif are processed by enfuse, only .tif files by enblend.
   The result will be .tif files named like the first image of each
   series plus _fusioned added to the file name.


Customisation:
--------------

You can create more versions of the batch file by copying to a
different name and changing the additional parameters. For
enfusing a focus stack f.e. use
 --wExposure=0 --wSaturation=0 --wContrast=1 --HardMask

If you want to move the result images to a different location, add a
line
 move "*_enfused.tif" "<Your Path>"
to the very end of the file. Of course replace <YourPath> with the
path you want the images moved to.

Bug reporting:
--------------

Please report all bugs and malfunctions to erik.krause@gmx.de or to
the hugin-ptx mailing list: http://groups.google.com/group/hugin-ptx
Thank you for helping to improve the scripts!

Version history:
----------------

v 0.1   - 05. Jan 2008, initial version
v 0.2   - 06. Jan 2008, fixed some bugs related to blanks in path
                        names.
v 0.2.1 - 15. Jan 2008, updated readme, minor beautifications
v 0.3   - 18. Jan 2008, implemented EXIF data copying
v 0.3.1 - 19. Jan 2008, enblend droplet, updated readme, minor
                        beautifications
v 0.3.2 - 25. Jan 2008, added pause if either executable returns an
                        error, shortcut necessary mentioned in readme
v 0.3.5 - 27. Jan 2008, omit own result images, don't overwrite result
                        images (count up), process .tif and .jpg
                        separately (folders processing only)
v 0.3.6 - 2. Feb 2008,  don't overwrite result images (count up) in
                        drop images mode.
v 0.4.0 - 4. Feb 2008,  added enfuse_auto_droplet.bat.
v 0.4.1 - 7. Mar 2008,  added ICC profile copy to enfuse_align_droplet.bat
                        (align_image_stack does not preserve ICC),
                        set enfuse default parameters to --wContrast=0 due to 
                        enfuse bug.

best regards
Erik Krause





