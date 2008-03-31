========================
 Read Me - HuginOSX 0.6
========================

HuginOSX (hugin on Mac OS X)
http://hugin.sourceforge.net/



License Agreements

The programs included in this package are mainly released under GPL although some of them may contain different licenses. Please refer to the License and Readme files attached to each program. If you do not agree with any of those license agreements, please delete the relevant program immediately. The license of HuginOSX follows that of hugin on the other platforms, which is found in the "readme and license" folder.



Requirements

Mac OS X 10.3.9 or above, either on PPC or Intel machine

(Users with Intel CPU may still experiense minor problems. We appreciate your report. If there is a serious problem, you can try running HuginOSX with Rosetta to work around: http://docs.info.apple.com/article.html?artnum=303120 )



Typical Installation Procedure

1. Read through the read-me and license files.

2. Copy HuginOSX.app to /Applications folder or other locations of your preference.

3. (Optional) Install Autopano-sift:
  - Download and install Mono Framework (http://mono-project.com/)
  - Download Autopano-sift for Mono environment (http://user.cs.tu-berlin.de/~nowozin/autopano-sift/)
  - Launch PutAutopanoSiftToHuginOSX
  - Select "bin" folder in the autopano-sift package, and then HuginOSX.app that you have just installed



Usage

Basic usage of HuginOSX is not different from that of other platforms. Some of helpful resources are found in the "Help" menu, and you can find more on the web. The basic steps are:
1 add images to the project, 
2 set the field of view of your lens (in degrees) etc.,
3 add control points, 
4 set the anchor point, which becomes the centre point of the resulting photo 
5 execute optimiser that puts the images in the best place according to the control points, and 
6 stitch the images together.
There are many tutorials on the web, which might assist you to get started with hugin and the panorama photography.



External commnad-line tools that can be used from inside HuginOSX

- enblend  by Andrew Mihal, http://enblend.sourceforge.net/
This command-line program blends your panorama nicely. The program is already integrated into HuginOSX and no custom installation is required. For a reference, you can read its readme file in the "readme and licence" folder, however using enblend from command line may require separate installation of enblend tool.

- autopano-sift  by Sebastian Nowozin, http://user.cs.tu-berlin.de/~nowozin/autopano-sift/
This is a set of .Net programs that can automatically add control points. We are not including this software in this package. Please read the tutorial in tools/autopano-sift/ folder in order to set up this software for your Mac and hugin. 

PTStitcher from the original Panorama Tools by Helmut Dersch and autopano by Alexandre Jenny have no source available to the public, and there are no versions available for OSX.




Free tools for doing panorama on Mac

Command-line tools whose Mac binaries are available from hugin project
- nona  The stitching engine of hugin. For use with other tools like clens.
- mergepto  Merges two hugin project files.
- clens  from Panorama Tools project, http://panotools.sourceforge.net/ 
- enblend  by Andrew Mihal, http://enblend.sourceforge.net/

Other recommended free tools
- PanoGLViewerOSX  from hugin Project, http://sourceforge.net/projects/hugin/download/
- autopano-sift  by Sebastian Nowozin, http://user.cs.tu-berlin.de/~nowozin/autopano-sift/
- MakeCubic  from Apple Computer, http://developer.apple.com/quicktime/quicktimeintro/tools/index.html#qtvr
- XBlend  from Kekus Digital, http://www.kekus.com/xblend/
- PangeaVR Plugin  from Pangea Software, http://www.pangeasoft.net/pano/plugin/
- PanoView (Japanese Only)  by Yoshiaki Katayanagi, http://www.jizoh.jp/pages/download.html



Known Problems 

- autopano-sift sometimes does not work when nested inside the application package.



Contacts

- See the homepage (http://hugin.sourceforge.net/) first.
- Visit the project page on SourceForge (http://sourceforge.net/projects/hugin) if you are interested in the developement.
- The mainling list for everyone who cares about hugin (http://groups.google.com/group/hugin-ptx) would welcome any questions and/or help from you.
- Recent OSX porting is mainly proceeded by Ippei UKAI (ippei_ukai@mac.com). He's on the above mailing list as well, so using the mailing list should work fine. Note he's a university student and probably busy during term times.



This document was originally written by Ippei UKAI (Copyleft 2006)
Last Modified: $Id: ReadMe-HuginOSX.txt 1719 2006-08-17 20:16:28Z ippei $
