Read Me - HuginOSX

-- THIS IS A RELEASE CANDIDATE --

HuginOSX
http://hugin.sourceforge.net/
http://homepage.mac.com/ippei_ukai/software/


License Agreements

The programs included in this package are mainly released under GPL although some of them contain different licenses. Please refer to the License and Readme files attached to each program. If you do not agree with any of those license agreements, please delete the relevant program immediately. HuginOSX's license follows that of hugin on the other platforms, which is found in "readme and license" folder.


Usage

Basic usage of HuginOSX is not different from that of other platforms. Some of helpful resources are found in the "Help" menu, and you can find more on the web. The basic steps are: 1.add images to the project, 2.set the degrees of view etc of your lens, 3.add control points, 4.set the anchor point, which becomes the centre point of the resulting photo 5.execute optimiser that puts the images in the best place according to the control points, 6.stitch the images together.


External commnad-line tools that can be used from inside hugin

- enblend  by Andrew Mihal, http://enblend.sourceforge.net/
This command-line program blends your panorama nicely. The binary file compiled for Mac is included together with its readme and license files in tools/enblend/. In order to use this program from within hugin, please turn on the "soft blend" option in Stitcher panel. When you use this option for the first time, you will be asked to specify where enblend is on your hard disk. You can also specify the path in the Preference. Alternatively, you can copy the binary file into HuginOSX's application bundle using attached PutEnblendToHuginOSX program. This allows you to put HuginOSX.app into Applications folder and delete other files. Please read the attached licence and readme files before use.

- autopano-sift  by Sebastian Nowozin, http://user.cs.tu-berlin.de/~nowozin/autopano-sift/
This is a set of .Net programs that can automatically add control points. We are not including this software in this package. Please read the tutorial in tools/autopano-sift/ folder in order to set up this software for your Mac and hugin.

(PTStitcher from original Panorama Tools and autopano by Alexandre Jenny have no source available for public, and currently do not have MacOSX versions.)


Free tools for doing panorama on Mac

From hugin project and included in this package
- nona, nona_gui  Open source alternative to PTStitcher. hugin contains nona's functionality within itself and those executables are for use with other tools like clens.
- PanoGLViewerOSX  Displays 360ºx180º rectilinear panorama image using OpenGL. This program was originally written by Fabian Wenzel <f.wenzel@gmx.net>
- mergepto  Merges two hugin project files.

From other projects but whose binary file is distributed together
- clens  from Panorama Tools project, http://panotools.sourceforge.net/ 
- enblend  by Andrew Mihal, http://enblend.sourceforge.net/
(Please read the attached licence/readme files respectively before use.)

Other recommended free tools
- autopano-sift  by Sebastian Nowozin, http://user.cs.tu-berlin.de/~nowozin/autopano-sift/
- MakeCubic  from Apple Computer, http://developer.apple.com/quicktime/quicktimeintro/tools/index.html#qtvr
- XBlend  from Kekus Digital, http://www.kekus.com/xblend/
- PangeaVR Plugin  from Pangea Software, http://www.pangeasoft.net/pano/plugin/


Known Problems 

- Optimizer on HuginOSX does not work on certain environment. We are trying to determine the cause. If you encounter this, please send me 1. where HuginOSX is installed, 2. where your project file is located, and 3. result of "ls -l Applications/HuginOSX.app/Contents/*". This bug seems to have something to do with paths with space or non-ASCII characters. You may be able to work around if you put HuginOSX and your project file in to a 'safe' locations like "/Applications" or "~/".

- autopano-sift sometimes does not work when nested inside the application package.


TODO (Any help appreciated.)

- fix Optimiser bug
- find bugs. 


Contacts

- See the homepage (http://hugin.sourceforge.net/) first.
- And SourceForge project page (http://sourceforge.net/projects/hugin) if you are interested in the developement.
- The mainling list for everyone who cares about hugin (http://www.email-lists.org/mailman/listinfo/ptx) would welcome any questions and/or helps from you.
- Recent OSX porting is mainly proceeded by Ippei UKAI (ippei_ukai@mac.com). He's on the above mailing list as well, so using the mailing list should work fine. Note he's a university student and probably busy during the term time.


Last modified for HuginOSX 0.5rc2
on: 2005-09-22
by: Ippei UKAI (ippei_ukai@mac.com)

This document was originally created by Ippei UKAI (Copyleft 2005)