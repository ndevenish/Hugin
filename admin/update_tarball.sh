#!/bin/sh
# download the developer cvs and put it on the website.
# change for your sf account..

SF_USER=dangelo

CVS_RSH=ssh

cdate=`date +"%Y_%m_%d-%H_%M"`

cd /tmp
if [ -d hugin ];
then
     echo "removing old checkout"
     rm -rf hugin;
fi
cvs -d:ext:${SF_USER}@cvs.sourceforge.net:/cvsroot/hugin export -r HEAD hugin
fname=hugin_${cdate}.tgz
tar cvzf $fname hugin
echo "copying to sf site"
scp $fname ${SF_USER}@shell1.sf.net:/home/groups/h/hu/hugin/htdocs/snapshots/

