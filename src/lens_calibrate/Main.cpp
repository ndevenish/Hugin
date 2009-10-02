/***************************************************************************
 *   Copyright (C) 2008 by Tim Nugent                                      *
 *   timnugent@gmail.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <string.h>
#include <fstream>
#include <vector>
#include <string>
#include "Globals.h"
#include "ProcessImage.h"
#include "vigra/diff2d.hxx"
#include "MapPoints.h"
#include "Straighten.h"
#include <hugin_version.h>

using namespace std;

static void parse_pto(string& f,vector<string>& images,vector<string>& pto_file_top,vector<string>&
pto_file_cps,vector<string>& pto_file_end, unsigned int& image_number){
 
 	// Open the .pto file and make sure it's good to go
	ifstream is(f.c_str());

  	if(!is.good()){
    		cout << "Couldn't open Hugin project file " << f << endl << endl;
    		exit(1);
  	}

        // Get line from stream
	string line;
	unsigned int current = 0, counter = 0, at_cps = 0, done_cps = 0;
		
        while(getline(is,line)){
		
		string start = line.substr(0, 1);
		string autopano_start = line.substr(0, 9);

		if (at_cps == 0){
			pto_file_top.push_back(line);
		}

		if (start == ("i") || autopano_start == ("#-imgfile")){
			
			// Parse images from lines beginning 'i'
			
			//vector<string> tokens;
			//tokenize(line, tokens);
			//string imagename = tokens.back().substr(2, tokens.back().length()-3);
			
			size_t found;
			string imagename = ("");
			
			// Hugin files
			if (start == ("i")){
									
				found = line.find("\"");
				imagename = line.substr(found + 1, line.length() - found - 2);
				//cout << imagename << endl;
				
			// (Old?) Autopano Pro files
			}else{
			
				found = line.find("\"");
				imagename = line.substr(found + 1, line.length() - found - 2);
			
			}
			
			if (fileexists(imagename)){
			
				// Put filenames into a vector
				//if(current == image_number) images.push_back(imagename);
				// Put all filenames into a vector
				images.push_back(imagename);
		
			// Seach for the image if we can't find it, using path of PTO file		
			}else{
			
				//cout << imagename << " doesn't exist." << endl;
			
				vector<string> tokens;
				imagename = ("");

				// *nix file paths
				if (f.find("/") < f.length() || f.substr(0, 1) == ("/")){

					if (f.substr(0, 1) == ("/")){
						imagename += ("/");
					}
					
					tokenize(f, tokens, "/");
					for (unsigned int p = 0; p < tokens.size() - 1; p++){
						imagename += tokens[p];
						imagename += "/";
					}
					imagename += line.substr(found + 1, line.length() - found - 2);
				
				// Windows file paths... experimental!	
				}else{

					tokenize(f, tokens, "\\");
					for (unsigned int p = 0; p < tokens.size() - 1; p++){
						imagename += tokens[p];
						imagename += "\\";
					}
					imagename += line.substr(found + 1, line.length() - found - 2);

				}
				
				//cout << "New search path " << imagename << endl;
				
				if (fileexists(imagename)){
								
					// Put filenames into a vector				
					if(current == image_number) images.push_back(imagename);
				
				}else{
				
					cout << "Couldn't open image " << line.substr(found + 2, line.length() - found -
					3) << endl;
					
					cout << "Also tried " << imagename << endl << endl; 
					exit(1);
				}
			}

			//cout << imagename << endl;
			current++;
			
		}else if (start == ("c")){
		
			pto_file_cps.push_back(line);
			//cout << line << endl;
		
			// Marker - we're at CPs
			at_cps = 1;
		}

		if (at_cps && start != ("c")){
		
			pto_file_end.push_back(line);
		
		}
		
	}

	is.close();

}

static void write_pto(string& output_pto,vector<string>& pto_file_top,vector<string>& pto_file_cps,vector<string>& pto_file_end, unsigned int& pto_image, unsigned int& cps_per_line, map<int,int>& lines_to_image_number){

	// File to write to
	ofstream out;
	out.open (output_pto.c_str());

  	if(!out.good()){
    		cout << "Couldn't write to Hugin project file " << output_pto << endl << endl;
    		exit(1);
  	}
	
	// Print the top of the file
	for (unsigned int l = 0; l < pto_file_top.size(); l++){	
		out << pto_file_top[l] << endl;	
	}
	for (unsigned int l = 0; l < pto_file_cps.size(); l++){
			out << pto_file_cps[l] << endl;				
	}

	// Add lines
	out << endl;
	int start = 1;
	double invert_size_factor = 1.0/sizefactor;
	for (unsigned int l = 0; l < lines.size(); l++){
		
		unsigned int line_count = l+3;		
		
		double interval = (lines[l].size()-1)/(1.0*cps_per_line);
		
		for(int i = 0; i < cps_per_line; i++){		
					int start = (int)(i * interval);
			int stop =  (int)((i+1) * interval);
		
			//cout << l << ":" << i << " - " << start << ": " << lines[l][start]->x << " - " << start << ":" << lines[l][start]->y << endl;
			//cout << l << ":" << i << " - " << stop << ": " << lines[l][start]->x << " - " << stop << ":" << lines[l][stop]->y << endl << endl;
				
			double x1 = lines[l][start]->x*invert_size_factor;
			double y1 = lines[l][start]->y*invert_size_factor;
			double x2 = lines[l][stop]->x*invert_size_factor;
			double y2 = lines[l][stop]->y*invert_size_factor;
			
			out << "c n" << lines_to_image_number[l] << " N" << lines_to_image_number[l] << " x" << x1 << " y" << y1;	
			out << " X" << x2 << " Y" << y2 << " t" << line_count << endl;		
		}		
		
		/*
		int d1 = abs(lines[l][0]->x - lines[l][lines[l].size()-1]->x);
		int d2 = abs(lines[l][0]->y - lines[l][lines[l].size()-1]->y);
		
		if (d1 > d2){		
			int segment_length = d1/(cps_per_line-1);
			for (int x = lines[l][0]->x; x < lines[l][0]->x+(2*cps_per_line*segment_length); x+= segment_length){
				int y = 0;
				for (unsigned int i = 0; i < lines[l].size(); i++){
					if (lines[l][i]->x == x) y = lines[l][i]->y;
				} 			
				int nx = (int)(x*invert_size_factor);
				int ny = (int)(y*invert_size_factor);	
				//if (y) out << "c n" << pto_image << " N" << pto_image << " x" << nx << " y" << ny << " X" << nx << " Y" << ny << " t" << line_count << endl;
				if (y){				
					//cout << start <<endl;
					if(start){
						out << "c n" << lines_to_image_number[l] << " N" << lines_to_image_number[l] << " x" << nx
							<< " y" << ny;
						start = 0;
				 	}else{
						out << " X" << nx << " Y" << ny << " t" << line_count << endl;
						start = 1;
					}
				 }
			}		
		}else{

			int segment_length = d2/(cps_per_line-1);
			for (int y = lines[l][0]->y; y < lines[l][0]->y+(2*cps_per_line*segment_length); y+= segment_length){
				int x = 0;
				for (unsigned int i = 0; i < lines[l].size(); i++){
					if (lines[l][i]->y == y) x = lines[l][i]->x;
				} 				
				int nx = (int)(x*invert_size_factor);
				int ny = (int)(y*invert_size_factor);
				//if (x) out << "c n" << pto_image << " N" << pto_image << " x" << nx << " y" << ny << " X" << nx << " Y" << ny << " t" << line_count << endl;
				if (x){
					//cout << start <<endl;
					if(start){
						out << "c n" << lines_to_image_number[l] << " N" << lines_to_image_number[l] << " x" << nx
							<< " y" << ny;
						start = 0;
					}else{
						out << " X" << nx << " Y" << ny << " t" << line_count << endl;
						start = 1;
					}
				}
			}			
		}
		*/
	
	}
	// Print bottom of file
	for (unsigned int l = 0; l < pto_file_end.size(); l++){	
		out << pto_file_end[l] << endl;	
	}

	out.close();

}

