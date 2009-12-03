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

#include <stdio.h> 
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <cctype>
#include <string>
#include <algorithm>
#include "svm.h"
#include "CelesteGlobals.h"
#include "Utilities.h"
#include "Celeste.h"
#include <sys/stat.h> 
#include "hugin_config.h" 
#include <hugin_version.h>

#ifdef __WIN32__
#include <windows.h>
#endif

using namespace std;

bool fileexists(string strFilename) {

	struct stat stFileInfo;
	bool blnReturn;
	int intStat;

	// Attempt to get the file attributes
	intStat = stat(strFilename.c_str(),&stFileInfo);
	if(intStat == 0) {

		blnReturn = true;
  	}else{

    		blnReturn = false;
  	}
  
  	return(blnReturn);
}

static void tokenize(const string& str, vector<string>& tokens, const string& delimiters = " "){
    
	// Skip delimiters at beginning
	string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	
	// Find first non-delimiter
	string::size_type pos = str.find_first_of(delimiters, lastPos);

 	while (string::npos != pos || string::npos != lastPos){
	
        	// Found a token, add it to the vector
        	tokens.push_back(str.substr(lastPos, pos - lastPos));
		
        	// Skip delimiters.  Note the not_of
        	lastPos = str.find_first_not_of(delimiters, pos);
		
        	// Find next non-delimiter
        	pos = str.find_first_of(delimiters, lastPos);
    	}
	
}

static void parse_pto(string& f,vector<string>& images,vector<string>& pto_file_top,vector<string>&
pto_file_cps,vector<string>& pto_file_end, unsigned int& mask, string& m, double& threshold,string&
mask_format,vector<double>& svm_responses){
  
 	// Create some vectors to store image and control point data
	vector<unsigned int> xcps;
	vector<unsigned int> ycps;
 
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
									
				found = line.find("n\"");
				imagename = line.substr(found + 2, line.find("\"",found + 2) - found - 2);
				//cout << imagename << endl;
				
			// (Old?) Autopano Pro files
			}else{
			
				found = line.find("\"");
				imagename = line.substr(found + 1, line.length() - found - 2);
			
			}
			
			if (fileexists(imagename)){
			
				// Put filenames into a vector
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
					images.push_back(imagename);
				
				}else{
				
					cout << "Couldn't open image " << line.substr(found + 2, line.length() - found -
					3) << endl;
					
					cout << "Also tried " << imagename << endl << endl; 
					exit(1);
				}
			}

			//cout << imagename << endl;

			
		}else if (start == ("c")){
		
			pto_file_cps.push_back(line);
			//cout << line << endl;
		
			// Marker - we're at CPs
			at_cps = 1;

			vector<string> tokens;
			tokenize(line, tokens);

			// Image number for accessing images vector			
			unsigned int image_no = atoi( tokens[1].substr(1, tokens[1].length()-1).c_str() );

			if (image_no != current){
			
				// Create the storage matrix
				gNumLocs = xcps.size();
				gLocations = CreateMatrix( (int)0, gNumLocs, 2);
							
				//cout << images[current] << " " << image_no <<  " (" << xcps.size() << "):" << endl;
								
				for (int cp = 0; cp < gNumLocs; cp++){
				
					//cout << "CP " << cp << ": " << xcps[cp] << "," << ycps[cp] << endl;
					
					gLocations[cp][0] = xcps[cp];
					gLocations[cp][1] = ycps[cp];
					
				}

				// Get response
				get_gabor_response(images[current],mask,m,threshold,mask_format,svm_responses);

				//gLocations = NULL;
				delete[] gLocations;

				xcps.clear();
				ycps.clear();
				current = image_no;

				// First CP of next image
				
				// Control point x,y
				unsigned int xcp, ycp;
				for ( int i = 0; i < tokens.size(); i++)
				  {
				    if (tokens[i][0] == 'x')
				      xcp = atoi( tokens[i].substr(1, tokens[i].length()-1).c_str() );
				    else if (tokens[i][0] == 'y')
				      ycp = atoi( tokens[i].substr(1, tokens[i].length()-1).c_str() );
				  }
	
				xcps.push_back(xcp);
				ycps.push_back(ycp);

				counter = 1;
				
			}else{
				unsigned int xcp, ycp;
				for ( int i = 0; i < tokens.size(); i++)
				  {
				    if (tokens[i][0] == 'x')
				      xcp = atoi( tokens[i].substr(1, tokens[i].length()-1).c_str() );
				    else if (tokens[i][0] == 'y')
				      ycp = atoi( tokens[i].substr(1, tokens[i].length()-1).c_str() );
				  }
				ycps.push_back(ycp);
				xcps.push_back(xcp);
			
				counter++;
			
				//cout << images[image_no] << " : " << xcp << "," << ycp << endl;
			}
			
		}else if (at_cps && start != ("c") && start != ("#") && done_cps == 0){

			// Create the storage matrix
			gNumLocs = xcps.size();
			gLocations = CreateMatrix( (int)0, gNumLocs, 2);
							
			//cout << images[current] << " " << current <<  " (" << xcps.size() << "):" << endl;
				
			for (unsigned int cp = 0; cp < counter; cp++){
				
				//cout << "CP " << cp << ": " << xcps[cp] << "," << ycps[cp] << endl;
					
				gLocations[cp][0] = xcps[cp];
				gLocations[cp][1] = ycps[cp];
					
			}

			// Get response
			get_gabor_response(images[current],mask,m,threshold,mask_format,svm_responses);

			//gLocations = NULL;
			delete[] gLocations;
			
			xcps.clear();
			ycps.clear();
			
			done_cps = 1;
				
		}

		if (at_cps && start != ("c")){
		
			pto_file_end.push_back(line);
		
		}
		
	}

	// Process last image - create mask if necessary
	//if (images.size() > 1){

	//	gNumLocs = 0;
	//	get_gabor_response(images.back(),mask,m,threshold,mask_format,svm_responses);
	
	//}
	
	// Close stream
	is.close();

}

