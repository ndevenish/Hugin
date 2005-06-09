#!/bin/bash

# by Pablo d'Angelo <pablo.dangelo@web.de>
# modified by Ippei UKAI <ippei_ukai@mac.com> for HuginOSX

set -e

myexit()
{
    sleep 10
    exit 1
}

MONO=$(which mono)
if [ "$MONO" = "" -o ! -x "$MONO" ]
then 
 echo "Error: Mono not found"
 echo "You need to install Mono the .Net environment to use autopano-sift."
 myexit
fi

# Set this to the directory you installed autopano-sift into, for example
#AUTOPANO_PATH="/Users/ippei/Download/HuginOSX/autopano-sift"
# Do not use a trailing backslash. If the executeables are within your path
# (recommended, you can leave the line below).
#AUTOPANO_PATH=$(dirname $(which generatekeys-sd.exe))
AUTOPANO_PATH=`dirname "$0"`/autopano-sift

usage()
{
	echo "usage: autopano-complete.sh [options] -o panoproject.pto image1 image2 [...]"
	echo
	echo "  -o name      filename of created panorama project"
	echo
	echo "[options]:"
#	echo "  -a path      specifies path to the directory where"
#	echo "              autopano.exe and generatekeys-sd.exe are."
	echo "  -s number    downsize images until width and height is"
	echo "              smaller than number, default 700"
	echo "  -p number    number of generated control points between,"
	echo "              each pair, default: 10"
	echo "  -n           no ransac detection, useful for fisheye images"
	echo "  -c           do not reuse keypoints detected in earlier runs,"
	echo "              deletes old keypoint files."
	myexit
}

echo `basename "$0"` "$@"
echo "AUTOPANO_PATH = \"$AUTOPANO_PATH\""

#NARG=$#
#if [ $NARG -lt 2 ]; then
#	usage
#	exit 1
#fi

#args=`getopt o:a:s:p:nch $*`
args=`getopt o:s:p:nch $*`
set -- $args

if [ $? != 0 ] ; then echo "Error! (parsing arguments failed)"; echo "Terminating..." >&2 ; myexit; fi


POINTS=10;
RANSAC=1;
CLEAN=0;
SIZE=800;

while true ; do
        case "$1" in
                -o) PANOFILE=$2; shift 2;;
#                -a) AUTOPANO_PATH=$2; shift 2;;
                -s) SIZE=$2; shift 2;;
                -p) POINTS=$2; shift 2 ;;
                -n) RANSAC=0; shift 1;;
                -c) CLEAN=1; shift 1;;
                -h) usage; shift 1;;
                --) shift ; break ;;
                *) echo "Command line parsing error at: $1" ; myexit ;;
        esac
done

echo "Remaining arguments ($#):"
for arg do echo '--> '"\`$arg'" ; done

# Allow user to override temporary directory.
TMP=${TMPDIR:-/tmp}

if [ ! -f "$AUTOPANO_PATH/generatekeys-sd.exe" ]
then echo error "Cannot locate generatekeys-sd.exe in $AUTOPANO_PATH"; myexit; fi
if [ ! -f "$AUTOPANO_PATH/autopano.exe" ]
then echo error "Cannot locate autopano.exe in $AUTOPANO_PATH"; myexit; fi

KEYFILES=""
for arg do
        FILENAME=$(basename $arg).key.gz
	KEYFILES="$KEYFILES $FILENAME"
	if [ -f $FILENAME ]; then
		if [ $CLEAN -ne 0 ]; then
            echo "$MONO \"$AUTOPANO_PATH/generatekeys-sd.exe\" $arg $FILENAME $SIZE"
			$MONO "$AUTOPANO_PATH/generatekeys-sd.exe" $arg $FILENAME $SIZE
		else
			echo "Using previously generated keypoint file: $FILENAME"
		fi
	else
		echo "$MONO \"$AUTOPANO_PATH/generatekeys-sd.exe\" $arg $FILENAME $SIZE"
		$MONO "$AUTOPANO_PATH/generatekeys-sd.exe" $arg $FILENAME $SIZE
	fi
done

echo "keyfiles: $KEYFILES"
ARG="--ransac $RANSAC --maxmatches $POINTS";
echo "$MONO \"$AUTOPANO_PATH/autopano.exe\" $ARG $PANOFILE $KEYFILES"
$MONO "$AUTOPANO_PATH/autopano.exe" $ARG $PANOFILE $KEYFILES

sleep 3;