void write_lines_to_file(string& filename1, string& filename2){

	// Create output file name
	string output = path;
	vector<string> tokens;

	if (filename1 == filename2){
		// *nix file paths
		if (filename2.find("/") < filename2.length() || filename2.substr(0, 1) == ("/")){					
			tokenize(filename2, tokens, "/");
			output.append(tokens[tokens.size()-1].substr(0,tokens[tokens.size()-1].length()-4));		
		// Windows file paths
		}else{
			tokenize(filename2, tokens, "\\");
			output.append(tokens[tokens.size()-1].substr(0,tokens[tokens.size()-1].length()-4));
		}
		output.append(".lines");
	}else{			
		// *nix file paths
		if (filename1.find("/") < filename1.length() || filename1.substr(0, 1) == ("/")){					
			tokenize(filename1, tokens, "/");
			output.append(tokens[tokens.size()-1].substr(0,tokens[tokens.size()-1].length()-4));			
		// Windows file paths
		}else{
			tokenize(filename1, tokens, "\\");
			output.append(tokens[tokens.size()-1].substr(0,tokens[tokens.size()-1].length()-4));
		}
		output.append("-");
		// *nix file paths
		if (filename2.find("/") < filename2.length() || filename2.substr(0, 1) == ("/")){					
			tokenize(filename2, tokens, "/");
			output.append(tokens[tokens.size()-1].substr(0,tokens[tokens.size()-1].length()-4));		
		// Windows file paths
		}else{
			tokenize(filename2, tokens, "\\");
			output.append(tokens[tokens.size()-1].substr(0,tokens[tokens.size()-1].length()-4));
		}
		output.append(".lines");			
	}


	// File to write to
	ofstream out;
	out.open (output.c_str());

  	if(!out.good()){
    		cout << "Couldn't write lines to file " << output << endl << endl;
    		exit(1);
  	}else{
		cout << "Writing lines to file " << output << endl << endl;
	}
	
	out << "# Lens type:\t\t\t" << lens_type << endl;
	out << "# Size factor:\t\t\t" << sizefactor << endl;
	out << "# Crop factor:\t\t\t" << cropFactor << endl;		
	out << "# Focal length (mm):\t\t" << focal_length << endl;
	out << "# Focal length (pixels):\t" << focal_length_pixels << endl;	
	out << "# Original width:\t\t" << original_width << endl;
	out << "# Original height:\t\t" << original_height << endl;	
	for (int i = 0; i < lines.size(); i++){		
		for (int j = 0; j < lines[i].size(); j++){		
			out << i+1 << "\t" << lines[i][j]->x << ","<< lines[i][j]->y << endl;
		}	
	}
	out.close();

}

