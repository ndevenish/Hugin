README for the Hugin Scripting Interface

This README gives an introduction to the Hugin Scripting Interface, hsi in short, and it's counterpart, the hugin plugin interface, hpi. 

0. Using hsi/hpi

If you have access to a ready made hugin binary that comes with hsi/hpi capability, you'll notice that the main menu now offers to run a Python script. This menu entry will show a file-select dialog where you can pick a plugin file, which will be executed without further ado. Currently there is no feedback from the plugin apart from it's success or failure, which is communicated in a dialog. If you're on Linux, you can start hugin from the command line and switch to the window you started it from while the plugin is running - then you'll see any console output it may produce. The effect of the plugin manifests itself after the plugin's termination. While the plugin runs, hugin will not respond. If the plugin's effect is undesirable you should be able to return to the previous state by using undo.

If you are using plugins that have been designed to also work as standalone Python programs, you can pass parameters to them on the command line. If called from hugin, no parametrization is possible, apart from modifying the Python code, using an .ini file, as demonstrated with woa.py, or taking input via GUI elements, as demonstrated in crop_cp.py, which will only succeed on Linux currently.

1. What is it?

Hugin provides a GUI and a set of command line tools, but there is no scripting/plugin interface. Scripting hugin/panotools currently means to exchange data by passing command line arguments and files. To change this and to make hugin's capabilities available for script writing, I have written interface code in SWIG, the 'Simplified Wrapper and Interface Generator'. With this code, SWIG can generate an interface for scripting languages in general; out of the available choices I have picked Python as the target language and generated a Python module which makes a large part of hugin's backend capabilities available to Python. Once the hugin data were accessible to Python, I was able to embed Python in hugin (and in other software that uses libhuginbase.so) and use the data compatibility to provide a plugin interface which allows execution of arbitrary Python code with extensive hugin type and method compatibility.

2. What is needed to get it up and running?

hsi/hpi is currently runing on Linux and Windows, Mac support is still missing as of this writing.

The Python support is a built time option. You need to activate the Python support at built time, it can not activated at run time.

3. Compiling hugin with python scripting support

You need the follow dependencies to compile Hugin with python scripting support:
* Python headers and library
* swig, at least version 2.0, for Python >=3.2 you need at least version 2.0.4 (http://www.swig.org/)
For running the scripting interface you need a working Python environment. It should work with Python 2.6, 2.7 and 3.x
For activate the python support supply set the CMake variable BUILD_HSI to ON. This can be easily done in the CMake GUI or by supply the switch -DBUILD_HSI:BOOL=On on the CMake command line.
Note that on Unix readymade swig 2.0 packets may install the swig executable named 'swig2.0' in which case cmake may not find it and you have to add something like -DSWIG_EXECUTABLE=/usr/bin/swig2.0 to the cmake command line.
Now you're ready to go. Proceed with a normal hugin build cycle, and if all goes well, the Python modules and the generated shared libraries have been made. If you run make install, everything should be put into place, alternatively you can make a package (that's on Linux) and install that.

4. Play with it

You may have noticed that the wrapper part of the module, where the hardcore stuff happens, _hsi.so, is quite large. This is because the wrap is generated from huginbase headers and I made an attempt to offer proxies for every data type and access to all methods and functions in Python, maintaining the same object-oriented interface to the data that is available in hugin. This fattens the interface; ultimately one might conceive of a slimmer interface definition, also because it maybe isn't really necessary to interface to every bit of hugin code declared in the headers that went into the interface. And this also makes it quite comfortable to use for someone who knows the C++ objects and their methods - the names are essentially the same.

On the other hand, the interface won't stop you from shooting yourself in the foot - you're probably quite safe loading a pto file and looking at it's data, but once you start changing things you may run into trouble. Let me give you a tiny idea of what you can do from Python:

from hsi import *         # load the module
p=Panorama()              # make a new Panorama object
ifs=ifstream('xx.pto')    # create a C++ std::ifstream
p.readData(ifs)           # read the pto file into the Panorama object
del ifs                   # don't need anymore
img0=p.getImage(0)        # access the first image
print img0.getWidth()     # print the image's width
cpv=p.getCtrlPoints()     # get the control points in the panorama
for cp in cpv[:30:2] :    # print some data from some of the CPs
  print cp.x1
cpv=cpv[30:50]            # throw away most of the CPs
p.setCtrlPoints(cpv)      # pass that subset back to the panorama
ofs=ofstream('yy.pto')    # make a c++ std::ofstream to write to
p.writeData(ofs)          # write the modified panorama to that stream
del ofs                   # done with it

But this is only the hsi side of things - calling hugin functionality from Python scripts. I have also created code to go the other way: use Python functionality from C++ code. I call this side hpi, 'hugin plugin interface'. In Python terminology, hsi is an extension (of Python), while hpi is embedding Python into another application (hugin, in this case). As mentioned before, hpi allows you to call arbitrary Python code from all programs that link to it; the hsi-enabled hugin does just that and offers an entry point for Python experiments via Edit->Run Python Script. There are simple sample Python plugins in the hugin_script_interface/plugins directory as well as some more involved ones to give you ideas, and hopefully a body of code will develop which you can use for templates. hpi uses hsi, so it can deal with all data types wrapped with hsi and call all their methods. So now there's both-way integration: you can call Python from hugin and just carry on in Python with the same objects, then return to C++. As a user, you are enabled to use and write Python plugins with access to the hugin dataverse. The plugin interface provided by hugin now is merely experimental, though, because there hasn't been any discussion yet how to provide the new features to users and what to do with them.
 
5. Technicalities

Some data don't wrap unaided. They pop up as return values from function calls and constitute 'pointer objects' without the accessor layer that is provided when wrapping these types. This isn't intended - in fact, if all is as I hope it shouldn't happen at all. To avoid it, all functionality would have to be properly tested, which is quite a task - I've put more effort in code-writing than in testing initially, but of course this will have to change once the groundwork is laid. If you get such a data type, you won't be able to access the object that is pointed to - Python simply has no idea what to do with it. If that happens, the i-file (hsi.i) has to be adapted to wrap the type. Likely candidates for this are instantiated templates and uncommon types - I've wrapped some but I'm bound to have overseen others. Some data types only look like the corresponding C++ types, notably all types from the vigra namespace. This is because I didn't want to pull the full vigra type system in; the wrapped types allow basic access to the content, and I've only wrapped very simple vigra types - but then, the hugin headers I've wrapped only use simple ones, like Point2D and Rect2D, and only to pass to and fro coordinates, so this minimal wrapping is probably sufficient. I've also wrapped C++ fstreams, because hugin uses them a lot and Python doesn't natively deal with them.

If you want more detailed information on what's wrapped and what isn't, Python's help system can come in handy. To get a listing of all wrapped classes and their methods an the methods' return types, try

python -c 'import hsi; help(hsi)' > help.txt

If you're exploring hsi in an interactive Python session, all the wrapped objects will provide some help if you're stuck, but it's admittedly quite basic.

There is sparse documentation of the hugin data types and their methods beyond call signatures and member lists, so you have to guess your way when you want to use them. Luckily most of them are aptly and expressively named, so you can figure it out. I hope that eventually something like an API documentation will arise.

6. a curious footnote:

On Kubuntu 10.10 / Python 2.6 I noticed a problem with cerr and hsi. Whenever anything is output to cerr by the C++ code, the program crashes with a memory fault after the output of the first string. I looked with the debugger and found that the memory error occurs in std::uncaught_exception(). The error only occurs if the SWIG module is linked with the hugin code. I'd be curious to hear if anyone else can reproduce this behaviour on Linux; T. Modes has already established that it doesn't occur on Windows. There's also a thread on hugin-ptx for it:

http://groups.google.com/group/hugin-ptx/browse_thread/thread/51bd6ca9ced92fc8#
