# ------------------
#    lensfun 
# ------------------
# $Id: libpng.sh 1902 2007-02-04 22:27:47Z ippei $
# Copyright (c) 2007, Ippei Ukaia
# This script, 2012  HvdW


# prepare

# -------------------------------
# 20120307 hvdw initial lensfun based on svn 152
# 20120415.0 hvdw now builds correctly
# 20120429.0 hvdw compile x86_64 with gcc-4.6 for lion and up openmp compatibility
# -------------------------------

# init

fail()
{
        echo "** Failed at $1 **"
        exit 1
}

ORGPATH=$PATH

#patch -Np0 < ../scripts/lensfun-patch-pkgconfig.diff

let NUMARCH="0"
for i in $ARCHS ; do
  NUMARCH=$(($NUMARCH + 1))
done

mkdir -p "$REPOSITORYDIR/bin";
mkdir -p "$REPOSITORYDIR/lib";
mkdir -p "$REPOSITORYDIR/include";


# compile

for ARCH in $ARCHS
do

 mkdir -p "$REPOSITORYDIR/arch/$ARCH/bin";
 mkdir -p "$REPOSITORYDIR/arch/$ARCH/lib";
 mkdir -p "$REPOSITORYDIR/arch/$ARCH/include";

 ARCHARGs=""
 MACSDKDIR=""

 if [ $ARCH = "i386" -o $ARCH = "i686" ] ; then
   TARGET=$i386TARGET
   MACSDKDIR=$i386MACSDKDIR
   ARCHARGs="$i386ONLYARG"
   OSVERSION="$i386OSVERSION"
   CC=$i386CC
   CXX=$i386CXX
   ARCHFLAG="-m32"
 elif [ $ARCH = "x86_64" ] ; then
   TARGET=$x64TARGET
   MACSDKDIR=$x64MACSDKDIR
   ARCHARGs="$x64ONLYARG"
   OSVERSION="$x64OSVERSION"
   CC=$x64CC
   CXX=$x64CXX
   ARCHFLAG="-m64"
 fi

 make clean;
 env \
  CC=$CC CXX=$CXX \
  CFLAGS="-isysroot $MACSDKDIR $ARCHFLAG $ARCHARGs $OTHERARGs -O3 -dead_strip -I$REPOSITORYDIR/include/glib-2.0 -I$REPOSITORYDIR/include/gio-unix-2.0 \
         -I$REPOSITORYDIR/arch/$ARCH/lib/glib-2.0/include -I$REPOSITORYDIR/arch/$ARCH/lib/gio/include -I$REPOSITORYDIR/include" \
  CXXFLAGS="-isysroot $MACSDKDIR $ARCHFLAG $ARCHARGs $OTHERARGs -O3 -dead_strip -I$REPOSITORYDIR/include/glib-2.0 -I$REPOSITORYDIR/include/gio-unix-2.0 \
         -I$REPOSITORYDIR/arch/$ARCH/lib/glib-2.0/include -I$REPOSITORYDIR/arch/$ARCH/lib/gio/include -I$REPOSITORYDIR/include" \
  CPPFLAGS="$ARCHFLAG -I$REPOSITORYDIR/include/glib-2.0 -I$REPOSITORYDIR/include/gio-unix-2.0 -I/usr/include \
         -I$REPOSITORYDIR/arch/$ARCH/lib/glib-2.0/include -I$REPOSITORYDIR/arch/$ARCH/lib/gio/include -I$REPOSITORYDIR/include" \
  LDFLAGS="$ARCHFLAG -L$REPOSITORYDIR/lib -L/usr/lib -mmacosx-version-min=$OSVERSION -dead_strip" \
  NEXT_ROOT="$MACSDKDIR" \
  ./configure --prefix="$REPOSITORYDIR/arch/$ARCH" --sdkdir="$REPOSITORYDIR/arch/$ARCH" --mode="release" \
  || fail "configure step for $ARCH";


# Very stupid lensfun doesn't listen very well to CFLAGS/CXXFLAGS etc.. so we have 
# to manually modify the config.mak
 cp config.mak config.mak.org
 sed -e "s+/opt/local/lib+$REPOSITORYDIR/arch/$ARCH/lib+g" -e "s+/opt/local/include+$REPOSITORYDIR/include+g" -e 's+png14+png15+g'  config.mak.org > config.mak


 make libs || fail "failed at make step of $ARCH";
 make install || fail "make install step of $ARCH";

 # somehow lensfun.h is not copied to REPOSITYDIR/include
 cp $REPOSITORYDIR/arch/$ARCH/include/lensfun.h $REPOSITORYDIR/include
 # and neither is the share folder with the public database
 mkdir -p $REPOSITORYDIR/share
 cp -a $REPOSITORYDIR/arch/$ARCH/share/lensfun  $REPOSITORYDIR/share
done

# merge lensfun libs

for liba in lib/liblensfun.dylib 
do

 if [ $NUMARCH -eq 1 ] ; then
   if [ -f $REPOSITORYDIR/arch/$ARCHS/$liba ] ; then
		 echo "Moving arch/$ARCHS/$liba to $liba"
  	 mv "$REPOSITORYDIR/arch/$ARCHS/$liba" "$REPOSITORYDIR/$liba";
  	 echo "Changing both install_names"
  	 install_name_tool -id "$REPOSITORYDIR/$liba" "$REPOSITORYDIR/$liba"
  	 install_name_tool -change "liblensfun.dylib" "$REPOSITORYDIR/$liba" "$REPOSITORYDIR/$liba"
	   #Power programming: if filename ends in "a" then ...
	   [ ${liba##*.} = a ] && ranlib "$REPOSITORYDIR/$liba";
  	 continue
   else
		 echo "Program arch/$ARCHS/$liba not found. Aborting build";
		 exit 1;
   fi
 fi

 LIPOARGs=""
 
 for ARCH in $ARCHS
 do
	if [ -f $REPOSITORYDIR/arch/$ARCH/$liba ] ; then
		echo "Adding arch/$ARCH/$liba to bundle"
		LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$liba"
	else
		echo "File arch/$ARCH/$liba was not found. Aborting build";
		exit 1;
	fi
 done

echo "Changing both install_names"
#        install_name_tool -id "$REPOSITORYDIR/lib/liblensfun.dylib" "$REPOSITORYDIR/arch/$ARCH/$liba"
install_name_tool -id "$REPOSITORYDIR/lib/liblensfun.dylib" "$REPOSITORYDIR/lib/liblensfun.dylib"
install_name_tool -change "liblensfun.dylib" "$REPOSITORYDIR/lib/liblensfun.dylib" "$REPOSITORYDIR/lib/liblensfun.dylib"



 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$liba";
 #Power programming: if filename ends in "a" then ...
 [ ${liba##*.} = a ] && ranlib "$REPOSITORYDIR/$liba";

done