bool parse_lines_file(string& lines_file){

       	ifstream is(lines_file.c_str());
        // Check to make sure the stream is ok
        if(!is.good()){
                cout << "Cannot open file "<< lines_file << endl << endl;
                return false;
	}else{
		cout << "Reading input file " << lines_file << "..." << endl << endl;
	}

        string line;
        char ch[150];       
        unsigned int l,lt,x,y,ow,oh;
	float fl,sf,flp,flmm,cf;

        // Get line from stream
        while(getline(is,line)){
                // copy the string to a character array
                strcpy(ch, line.c_str());			
                if(sscanf(ch, "%d %d%*c%d", &l,&x,&y) == 3){		
			if(lines.size() < l){
				vector<vigra::Point2D> tmp;
				lines.push_back(tmp);
			}
			lines[l-1].push_back(vigra::Point2D((int)x,(int)y));	
                }else if (sscanf(ch, "# Focal length (pixels): %e", &fl) == 1){
			focal_length_pixels = fl;
			cout << "Found focal length (pixels):\t" << fl << endl;
		}else if(sscanf(ch, "# Lens type: %d", &lt) == 1){
			cout << "Found lens type "<<lt<<":\t\t";
			if (lt == 7){
				cout << "Universal model";
			}else if (lt == 6){
				cout << "Generic sine model";
			}else if (lt == 5){
				cout << "Generic tangent model";
			}else if (lt == 4){
				cout << "Stereographic fisheye";
			}else if (lt == 3){
				cout << "Equal-area fisheye";
			}else if(lt == 2){
				cout << "Equal-angle fisheye";
			}else if(lt == 1){
				cout << "Rectilinear";
			}else{
				cout << "Other";
			}
			cout << endl;			
			lens_type = lt;
		}else if(sscanf(ch, "# Size factor: %f", &sf) == 1){
			cout << "Found size factor:\t\t" << sf << endl;
			sizefactor = sf;
		}else if(sscanf(ch, "# Crop factor: %f", &cf) == 1){
			cout << "Found crop factor:\t\t" << cf << endl;
			cropFactor = cf;
		}else if(sscanf(ch, "# Focal length (mm): %f", &flmm) == 1){
			cout << "Found focal length (mm):\t" << flmm << endl;
			focal_length = flmm;
		}else if(sscanf(ch, "# Focal length (pixels): %f", &flp) == 1){
			cout << "Found focal length (pixels):\t\t" << flp << endl;
			focal_length_pixels = flp;
		}else if(sscanf(ch, "# Original height: %d", &oh) == 1){
			cout << "Found original height:\t\t" << oh << endl;
			original_height = oh;
		}else if(sscanf(ch, "# Original width: %d", &ow) == 1){
			cout << "Found original width:\t\t" << ow << endl;
			original_width = ow;
		}
        }	

	pixel_density = focal_length_pixels / focal_length;
	is.close();
	cout << endl;
/*
	for (int i = 0; i < lines.size(); i++){
		cout << "Found line " << i+1 << ". Pixels:\t\t" << lines[i].size() << endl;	
	}	
*/
	cout << endl;
	return true;
}

