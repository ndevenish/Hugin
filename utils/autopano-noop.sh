#!/bin/sh

# Note for packagers: to configure hugin to use this notice instead
# of autopano-complete.sh or autopano-c-complete.sh, simply perform
# this substitution before compilation:
# 
# sed -i 's/"autopano-complete.sh"/"autopano-noop.sh"/' \
#    src/hugin1/hugin/config_defaults.h
# 
# ..and place this script somewhere in the $PATH.

echo "
 READ THIS -- READ THIS -- READ THIS -- READ THIS -- READ THIS -- READ THIS

 If you see this message then your version of hugin has been
 configured without support for automatic generation of control
 points.

 Probably your system administrator or Linux distribution did this
 because the SIFT algorithm used by autopano-sift and autopano-sift-C
 is encumbered by software patents in the United States of America.

 If this is in error and you do have access to one of these tools,
 then you can reconfigure hugin in the Preferences menu.

 Otherwise don't panic.  Hugin is still very usable with control
 points set manually in the 'Control Points' tab, see the tutorials
 on hugin.sourceforge.net for more details.
"

sleep 120
