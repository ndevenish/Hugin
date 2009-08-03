BUILDING A UNIVERSAL HUGIN BUNDLE WITH SCRIPTS

***
This is in alpha stage.
***

INTRODUCTION
Until now the way to build a Universal Hugin bundle was to do it via Xcode.
Recently Ippei Ukai, the creator of the XCode project, updated the XCode project
with functionality that is not supported on tiger (at least it seams like that).
Leopard uses XCode 3.0, whereas Tiger uses 2.5 (or 2.4.1). Apple said that
as long as you did not use the extra functionality of 3.0, the XCode projects
are backwards compatible. This is not true however for the Hugin project.
That's why I built a scripted version.

PREREQUISITES
1. You need to have XCode installed
2. You need to have cmake installed
3. You need to have subversion (SVN) installed
4. You need to have built the dependent libraries and binaries
   with the scripts in the <hugin_trunk>/mac/ExternalPrograms directory.
5. You need to have a correct SetEnv-universal.txt available (which is a
   prerequisite for 4 anyway) and you need to have a configured 
   bundle_constants.txt (in this directory).

HOWTO BUILD A UNIVERSAL HUGIN BUNDLE 
1. Build all libraries and binaries from <hugin_trunk>/mac/ExternalPrograms.

2. Compile Hugin
   cd into the hugin main source directory. From that directory you need to
   run the script:  ./mac/scripted_universal_build/01-Compile_hugin.sh
   Wait till it's finished (and that will take quite some time).

3. Build Universal portable bundle
   Cd into the mac/scripted_universal_build directory and issue the 
   command:  ./02-Make_bundle.sh

4. Add localisation and help to the bundle.
   From the mac/scripted_universal_build directory issue the 
   command: ./03-localise-bundle.sh
   
   After this step you have a fully portable Hugin bundle.

5. Make archive for upload
   From the mac/scripted_universal_build directory issue the 
   command: ./04-create_dmg_tgz.sh

This will finish it up. Share your bundle with the community.

   
