woa - generation of control points from pre-aligned images

copyright Kay F. Jahnke 2011, 2012

You're free to spread this file unmodified.

0. For the impatient

You've got an hsi/hpi-enabled version of hugin, and it has this ominous 'warped overlap analysis' plug-in in the 'Actions' menu. What is it? It's a tool to find more, good quality control points for images which are already roughly aligned. You might call it a 'panorama refiner'. Try it out on a panorama you already have: choose two or more overlapping images to be 'active' in the fast preview, then launch woa. If everything is set up properly, you'll have to wait a while - longer if you've picked more images - and eventually your project will be a number of control points richer. There is no progress bar or such (sorry). While woa runs, hugin will be unresponsive, you'll just have to wait for it to complete. Most of the time, this is all you want and need, but you can tweak the process, and it may help if you know a bit of the background as well to get optimal results.

Alternatively, something may not be set up as it should be, in which case the script will return a negative number and display a little popup informing you of this event. This popup may not actually pop up but be hidden by other windows, in which case you have to Alt-Tab to it and okay it before hugin will be available again. Chances are you can mend the problem by installing some more software (see section 2 - all you need is FOSS)

1. Introduction

In panorama photography, finding good control points between images isn't always easy. The automatic control point generators (CPGs) often do a good job, but there are situations where it's desirable to find more, better or more evenly distributed control points. The straightforward approach - to simply feed the images into a CPG, doesn't work very well in certain situations:

- if the parts of the images to be matched are very distorted
  (often the case with the edges of fisheye images)

- if the images have been taken with different lenses, so their scale
  and/or lens geometry is different

The CPGs can cope with these problems to some extent - their feature detectors aim to be invariant to affine transformations, which are similar locally to lens distortions and change of focal length. Some CPGs also use lens information and/or remap images to a specific projection which allows good detection. Nevertheless, these methods only go so far. I am instead using a computationally slightly expensive, yet effective and pragmatic approach to minimize problems resulting from lens geometry and focal length, which works on pre-aligned images. The method is easily explained:
    
For each pair of overlapping images,

- Create a 'subset' project file with just these two images,
  and all other images removed

- Move the overlapping area to the center of the panorama,
  so that it's as undistorted as possible

- mask out areas of the images which don't overlap

- stitch to individual images - I call these 'warped overlap images'

- Once you have these warped overlap images, they should be quite
  similar and undistorted. Therefore they are good source material
  for a CPG. So use the CPG on the warped overlap images.

- Transform the control points you have obtained back to the
  original image coordinates.

woa does all these processing steps on all chosen image pairs (which images are chosen depends on the arguments passed to woa, explained below) - it's all fully automatic and requires no manual intervention.

The script performs these operations using hsi, the hugin scripting interface. The source material has to be in a pto file, the images must be pre-aligned, be it automatically, be it manually - the pre-alignment can be sloppy, but the overlap calculation is very precise, and if matching bits are outside the overlap, they will not be found - and the better the pre-alignment, the less dissimilar the warped overlap images will turn out, resulting in better control points. It's quite feasible and sometimes advisable to use woa iteratively to approach an ideal solution, meaning you use woa, then optimize, then use it again. Usually, though, a single run will be all you need.

What I usually do is run cpfind on the images and do an r,p,y optimization. This is quite fast and precise, and I have a good base to start from. Next I make sure that I don't have disconnected groups - if I do I manually add a few control points and optimize, or I manually move the disconnected image groups in place to get the rough placement. Then I use woa in a second step to find many more, good quality, evenly distributed control points. Another good starting point is image sets which have been taken with a standardized shooting pattern so that the rough positions can be set because they are known from the shooting pattern.

2. what's needed to run the script

As far as your pre-aligned images are concerned, I've developed and used woa for panoramas, not mosaics. You may have some success with mosaics, but really it's a tool for panoramas and uses equirectangular projection, and, frankly, I haven't tested it on mosaics. So you need a prealigned panorama, either open in hugin or stored as pto file.

