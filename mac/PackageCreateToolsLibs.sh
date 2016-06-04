rm -r $1 2> /dev/null
mkdir -p $1 || exit 1
cd $1		|| exit 1

#ignore first arg
shift

# iterate
while test ${#} -gt 0
do
  ln -sf "../Hugin.app/Contents/MacOS/${1}" "."
  shift
done

