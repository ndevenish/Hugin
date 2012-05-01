# ------------------
#    gettext 
# ------------------
# Based on the works of (c) 2007, Ippei Ukai
# Created for Hugin by Harry van der Wolf 2009

# download location ftp.gnu.org/gnu/gettext/

# prepare

# export REPOSITORYDIR="/PATH2HUGIN/mac/ExternalPrograms/repository" \
# ARCHS="ppc i386" \
#  ppcTARGET="powerpc-apple-darwin8" \
#  i386TARGET="i386-apple-darwin8" \
#  ppcMACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  i386MACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  ppcONLYARG="-mcpu=G3 -mtune=G4" \
#  i386ONLYARG="-mfpmath=sse -msse2 -mtune=pentium-m -ftree-vectorize" \
#  OTHERARGs="";

# -------------------------------
# 20091206.0 sg Script tested and used to build 2009.4.0-RC3
# 20100116.0 HvdW Correct script for libintl install_name in libgettext*.dylib
# 20100624.0 hvdw More robust error checking on compilation
# -------------------------------

# init

fail()
{
        echo "** Failed at $1 **"
        exit 1
}

ORGPATH=$PATH

let NUMARCH="0"

for i in $ARCHS
do
  NUMARCH=$(($NUMARCH + 1))
done

mkdir -p "$REPOSITORYDIR/bin";
mkdir -p "$REPOSITORYDIR/lib";
mkdir -p "$REPOSITORYDIR/include";

GETTEXTVER_M="0"
GEETTEXTVER_FULL="$GETTEXTVER_M.17"
MAIN_LIB_VER="0"
FULL_LIB_VER="$MAIN_LIB_VER.17"
ASPRINTFVER_F="0.0.0"
ASPRINTFVER_M="0"
GETTEXTVERPO_M="0"
GETTEXTVERPO_F="0.4.0"
LIBINTLVER_F="8.0.2"
LIBINTLVER_M="8"

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
#   ARCHARGs="$i386ONLYARG"
   ARCHARGs='-march=prescott -mtune=pentium-m -ftree-vectorize -mmacosx-version-min=10.5'
   OSVERSION="$i386OSVERSION"
   CC=$i386CC
   CXX=$i386CXX
   myPATH=/usr/bin:$ORGPATH
   ARCHFLAG="-m32"
 elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ] ; then
   TARGET=$ppcTARGET
   MACSDKDIR=$ppcMACSDKDIR
   ARCHARGs="$ppcONLYARG"
   OSVERSION="$ppcOSVERSION"
   CC=$ppcCC
   CXX=$ppcCXX
 elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ] ; then
   TARGET=$ppc64TARGET
   MACSDKDIR=$ppc64MACSDKDIR
   ARCHARGs="$ppc64ONLYARG"
   OSVERSION="$ppc64OSVERSION"
   CC=$ppc64CC
   CXX=$ppc64CXX
 elif [ $ARCH = "x86_64" ] ; then
   TARGET=$x64TARGET
   MACSDKDIR=$x64MACSDKDIR
   ARCHARGs="$x64ONLYARG"
   OSVERSION="$x64OSVERSION"
   #CC="gcc-4.6"
   #CXX="g++-4.6"
   CC=$x64CC
   CXX=$x64CXX
   ARCHFLAG="-m64"
   #myPATH=/usr/local/bin:$PATH
   myPATH=/usr/bin:$ORGPATH
 fi

 # first patch the make file
 #patch -po < patch-gettext-tools-Makefile.in


 env \
  PATH=$myPATH \
  CC=$CC CXX=$CXX \
  CFLAGS="-isysroot $MACSDKDIR $ARCHFLAG $ARCHARGs $OTHERARGs -O3 -dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR $ARCHFLAG $ARCHARGs $OTHERARGs -O3 -dead_strip" \
  CPPFLAGS="-I$REPOSITORYDIR/include -I/usr/include -no-cpp-precomp" \
  LDFLAGS="-L$REPOSITORYDIR/lib -L/usr/lib -mmacosx-version-min=$OSVERSION -dead_strip $ARCHFLAG" \
  NEXT_ROOT="$MACSDKDIR" \
  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
    --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
    --enable-shared --enable-static --disable-csharp --disable-java \
    --with-included-gettext --with-included-glib --disable-openmp \
    --with-included-libxml --without-examples --with-libexpat-prefix=$REPOSITORYDIR \
    --with-included-libcroco  --without-emacs --with-libiconv-prefix=$REPOSITORYDIR || fail "configure step for $ARCH" ;

 make clean;
 make || fail "failed at make step of $ARCH";
 make install || fail "make install step of $ARCH";