The most important bit to run the script - apart from Python - is hsi, the hugin scripting interface. This is a Python module making most of hugin's backend functionality available to Python. This module is not available in binary form on all platforms, you may have to compile hugin yourself with hsi enabled. Currently, only Linux and Windows are supported; some Linux binaries in circulation have hsi active per default, while Windows binaries currently don't (as of early 2012). I've developed and tested the script on Kubuntu 10.10 and 11.4 with bleeding-edge hugin compiles (up to 2011.5) and Python 2.6 and 2.7, I hope it will work on other platforms. Compiling on Windows and use with Python 3.X have also been coded and should work.

If you're running woa as a hugin plug-in, you'll need hugin with built-in hpi (hugin plug-in interface). This is the hugin you get when you compile with the -DBUILD_HSI=ON cmake switch - it has an embedded Python interpreter, and you can pick woa.py as a plug-in to be used on your currently loaded project. On my system, it shows up in the 'Actions' menu as 'warped overlap analysis'.

You need nona, which is part of the hugin distribution, so it's very likely you have it already.

You also need tiffdump, which is part of the libtiff package - chances are you have it on your machine already, if not, you'll have to get it from http://www.libtiff.org. Tiffdump is needed to cope with the cropped TIFF images nona produces. You should set hugin to make nona output cropped TIFFs - I think it's the default, but do check. woa will process uncropped TIFF, but it's usually slower, and it will still use tiffdump even if your images are uncropped (because it can't find that out otherwise).

If you want to run woa standalone from the command line, you also need the python-argparse module. This is used to process the command line to woa. If your python is 2.7, argparse is part of the standard library. For older python versions, you need to download it separately.

As a CPG, you need autopano-sift-c or cpfind. I used to recommend autopano-sift-c, since it generates large amounts of evenly-distributed control points when used with the parameters it's fed by woa. Recently though, cpfind has developed nicely and now satisfies my requirements for use with woa, so I've changed the default and woa will now use cpfind instead. You can still use autopano-sift-c if you prefer it.

Tiffdump, nona and the CPG have to be in your system path; woa checks if it can access them and will refuse to work if it can't.

3. Calling the script from the command line

If you call woa.py from the command line without arguments, it will print a brief help text:

usage: woa.py [-h] [-c <overlap ceiling>] [-e END]
              [-g <cpfind or autopano-sift-c>]
              [-f <image number>] [-i <image number> [<image number> ...]]
              [-m <margin>] [-o <output file>] [-b <base name>] [-p]
              [-t <overlap threshold>] [-s <scaling factor>] [-v]
              [-x <image number> [<image number> ...]]
              <pto file>

    woa.py - warped overlap analysis
    generate control points from warped overlap images
    
    Copyright (C) 2011  Kay F. Jahnke

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    woa will look at every possible pair of images that can be made
    from the image numbers passed as -i parameters, or from all images
    in the pto if no -i parameters are passed. If the images in a pair
    overlap significantly, the overlapping parts will be warped and
    a CPG run on them. The resulting CPs will be transferred back into
    the original pto file.
    

positional arguments:
  <pto file>            pto file to be processed

optional arguments:
  -h, --help            show this help message and exit
  -c <overlap ceiling>, --ceiling <overlap ceiling>
                        ignore overlaps above this value (0.0-1.0)
  -e END, --end END     dummy: end a group of image numbers
  -g <cpfind or apsc>, --cpg <cpfind or apsc>
                        choose which CP generator to use
  -f <image number>, --focus <image number>
                        only look for overlaps with this image
  -i <image number> [<image number> ...], --images <image number> [<image number> ...]
                        image numbers to process
  -m <margin>, --margin <margin>
                        widen examined area [in pixels]
  -o <output file>, --output <output file>
                        write output to a different file
  -b <base name>, --basename <base name>
                        common prefix for intermediate files
  -p, --prolific        keep all intermediate files
  -t <overlap threshold>, --threshold <overlap threshold>
                        ignore overlaps below this threshold (0.0-1.0)
  -s <scaling factor>, --scale <scaling factor>
                        scaling for warped overlap images
  -v, --verbose         produce verbose output
  -x <image number> [<image number> ...], --exclude <image number> [<image number> ...]
                        image numbers to exclude
  -z <stop file>, --stop <stop file>
                        stop woa if this file is present
                        
                        
