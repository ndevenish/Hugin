# ------------------
#      boost
# ------------------
# $ID: $

# prepare
# export REPOSITORYDIR="/PATH2HUGIN/mac/ExternalPrograms/repository";


# install

rm -rf "$REPOSITORYDIR/include/boost";
cp -R "./boost" "$REPOSITORYDIR/include/";


# patch

gcc_hpp="$REPOSITORYDIR/include/boost/config/compiler/gcc.hpp"

mv "$gcc_hpp" "$gcc_hpp copy";

cat "$gcc_hpp copy" | sed /^.*versions\ check:$/q > "$gcc_hpp";
echo "// -- version check removed in order to use newer gcc. --" >> "$gcc_hpp";
