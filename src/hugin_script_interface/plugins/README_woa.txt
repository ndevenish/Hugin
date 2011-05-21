woa - generation of control points from prealigned images

copyright Kay F. Jahnke 2011

You're free to spread this file unmodified.

1. Introduction

In panography, finding good control points between images isn't always easy. The automatic control point generators (CPGs) often do a good job, but there are situations where it's desirable to find more, better or more evenly distributed control points. The straightforward approach - to simply feed the images into a CPG, doesn't work very well in certain situations:

- the parts of the images to be matched are very distorted
  (often the case with the edges of fisheye images)

- the images have been taken with different lenses, so their scale
  and/or lens geometry is different

The CPGs can cope with these problems to some extent - their feature detectors aim to be invariant to affine transformations, which are similar locally to lens distortions and change of focal length. Nevertheless, this invariance only goes so far. I am using a computationally slightly expensive, yet effective and pragmatic approach to minimize problems resulting from lens geometry and focal length, which works on pre-aligned images. The method is easily explained: for each pair of overlapping images,

- Take the overlap as your target area.

- Choose a projection where this area is quite undistorted.

- Calculate the partial image for each image that corresponds
  to the target area, using the same projection (warp).

- Once you have these warped overlap images, they should be quite
  similar and undistorted. Therefore they are good source material
  for a CPG. Use the CPG on the warped overlap images.

- Transform the control points you have obtained back to the
  original image coordinates.

The script performs these operations using hsi, the hugin scripting interface. The source material has to be in a pto file, the images must be prealigned, be it automatically, be it manually - the prealignment can be sloppy, but the overlap calculation is very precise, and if matching bits are outside the overlap, they'll not be found - and the better the prealignment, the less dissimilar the warped overlap images will turn out, resulting in better control points. It's even quite feasible to use woa iteratively to approach an ideal solution.

What I usually do is run cpfind on the images and do a r,p,y optimization. This is quite fast and precise, and I have a good base to start from. Next I make sure that I don't have disconnected groups - if I do I manually add a few control points and optimize, or I manually move the disconnected image groups in place to get the rough placement. Then I use woa in a second step to find many more, good quality, evenly distributed control points.

2. what's needed to run the script

As far as your prealigned images are concerned, I've developed and used woa for panoramas, not mosaics. You may have some success with mosaics, but really it's a tool for panoramas and uses equirectangular projection.

The most important bit to run the script - apart from Python - is hsi, the hugin scripting interface. This is a Python module making most of hugin's backend functionality available to Python. This module is not yet available in binary form, you have to compile hugin's python_scripting branch yourself. Currently, only Linux and Windows are supported. I've developed and tested the script on Kubuntu 10.10 with bleeding-edge hugin compiles (2011.X) and Python 2.6, I hope it will work on other platforms.

If you're running woa as a hugin plugin, you'll need hugin with built-in hpi (hugin plugin interface). This is the hugin you get when you compile with the -DBUILD_HSI=ON cmake switch - it has an embedded Python interpreter, and you can pick woa.py as a plugin to be used on your currently loaded project.

You also need tiffdump, which is part of the libtiff package. This is needed to cope with the cropped TIFF images nona produces. You should set hugin to make nona output cropped TIFFs - I think it's the default, but do check.

You also need python-argparse. This is used to process the command line - I think it'll become part of Python's standard library, but it isn't yet as of this writing.

As a CPG, you need autopano-sift-c or cpfind. I currently recommend autopano-sift-c, since it generates large amounts of evenly-distributed control points when used with the parameters it's fed by woa. I have failed to get cpfind to do the same (I had hoped to be able to do so by using --fullscale, but there seems to be an issue here and I only get 25 points). Cpfind comes with hugin and is FOSS, autopano-sift-c is patent-encumbered, and you may be restricted in your use of it, please refer to the message it outputs and figure out if you may or may not use it.

Tiffdump, nona and the CPG have to be in your system path.

3. Calling the script from the command line

If you call woa.py from the command line without arguments, it will print a brief help:

usage: woa1.py [-h] [-c <overlap ceiling>] [-g <cpfind or apsc>]
               [-f <image number>] [-i <image number> [<image number> ...]]
               [-m <margin>] [-o <output file>] [-b <base name>] [-p]
               [-t <overlap threshold>] [-s <scaling factor>] [-v]
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