static void write_pto(double& threshold,vector<string>& images,string& output_pto,vector<string>&
pto_file_top,vector<string>& pto_file_cps,vector<string>& pto_file_end,vector<double> svm_responses){

	// File to write to
	ofstream out;
	out.open (output_pto.c_str());

  	if(!out.good()){
    		cout << "Couldn't write to Hugin project file " << output_pto << endl << endl;
    		exit(1);
  	}
	
	// Print the top of the file
	for (unsigned int l = 0; l < pto_file_top.size() - 1; l++){
	
		out << pto_file_top[l] << endl;
	
	}

	unsigned int current = 0;

	cout << images[current] << ":" << endl;

	// Print control points
	for (unsigned int l = 0; l < pto_file_cps.size(); l++){

		//cout << l << ":  " << pto_file_cps[l] << endl;

		vector<string> tokens;
		tokenize(pto_file_cps[l], tokens);
		unsigned int image_no = atoi( tokens[1].substr(1, tokens[1].length()-1).c_str() );
		if (image_no != current){
			cout << endl << images[image_no] << ":" << endl;
		}
		current = image_no;
	
		cout << "CP ";
		unsigned int len = 0;
		for (unsigned int t = 1; t < tokens.size(); t++){
			len += tokens[t].length();
			cout << tokens[t] << " ";
		}

		//cout << pto_file_cps[l] << "\t";

		unsigned int count_spaces = 1;
		int t = 80 - len;
		if (t > 0) count_spaces = t;

		for (unsigned int s = 0; s < count_spaces; s++){
		
			cout << " ";
		}

		if (svm_responses[l] >= threshold){
			cout << "Removed  (score " << svm_responses[l] << " >= " << threshold << ")" << endl;
		}else{
			cout << "OK       (score " << svm_responses[l] << " < " << threshold << ")" << endl;
				
			// Write to file
			out << pto_file_cps[l] << endl;				
		}

	}

	// Print bottom of file
	for (unsigned int l = 0; l < pto_file_end.size(); l++){
	
		out << pto_file_end[l] << endl;
	
	}

	out.close();

}

static void usage(){

	// Print usage and exit
	cout << endl << "Celeste: Removes cloud-like control points from Hugin project files and creates image masks" << endl;
	cout << "using Support Vector Machines." << endl;
	cout << endl << "Version " << DISPLAY_VERSION << endl;
	cout << endl << "Usage: celeste_standalone [options] image1 image2 [..]" << endl << endl;
	cout << "Options:" << endl << endl;
	cout << "  -i <filename>   Input Hugin PTO file. Control points over SVM threshold will" << endl;
	cout << "                  be removed before being written to the output file. If -m is" << endl;
	cout << "                  set to 1, images in the file will be also be masked." << endl;
	cout << "  -o <filename>   Output Hugin PTO file. Default: '<filename>_celeste.pto'" << endl;
	cout << "  -d <filename>   SVM model file. Default: 'data/celeste.model'" << endl;
	cout << "  -s <int>        Maximum dimension for re-sized image prior to processing. A" << endl;
	cout << "                  higher value will increase the resolution of the mask but is" << endl;
	cout << "                  significantly slower. Default: 800" << endl;
	cout << "  -t <float>      SVM threshold. Raise this value to remove fewer control points," << endl;
	cout << "                  lower it to remove more. Range 0 to 1. Default: 0.5" << endl;
	cout << "  -m <1|0>        Create masks when processing Hugin PTO file. Default: 0" << endl;
	cout << "  -f <string>     Mask file format. Options are PNG, JPEG, BMP, GIF and TIFF." << endl;
	cout << "                  Default: PNG" << endl;
	cout << "  -r <1|0>        Filter radius. 0 = large (more accurate), 1 = small (higher" << endl;
	cout << "                  resolution mask, slower, less accurate). Default: 0" << endl;
	cout << "  -h              Print usage." << endl; 
	cout << "  image1 image2.. Image files to be masked." << endl << endl;
	exit(1);

}

