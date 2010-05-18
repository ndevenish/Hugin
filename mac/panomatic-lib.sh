# First export a path. On some systems the environment is not correctly set/used
export PATH=/opt/local/bin:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/sw/bin

if [ -d "../src/panomatic-lib" ]
then
 cd ../src/panomatic-lib
 bzr merge
else
 cd ../src
 bzr branch lp:~pablo.dangelo/hugin/panomatic-lib
fi