I'll explain the parameters in turn.

The most important parameter, and the only one which isn't prefixed by an argument name, is the pto file.

At times you may not want the original pto file to be modified, in which case you can specify another file as output by using -o.

Per default, woa will look at all possible image pairs and test them for overlaps. You can limit the search to a subset by specifying the image numbers after the -i flag. Numbering starts with 0, as in hugin.

Sometimes, you just want to find (more) control points for one specific image. That's when you use the -f (focus) parameter. If you specify a 'focus' image, only overlaps with this image will be analyzed and used to make control points.

The scale of the warped overlap images can be varied with the -s flag. Using 1.0 here is analogous to hugin's 'optimal scale'. The default is .25, which I found good enough for most cases - I'm using a 12MP APSC DSLR sensor.

The threshold and ceiling parameters (-t and -c) determine which size overlaps are considered. They sensibly take values between 0 and 1 which refer to the fraction of the image that overlaps. I use a low threshold of about 0.01 to catch even small overlaps and a ceiling of 1.0, so that even total overlaps are processed. Depending on your specific use scenario you may want to use different values here: If your images are all of the same lens and focal length, you may want to raise the threshold, and if you're confident that well-overlapping images are already linked by enough control points, you may want to lower the ceiling.

The -g flag chooses the CPG to use. As I said above, I currently recommend using autopano-sift-c, but beware of the use restrictions and make sure you qualify.

If you use the -v flag, woa will become more verbose. Some of the output you see is from external programs (like nona and the CPG), and you can't get rid of that.

woa produces lots of intermediate data. Usually, you'll only see the last few, which I don't remove because I'm lazy - the intermediate files conventionally begin with 'woa', but you can choose a different prefix with the -b flag. If you want woa to keep all intermediate files (maybe because you're curious to see the warped overlap images) - you can use the -p flag. woa mercilessly overwrites existing files.

4. Using woa as a hugin plugin

The current state of hpi, the hugin plugin interface, is such that you can't pass any parameters to the plugin via the GUI. You can use woa as a plugin without any further ado if you accept the defaults coded into it. If you want to use the same parameters as in the command line version, you have to set up a file called woa.ini in your home directory - that would be /home/<your username> on Linux. In this file, you specify the parameters to use in Python configparser syntax. This is simple, the values are given in a: b or a = b notation and there's only one section called woa arguments; here's an example:

[woa arguments]

# use this prefix for intermediate files:

basename  = woa

# don't look at overlaps with overlap ratio greater than: [0.0 - 1.0]

ceiling   = 1.0

# use this CPG: [apsc or cpfind]

cpg       = apsc

# blow exclude masks up by this amount of pixels:

margin    = 0

# set to true to keep all intermediate files:

prolific  = off

# scaling factor for warped images, 1.0 means 'optimal' size:

scale     = 0.5

# don't look at overlaps with overlap ration less than: [0.0 - 1.0]

threshold = 0.01

# set to false to see less output

verbose   = true

As you can see, the names of the parameters are the same as the long names of the command line parameters. The syntax is quite flexible and should be obvious. You may wonder where the -i or -f parameters are. If you use woa as a plugin, image selection is done differently to the CLI version. What counts is which of your images are 'active' (you can switch them on/off in the preview). If you have only one active image, it will be taken as the 'focus' image. If you have more, all possible pairs from the active images are looked at - up to looking at all possible image pairs when all images are active.

The plugin may take a good while to do it's thing. During that time hugin will be unresponsive and currently you won't see a progress bar or such. On Linux, a good way to see the progress is starting hugin from the command line. Then you can simply switch to the terminal window you started hugin from and see all the console output from woa and the programs it uses. On Windows, I've been told, this is not possible. On my system, hugin shows a small dialog on completion, informing me of the plugin's return code. The dialog isn't put into 'always on top' mode and may be hidden behind other windows, resulting in the impression that hugin has crashed. If this happens to you as well, Alt-tab through your windows to find the covered-up dialog and Okay it. After that, you'll be back in hugin with the newly generated CPs. There may be a great number of them - I had occasions where the numbers were in the tens of thousands - it depends on the parameters you use for woa and the images. Very large numbers of CPs will slow down some operations in hugin considerably. If you're unhappy with what woa did to your project, you can undo it's effect. 