int main(int argc, const char* argv[]){

        // Exit with usage unless filename given as argument
        if (argc < 2){
                usage();
        }

        unsigned int i = 1, mask = 0;
	double threshold = 0.5;
	vector<string> images_to_mask;
        string pto_file = (""),output_pto = ("");
	string mask_format = ("PNG");
	string model_file = ("celeste.model");
	int course_fine = 0;

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
				case 'i' : {pto_file += argv[i]; break;}
				case 'o' : {output_pto += argv[i]; break;}
				case 't' : {threshold = atof(argv[i]); break;}
				case 'm' : {mask = atoi(argv[i]); break;}
				case 'f' : {mask_format = argv[i]; break;}
				case 'd' : {model_file = argv[i]; break;}
				case 'r' : {course_fine = atoi(argv[i]); break;}
				case 's' : {resize_dimension = atoi(argv[i]); break;}
                        }

   
                }else{
			images_to_mask.push_back(argv[i]);
                }

                i++;
        }

	// Check model file
	if (!fileexists(model_file)){
	
#if __WIN32__
        char buffer[MAX_PATH];//always use MAX_PATH for filepaths
        GetModuleFileName(NULL,buffer,sizeof(buffer));
        string working_path=(buffer);
        string install_path_model="";
        //remove filename
        std::string::size_type pos=working_path.rfind("\\");
        if(pos!=std::string::npos)
        {
            working_path.erase(pos);
            //remove last dir: should be bin
            pos=working_path.rfind("\\");
            if(pos!=std::string::npos)
            {
                working_path.erase(pos);
                //append path delimiter and path
                working_path.append("\\share\\hugin\\data\\");
                install_path_model=working_path;
            }
        }
#else
        string install_path_model = (INSTALL_DATA_DIR);
#endif

		install_path_model.append(model_file);
		
		if (!fileexists(install_path_model)){
		
    			cout << endl << "Couldn't open SVM model file " << model_file << endl;
			cout << "Also tried " << install_path_model << endl << endl; 
    			exit(1);

		}else{
		
			model_file = install_path_model;
		
		}
  	}

	// Set output .pto filename if not given
	if (output_pto == ("") && pto_file != ("")){
		output_pto = pto_file.substr(0,pto_file.length()-4).append("_celeste.pto");
	}

	// Convert mask format to upper case
	transform(mask_format.begin(), mask_format.end(), mask_format.begin(),(int(*)(int)) toupper);
	if (mask_format == ("JPG")){
		mask_format = ("JPEG");
	}
	if (mask_format != ("PNG") &&mask_format != ("BMP") && mask_format != ("GIF") && mask_format !=	("JPEG") && mask_format != ("TIFF")){
		mask_format = ("TIFF");
	}

	// Print some stuff out
	cout << endl << "Celeste: Removes cloud-like control points from Hugin project files and creates image masks" << endl;
	cout << "using Support Vector Machines." << endl;
	cout << endl << "Version " << DISPLAY_VERSION << endl << endl;
	cout << "Arguments:" << endl;
	cout << "Input Hugin file:\t" << pto_file << endl;
	cout << "Output Hugin file:\t" << output_pto << endl;
	cout << "SVM model file:\t\t" << model_file << endl;
	cout << "Max dimension:\t\t" << resize_dimension << endl;
	cout << "SVM threshold:\t\t" << threshold << endl;
	cout << "Create PTO masks:\t";
	if (mask){
		cout << "Yes" << endl;
	}else{
		cout << "No" << endl;
	} 
	cout << "Mask format:\t\t" << mask_format << endl;
	cout << "Filter radius:\t\t";

	// Mask resolution
	if (course_fine){	
		gRadius = 10;
		spacing = (gRadius * 2) + 1;
		cout << "Small" << endl << endl;
		
	}else{
		cout << "Large" << endl << endl;
	} 
	
	// Convert mask format to lower case
	transform(mask_format.begin(), mask_format.end(), mask_format.begin(),(int(*)(int)) tolower);

	// Vectors to store SVM responses and PTO file info etc
	vector<string> images,pto_file_top,pto_file_cps,pto_file_end;
	vector<double> svm_responses;

	// Mask images
	if (images_to_mask.size()){

		cout << "Masking images..." << endl << endl;
		unsigned int n = 1;
		for (unsigned int l = 0; l < images_to_mask.size(); l++){
			gNumLocs = 0;		
			get_gabor_response(images_to_mask[l],n,model_file,threshold,mask_format,svm_responses);			
		}
	}

	// Process PTO file
	if (pto_file != ("")){

  		cout << "Parsing Hugin project file " << pto_file << endl << endl;
		
		parse_pto(pto_file,images,pto_file_top,pto_file_cps,pto_file_end,mask,model_file,threshold,mask_format,svm_responses); 

		// Prune CPs and write new pto file
		write_pto(threshold,images,output_pto,pto_file_top,pto_file_cps,pto_file_end,svm_responses);
		cout << endl << "Written file " << output_pto << endl << endl;

	}

	return(1);
	
}	
