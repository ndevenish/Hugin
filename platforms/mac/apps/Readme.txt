Enfuse_droplet 0.02
Harry van der Wolf
15 Mar 2008

This little application works in two ways.

1. Just double-click it and it will ask for the images to be enfused.
2. Drop images onto it and it will start fusing them.
note: adding a folder (with images) or dropping a folder (with images) onto the application does not work. 

Prerequisites:
- The Enfuse_droplet.app needs to be in the same directory as enfuse and align_image_stack. You can create an alias and
  drag that alias to any place you like. 
  The "most comfortable way" to use it is to drag it into the left pane of your Finder where other useful
  aliases (for your home directory, Applications, Documents, Pictures and so on) are also located. This is the easiest 
  way to drop images onto it. 
- The "BP Progress Bar.app" needs to be in the same directory as the EnfuseWrapper.app.
- Previous versions were based on Hugin.app. This one is obviously based on this enblend/enfuse/aligimagestack package.


Installation:
- Just copy everything inside this bundle into the same directory.
- You can You can drag the Enfuse_droplet.app in the left pane of your finder or optionally create one or more "aliases" 
  on the desktop or any place of your liking.

Changelog:
version 0.0.2 2008-03-15
- Renamed from Enfuse_wrapper to Enfuse_droplet to stay in line with Erik Krause.
- changed from application depending on Hugin to application using standalone enfuse, 
  enblend, alignimagestack.
- when no file extension is given for the new fused image automatically the .tif extension
  will be added.
- when no filename is given it will automatically be named to untitled.tif.
- Added "Show parameters" dialogs (like about function) to alignimagestack and enfuse parameter popups.


TODO:
- Make it possible to add a folder or drop a folder. At this moment align_image_stack crashes on it (is it my script or is it align_image_stack?)
- Make an Enfuse_Auto.app which uses an ini file for settings and will only ask for a filename to be created.
- Create some alternative forms like Erik Krause has done with his droplets.
- Add source and App (only 80 Kb) to the Hugin SVN (platforms/mac).