I'll explain the parameters in turn.

The most important parameter, and the only one which isn't prefixed by an argument name, is the pto file.

At times you may not want the original pto file to be modified, in which case you can specify another file as output by using -o.

Per default, woa will look at all possible image pairs and test them for overlaps. You can limit the search to a subset by specifying the image numbers after the -i flag. Numbering starts with 0, as in hugin.

Sometimes, you just want to find (more) control points for one specific image. That's when you use the -f (focus) parameter. If you specify a 'focus' image, only overlaps with this image will be analysed and used to make control points.

The scale of the warped overlap images can be varied with the -s flag. Using 1.0 here is analogous to hugin's 'optimal scale'. The default is .25, which I found good enough for most cases - I'm using a 12MP APSC DSLR sensor.

The -z, or --stop parameter allows you to set a 'stop file'. At times you may want to stop woa, but still keep all the CPs it has accumulated so far, which would be lost if you simply killed it. If you define a stop file, woa will look for this file before every iteration, and if it's present, it will not look at any more image pairs and finish normally, keeping all CPs it has found so far. On a Unix system, you'd simply 'touch' the file in question, and once woa has terminated, remove it. Per default no stop file is set. I use /tmp/stop_woa in my woa.ini file.

The threshold and ceiling parameters (-t and -c) determine which size overlaps are considered. They sensibly take values between 0 and 1 which refer to the fraction of the image that overlaps. I use a low threshold of about 0.01 to catch even small overlaps and a ceiling of 1.0, so that even total overlaps are processed. Depending on your specific use scenario you may want to use different values here: If your images are all of the same lens and focal length, you may want to raise the threshold, and if you're confident that well-overlapping images are already linked by enough control points, you may want to lower the ceiling.

The -g flag chooses the CPG to use. As I said above, I currently recommend using cpfind. If you want to use autopano-sift-c, beware of the use restrictions and make sure you qualify.

If you use the -v flag, woa will become more verbose. Some of the output you see then may be from external programs. But the output becomes hard to read if a lot of images are processed, so I usually leave verbose off.

woa produces lots of intermediate data. Usually, you'll not see them because woa cleans up after itself. The intermediate files conventionally begin with 'woa' ot '_woa', but you can choose a different prefix with the -b flag. If you want woa to keep all intermediate files (maybe because you're curious to see the warped overlap images and the pto files involved) - you can use the -p ('prolific') flag - you may get a lot of stuff this way. woa overwrites existing files without asking. When woa is used as a hugin plug-in, the intermediate files will show up in the space for temporary files, which is where hugin's working directory is (at least on my machine). When you use woa from the command line, they'll show up in the present working directory.

4. Using woa as a hugin plug-in

The current state of hpi, the hugin plug-in interface, is such that you can't pass any parameters to the plug-in via the GUI. You can use woa as a plug-in without any further ado if you accept the defaults coded into it. If you want to use the same parameters as in the command line version, you have to set up a file called woa.ini in your home directory - that would be /home/<your username> on Linux, or, in short, ~. In this file, you specify the parameters to use in Python configparser syntax, just what you know from any old .ini file. This is simple, the values are given in a: b or a = b notation and there's only one section called woa arguments; here's an example (all lines, including the [woa arguments] line and the comments starting with '#' are part of the file)

[woa arguments]

# use this prefix for intermediate files:

basename  = woa

# don't look at overlaps with overlap ratio greater than: [0.0 - 1.0]

ceiling   = 1.0

# use this CPG: [apsc or cpfind]

cpg       = cpfind

# blow exclude masks up by this amount of pixels:

margin    = 0

# set to true to keep all intermediate files:

prolific  = off

# scaling factor for warped images, 1.0 means 'optimal' size:

scale     = 0.25

# stop file - stop woa if this file is present

stop      = /tmp/stop_woa

# don't look at overlaps with overlap ratio less than: [0.0 - 1.0]

