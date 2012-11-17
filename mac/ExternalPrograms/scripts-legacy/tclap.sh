# ------------------
#     libtclap
# ------------------
# $Id: libtclap.sh 1902 2008-01-02 22:27:47Z Harry $
# Copyright (c) 2007, Ippei Ukai
# script skeleton Copyright (c) 2007, Ippei Ukai
# tclap specifics 2011, Harry van der Wolf


# prepare


# -------------------------------
# 20110326 hvdw First script
# update from 1.20 to 1.21
# -------------------------------

# init

fail()
{
        echo "** Failed at $1 **"
        exit 1
}


let NUMARCH="0"

for i in $ARCHS
do
  NUMARCH=$(($NUMARCH + 1))
done

mkdir -p "$REPOSITORYDIR/bin";
mkdir -p "$REPOSITORYDIR/lib";
mkdir -p "$REPOSITORYDIR/include";

# compile

 ARCHARGs=""
 MACSDKDIR=""

TARGET=$x64TARGET
MACSDKDIR=$x64MACSDKDIR
ARCHARGs="$x64ONLYARG"
OSVERSION="$x64OSVERSION"
CC=$x64CC
CXX=$x64CXX
   
mv Makefile.am Makefile.am.org
mv Makefile.in Makefile.in.org
sed 's/ docs tests//g' Makefile.am.org > Makefile.am
sed 's/ docs tests//g' Makefile.in.org > Makefile.in

# only make "universal" version
 env \
  CC=$CC CXX=$CXX \
  CFLAGS="-isysroot $MACSDKDIR -O2-dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR -O2 -dead_strip" \
  CPPFLAGS="-I$REPOSITORYDIR/include" \
  LDFLAGS="-L$REPOSITORYDIR/lib -mmacosx-version-min=$OSVERSION -dead_strip" \
  NEXT_ROOT="$MACSDKDIR" \
  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
  --host="$TARGET" --exec-prefix=$REPOSITORYDIR \
  --enable-static --enable-shared --with-zlib=$MACSDKDIR/usr/lib \
  --disable-doxygen --disable-test || fail "configure step for universal";

 make clean
 make || fail "failed at make step of universal";
 make install || fail "make install step of universal";