static void usage(){
  // Print usage and exit
	cout << "Usage: calibrate_lens [options] image1 image2.." << endl << endl;
	cout << "Options:" << endl << endl;
	cout << "  -p <filename>   Input Hugin PTO file." << endl;	
	//cout << "  -i <int>        Image in Hugin PTO file to process. Default 0" << endl;
	cout << "  -j <filename>   Input file containing line data." << endl;
	//cout << "  -c <int>        Control points to add to PTO file per line. Default 10." << endl;
	//cout << "  -e <int>        Edge detector. 1 = Canny (default)" << endl;
	//cout << "                                 2 = Shen-Castan" << endl;
	//cout << "                                 3 = Boundary tensor" << endl;
	//cout << "  -a <int>        Use ShortStraw algorithm instead of boundary tensor. Default 0" << endl;
	cout << "  -d <int>        Maximum dimension for re-sized image prior to processing. Default 1600" << endl;
	//cout << "  -f <string>     Output file format. Default .JPG" << endl;
	//cout << "  -o <path>       Output path. Default output/" << endl;
	//cout << "  -s <float>      Edge detector scale. Default 2" << endl;
	cout << "  -t <float>      Edge detector threshold. Default 4" << endl;
	//cout << "  -b <float>      Boundary tensor scale. Default 1.45" << endl;
	//cout << "  -b <float>      Boundary tensor threshold. Default 75" << endl;
	//cout << "  -g <int>        Gap in pixels permitted within a line. Default 4" << endl;
	cout << "  -m <float>      Minimum line length as a fraction of longest dimension. Default 0.1" << endl;	
	//cout << "  -m <float>      Sigma parameter for hourglass filter. Default 1.4" << endl;
	cout << "  -y <float>      Corner threshold. Default 150" << endl;
	//cout << "  -i <float>      Pixel density in pixels/inch. Default 4000" << endl;	
	cout << "  -f <float>      Focal length (mm). Read from EXIF data." << endl;	
	cout << "  -c <float>      Crop factor. Read/calculated from EXIF data." << endl;
	cout << "  -s <float>      Shortest dimension of sensor (mm), used to calculate crop factor." << endl;
	cout << "  -l <int>        Lens model  1 = Rectilinear (default)" << endl;
	cout << "                              2 = Equal-area fisheye" << endl;
	cout << "                              3 = Equal-angle fisheye" << endl;
	cout << "                              4 = Stereographic fisheye" << endl;
	cout << "                              5 = Orthographic fisheye" << endl;
	cout << "                              6 = Generic fisheye" << endl;
	cout << "                              7 = Universal" << endl;

//	cout << "  -x <int>        Radial poly 1 = square + quartic (a,c) only (default)" << endl;
//	cout << "                              2 = add cubic (Panotools a,b,c)" << endl;
//	cout << "                              3 = add cubic & quintic (a,b,c,d)" << endl;	
	
	//cout << "  -i <0|1>        Generate intermediate images. Default 1" << endl;
	cout << "  -z <0|1>        Optimise image centre. Default 1" << endl;
//	cout << "  -w <0|1>        Straighten image using vertical lines. Skips lens optimisation. Default 0" << endl;
	cout << "  -v <0|1>        Verbose. Default 0" << endl;
	cout << "  -h              Print usage." << endl;
	cout << endl; 
	exit(1);

}

