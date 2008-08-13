# ------------------
#    gettext 
# ------------------
# Based on the works of (c) 2007, Ippei Ukai
# Created for Tiger by Harry van der Wolf 2008

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


# init

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


# compile

for ARCH in $ARCHS
do

 mkdir -p "$REPOSITORYDIR/arch/$ARCH/bin";
 mkdir -p "$REPOSITORYDIR/arch/$ARCH/lib";
 mkdir -p "$REPOSITORYDIR/arch/$ARCH/include";

 ARCHARGs=""
 MACSDKDIR=""

 if [ $ARCH = "i386" -o $ARCH = "i686" ]
 then
  TARGET=$i386TARGET
  MACSDKDIR=$i386MACSDKDIR
  ARCHARGs="$i386ONLYARG"
 elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ]
 then
  TARGET=$ppcTARGET
  MACSDKDIR=$ppcMACSDKDIR
  ARCHARGs="$ppcONLYARG"
 elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ]
 then
  TARGET=$ppc64TARGET
  MACSDKDIR=$ppc64MACSDKDIR
  ARCHARGs="$ppc64ONLYARG"
 elif [ $ARCH = "x86_64" ]
 then
  TARGET=$x64TARGET
  MACSDKDIR=$x64MACSDKDIR
  ARCHARGs="$x64ONLYARG"
 fi


# export PATH=/usr/bin:$PATH

 env CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CPPFLAGS="-I$REPOSITORYDIR/include -I/usr/include -no-cpp-precomp" \
  LDFLAGS="-L$REPOSITORYDIR/lib -L/usr/lib -dead_strip" \
  NEXT_ROOT="$MACSDKDIR" \
  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
  --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
  --enable-shared --disable-csharp --without-emacs \
  --with-included-gettext --with-included-glib \
  --with-included-libxml --without-examples \
  --with-included-libcroco  \
;


 make clean;
 make;
 make install;

done


# merge libgettext

for liba in lib/libgettext.a lib/libgettextlib-$FULL_LIB_VER.dylib lib/libgettextpo.0.4.0.dylib lib/libgettextsrc-$FULL_LIB_VER.dylib lib/libasprintf.0.0.0.dylib lib/libasprintf.a lib/libintl.8.0.2.dylib lib/libintl.a  
do

 if [ $NUMARCH -eq 1 ]
 then
  mv "$REPOSITORYDIR/arch/$ARCHS/$liba" "$REPOSITORYDIR/$liba";
  ranlib "$REPOSITORYDIR/$liba";
  continue
 fi

 LIPOARGs=""
 
 for ARCH in $ARCHS
 do
  LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$liba"
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$liba";
 ranlib "$REPOSITORYDIR/$liba";

done

if [ -f "$REPOSITORYDIR/lib/libgettextlib-$FULL_LIB_VER.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libgettextlib-$FULL_LIB_VER.dylib" "$REPOSITORYDIR/lib/libgettextlib-$FULL_LIB_VER.dylib"
 ln -sfn libgettextlib-$FULL_LIB_VER.dylib $REPOSITORYDIR/lib/libgettextlib.dylib;
fi

if [ -f "$REPOSITORYDIR/lib/libgettextsrc-$FULL_LIB_VER.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libgettextsrc-$FULL_LIB_VER.dylib" "$REPOSITORYDIR/lib/libgettextsrc-$FULL_LIB_VER.dylib"
 ln -sfn libgettextsrc-$FULL_LIB_VER.dylib $REPOSITORYDIR/lib/libgettextsrc.dylib;
fi

if [ -f "$REPOSITORYDIR/lib/libgettextpo.0.4.0.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libgettextpo.0.4.0.dylib" "$REPOSITORYDIR/lib/libgettextpo.0.4.0.dylib"
 ln -sfn libgettextpo.0.4.0.dylib $REPOSITORYDIR/lib/libgettextpo.0.dylib;
 ln -sfn libgettextpo.0.4.0.dylib $REPOSITORYDIR/lib/libgettextpo.dylib;
fi

if [ -f "$REPOSITORYDIR/lib/libasprintf.0.0.0.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libasprintf.0.0.0.dylib" "$REPOSITORYDIR/lib/libasprintf.0.0.0.dylib"
 ln -sfn libasprintf.0.0.0.dylib $REPOSITORYDIR/lib/libasprintf.0.dylib;
 ln -sfn libasprintf.0.0.0.dylib $REPOSITORYDIR/lib/libasprintf.dylib;
fi

if [ -f "$REPOSITORYDIR/lib/libintl.8.0.2.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libintl.8.0.2.dylib" "$REPOSITORYDIR/lib/libintl.8.0.2.dylib"
 ln -sfn libintl.8.0.2.dylib $REPOSITORYDIR/lib/libintl.8.dylib;
 ln -sfn libintl.8.0.2.dylib $REPOSITORYDIR/lib/libintl.dylib;
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

 if [ $NUMARCH -eq 1 ]
 then
  mv "$REPOSITORYDIR/arch/$ARCHS/$program" "$REPOSITORYDIR/$program";
  strip "$REPOSITORYDIR/$program";
  continue
 fi

 LIPOARGs=""

 for ARCH in $ARCHS
 do
  LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$program"
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$program";

 strip "$REPOSITORYDIR/$program";

done



