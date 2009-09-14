#!/bin/sh

# Tivial wrapper script for autopano-sift-c

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public
# License along with this software; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

if which 'autopano-sift-c' 2>/dev/null >/dev/null
then
  'autopano-sift-c' "$@"
else

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

fi