int main(int argc, const char* argv[]){

	cout << endl << "Lens calibration tool" << endl;
	cout << endl << "Version " << DISPLAY_VERSION << endl << endl;

	unsigned int i = 1;
	unsigned int pto_image = 0;
	unsigned int cps_per_line = 10;
	string pto_file = (""),output_pto = (""),lines_file = ("");
	vector<string> images,pto_file_top,pto_file_cps,pto_file_end;
	map<int,int> lines_to_image_number;

	// Deal with arguments
        while(i < argc){

                if( argv[i][0] == '-'){
    
    			if (argc == 2){
				usage();
			}
    
                        i++;
                        switch(argv[i-1][1]){

                                // Put some more argument options in here later
                                case 'h' : {usage();}
				case 'o' : {path = argv[i]; break;}
				case 'p' : {pto_file = argv[i]; break;}
				case 'j' : {lines_file = argv[i]; break;}
				case 'i' : {pto_image = atoi(argv[i]); break;}
				//case 'c' : {cps_per_line = atoi(argv[i]); break;}
				//case 'f' : {format = argv[i]; break;}				
				//case 'e' : {detector = atoi(argv[i]); break;}
				//case 'g' : {gap_limit = atoi(argv[i]); break;}				
				//case 'x' : {poly_type = atoi(argv[i]); break;}
				case 'z' : {optimise_centre = atoi(argv[i]); break;}
				case 'v' : {verbose = atoi(argv[i]); break;}
				//case 'w' : {straighten_verts = atoi(argv[i]); break;}
				case 'd' : {resize_dimension = atoi(argv[i]); break;}
				//case 's' : {scale = atof(argv[i]); break;}
				case 's' : {cropFactor = 24.0/atof(argv[i]); break;}
				//case 'b' : {tscale = atof(argv[i]); break;}
				//case 'a' : {a = atof(argv[i]); break;}
				case 'a' : {ss = atoi(argv[i]); break;}
				case 't' : {threshold = atof(argv[i]); break;}
				case 'b' : {bt_threshold = atof(argv[i]); break;}
				case 'y' : {corner_threshold = atof(argv[i]); break;}
				case 'm' : {length_threshold = atof(argv[i]); break;}
				case 'c' : {cropFactor = atof(argv[i]); break;}
				//case 'i' : {pixel_density = atof(argv[i]); break;}
				case 'l' : {lens_type = atoi(argv[i]); break;}
				case 'f' : {focal_length = atof(argv[i]); break;}
				//case 'm' : {sigma = atof(argv[i]); break;}
				case 'r' : {r = atof(argv[i]); break;}	
                        }

   
                }else{
			images.push_back(argv[i]);
                }

                i++;
        }

		if (gap_limit < 2) gap_limit = 2;

	// Set output .pto filename if not given
	if (pto_file != ("")){	
		output_pto = pto_file.substr(0,pto_file.length()-4).append("_calibrate.pto");
		// Process PTO file
		if (fileexists(pto_file)){
  			//cout << "Parsing Hugin project file " << pto_file << endl;
			parse_pto(pto_file,images,pto_file_top,pto_file_cps,pto_file_end,pto_image); 
		}else{
			cout << endl << "Couldn't open Hugin project file " << pto_file << endl << endl; 
    			exit(1);		
		}
	}

  // read saved lines
	if (lines_file != ("") && parse_lines_file(lines_file) ){
		images.clear();
	}

	if (images.size() == 0 && lines.size() == 0){
		//cout << "No images provided!" << endl << endl;
		usage();
	}

	int plotted = 0;
	for(i = 0; i < images.size();i++){				
		ifstream ifile(images[i].c_str());
		if (!ifile){
			cout << "File " << images[i] << " doesn't exist." << endl << endl;
			exit(1);
		}
		cout << "Processing image " << images[i].c_str() << endl << endl;
		int previous = plotted;
		process_image(images[i],plotted);
		
		// Use this map for writing lines to PTO file
		for(int j = previous; j < plotted; j++){
			lines_to_image_number[j] = i;
			//cout << "Line " << j << " img no.:" << i << endl;
		}	
		previous = plotted;	
	}

	cout << "Found "<< lines.size() <<" lines."<< endl << endl;	

	// Save lines found in images 
	if (images.size()){
		write_lines_to_file(images[0],images[images.size()-1]);	
	}	

	// Map points and optimise parameters		
	if (lines.size() >= 12 ){		
		map_points();		
	}else{		
		cout << "Not enough lines to fit parameters. "<< endl << endl;		
		return 3;	
	}

	if (pto_file != ("")){
		write_pto(output_pto,pto_file_top,pto_file_cps,pto_file_end,pto_image,cps_per_line,lines_to_image_number);
		cout << endl << "Written file " << output_pto << endl;
	}
	//cout << endl;
	return(1);
	
}	
