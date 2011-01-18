// simplified call interface to the Python Plugin facility.
// this function expects the following arguments:
// - the name of the plugin. This must name a python file
//   which is in PYTHONPATH, without path and extension.
//   so if your file is ~/mine/plugin.py you pass 'plugin'
// - the number of arguments. This is the number of arguments
//   to the plugin, not the number of argument to this function.
// - for every argument, two more values:
//   - a string containing the name of the type (this may need
//     to be fully qualified, like 'HuginBase::Panorama*'
//   - a void* to the actual hugin object

namespace hsi {

  extern int callhpi ( const char * plugin_name ,
	             int argc ,
	             ... ) ;

}