5. What you can expect

Of course your milage will vary, depending on the quality of your images, the lenses used, the content... just as in normal hugin use. And I haven't tested on other platforms or with nasty corner cases. When I use woa with the defaults, I usually get about a couple of hundred control points for each decent-sized overlap. Some of these are false matches, but most are pretty good - I usually optimize and then throw away the worst ones by 'cleaning' the CPs or doing it manually by a threshold. Since geometric distortions are minimized, if the content is up to it, the control points will be evenly distributed over the overlaps.

6. Some use suggestions

The primary use for woa will be to augment your panorama with more, good, and evenly distributed control points. One situation when you really want as many of these as possible is lens calibration. If you have a 360X180 image set done with a well-setup pano head in a rich content environment (like, inside an ornate building) and plenty of overlap, and do a woa run with large scale (like 1.0), you'll end up with literally thousands of control points. This is ideal for lens calibration: if you calculate a,b and c parameters from such an image set, they should be close to ideal.

Another situation where I found woa really useful is with combinations of differnt focal lengths. I do landscapes, and in landscapes you often have parts which are more interesting (mountain ranges on the horizon) than others (the clouds or the ground you're standing on). So an obvious thing to do is to shoot a 360X180 with a fisheye and use other lenses to capture interesting content in higher resolution. I often found it difficult to match the images from different lenses very precisely. With woa this becomes easy, since the overlap images are scaled to a joint size and geometry. I even get hundreds of good matches from my Samyang 8mm fisheye to the longest setting of my standard zoom lens (55mm, or about 80mm 35 mm equivalent), where without woa I sometimes get no matches at all. Having lots of good control points allows me to optimize for 'everything but translation' and come up with very good matches indeed.

Finally, you may only have sparse coverage, meaning that your overlaps are small and possibly only in the (distorted) corners and edges of your images. If you feed such a panorama to woa, you may still get sufficient control points for a very good placement, whereas without woa you may have trouble getting any control points at all.

7. Downsides

hsi/hpi is currently slightly exotic. I hope this will soon change, so you can easily obtain binaries for the supported platforms. Until then, you'll have to have hugin compile ability, plus the other bits and bobs I mentioned earlier.

One obvious drawback is the requirement of a prealigned panorama. If you're using a shooting pattern (like, 6 around, 1 up 1 down) and work reasonably precisely, you can just set the initial r,p and y value accordingly and start woa on that. I found that this approach works, even if you're a few degrees off.

Another drawback I found was that nona takes longer to warp masked images than it takes for unmasked ones. This came as a surprise to me - I thought that if I was rendering only part of an image this should be faster than rendering the whole. With the current default mask step width of 100 pixels, the effect is not too noticable, and having to look at smaller images with the CPG saves some time on the other end. Whatever you want it to do, this script isn't going to do it very quickly.

For very large panoramas, looking at all possible image combinations may take long. The test for overlap and the calculation and application of the masks take in the order of hundreds of miliseconds per image pair on my machine - the actual warping and generation of control points for those images that actually do overlap takes much longer, in the order of magnitude of tens of seconds per pair in optimal size - they are only executed if an overlap is detected. But still, if you have many images, even the first, fast tests will run up to long processing times. I may introduce linear matching or other more sophisticated pairing mechanisms later on - if you want to do it yourself, look at process_image_set() where the pairing is done.

8. The hidden agenda

You're still with me? You like the idea and want to try woa? That's what I intend. Because now you'll want hsi/hpi for your machine. Why would I want you to want that? Because I've written it, and I would like to see it distributed and used. If you don't have hugin compile ability, ask around, maybe someone else can help you with a binary for your machine. Or you might be enticed to go ahead and compile yourself - I can assure you that at least on Linux it isn't too hard if you follow the wiki. If you can wait, hopefully it won't be too long until binaries become available for download. If you can't get it (yet) I apologize for putting out the bait and leaving you disappointed.

9. License

I've put woa under the GPL.