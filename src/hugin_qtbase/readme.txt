hugin_qtbase overview

 This section currently contains three groups of files for ongoing GUI development in Hugin Project using Qt toolkit.

 Please note you must try your best not to put any complicated data processing in the GUI code. You should take a look at hugin_base/algorithm if that is concerned with Panorama. If what you want to do really requires Qt, then you may start to consider adding it here.


hugin_qtbase/whitepaper/
 This directory is for the documentation about the framework designs and development roadmaps.

hugin_qtbase/qtappbase/
 This is the general document-base application framework. The framework is based around the AppBase classes in hugin_base.

hugin_qtbase/
 These are GUI framework more specific to Hugin. We take modular approach to the controller classes in order to avoid having one controller with everything in it. The GUI layout is designed to be flexible as well.