done

# merge libgettext

#for liba in lib/libgettext.a lib/libgettextlib-$FULL_LIB_VER.dylib lib/libgettextpo.$GETTEXTVERPO_F.dylib lib/libgettextsrc-$FULL_LIB_VER.dylib lib/libasprintf.$ASPRINTFVER_F.dylib lib/libasprintf.a lib/libintl.$LIBINTLVER_F.dylib lib/libintl.a  
for liba in lib/libgettextlib-$FULL_LIB_VER.dylib lib/libgettextpo.$GETTEXTVERPO_F.dylib lib/libgettextsrc-$FULL_LIB_VER.dylib lib/libasprintf.$ASPRINTFVER_F.dylib lib/libasprintf.a lib/libintl.$LIBINTLVER_F.dylib lib/libintl.a  
do

 if [ $NUMARCH -eq 1 ] ; then
   if [ -f $REPOSITORYDIR/arch/$ARCHS/$liba ] ; then
		 echo "Moving arch/$ARCHS/$liba to $liba"
  	 mv "$REPOSITORYDIR/arch/$ARCHS/$liba" "$REPOSITORYDIR/$liba";
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

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$liba";
 #Power programming: if filename ends in "a" then ...
 [ ${liba##*.} = a ] && ranlib "$REPOSITORYDIR/$liba";
 

done

if [ -f "$REPOSITORYDIR/lib/libgettextlib-$FULL_LIB_VER.dylib" ] ; then
 install_name_tool -id "$REPOSITORYDIR/lib/libgettextlib-$FULL_LIB_VER.dylib" "$REPOSITORYDIR/lib/libgettextlib-$FULL_LIB_VER.dylib"
 ln -sfn libgettextlib-$FULL_LIB_VER.dylib $REPOSITORYDIR/lib/libgettextlib.dylib;
fi

if [ -f "$REPOSITORYDIR/lib/libgettextsrc-$FULL_LIB_VER.dylib" ] ; then
 install_name_tool -id "$REPOSITORYDIR/lib/libgettextsrc-$FULL_LIB_VER.dylib" "$REPOSITORYDIR/lib/libgettextsrc-$FULL_LIB_VER.dylib"
 ln -sfn libgettextsrc-$FULL_LIB_VER.dylib $REPOSITORYDIR/lib/libgettextsrc.dylib;
fi

if [ -f "$REPOSITORYDIR/lib/libgettextpo.$GETTEXTVERPO_F.dylib" ] ; then
 install_name_tool -id "$REPOSITORYDIR/lib/libgettextpo.$GETTEXTVERPO_F.dylib" "$REPOSITORYDIR/lib/libgettextpo.$GETTEXTVERPO_F.dylib"
 ln -sfn libgettextpo.$GETTEXTVERPO_F.dylib $REPOSITORYDIR/lib/libgettextpo.0.dylib;
 ln -sfn libgettextpo.$GETTEXTVERPO_F.dylib $REPOSITORYDIR/lib/libgettextpo.dylib;
fi

if [ -f "$REPOSITORYDIR/lib/libasprintf.$ASPRINTFVER_F.dylib" ] ; then
 install_name_tool -id "$REPOSITORYDIR/lib/libasprintf.$ASPRINTFVER_F.dylib" "$REPOSITORYDIR/lib/libasprintf.$ASPRINTFVER_F.dylib"
 ln -sfn libasprintf.$ASPRINTFVER_F.dylib $REPOSITORYDIR/lib/libasprintf.0.dylib;
 ln -sfn libasprintf.$ASPRINTFVER_F.dylib $REPOSITORYDIR/lib/libasprintf.dylib;
fi

if [ -f "$REPOSITORYDIR/lib/libintl.8.0.2.dylib" ] ; then
 install_name_tool -id "$REPOSITORYDIR/lib/libintl.$LIBINTLVER_F.dylib" "$REPOSITORYDIR/lib/libintl.$LIBINTLVER_F.dylib"
 ln -sfn libintl.$LIBINTLVER_F.dylib $REPOSITORYDIR/lib/libintl.8.dylib;
 ln -sfn libintl.$LIBINTLVER_F.dylib $REPOSITORYDIR/lib/libintl.dylib;
fi

#Copy shell script
for ARCH in $ARCHS
do
 mkdir -p "$REPOSITORYDIR/bin";
 cp "$REPOSITORYDIR/arch/$ARCH/bin/gettext.sh" "$REPOSITORYDIR/bin/gettext.sh";
 sed 's/^exec_prefix.*$/exec_prefix=\$\{prefix\}/' "$REPOSITORYDIR/arch/$ARCH/bin/gettextize" > "$REPOSITORYDIR/bin/gettextize";
 sed 's/^exec_prefix.*$/exec_prefix=\$\{prefix\}/' "$REPOSITORYDIR/arch/$ARCH/bin/autopoint" > "$REPOSITORYDIR/bin/autopoint";
 break;
done
chmod a+x "$REPOSITORYDIR/bin/gettextize" "$REPOSITORYDIR/bin/autopoint"


# merge execs
for program in bin/gettext bin/ngettext bin/xgettext bin/msgattrib bin/msgcmp bin/msgconv bin/msgexec bin/msgfmt bin/msginit bin/msgunfmt bin/msgcat bin/msgcomm bin/msgen bin/msgfilter bin/msggrep bin/msgmerge bin/msguniq bin/envsubst bin/recode-sr-latin 
do

 if [ $NUMARCH -eq 1 ] ; then
   if [ -f $REPOSITORYDIR/arch/$ARCHS/$program ] ; then
		 echo "Moving arch/$ARCHS/$program to $program"
  	 mv "$REPOSITORYDIR/arch/$ARCHS/$program" "$REPOSITORYDIR/$program";
  	 strip "$REPOSITORYDIR/$program";
  	 continue
	 else
		 echo "Program arch/$ARCHS/$program not found. Aborting build";
		 exit 1;
	 fi
 fi

 LIPOARGs=""

 for ARCH in $ARCHS
 do
	if [ -f $REPOSITORYDIR/arch/$ARCH/$program ] ; then
		echo "Adding arch/$ARCH/$program to bundle"
		LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$program"
	else
		echo "File arch/$ARCH/$program was not found. Aborting build";
		exit 1;
	fi
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$program";

 strip "$REPOSITORYDIR/$program";

done

# Last step for gettext libs. They are linked during build against libinitl and therefore have an install_name
# based on the arch/$ARCH directory. We need to change that. Unfortunately we need to do it for every arch even 
# though it is only mentioned once for one of the arc/$ARCHs this due to the order in which the $ARCHS are processed
# lipo merged (This should be deductable as the install_name should be the last in the ARCHS="..." variable, but I 
# don't care.

for ARCH in $ARCHS
do
 for gettextlib in $REPOSITORYDIR/lib/libgettextlib-$FULL_LIB_VER.dylib $REPOSITORYDIR/lib/libgettextpo.$GETTEXTVERPO_F.dylib $REPOSITORYDIR/lib/libgettextsrc-$FULL_LIB_VER.dylib
 do
   for lib in $(otool -L $gettextlib | grep $REPOSITORYDIR/arch/$ARCH/lib | sed -e 's/ (.*$//' -e 's/^.*\///')
   do
    echo " Changing install name for: $lib inside : $gettextlib"
    install_name_tool -change "$REPOSITORYDIR/arch/$ARCH/lib/$lib" "$REPOSITORYDIR/lib/$lib" $gettextlib
   done
 done
done


