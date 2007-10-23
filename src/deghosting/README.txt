-------------------------
TABLE OF CONTENTS
-------------------------

1. Introduction
2. Usage
	a. options
3. Helpful Tips
	a. deghosting small motion or crowded scenes
	b. improving deghosted results
	c. reducing noise
	d. checking progress
	e. large images and images that mostly overlap

-------------------------
INTRODUCTION
-------------------------

This program is the end product of Google's Summer of Code 2007. The up-to-date
version is included with the hugin program.

This program is a command line tool for creating and deghosting HDR panoramas.
This program requires a set of OpenEXR images that are geometrically and
radiometrically aligned, paired with a set of grayscale images that store the
original radiance of each image. The program allows three methods to combine
the images. This readme will focus on the Khan (default) algorithm since the
other two are for testing purposes.

The Khan algorithm reduces ghosts in the image by assuming that at any pixel,
most images represent the background, and that if a pixel's neighbors are
in the background, then the pixel is most likely also in the background. The
algorithm deghosts by aiming to use the background for the final image.

-------------------------
USAGE
-------------------------
Command:
hugin_hdrmerge [options] -o OUTPUT_FILE <INPUT_FILES>

OUTPUT_FILE is the path to the output file, which must be OpenEXR (.exr) format.
INPUT_FILES consist of aligned OpenEXR (.exr) images and an equal number of 
grayscale (e.g. pgm) images that store each input's radiance. The grayscale
images must be listed in the same order as the OpenEXR images.

==== A. OPTIONS ====

-m method: Specify which method to use to combine images. Can be avg, avg_slow, 
			or khan (default). This readme will only focus on options valid to 
			all and khan.
			- Options that apply to all: -v, -h
			- Options that apply with avg: -c
			- Options that apply with khan: -a -e, -i, -l, -s
	
-a calc: Specify which calculation should be performed in addtion to or in
			conjunction with the basic Khan algorithm. Can be one or more of
			b, c, d, h, m, s, or u.
			b: After all iterations, distribute the weights logarithmically
			c: After all iterations, choose the pixel with the largest weight
				in each layer instead of blending all layers according to weight
			d: Same as c, but only when the smallest weight is within 10% of
				the largest weight
			h: When calculating initial weights, favor a high signal to noise
				ratio (radiance closer to 220/255)
			i: ignore alpha channels of input files

-e : Export calculated initial weights to source image folder under the name
		<source_image>_iw.jpg

-i num: Specify the number of iterations to execute. The more iterations, the
		better the result may be, but each iteration takes a long time. Default 
		is 1 iteration.

-l: Import previously exported weights from source image folder under the name
	<source_image>_iw.jpg
		
-s op: Specify whether to save intermediary files for debugging. Can be one or
		more of a, r, s, or w.
		a: save all files. "-s a" is Equivalent to "-s rsw"
		r: resulting HDR images from each iteration in OpenExr format
		s: source images and alpha images
		w: weights calculated for each input at each iteration

-------------------------
HELPFUL TIPS
-------------------------

For most images, running 9 to 11 iterations of the algorithm should produce
a good result. Here are some tips to improve results and decrease run time.

==== A. DEGHOSTING SMALL MOTION OR CROWDED SCENES ====

The Khan algorithm doesn't produce very good result when used on scenes where
the background isn't presented in a majority of the sources. Some examples are
scenes with water ripples, small tree branch movements, and crowded places. The
fact that a 0 in the initial weight results in a 0 in final weight can be taken
advantage of to solve this problem.

Method A: modifying initial weights
1. run the program with -e and -l options
2. when a message that initial weights have been exported appears on screen,
   go to the source images' folder and open all files under the name
   <source_image_name>_iw.jpg
3. choose an image where the area to be deghosted is closest to the desired 
   result (the next step is going to make sure that only this image is
   considered for the final result of this area)
4. in all *other* images, color that area black and save the images
5. enter 'c' on the command line to load the files that were modified

Method B: modifying input radiance images
1. open all grayscale images that indicate original radiance values
2. color areas with undesirable objects black so that they are not considered
   when merging the images
3. run the program as usual

==== B. IMPROVING DEGHOSTED RESULTS ====

If, after many iterations, there is still traces of ghosting, try running
the program with the option -b , -d, or -c individually. With these options,
more definitive results can be produced in less iterations.

==== C. REDUCING NOISE ====

This program can also be used to combine input images that have a lot of noise.
Just run the program without using -c or -d. 

==== D. CHECKING PROGRESS ====

There are several ways to be informed of how far the program is at processing
the images.

Method A: displaying progress
run the program with "-v", or "-v -v" for large/many images

Method B: displaying results
run the program with "-s r" to see results for each iteration stored at the
folder where the program is run, so if it doesn't look right, 
you don't have to wait until the end to find out.

==== E. LARGE IMAGES AND IMAGES THAT MOSTLY OVERLAP ====

To save memory when processing large images or images with a lot of overlap,
run the program with the -a i option to discard the alpha channel information
during processing. Discarding the alpha channel causes the program to consider
all the pixels in the image - valid or not, but if the input images mostly
overlap, then there isn't a lot of varying opacity from image to image. The
alpha channel takes almost as much memory as the input images to store, so,
especially when input images are extremely large, discarding the alpha channel
saves a lot of memory.
	
-------------------------
Updated: August 20, 2007