threshold = 0.01

# set to false to see less output

verbose   = true

As you can see, the names of the parameters are the same as the long names of the command line parameters. The syntax is quite flexible and should be obvious. You may wonder where the -i or -f parameters are. If you use woa as a plug-in, image selection is done differently to the CLI version. What counts is which of your images are 'active' (you can switch them on/off in the preview). If you have only one active image, it will be taken as the 'focus' image. If you have more, all possible pairs from the active images are looked at - up to looking at all possible image pairs when all images are active.

The plug-in may take a good while to do it's thing. During that time hugin will be unresponsive and currently you won't see a progress bar or such. On Linux, a good way to see the progress is starting hugin from the command line. Then you can simply switch to the terminal window you started hugin from and see all the console output from woa. On Windows, I've been told, this is not possible. On my system, if an error occurs, hugin displays a small dialog on completion, showing of the plugin's return code. The dialog isn't put into 'always on top' mode and may be hidden behind other windows, resulting in the impression that hugin has crashed. If this happens to you as well, Alt-tab through your windows to find the covered-up dialog (it's called 'Ergebnis' on my system, so it may be 'result' or so on yours) and Okay it. After that, you'll be back in hugin. If woa ran without a problem, the popup won't show and you'll simply have the new CPs, if any. There may be a great number of them - I had occasions where the numbers were in the tens of thousands - it depends on the parameters you use for woa and on the images. Very large numbers of CPs will slow down some operations in hugin considerably. If you're unhappy with what woa did to your project, you can undo it's effect.

Older versions of woa did terminate operation if any of the attempts to process an image pair ran into problems and threw an exception. Things can still go wrong - the double transformation used to project CPs from one source image into the other can be insufficiently precise for the algorithm to succeed, and there's other sources for errors which occasionally occur - but I put in an catch-all exception handler which will simply ignore any image pair for which an exception occurs, because usually most image pairs process just fine and it's really annoying if you already have good data for 100 pairs and just the 101st pair failing aborts the whole script. So now you may at times find that no CPs were found for some image pairs. The (verbose) output will make this known, but you'd have to look for the relevant message. If there was an exception, it's shown with a stack dump, so you can find where precisely it occurred.

5. What you can expect

Of course your mileage will vary, depending on the quality of your images, the lenses used, the content... just as in normal hugin use. And I haven't tested on other platforms or with really nasty corner cases. When I use woa with the defaults, I usually get up to the maximum number cpfind will produce with the given settings for each decent-sized overlap. Few of these are false matches, and most are pretty good - I usually optimize and then throw away the worst ones by 'cleaning' the CPs or doing it manually by a threshold. Since geometric distortions are minimized, if the content is up to it, the control points will be evenly distributed over the overlaps.

Using woa for simple panoramas with images all from one lens is often overkill; woa excels where you have multiple lenses in one panorama. I create mixed-lens panoramas quite regularly, and with woa I often get as many and as good CPs for pairs with different lenses as with pairs with the same lens, whereas the standard CPGs can have considerable difficulties matching images from different lenses. Combinations of fisheye images and rectilinear images can at times be very hard to detect with a standard CPG, whereas it's no problem for woa (in fact that was my main reason for making it).

6. Some use suggestions

