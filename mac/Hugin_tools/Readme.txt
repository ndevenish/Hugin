Readme.txt for Hugin_tools package
version 0.2, 11 January 2010, Harry van der Wolf


== How to "install" this package ==
- Copy the Hugin_tools directory inside this dmg to a location of your liking.


== How to use the tools inside the Hugin_tools directory ==
1. Open a terminal
2. cd into the Hugin_tools directory
3. issue the command "source ./set_environment.txt"   (without the double quotes)

The tools reside inside the bin directory but are system wide available due to the command
issued in (3.).The bin directory inside this Hugin_tools directory is set as first entry 
in your PATH statement. It means that you can execute the tools without needing to use paths. 
Note: This addition is not a permanent change but only in this terminal session.

Note2: due to license restrictions autopano-sift-c and panomatic are not part of this package.

===================================================================================================================
License Agreements

The programs included in this package are released under the GNU General Public License version 2. 
If you do not agree with the GPL vs. 2 license agreement, please delete this package and it's contents immediately.

This package is completely based on the Hugin project and the tools it contains. Hugin can be downloaded from http://hugin.sourceforge.net. 
Next to the Hugin tools also enblend, enfuse and the PT* binaries from panotools are included.

