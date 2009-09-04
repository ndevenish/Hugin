#ifndef GLOBALS_H
#define GLOBALS_H

#include <map>
#include <string>
#include <vector>
#include "vigra/diff2d.hxx"

extern unsigned int verbose;
extern unsigned int resize_dimension;
extern unsigned int detector;
extern unsigned int lens_type;
extern unsigned int current_line;
extern unsigned int optimise_centre;
extern unsigned int optimise_radius;
extern unsigned int poly_type;
extern unsigned int straighten_verts;
extern unsigned int ss;
extern double sensor_width;
extern double sensor_height;
extern double original_width;
extern double original_height;
extern double focal_length;
extern double pixel_density;
extern double focal_length_pixels;
extern double cropFactor;
extern std::string path;
extern std::string format;
extern double sizefactor;
extern double threshold;
extern double bt_threshold;
extern double length_threshold;
extern double min_line_length_squared;
extern double scale;
extern double tscale;
extern double a;	
extern double corner_threshold;
extern unsigned int gap_limit;	
extern unsigned int horizontal_slices;	
extern unsigned int vertical_slices;	
extern unsigned int generate_images;
extern unsigned int generate_images;
extern std::vector<std::vector<vigra::Point2D> > lines;
extern double sigma;
extern double r;
extern std::map<int,double> line_errors;
extern std::map<int,double> line_weights;
extern double RMS_error;
#endif

