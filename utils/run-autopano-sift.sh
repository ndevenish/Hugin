#!/bin/sh


usage()
{
	echo "usage: run-autopano-sift.sh [options] -o panoproject.pto image1 image2 [...]"
	echo
	echo "options can be:  -o | --output   name    filename of created panorama project"
	echo "                 -s | --size     number  downsize images until width and height is"
	echo "                                         smaller than number, default 700"
	echo "                 -p | --points   number  number of generated control points between,"
	echo "                                         each pair, default: 10"
	echo "                 -n | --noransac         no ransac detection, useful for fisheye images"
	echo "                 -c | --clean            do not reuse keypoints detected in earlier runs,"
        echo "                                         deletes old keypoint files."
	exit 1
}


TEMP=`getopt -o o:s:p:nch -l output:,size:,points:,noransac,clean,help \
     -n "$0" -- "$@"`

if [ $? != 0 ] ; then echo "Terminating..." >&2 ; exit 1 ; fi

# Note the quotes around `$TEMP': they are essential!
eval set -- "$TEMP"

POINTS=10;
RANSAC=1;
CLEAN=0;

while true ; do
        case "$1" in
                -o|--outout) PANOFILE=$2; shift 2;;
                -s|--size)   SIZE=$2; shift 2;;
                -p|--points) POINTS=$2; shift 2 ;;
                -n|--noransac) RANSAC=0; shift 1;;
                -c|--clean) CLEAN=1; shift 1;;
                -h|--help) usage; shift 1;;
                --) shift ; break ;;
                *) echo "Command line parsing error at: $1" ; exit 1 ;;
        esac
done

NARG=$#
if [ $NARG -lt 2 ]; then
	usage
	exit 1
fi


echo "Remaining arguments ($#):"
for arg do echo '--> '"\`$arg'" ; done

TMP=/tmp

KEYFILES=""
for arg do
        FILENAME=$(echo -n $arg | sed -r 's@.+/([^/]+$)@\1@').key.gz
	KEYFILES="$KEYFILES $FILENAME"
	if [ -f $FILENAME ]; then
		if [ $CLEAN -ne 0 ]; then
			generatekeys.exe $arg $FILENAME $SIZE
		else
			echo "Using previously generated keypoint file: $FILENAME"
		fi
	else
		generatekeys.exe $arg $FILENAME $SIZE
	fi
done

echo "keyfiles: $KEYFILES"
ARG="--ransac $RANSAC";
autopano.exe $ARG $PANOFILE $KEYFILES

