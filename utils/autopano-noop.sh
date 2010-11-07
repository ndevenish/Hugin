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

 If you see this message then you have upgraded from an earlier
 version of Hugin and have no Control Point Detector configured.

 Please open the Preferences window and Load Defaults for the
 Control Point Detectors setting.

 This will enable the new built-in Control Point Detector and
 you won't see this message again.
"

sleep 120

fi

exit 0
