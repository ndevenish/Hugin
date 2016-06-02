mkdir -p $1 || exit 1
cd $1		|| exit 1
rm *

#ignore first arg
shift

# iterate
while test ${#} -gt 0
do
  ln -sf "../Hugin.app/Contents/MacOS/${1}" "."
  shift
done

