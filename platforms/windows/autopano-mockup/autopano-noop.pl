#!/usr/bin/perl
# Note for packagers: to configure hugin to use this notice instead
# of autopano-complete.sh or autopano-c-complete.sh, simply perform
# this substitution before compilation:
# 
# sed -i 's/"autopano-sift-c"/"autopano-noop.sh"/' \
#    src/hugin1/hugin/config_defaults.h
# 
# ..and place this script somewhere in the $PATH.


print STDERR "READ THIS -- READ THIS -- READ THIS -- READ THIS -- READ THIS -- READ THIS\n";
print STDERR "\n";
print STDERR " If you see this message then your version of hugin has been\n";
print STDERR " configured withERR support for automatic generation of control\n";
print STDERR " points.\n";
print STDERR "\n";
print STDERR " This is probably because the SIFT algorithm used by autopano-sift\n";
print STDERR " and autopano-sift-C is encumbered by software patents in the\n";
print STDERR " United States of America.\n";
print STDERR "\n";
print STDERR " If this is in error and you do have access to one of these tools,\n";
print STDERR " then you can reconfigure hugin in the Preferences menu.\n";
print STDERR "\n";
print STDERR " Otherwise don't panic.  Hugin is still very usable with control\n";
print STDERR " points set manually in the 'Control Points' tab, see the tutorials\n";
print STDERR " on hugin.sourceforge.net for more details.\n";

sleep 120;