The primary use for woa will be to augment your panorama with more, good, and evenly distributed control points. One situation when you really want as many of these as possible is lens calibration. If you have a 360X180 image set done with a well-set-up pano head in a rich content environment (like, inside an ornate building) and plenty of overlap, and do a woa run with large scale (like 1.0 or even more), you'll end up with literally thousands of control points. This is ideal for lens calibration: if you calculate a,b and c parameters from such an image set, they should be close to ideal. If the amount of CPs you get this way still isn't enough for your purpose, you can edit the parameters used for the CPG run in woa.py. Currently these parameters are hard-coded, so you have to touch the script if you want to change them. Look for the bit in the file which goes

        command = [ 'cpfind' ,

there the CPG commands are put together and you'll spot the familiar parameters. You may need root privileges to edit the installed woa.py; the default installation process on my machine puts woa.py in /usr/local/share/hugin/data/plugins and I need to use sudo to edit the file there. Of course you can have a user-editable copy anywhere you want, and this may be a good idea if you intend tinkering with the script.

Another situation where I found woa really useful is with combinations of different focal lengths. I do landscapes, and in landscapes you often have parts which are more interesting (mountain ranges on the horizon) than others (the clouds or the ground you're standing on). So an obvious thing to do is to shoot a 360X180 with a fisheye and use other lenses to capture interesting content in higher resolution. I often found it difficult to match the images from different lenses very precisely. With woa this becomes easy, since the overlap images are scaled to a joint size and geometry. I even get hundreds of good matches from my Samyang 8mm fisheye to the longest setting of my standard zoom lens (55mm, or about 80mm 35 mm equivalent), where without woa I sometimes get no matches at all. Having lots of good control points allows me to optimize for 'everything but translation' and come up with very good matches indeed.

Finally, you may only have sparse coverage, meaning that your overlaps are small and possibly only in the (distorted) corners and near the edges of your images. If you feed such a panorama to woa, you may still get sufficient control points for a very good placement, whereas without woa you may have trouble getting any control points at all.

7. Downsides

hsi/hpi is currently slightly exotic. I hope this will eventually change, so you can easily obtain binaries for the supported platforms. Until then, you may need hugin compile ability, plus the other bits and bobs I mentioned earlier.

One obvious drawback is the requirement of a pre-aligned panorama. If you're using a shooting pattern (like, 6 around, 1 up 1 down) and work reasonably precisely, you can just set the initial r,p and y values accordingly and start woa on that. I found that this approach works, even if you're a few degrees off. More than a few degrees off won't work well, and if the pre-placed images don't overlap, or the 'overlap' does not actually contain the same content, woa obviously can't find CPs. And when it comes to multi-lens panoramas, one option is to first process the images from each lens separately, then combine the ptos and place the connected subsets manually or with a few manually set CPs before using woa. I now trust woa to work pretty reliably and my routine action in this situation is actually to delete all CPs from the combined pto before I call woa - it will usually find better CPs than the ones which were already there, plus the inter-lens ones on top.

Another drawback I found was that nona takes longer to warp masked images than it takes for unmasked ones. This came as a surprise to me - I naively thought that if I was rendering only part of an image this should be faster than rendering the whole. With the current default mask step width of 100 pixels, the effect is not too noticeable, and having to look at smaller images with the CPG saves some time on the other end. Whatever you want it to do, this script isn't going to do it very quickly.

For very large panoramas, looking at all possible image combinations may take long. The test for overlap and the calculation and application of the masks take in the order of tens of milliseconds per image pair on my machine - the actual warping and generation of control points for those images that actually do overlap takes much longer, up to the order of magnitude of tens of seconds per pair in optimal size - but they are only executed if an overlap is detected. But still, if you have many images, even the first, fast tests will run up to some processing times. I may introduce linear matching or other more sophisticated pairing mechanisms later on - if you want to do it yourself, look at process_image_set() where the pairing is done. So you may want to use woa on cleverly chosen subsets of your images - keep in mind you can do that by specifying an image subset (or activate only some images in the preview) - or by setting a 'focus' image.

A large drawback is the interface with hugin. Hugin becomes unresponsive while woa runs, there is no progress bar, and if a problem occurs which makes woa terminate abnormally, the 'popup' window may be hard to find. And finally, all the console output is usually invisible if you don't use the trick to start hugin from the command line in the first place. Keep in mind, though, that woa is a standalone program as well - you can simply use it from the command line, and then you'll see all it's console output as well. Being able to use it as a plugin from hugin is a nice extra, but the integration is admittedly not very good.

Not really a drawback, but something to keep in mind is the way woa treats masks. If you have defined masks on your images, these masks will be honoured in the generation of the warped overlap images. This may lead to undesired effects - you may need differnt masking for running woa than to stitching the final panorama. The behaviour is deliberate - it allows you to prevent parts of your images to be submitted to woa's CP detection.

8. License

I've put woa under the GPL.
