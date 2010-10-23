/***************************************************************************
 *   Copyright (C) 2009 by Tim Nugent                                      *
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

#define HUGIN_USE_EXIV2

//#include <jhead/jhead.h>

#ifdef HUGIN_USE_EXIV2
#include <exiv2/exif.hpp>
#include <exiv2/image.hpp>
#endif

#include "vigra/impex.hxx"
#include "vigra/stdimage.hxx"
#include "vigra/edgedetection.hxx"
#include "vigra/stdimagefunctions.hxx"
#include "vigra/tensorutilities.hxx"
#include "vigra/boundarytensor.hxx"
#include <vigra/orientedtensorfilters.hxx>
#include "vigra/diff2d.hxx"
#include <vigra/rgbvalue.hxx>
#include <vigra/utilities.hxx>
#include <sys/stat.h> 
#include "Globals.h"
#include "ProcessImage.h"
#include "MapPoints.h"
#include <ANN/ANN.h>

#include "find_N8_lines.h"

#if  _MSC_VER  //def _WIN32
#include <time.h>
#endif

using namespace vigra; 
using namespace std;

typedef vigra::BRGBImage::PixelType RGB;

double line_length_squared(int& x1, int& y1, int& x2, int& y2){
	int d1 = x1 - x2;
	int d2 = y1 - y2;
	return(d1*d1+d2*d2);
}

bool compare_yx (const vigra::Point2D i,const vigra::Point2D j){
	
	//return (j->y<i->y);
	
	if(j->x == i->x){
		return (j->y>i->y);
	}else{
		return (j->x<i->x);
	}
}

bool compare_xy (const vigra::Point2D i,const vigra::Point2D j){
	
	//return (i->x<j->x);
	
	if(i->y == j->y){
		return (i->x>j->x);
	}else{
		return (i->y<j->y);
	}
}

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

void tokenize(const string& str, vector<string>& tokens, const string& delimiters = " "){
    
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

void sort_inliers(vector<Point2D>& inliers){

	sort(inliers.begin(), inliers.end(), compare_xy);	

	/*
	unsigned int top = 0;
	unsigned int bottom = 100000;
	unsigned int left = 100000;
	unsigned int right = 0;

	for (unsigned int il = 0; il < inliers.size(); il++){
	
		if(inliers[il]->x < left) left = inliers[il]->x;
		if(inliers[il]->x > right) right = inliers[il]->x;
		if(inliers[il]->y < bottom) bottom = inliers[il]->y;
		if(inliers[il]->y > top) top = inliers[il]->y;
		
	}	
	
	// Sort pixels for adding CPs to PTO file
	int d1 = abs(top-bottom);
	int d2 = abs(right-left);
	if(d2 > d1){
		sort(inliers.begin(), inliers.end(), compare_xy);	
	}else{
		sort(inliers.begin(), inliers.end(), compare_yx);	
	}
	*/
}

void plot_inliers(string& filename, BImage& image, vector<Point2D>& inliers, int i){

	vigra::BRGBImage tmp(image.width(),image.height());		
	vigra::initImage(srcIterRange(tmp.upperLeft(),
			tmp.upperLeft() + vigra::Diff2D(image.width(),image.height())),
                	RGB(255,255,255) );
	
	for (unsigned int il = 0; il < inliers.size(); il++){

		/*
		// Careful, this might segfault if the box goes off the image..	remove -3 and change
		// +3 to +1 to avoid this	
		if(il == 0 )vigra::initImage(srcIterRange(tmp.upperLeft() +
		vigra::Diff2D(inliers[il]->x-3, inliers[il]->y-3),
				tmp.upperLeft() + vigra::Diff2D(inliers[il]->x+3, inliers[il]->y+3)),
                		RGB(255,0,0) );	

		if(il == 10 )vigra::initImage(srcIterRange(tmp.upperLeft() +
		vigra::Diff2D(inliers[il]->x-3, inliers[il]->y-3),
				tmp.upperLeft() + vigra::Diff2D(inliers[il]->x+3, inliers[il]->y+3)),
                		RGB(0,255,0) );	
		*/
		
		vigra::initImage(srcIterRange(tmp.upperLeft() + vigra::Diff2D(inliers[il]->x, inliers[il]->y),
				tmp.upperLeft() + vigra::Diff2D(inliers[il]->x+1, inliers[il]->y+1)),
                		RGB(255,0,0) );			

		// Make end point green
		if(il == 0 || il == inliers.size()-1)vigra::initImage(srcIterRange(tmp.upperLeft() +
		vigra::Diff2D(inliers[il]->x, inliers[il]->y),
				tmp.upperLeft() + vigra::Diff2D(inliers[il]->x+1, inliers[il]->y+1)),
                		RGB(0,255,0) );	

	}	

	if (generate_images){

		string output = path;
		vector<string> tokens;

		// *nix file paths
		if (filename.find("/") < filename.length() || filename.substr(0, 1) == ("/")){					
			tokenize(filename, tokens, "/");
			output.append(tokens[tokens.size()-1].substr(0,tokens[tokens.size()-1].length()-4));		
		// Windows file paths
		}else{
			tokenize(filename, tokens, "\\");
			output.append(tokens[tokens.size()-1].substr(0,tokens[tokens.size()-1].length()-4));
		}
		output.append("_line_");
		stringstream ii;
		ii << i;
		output.append(ii.str());
		output.append(format);
		
		//if(!fileexists(output)){
			cout << "Plotting line " << i << ". Pixels:\t" << inliers.size() << endl;
			exportImage(srcImageRange(tmp), ImageExportInfo(output.c_str()).setPixelType("UINT8"));
		//}
	}
}

void plot_lines(string& filename, BImage& image, vector< vector< Point2D > >& lines, 				int i, int j ){

	vigra::BRGBImage tmp(image.width(),image.height());		
	vigra::initImage(srcIterRange(tmp.upperLeft(),
			tmp.upperLeft() + vigra::Diff2D(image.width(),image.height())),
                	RGB(255,255,255) );
	for( unsigned iln = i; iln < j; iln++ ){
		vector<Point2D> & inliers = lines.at( iln );
		for (unsigned int il = 0; il < inliers.size(); il++){
			vigra::initImage(srcIterRange(tmp.upperLeft() + vigra::Diff2D(inliers[il]->x, inliers[il]->y),
					tmp.upperLeft() + vigra::Diff2D(inliers[il]->x+1, inliers[il]->y+1)),
                			RGB(255,0,0) );			

			// Make end point green
			if(il == 0 || il == inliers.size()-1)vigra::initImage(srcIterRange(tmp.upperLeft() +
			vigra::Diff2D(inliers[il]->x, inliers[il]->y),
					tmp.upperLeft() + vigra::Diff2D(inliers[il]->x+1, inliers[il]->y+1)),
                			RGB(0,255,0) );	

		}	
	}

	string output = path;
	vector<string> tokens;

	// *nix file paths
	if (filename.find("/") < filename.length() || filename.substr(0, 1) == ("/")){					
		tokenize(filename, tokens, "/");
		output.append(tokens[tokens.size()-1].substr(0,tokens[tokens.size()-1].length()-4));		
	// Windows file paths
	}else{
		tokenize(filename, tokens, "\\");
		output.append(tokens[tokens.size()-1].substr(0,tokens[tokens.size()-1].length()-4));
	}
	output.append("_line_");
	stringstream ii;
	ii << i+1 <<"-"<<j+1;
	output.append(ii.str());
	output.append(format);
	
	cout << "Plotting lines " << i+1 << " thru " << j+1 << endl;
		exportImage(srcImageRange(tmp), ImageExportInfo(output.c_str()).setPixelType("UINT8"));
	
}

void find_ann(vector<Point2D>& coords, FVector2Image& edgeness, vector<Point2D>& inliers, unsigned int& good_lines, vector<vector<Point2D> >& short_segments){
	
	int k = 1;                          // Number of nearest neighbours
	int dim = 3;                        // Dimensions  
	double eps = 0.0;             	    // Error bound
	
	ANNpoint queryPt;                   // Query point
	queryPt = annAllocPt(dim,0);        // Allocate query point
	//srand(time(0));
	//int queryidx = rand() % coords.size();
	int queryidx = 0;
	queryPt[0] = coords[queryidx]->x;
	queryPt[1] = coords[queryidx]->y;

	double orientation_scale = 1;
	double orientation_threshold = 0.2;
	// Set high to disable
	orientation_threshold = 1000;

	// Add direction as 3rd dimension
	double first_o;
	if(dim > 2){
		TinyVector<float, 2> value = edgeness(queryPt[0],queryPt[1]);
		queryPt[2] = value[1] * orientation_scale;
		first_o = queryPt[2];
	}

	int first_x = coords[queryidx]->x;
	int first_y = coords[queryidx]->y;

	//if (verbose) cout << "Initial query:\t(" << first_x << "," << first_y << "," << first_o <<
	//")" << endl;

	// Select query point, then remove from coords
	inliers.push_back(coords[queryidx]);
	coords.erase(coords.begin()+queryidx);
	
	if(verbose) cout << "Random query point index:\t" << queryidx << endl << endl;

	int maxPts = coords.size();         // Max number of data points
	int nPts = coords.size();           // Actual number of data points
	int cont = 1;
	int rev = 0;
	double previous_length = 0;
	//int first_line_end_x = first_x, first_line_end_y = first_y;
	int first_line_end_x = -1, first_line_end_y = -1;
	int second_line_end_x = -1, second_line_end_y = -1;
	int first_line_end_index;
	
	if(verbose) cout << "Query Pixel\tNN Pixel\tIndex\tDistance\tOrientation diff" << endl;
	
	while (cont){
		
		ANNkd_tree* kdTree;                 // Search structure	
		ANNpointArray dataPts;              // Data points	
		dataPts = annAllocPts(maxPts, dim); // Allocate data points	

		ANNidxArray nnIdx;                  // Near neighbor indices
		nnIdx = new ANNidx[k];              // Allocate near neighbor indices
		ANNdistArray dists;                 // Near neighbor distances
		dists = new ANNdist[k];             // Allocate near neighbor distances	
	
		// Only load data points that are within gap_limit of query pixel	
		vector<Point2D> close_neighbors;
		vector<int> coords_index;	
		for (unsigned int c = 0; c < coords.size(); c++){ 		
			
			/*
			// Fix problem with wrapping around
			double min_x,max_x,min_y,max_y;
			
			if ((queryPt[0]-gap_limit) < 0){
				min_x = 0;
			}else{
				min_x = queryPt[0]-gap_limit;
			}
			if ((queryPt[0]+gap_limit) > edgeness.width()){
				max_x = edgeness.width();
			}else{
				max_x = queryPt[0]+gap_limit;
			}			

			if ((queryPt[1]-gap_limit) < 0){
				min_y = 0;
			}else{
				min_y = queryPt[1]-gap_limit;
			}
			if ((queryPt[1]+gap_limit) > edgeness.height()){
				max_y = edgeness.height();
			}else{
				max_y = queryPt[1]+gap_limit;
			}	
			*/

			if(coords[c]->x > queryPt[0]-gap_limit && coords[c]->x < queryPt[0]+gap_limit){	
				if(coords[c]->y > queryPt[1]-gap_limit && coords[c]->y < queryPt[1]+gap_limit){					
					close_neighbors.push_back(coords[c]);	
					coords_index.push_back(c);
				}
			}			
		}
		
		nPts = close_neighbors.size();
		if(nPts){

			//cout << "nPts:\t" << nPts << endl;
			for (unsigned int c = 0; c < nPts; c++){ 
				dataPts[c][0] = close_neighbors[c]->x;
				dataPts[c][1] = close_neighbors[c]->y;
				
				// Add direction as 3rd dimension
				if(dim > 2){
					TinyVector<float, 2> value = edgeness(dataPts[c][0],dataPts[c][1]);
					dataPts[c][2] = value[1] * orientation_scale;
				}				
			}	

			kdTree = new ANNkd_tree(dataPts,nPts,2);
			//kdTree = new ANNkd_tree(dataPts,nPts,dim);
			kdTree->annkSearch(queryPt,k,nnIdx,dists,eps);	

			dists[0] = sqrt(dists[0]);
			int x2 = close_neighbors[nnIdx[0]]->x;
			int y2 = close_neighbors[nnIdx[0]]->y;
						
			double this_length = line_length_squared(first_x,first_y,x2,y2);			
			
			double orientation_diff = abs(queryPt[2] - dataPts[nnIdx[0]][2]);
			//double orientation_diff = abs(first_o - dataPts[nnIdx[0]][2]);

			if (this_length < previous_length){
				if(verbose) cout << "(" << queryPt[0] << "," << queryPt[1] << "," << queryPt[2] << ")\t(" << close_neighbors[nnIdx[0]]->x << "," << close_neighbors[nnIdx[0]]->y << ")" << "\t" << nnIdx[0] << "\t" << dists[0] <<  "\t" << orientation_diff << endl;
				if(verbose) cout << "This point is closer to the inital query pixel than the previous one..." << endl;
				
				// Go back to the start point and go down the line in the reverse direction
				if (rev == 0){
					if(verbose) cout << "End of line reached. Returning to inital query pixel..." << endl;
					
					if(verbose) cout << "Setting line end point to " << close_neighbors[nnIdx[0]]->x << "," << close_neighbors[nnIdx[0]]->y << endl;
				
					if (first_line_end_x == -1 && first_line_end_y == -1){
						first_line_end_x = close_neighbors[nnIdx[0]]->x;
						first_line_end_y = close_neighbors[nnIdx[0]]->y;
						first_line_end_index = inliers.size()-1;
					}else{
						second_line_end_x = close_neighbors[nnIdx[0]]->x;
						second_line_end_y = close_neighbors[nnIdx[0]]->y;			 
					}
					
					//first_line_end_x = close_neighbors[nnIdx[0]]->x;
					//first_line_end_y = close_neighbors[nnIdx[0]]->y;
					queryPt[0] = first_x;
					queryPt[1] = first_y;
					
					// Add direction as 3rd dimension
					if(dim > 2) queryPt[2] = first_o;
				
					previous_length = 0;
					rev = 1;
				}else{

					// Erase it from coords so we don't see it again
					coords.erase(coords.begin()+coords_index[nnIdx[0]]);


					//if(verbose) cout << "End of line reached." << endl;
					//cont = 0;
				}
			}else{			
				
				if (orientation_diff < orientation_threshold){
			
					if(verbose) cout << "(" << queryPt[0] << "," << queryPt[1] << "," << queryPt[2] << ")\t(" << close_neighbors[nnIdx[0]]->x << "," <<
					close_neighbors[nnIdx[0]]->y << ")" << "\t" << nnIdx[0] << "\t" << dists[0] << "\t" << orientation_diff << endl;
								
					// Set query point to NN
					queryPt[0] = close_neighbors[nnIdx[0]]->x;
					queryPt[1] = close_neighbors[nnIdx[0]]->y;		
				
					// Add direction as 3rd dimension
					if (dim > 2){
						TinyVector<float, 2> value = edgeness(queryPt[0],queryPt[1]);
						queryPt[2] = value[1] * orientation_scale;
					}
				
					// Add NN to inliers				
					inliers.push_back(close_neighbors[nnIdx[0]]);
				
					// Erase it from coords so we don't see it again
					coords.erase(coords.begin()+coords_index[nnIdx[0]]);
					previous_length = this_length;
				}else{

					if(verbose) cout << "Pixel orientation is significantly different from query:\t" << orientation_diff << endl;

					// Erase it from coords so we don't see it again
					coords.erase(coords.begin()+coords_index[nnIdx[0]]);
				
				}
					
			}
	
			// Clean up
			delete [] nnIdx;
			delete [] dists;
			delete kdTree;

		}else{
			if (rev == 0){
				if(verbose) cout << "No more pixels in this region. Returning to inital query pixel..." << endl;
				if (inliers.size()){
				
					//if(verbose) cout << "Setting line end point to " << inliers[inliers.size()-1]->x << "," << inliers[inliers.size()-1]->y << endl;
					
				
					if (first_line_end_x == -1 && first_line_end_y == -1){
						first_line_end_x = inliers[inliers.size()-1]->x;
						first_line_end_y = inliers[inliers.size()-1]->y;
						first_line_end_index = inliers.size()-1;
					}else{
						second_line_end_x = inliers[inliers.size()-1]->x;
						second_line_end_y = inliers[inliers.size()-1]->y;					
					}
				
					//first_line_end_x = inliers[inliers.size()-1]->x;
					//first_line_end_y = inliers[inliers.size()-1]->y;					
					queryPt[0] = first_x;
					queryPt[1] = first_y;
					
					// Add direction as 3rd dimension
					if(dim > 2) queryPt[2] = first_o;
					previous_length = 0;
					rev = 1;
					
				}else{

					//if(verbose) cout << "Setting line end point to " << queryPt[0] << "," << queryPt[1] << endl;
				
					if (first_line_end_x == -1 && first_line_end_y == -1){
						first_line_end_x = (int)queryPt[0];
						first_line_end_y = (int)queryPt[1];
						first_line_end_index = inliers.size()-1;
					}else{
						second_line_end_x = (int)queryPt[0];
						second_line_end_y = (int)queryPt[1];		 
					}

					if(verbose) cout << "No more pixels." << endl;
					cont = 0;
				}					
			}else{

				//if(verbose) cout << "Setting line end point to " << queryPt[0] << "," << queryPt[1] << endl;
					
				if (first_line_end_x == -1 && first_line_end_y == -1){
					first_line_end_x = (int)queryPt[0];
					first_line_end_y = (int)queryPt[1];
					first_line_end_index = inliers.size()-1;
				}else{
					second_line_end_x = (int)queryPt[0];
					second_line_end_y = (int)queryPt[1];		 
				}

				if(verbose) cout << "No more pixels in this region." << endl;			
				cont = 0;
			}
		}		
		annDeallocPts(dataPts);		
	}	
	annClose();


	// Sort points
	vector<Point2D> tmp;
	for(int i = first_line_end_index; i >= 0; i--){
		tmp.push_back(inliers[i]);;
	}
	for(int i = first_line_end_index+1; i < inliers.size(); i++){
		tmp.push_back(inliers[i]);;
	}
	inliers = tmp;

	//int x2 = inliers[inliers.size()-1]->x;
	//int y2 = inliers[inliers.size()-1]->y;
	//double length_sq = line_length_squared(first_line_end_x, first_line_end_y,x2,y2);
	double length_sq = line_length_squared(first_line_end_x,first_line_end_y,second_line_end_x,second_line_end_y);

	if(verbose) cout << endl << "Measuring distance from " << first_line_end_x << "," << first_line_end_y << " to " << second_line_end_x << "," <<
	second_line_end_y <<endl;
	
	//if(verbose) cout << endl << "Measuring distance from " << first_line_end_x << "," << first_line_end_y << " to " << inliers[inliers.size()-1]->x << "," <<
	//inliers[inliers.size()-1]->y <<endl;
	
	if(verbose) cout << "Length squared:\t" << length_sq << "\t";

	if(length_sq >= min_line_length_squared){
	 	good_lines++;
		if(verbose) cout << "Above theshold (" << min_line_length_squared << ")" << endl;
	}else{
		// Collect shorter segments
		if(length_sq >= min_line_length_squared/20){
			short_segments.push_back(inliers);
		}	
		inliers.clear();		
		if(verbose) cout << "Below theshold (" << min_line_length_squared << ")" << endl;
	}

	if(verbose) cout << "##############################################" << endl << endl;

}

void extract_coords(BImage& image, vector<Point2D>& coords){

	// Gather black pixels
        for (unsigned int h = 0; h < image.height(); h++){
               for (unsigned int w = 0; w < image.width(); w++){
			RGBValue<int,0u,1u,2u> value = image(w,h);
			if (value[0] != 255){	
				coords.push_back(Point2D(w,h));		
			}    
               }
        }
}

void nuke_corners(BImage& image, FImage& corners, string& filename){

	cout << "Removing corners..." << endl;

        for (unsigned int h = 0; h < corners.height(); h++){
               for (unsigned int w = 0; w < corners.width(); w++){

			// Change this threshold to remove more corner pixels
			if (corners(w,h) > corner_threshold){	
	
				// Change the size of the box to draw around corner pixel
				int size = 3;				
				int left = w-size;
				int top = h-size;
				int right = w+size;
				int bottom = h+size;

				if(left<0) left = 0;
				if(right>image.width()) right = image.width();
				if(top<0) top = 0;
				if(bottom > image.height()) bottom = image.height();

				// Draw a white box around the corner pixel
				// in the edge image
				vigra::initImage(srcIterRange(image.upperLeft() +
				vigra::Diff2D(left, top),
					image.upperLeft() + vigra::Diff2D(right, bottom)),
                			255 );	
				//cout << value[0] << "\t(" << w << "," << h << ")" << endl;         
			}    
               }
        }	

	/*
	// Smooth with hourglass filter
    	FVector2Image gradient(image.width(),image.height());
    	FVector3Image tensor(image.width(),image.height()), smoothedTensor(image.width(),image.height());
    	gaussianGradient(srcImageRange(image), destImage(gradient), 1.0);
    	vectorToTensor(srcImageRange(gradient), destImage(tensor));
    	hourGlassFilter(srcImageRange(tensor), destImage(smoothedTensor), sigma, rho);
	if (generate_images){

		string output = path;
		output.append("hourglass_sigma_");
		stringstream dt;
		dt << sigma;
		output.append(dt.str());
		output.append("_rho_");
		stringstream ds;
		ds << rho;
		output.append(ds.str());
		output.append(format);
		cout << "Writing " << output << endl;
        	exportImage(srcImageRange(smoothedTensor), ImageExportInfo(output.c_str()).setPixelType("UINT8"));	
	}
	*/	
	
	if (generate_images){

		string output = path;
		vector<string> tokens;

		// *nix file paths
		if (filename.find("/") < filename.length() || filename.substr(0, 1) == ("/")){					
			tokenize(filename, tokens, "/");
			output.append(tokens[tokens.size()-1].substr(0,tokens[tokens.size()-1].length()-4));		
		// Windows file paths
		}else{
			tokenize(filename, tokens, "\\");
			output.append(tokens[tokens.size()-1].substr(0,tokens[tokens.size()-1].length()-4));
		}
		output.append("_edges_minus_corners");
		output.append(format);

		//if(!fileexists(output)){
			cout << "Writing " << output << endl << endl;
        		exportImage(srcImageRange(image), ImageExportInfo(output.c_str()).setPixelType("UINT8"));
		//}
	}
}

void detect_edge(BImage& image, string& filename, BImage& edge_image){

	try{   
        	// Paint output image white
		edge_image = 255;
        
		cout << "Detecting edges..." << endl;
	
        	if(detector == 2){			
            		// Call Shen-Castan detector algorithm
            		differenceOfExponentialEdgeImage(srcImageRange(image), destImage(edge_image),
                           	scale, threshold, 0);
        	}else{			
            		// Call Canny detector algorithm
            		cannyEdgeImage(srcImageRange(image), destImage(edge_image),
                           	scale, threshold, 0);
        	}

		if (generate_images){

			string output = path;
			vector<string> tokens;

			// *nix file paths
			if (filename.find("/") < filename.length() || filename.substr(0, 1) == ("/")){					
				tokenize(filename, tokens, "/");
				output.append(tokens[tokens.size()-1].substr(0,tokens[tokens.size()-1].length()-4));		
			// Windows file paths
			}else{
				tokenize(filename, tokens, "\\");
				output.append(tokens[tokens.size()-1].substr(0,tokens[tokens.size()-1].length()-4));
			}
			output.append("_edges_threshold_");
			stringstream dt;
			dt << threshold;
			output.append(dt.str());
			output.append("_scale_");
			stringstream ds;
			ds << scale;
			output.append(ds.str());
			output.append(format);

			//if(!fileexists(output)){
				cout << "Writing " << output << endl;
        			exportImage(srcImageRange(edge_image), output.c_str());
			//}
		}	
    	}    
    	catch (StdException & e){
        	cout << e.what() << endl;
    	}	
}

void generate_boundary_tensor(BImage& image, FVector2Image& edgeness, FImage& cornerness, string& filename){

	// Get cornerness from edge detection output
	// Create image of appropriate size for boundary tensor
	FVector3Image boundarytensor(image.width(), image.height());
	
	// Calculate the boundary tensor
	cout << "Calculating boundary tensor..." << endl;
	boundaryTensor1(srcImageRange(image), destImage(boundarytensor), tscale);

	tensorToEdgeCorner(srcImageRange(boundarytensor), destImage(edgeness), destImage(cornerness));

	//FImage boundarystrength(image.width(), image.height());
	//tensorTrace(srcImageRange(boundarytensor), destImage(boundarystrength));


	//int bt_edge_threshold = 30;
	//for (int i = 30; i < 71; i++){
	
		BImage tb_edgness(image.width(), image.height());
		tb_edgness = 255;	

		for(int h = 0; h < edgeness.height(); h++){
			for(int w = 0; w < edgeness.width(); w++){
				TinyVector<float, 2> value = edgeness(w,h);

				//if (value[0] > bt_edge_threshold){
				if (value[0] > bt_threshold){			
					vigra::initImage(srcIterRange(tb_edgness.upperLeft() + Diff2D(w, h),
					tb_edgness.upperLeft() + vigra::Diff2D(w+1, h+1)),
					BImage::PixelType(0) );	
					//cout << w << "," << h << ":\t" << value[0] << endl;				
				}			
			}
		}
		// Use this output for edges rather than Canny
		//if(tb_edge_threshold == bt_threshold && detector == 3) image = tb_edgness;
		if(detector == 3) image = tb_edgness;

		if (generate_images){

			string output = path;
			vector<string> tokens;

			// *nix file paths
			if (filename.find("/") < filename.length() || filename.substr(0, 1) == ("/")){					
				tokenize(filename, tokens, "/");
				output.append(tokens[tokens.size()-1].substr(0,tokens[tokens.size()-1].length()-4));		
			// Windows file paths
			}else{
				tokenize(filename, tokens, "\\");
				output.append(tokens[tokens.size()-1].substr(0,tokens[tokens.size()-1].length()-4));
			}
		
			string edgeness_filename = output;
			string cornerness_filename = output;		
		
			cornerness_filename.append("_bt_cornerness");			
			cornerness_filename.append(format);
			//if(!fileexists(cornerness_filename)){
				cout << "Writing " << cornerness_filename << endl;
				exportImage(srcImageRange(cornerness), ImageExportInfo(cornerness_filename.c_str()).setPixelType("UINT8"));
			//}
		
			edgeness_filename.append("_bt_edgeness_");
		
			stringstream dt;
			//dt << bt_edge_threshold;
			dt << bt_threshold;
			edgeness_filename.append(dt.str());

			edgeness_filename.append(format);
			//if(!fileexists(edgeness_filename)){
				cout << "Writing " << edgeness_filename << endl;			
				exportImage(srcImageRange(tb_edgness), ImageExportInfo(edgeness_filename.c_str()).setPixelType("UINT8"));
			//}

		}
	
		//tb_edge_threshold++;
	//}
}

bool getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, long & value){
	Exiv2::ExifKey key(keyName);
	Exiv2::ExifData::iterator itr = exifData.findKey(key);
	if (itr != exifData.end() && itr->count()) {
		value = itr->toLong();
		//DEBUG_DEBUG("" << keyName << ": " << value);
		return true;
	}else{
		return false;
	}
}

bool getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, float & value){
	Exiv2::ExifKey key(keyName);
	Exiv2::ExifData::iterator itr = exifData.findKey(key);
	if (itr != exifData.end() && itr->count()) {
		value = itr->toFloat();
		//DEBUG_DEBUG("" << keyName << ": " << value);
	return true;
	}else{
		return false;
	}
}

void calculate_focal_length_pixels(){

	if(lens_type == 1) cout << "Rectilinear lens" << endl;
	if(lens_type == 2) cout << "Equal-area fisheye lens" << endl;
	if(lens_type == 3) cout << "Equal-angle fisheye lens" << endl;

	if (cropFactor) cout << "Crop factor:\t\t\t" << cropFactor << endl;		

	double pixels_per_mm = 0;	
	if (cropFactor != 0){	
		if (cropFactor > 1){	
			if (original_width > original_height){	
				pixels_per_mm = original_height * (cropFactor/24.0);
	
			}else{
				pixels_per_mm = original_width * (cropFactor/24.0);	
			}
		}else{
			if (original_width > original_height){	
				pixels_per_mm = original_height * (24.0/cropFactor);
	
			}else{
				pixels_per_mm = original_width * (24.0/cropFactor);	
			}		
		}
		pixel_density = pixels_per_mm;  // post global
		cout << "Pixel density (pixels/mm):\t" << pixels_per_mm << endl;			
	}else{	
		// Need command line param		
		if(!straighten_verts){
			cout << endl << "Could not identify crop factor from EXIF data. Please provide it as a command line parameter with the -c flag." << endl << endl;
			exit(1);
		}	
	}
	
	if (focal_length != 0){	
	
		focal_length_pixels = focal_length*pixels_per_mm;	
	
		cout << "Focal length (mm):\t\t" << focal_length << endl;
		cout << "Focal length (pixels):\t\t" << focal_length_pixels << endl << endl;	
	}else{
		// Need command line param	
		if(!straighten_verts){
			cout << endl << "Could not identify focal length from EXIF data. Please provide it as a command line parameter with the -f flag." << endl << endl;
			exit(1);
		}	
	}

}

int get_exif_data(string& filename){

	// Read exif data using jhead
	// Needed to check if exif.CameraModel == "Canon EOS 20D"
        /*
	ImageInfo_t exif;
        ResetJpgfile();
        memset(&exif, 0, sizeof(exif));
        ReadJpegFile(exif,filename.c_str(), READ_EXIF);
	*/

	// Read exif data using exiv2
    	#ifdef HUGIN_USE_EXIV2 

	typedef struct {
		double x, y;
	} sensor;
        	   
   	double width = original_width;
	double height = original_height;
	    
    	Exiv2::Image::AutoPtr image;
    	try {
        	image = Exiv2::ImageFactory::open(filename.c_str());
    	}catch(...) {
        	if(!straighten_verts){
			cout << "Error opening file " << filename << endl;
			cout << "Please provide crop factor and focal length values via command line parameters." << endl << endl;
        		exit(1);
		}
    	}
    	if (image.get() == 0) {
        	if(!straighten_verts){
			cout << "Unable to open file " << filename << " to read EXIF data." << endl;
			cout << "Please provide crop factor and focal length values via command line parameters." << endl << endl;
        		exit(1);
		}
    	}

    	image->readMetadata();
    	Exiv2::ExifData &exifData = image->exifData();
    	if (exifData.empty()) {
        	if(!straighten_verts){
			cout << "Unable to read EXIF data from file " << filename << endl;
			cout << "Please provide crop factor and focal length values via command line parameters." << endl << endl;
        		exit(1);
		}
    	}
    
    	long eWidth = 0;
    	getExiv2Value(exifData,"Exif.Image.ImageWidth",eWidth);

    	long eLength = 0;
    	getExiv2Value(exifData,"Exif.Image.ImageLength",eLength);

    	double sensorPixelWidth = 0;
    	double sensorPixelHeight = 0;
    	if (eWidth > 0 && eLength > 0) {
        	sensorPixelHeight = (double)eLength;
        	sensorPixelWidth = (double)eWidth;
    	} else {
        	// No EXIF information, use number of pixels in image
        	sensorPixelWidth = width;
        	sensorPixelHeight = height;
    	}

    	// force landscape sensor orientation
    	if (sensorPixelWidth < sensorPixelHeight ) {
        	double t = sensorPixelWidth;
        	sensorPixelWidth = sensorPixelHeight;
        	sensorPixelHeight = t;
    	}

    	// some cameras do not provide Exif.Photo.FocalPlaneResolutionUnit
    	// notably some Olympus
    	long exifResolutionUnits = 0;
    	getExiv2Value(exifData,"Exif.Photo.FocalPlaneResolutionUnit",exifResolutionUnits);

    	float resolutionUnits= 0;
    	switch (exifResolutionUnits) {
        	case 3: resolutionUnits = 10.0; break;  //centimeter
        	case 4: resolutionUnits = 1.0; break;   //millimeter
        	case 5: resolutionUnits = .001; break;  //micrometer
        	default: resolutionUnits = 25.4; break; //inches
    	}

    	float fplaneXresolution = 0;
    	getExiv2Value(exifData,"Exif.Photo.FocalPlaneXResolution",fplaneXresolution);

    	float fplaneYresolution = 0;
    	getExiv2Value(exifData,"Exif.Photo.FocalPlaneYResolution",fplaneYresolution);

    	float CCDWidth = 0;
    	if (fplaneXresolution != 0) { 
        	CCDWidth = (float)(sensorPixelWidth / ( fplaneXresolution / resolutionUnits));
   	 }

    	float CCDHeight = 0;
    	if (fplaneYresolution != 0) {
        	CCDHeight = (float)(sensorPixelHeight / ( fplaneYresolution / resolutionUnits));
    	}

    	// calc sensor dimensions if not set and 35mm focal length is available
    	sensor sensorSize;
    	if (CCDHeight > 0 && CCDWidth > 0) {
        	// read sensor size directly.
        	sensorSize.x = CCDWidth;
        	sensorSize.y = CCDHeight;
        	/*
		if (exif.CameraModel == "Canon EOS 20D") {
            		// special case for buggy 20D camera
            		sensorSize.x = 22.5;
            		sensorSize.y = 15;
        	}
		*/
        	//
        	// check if sensor size ratio and image size fit together
        	double rsensor = (double)sensorSize.x / sensorSize.y;
        	double rimg = (double) width / height;
        	if ( (rsensor > 1 && rimg < 1) || (rsensor < 1 && rimg > 1) ) {
            		// image and sensor ratio do not match
            		// swap sensor sizes
            		float t;
            		t = sensorSize.y;
            		sensorSize.y = sensorSize.x;
            		sensorSize.x = t;
        	}

        	cropFactor = sqrt(36.0*36.0+24.0*24.0) /
            	sqrt(sensorSize.x*sensorSize.x + sensorSize.y*sensorSize.y);
        	// FIXME: HACK guard against invalid image focal plane definition in EXIF metadata with arbitrarly chosen limits for the crop factor ( 1/100 < crop < 100)
        	if (cropFactor < 0.01 || cropFactor > 100) {
            		cropFactor = 0;
        	}
    	}else{
        	// alternative way to calculate the crop factor for Olympus cameras

        	// Windows debug stuff
        	// left in as example on how to get "console output"
        	// written to a log file    
        	// freopen ("oly.log","a",stdout);
        	// fprintf (stdout,"Starting Alternative crop determination\n");        
        	float olyFPD = 0;
        	getExiv2Value(exifData,"Exif.Olympus.FocalPlaneDiagonal",olyFPD);

        	if (olyFPD > 0.0) {        
            		// Windows debug stuff
            		// fprintf(stdout,"Oly_FPD:");
            		// fprintf(stdout,"%f",olyFPD);
            		cropFactor = sqrt(36.0*36.0+24.0*24.0) / olyFPD;
        	}
   
    	}

    	float efocal_length = 0;
    	getExiv2Value(exifData,"Exif.Photo.FocalLength",efocal_length);

    	float efocal_length35 = 0;
    	getExiv2Value(exifData,"Exif.Photo.FocalLengthIn35mmFilm",efocal_length35);

	double new_focal_length = 0,new_cropFactor = 0;

    	//The various methods to detmine crop factor
    	if (efocal_length > 0 && cropFactor > 0) {
        	// user provided crop factor
       		new_focal_length = efocal_length;
    	}else if (efocal_length35 > 0 && efocal_length > 0) {
        	new_cropFactor = efocal_length35 / efocal_length;
        	new_focal_length = efocal_length;
    	}else if (efocal_length35 > 0) {
        	// 35 mm equiv focal length available, crop factor unknown.
        	// do not ask for crop factor, assume 1.  Probably a full frame sensor
        	new_cropFactor = 1;
        	new_focal_length = efocal_length35;
    	}else if (efocal_length > 0 && cropFactor <= 0) {
        	// need to redo, this time with crop
        	new_focal_length = efocal_length;
        	new_cropFactor = 0;
    	}
	
	if(focal_length != 0 && focal_length != new_focal_length && new_focal_length){
		cout << "Warning: This image appears to have a different focal length from the previous one." << endl << endl;
	}else{
		if(new_focal_length) focal_length = new_focal_length;
	}

	if(cropFactor && cropFactor != new_cropFactor && new_cropFactor){
		cout << "Warning: This image appears to have a different crop factor from the previous one." << endl << endl;
		//cout << "cropFactor:\t" << cropFactor << " --- new_cropFactor:\t" << new_cropFactor<< endl;
	}else{
		if(new_cropFactor) cropFactor = new_cropFactor;
	}
	
	/*
 	cout << "Width:\t\t" << width << endl;
	cout << "Height:\t\t" << height << endl;
 	cout << "eWidth:\t\t" << eWidth << endl;
	cout << "eHeight:\t" << eLength << endl;
	cout << "Focal length:\t" << focal_length << endl;
	cout << "Crop factor:\t" << cropFactor << endl;
        cout << "sensorSize.x:\t" << sensorSize.x << endl;
	cout << "sensorSize.y:\t" << sensorSize.y << endl;
    	cout << "CCDHeight:\t" << CCDHeight << endl;
    	cout << "CCDWidth:\t" << CCDWidth << endl;
    	cout << "sensorPixelWidth:\t" << sensorPixelWidth << CCDWidth << endl;
    	cout << "sensorPixelHeight:\t" << sensorPixelHeight << CCDWidth << endl;
    	cout << "Resolution Units:\t" << resolutionUnits << CCDWidth << endl << endl;
	*/
#endif
	calculate_focal_length_pixels();
	return(1);

}

void resize_image(UInt16RGBImage& in, int& nw, int& nh){
	
        // Re-size to max dimension
	if (in.width() > resize_dimension || in.height() > resize_dimension){
				
		if (in.width() >= in.height()){
			sizefactor = (double)resize_dimension/in.width();
        		// calculate new image size
        		nw = resize_dimension;
        		nh = static_cast<int>(0.5 + (sizefactor*in.height()));
			min_line_length_squared = (length_threshold*nw)*(length_threshold*nw);
		}else{
			sizefactor = (double)resize_dimension/in.height();
       			// calculate new image size
        		nw = static_cast<int>(0.5 + (sizefactor*in.width()));
        		nh = resize_dimension;
			min_line_length_squared = (length_threshold*nh)*(length_threshold*nh);
		}        			
		
		cout << "Scaling by:\t\t" << sizefactor << endl;
		cout << "New dimensions:\t\t" << nw << " x " << nh << endl;
		cout << "Minimum line length:\t" << sqrt(min_line_length_squared) << endl << endl;
			
        	// create an image of appropriate size    		
		UInt16RGBImage out(nw, nh);
	
		// resize the image, using a bi-cubic spline algorithm
		resizeImageNoInterpolation(srcImageRange(in),destImageRange(out));

		in = out;
	}
}

bool compare_line_length(vector<vigra::Point2D> l1, vector<vigra::Point2D> l2){

	// Compare by line length
	//return(line_length_squared(l1[0].x,l1[0].y,l1[l1.size()-1].x,l1[l1.size()-1].y) > line_length_squared(l2[0].x,l2[0].y,l2[l2.size()-1].x,l2[l2.size()-1].y));
	
	// Compare by number of pixels
	return(l1.size() > l2.size());
}

void sort_lines_by_length(){
	sort(lines.begin(), lines.end(), compare_line_length);
}

double point_line_length_squared(Point2D& a, Point2D& b){
	//cout << a->x << " - " << b->x << " = " <<  a->x - b->x << endl;
	//cout << a->y<< " - " << b->y <<" = " << a->y - b->y << endl;
	int d1 = a->x - b->x;
	int d2 = a->y - b->y;
	return(d1*d1+d2*d2);
}

int min_x_index(vector<Point2D>& a){
        int smallest = 100000000;
	int index = 0;
        for(int i = 0; i < a.size(); i++){
                if(a[i]->x < smallest){
			smallest = a[i]->x;
                        index = i;
                }
        }
        return(index);
}

int max_x_index(vector<Point2D>& a){
        int largest = -100000000;
	int index = 0;
        for(int i = 0; i < a.size(); i++){
                if(a[i]->x > largest){
			largest = a[i]->x;
                        index = i;
                }
        }
        return(index);
}
int min_y_index(vector<Point2D>& a){
        int smallest = 100000000;
	int index = 0;
        for(int i = 0; i < a.size(); i++){
                if(a[i]->y < smallest){
			smallest = a[i]->y;
                        index = i;
                }
        }
        return(index);
}

int max_y_index(vector<Point2D>& a){
        int largest = -100000000;
	int index = 0;
        for(int i = 0; i < a.size(); i++){
                if(a[i]->y > largest){
			largest = a[i]->y;
                        index = i;
                }
        }
        return(index);
}

double gradient(vector<Point2D>& a){	
	
	int max_x = max_x_index(a);
	int min_x = min_x_index(a);
	int max_y = max_y_index(a);
	int min_y = min_y_index(a);
	double d1 = a[max_x]->x - a[min_x]->x;
	double d2 = a[max_y]->y - a[min_y]->y;
	double grad = d2/d1;		
	return(grad);
}

double inv_gradient(vector<Point2D>& a){	
	
	int max_x = max_x_index(a);
	int min_x = min_x_index(a);
	int max_y = max_y_index(a);
	int min_y = min_y_index(a);
	double d1 = a[max_x]->x - a[min_x]->x;
	double d2 = a[max_y]->y - a[min_y]->y;
	double grad = d1/d2;		
	return(grad);
}

void join_short_segments(vector<vector<Point2D> >& short_segments, unsigned int& good_lines, BImage& image){

	/*
        string jseg("joined_");
        string seg("seg_");        
        for(int i = 0; i < short_segments.size(); i++){
                plot_inliers(seg, image, short_segments[i], i);
		//cout << "Gradient:\t" << gradient(short_segments[i]) << endl;
        }	
	*/
	
	int js = 0;
	
	// Distance squared that two segments can be separated by
	int limit = 1000;
	
	// Max difference allowed between gradients of two lines
	double gradient_diff = 0.5;
	
	// For horizontal lines, max difference in y coordinate between closest points
	// and in x coordinate for vertical lines
	double diff = 4;
	
	vector<Point2D> joined_segs;
	map<double,int> closest_index;	
	int count = 0, original_line_size = 0;
	while(short_segments.size()){

		// Of the close segments found, add the closest segment to joined_segs
		if(closest_index.size()){
			int j = 0, c = 0;	
			map<double,int>::iterator it_h1;	
			for (it_h1 = closest_index.begin(); it_h1 != closest_index.end(); it_h1++){
				if(!c){
					//cout << js << " Dist:\t" << (*it_h1).first<< endl;
					j = (*it_h1).second;
				}
				c++;
			}					
					
			for(int k = 0; k < short_segments[j].size(); k++){
				joined_segs.push_back(short_segments[j][k]);
			}
			short_segments.erase(short_segments.begin()+j);		
			closest_index.clear();
		}else{
                	joined_segs = short_segments[0];
                	original_line_size = joined_segs.size();
                	short_segments.erase(short_segments.begin());
		}

                for(int j = 0; j < short_segments.size(); j++){

			//plot_inliers(seg, image, short_segments[j], j);
                        //cout << "Lines " << count << " vs " << j << endl;
			
 			// Get indexes of max/min x and y points
                        int i_x_max = max_x_index(joined_segs);    
			int i_x_min = min_x_index(joined_segs);
			int i_y_max = max_y_index(joined_segs);
                        int i_y_min = min_y_index(joined_segs);
                        int j_x_max = max_x_index(short_segments[j]);
                        int j_x_min = min_x_index(short_segments[j]);
                        int j_y_max = max_y_index(short_segments[j]);
                        int j_y_min = min_y_index(short_segments[j]);

			// Check to see if lines overlap on x or y axes
			int overlaps = 0;
 			int i_horiz = 0, j_horiz = 0;;
			int other_dim_diff;
			if(abs(joined_segs[i_x_max]->x - joined_segs[i_x_min]->x) > abs(joined_segs[i_y_max]->y - joined_segs[i_y_min]->y)){
				i_horiz = 1;
				 // Horizontal line
                                 if (joined_segs[i_x_max]->x > short_segments[j][j_x_max]->x){
					if(short_segments[j][j_x_max]->x > joined_segs[i_x_min]->x) overlaps = 1;
					other_dim_diff = abs(short_segments[j][j_x_max]->y - joined_segs[i_x_min]->y);
	
				}else{
					if(joined_segs[i_x_max]->x > short_segments[j][j_x_min]->x) overlaps = 1;
					other_dim_diff = abs(joined_segs[i_x_max]->y - short_segments[j][j_x_min]->y);
				}
			}else{
				// Vertical line
				if (joined_segs[i_y_max]->y > short_segments[j][j_y_max]->y){
					if(short_segments[j][j_y_max]->y > joined_segs[i_y_min]->y) overlaps = 1; 					
					other_dim_diff = abs(short_segments[j][j_y_max]->x - joined_segs[i_y_min]->x);
	
				}else{
					if(joined_segs[i_y_max]->y > short_segments[j][j_y_min]->y) overlaps = 1;					
					other_dim_diff = abs(joined_segs[i_y_max]->x - short_segments[j][j_y_min]->x);

				}
			}
			if(abs(short_segments[j][j_x_max]->x - short_segments[j][j_x_min]->x) > abs(short_segments[j][j_y_max]->y - short_segments[j][j_y_min]->y)){
				j_horiz = 1;
			}

			// Make sure both lines appear to be horizontal or vertical
			if(i_horiz == j_horiz){			
				// Measure this distance between the nearest points of the two segments
				double dist;
				if(!overlaps){
					if(i_horiz){
						if (joined_segs[i_x_max]->x > short_segments[j][j_x_max]->x){					
							dist = point_line_length_squared(joined_segs[i_x_min],short_segments[j][j_x_max]);					
						}else{
							dist = point_line_length_squared(short_segments[j][j_x_min],joined_segs[i_x_max]);
						}
					}else{
						if (joined_segs[i_y_max]->y > short_segments[j][j_y_max]->y){					
							dist = point_line_length_squared(joined_segs[i_y_min],short_segments[j][j_y_max]);					
						}else{
							dist = point_line_length_squared(short_segments[j][j_y_min],joined_segs[i_y_max]);
						}
					}				
				}	
				// Add this segment index to closest_index if it doesn't overlap and is close enough
				if(!overlaps && dist < limit){
					// Check to make sure segment gradients are similar
					// Differences in vertical line gradients can be huge so invert them
					double grad_i,grad_j;					
					if(i_horiz == 1){
						grad_i = gradient(joined_segs);
						grad_j = gradient(short_segments[j]);					
					}else{
						grad_i = inv_gradient(joined_segs);
						grad_j = inv_gradient(short_segments[j]);						
					}
					
	
					if(abs(abs(grad_j)-abs(grad_i)) < gradient_diff){
						//cout << "OK " << js << " - gradient diff:\t" << abs(grad_j-grad_i) << endl;
						//cout << grad_i << " vs " << grad_j << endl << endl;
						//cout << a << endl << b << endl << c << endl << d << endl << endl;
  						// Add to map, we'll combine the closest segment to joined_segs
			
						//closest_index[dist] = j;
												
						if(other_dim_diff <= diff){
							closest_index[(double)other_dim_diff] = j;
						}
						
						/*
						printf("i:  x_min: %d y_min: %d --- x_max: %d y_max: %d\n",joined_segs[i_x_min]->x,joined_segs[i_y_min]->y,joined_segs[i_x_max]->x,joined_segs[i_y_max]->y);							
						printf("j:  x_min: %d y_min: %d --- x_max: %d y_max: %d\n",short_segments[j][j_x_min]->x,short_segments[j][j_y_min]->y,short_segments[j][j_x_max]->x,short_segments[j][j_y_max]->y);
						cout << "other_dimension:\t" << other_dim_diff << endl;
						cout << "gradients:\t" << grad_i << " vs " << grad_j << endl;
						cout << "dist:\t" << dist << "--------------" << endl << endl;
						*/
						
						//plot_inliers(jseg, image, joined_segs, js);
						//js++;
 					}else{
						//cout << "Gradients differ too much:\t" << abs(abs(grad_j)-abs(grad_i)) << endl;
						//cout << grad_i << " vs " << grad_j << endl << endl;
					}
				}else{
					if (dist > limit){
						//cout << "Dist " << dist << " > " << limit << endl << endl;
					}else{
						//cout << "Lines overlap." << endl << endl;
					}
				}
			}else{
				//cout << "Lines don't seem to have the same orientation." << endl << endl;
			}
                }

		// Add joined_seq to lines vector if it's long enough
                if(joined_segs.size() > original_line_size && closest_index.size() == 0){
                        double length_sq;
                        int i_x_max = max_x_index(joined_segs);
                        int i_x_min = min_x_index(joined_segs);
                        int i_y_max = max_y_index(joined_segs);
                        int i_y_min = min_y_index(joined_segs);
                        if(abs(i_x_max - i_x_min) > abs(i_y_max - i_y_min)){
                                length_sq = point_line_length_squared(joined_segs[i_x_max],joined_segs[i_x_min]);
				sort(joined_segs.begin(), joined_segs.end(), compare_yx);	
                        }else{
                                length_sq = point_line_length_squared(joined_segs[i_y_max],joined_segs[i_y_min]);
				sort(joined_segs.begin(), joined_segs.end(), compare_yx);	
                        }
                        if(length_sq >= min_line_length_squared){
                                //plot_inliers(jseg, image, joined_segs, js);
                                lines.push_back(joined_segs);
                                good_lines++;
                                js++;
			}
                }
		count++;
        }
}


double path_distance(vector<Point2D>& coords, int& a, int& b){
	double d = 0;
	for (unsigned int i = a; i < b; i++){ 	
		//cout <<"From path_dist - point_line_length_squared " << i << "-" << i+1 << endl;
		//if(a < coords.size() && b < coords.size()){
			
			d += point_line_length_squared(coords[i],coords[i+1]);
		//}else{
		//	cout << a << "-" << b << " path_dist, size = " << coords.size() << " " << i << " " << i+1<< " : " << coords[i]->x << "-" << coords[i]->y << "--->" << coords[i+1]->x << "," << coords[i+1]->y << endl;
		//	d += point_line_length_squared(coords[i],coords[i+1]);
		//}
	}
	return(d);
}


bool is_line(vector<Point2D>& coords, int& a, int& b){
	double threshold = 0.95;
	//cout <<"From is_line - point_line_length_squared " << a << "-" << b << endl;
	double distance = sqrt(point_line_length_squared(coords[a],coords[b]));
	double path_dist = path_distance(coords,a,b);
	if(distance/path_dist > threshold){
		return(true);
	}else{
		return(false);
	}
}

int halfway_corners(vector<double>& straws, int& a, int& b){
	int quarter = (b-a)/4;
	int MinIndex;
	double MinValue = 100000000;	
	for (unsigned int i = a+quarter; i < b-quarter; i++){
		if(straws[i] < MinValue){
			MinValue = straws[i];
			MinIndex = i;
		}	
	}
	return(MinIndex);
}

double sample_spacing(vector<Point2D>& coords){
	int x_max = max_x_index(coords);
	int x_min = min_x_index(coords);
	int y_max = max_y_index(coords);
	int y_min = min_y_index(coords);
	Point2D top_left(coords[x_min]->x,coords[y_min]->y);
	Point2D bottom_right(coords[x_max]->x,coords[y_max]->y);	
	return(sqrt(point_line_length_squared(top_left,bottom_right)/40.0));
}

void resample(vector<Point2D>& coords, vector<Point2D>& resampled, double& s){
	double D = 0;
	resampled.push_back(coords[0]);
	for (unsigned int i = 1; i < coords.size(); i++){
		double d = sqrt(point_line_length_squared(coords[i-1],coords[i]));
		if (d+D >= s){		
			Point2D q;
			// Need to cast to int
			q.x = (int)(coords[i-1]->x + ((s-D)/d) * (coords[i]->x - coords[i-1]->x));
			q.y = (int)(coords[i-1]->y + ((s-D)/d) * (coords[i]->y - coords[i-1]->y));
			resampled.push_back(q);
			vector<Point2D>::iterator it = coords.begin();;
			coords.insert(it+i,q);
			D = 0;
		}else{
			D += d;
		}		
	}
}

void short_straw(BImage& image, vector<Point2D>& coords, string& filename){
	
	vector<Point2D> coords_original = coords;;
	vector<Point2D> points;

	// Resample points?
	if(0){
		cout << "Resampling points.." << endl;
		double s = sample_spacing(coords);
		vector<Point2D> resampled;
		resample(coords,resampled,s);	
		points = resampled;
	}else{
		points = coords;
	}

	vector<int> corners,processed_corners;
	corners.push_back(0);
	int w = 3;
	vector<double> straws, sorted_straws;

	for (unsigned int i = w; i < points.size() - w; i++){ 		
		straws.push_back(sqrt(point_line_length_squared(points[i-w],points[i+w])));
	}
	sorted_straws = straws;
	sort (sorted_straws.begin(),sorted_straws.end());

     	double median_t;
	int middle_one,middle_two;
           
     	if ((sorted_straws.size() % 2) == 0){
        	middle_one = (sorted_straws.size()/2);
        	middle_two = (sorted_straws.size()/2)+1;
       	 	median_t = (sorted_straws[middle_one-1] + sorted_straws[middle_two-1])/2;
     	}else{
		middle_one = (sorted_straws.size()/2)+1;
		median_t = sorted_straws[middle_one-1];
     	}
	median_t *= 0.95;

	double localMin;
	int localMinIndex;
	// Get corners
	for (unsigned int i = w; i < points.size() - w; i++){ 
		if(straws[i] < median_t){		
			localMin = 100000000;
			localMinIndex = i;			
			while(i < straws.size() && straws[i] < median_t){			
				if(straws[i] < localMin){				
					localMin = straws[i];
					localMinIndex = i;
				}
				i++;		
			}
			corners.push_back(localMinIndex);
		}
	}
	corners.push_back(points.size()-1);

	if(1){
		// Post-process corners
		cout << "Post-processing corners..." << endl;
		bool cont = false;
		while(!cont){
			cont = true;
			for (unsigned int i = 1; i < corners.size(); i++){
				cout << i << "/" << corners.size() << endl;	
				int c1 = corners[i-1];
				int c2 = corners[i];			
				if(!is_line(points,c1,c2)){			
					int newcorner = halfway_corners(straws,c1,c2);
					vector<int>::iterator it = corners.begin();
					corners.insert(it+i,newcorner);
					cont = false;
				}		
			}	
		}
		cout << " Retaining good corners..." << endl;
		for (unsigned int i = 1; i <corners.size()-1; i++){
			int c1 = corners[i-1];
			int c2 = corners[i+1];
			if(!is_line(points,c1,c2)){
				processed_corners.push_back(i);		
			}	
		}
		corners = processed_corners;
	}

	// Write files/remove corners
	BImage ss_corners(image.width(), image.height());
	ss_corners = 255;

	for (unsigned int i = w; i < corners.size() - w; i++){ 	
	
		int w = coords_original[corners[i]]->x;
		int h = coords_original[corners[i]]->y;

		// Change the size of the box to draw around corner pixel
		int size = 3;				
		int left = w-size;
		int top = h-size;
		int right = w+size;
		int bottom = h+size;

		if(left<0) left = 0;
		if(right>image.width()) right = image.width();
		if(top<0) top = 0;
		if(bottom > image.height()) bottom = image.height();

		// Draw a white box around the corner pixel
		// in the edge image
		vigra::initImage(srcIterRange(ss_corners.upperLeft() +
		vigra::Diff2D(left, top),
			ss_corners.upperLeft() + vigra::Diff2D(right, bottom)),
                	0 );	

		vigra::initImage(srcIterRange(image.upperLeft() +
		vigra::Diff2D(left, top),
			image.upperLeft() + vigra::Diff2D(right, bottom)),
                	255 );	
	
	}

	if (generate_images){

		string output = path;
		vector<string> tokens;

		// *nix file paths
		if (filename.find("/") < filename.length() || filename.substr(0, 1) == ("/")){					
			tokenize(filename, tokens, "/");
			output.append(tokens[tokens.size()-1].substr(0,tokens[tokens.size()-1].length()-4));		
		// Windows file paths
		}else{
			tokenize(filename, tokens, "\\");
			output.append(tokens[tokens.size()-1].substr(0,tokens[tokens.size()-1].length()-4));
		}
		string output_minus_ss_corners = output;
		output.append("_short_straw_corners");
		output.append(format);

		//if(!fileexists(output)){
			cout << "Writing " << output << endl;
        		exportImage(srcImageRange(ss_corners), ImageExportInfo(output.c_str()).setPixelType("UINT8"));
		//}
		
		
		output_minus_ss_corners.append("_minus_short_straw_corners");
		output_minus_ss_corners.append(format);

		//if(!fileexists(output)){
			cout << "Writing " << output_minus_ss_corners << endl << endl;
        		exportImage(srcImageRange(image), ImageExportInfo(output_minus_ss_corners.c_str()).setPixelType("UINT8"));
		//}
	}
}

void process_image(string& filename, int& plotted){

	ImageImportInfo info(filename.c_str());
        
	UInt16RGBImage in(info.width(), info.height());

	importImage(info, destImage(in));

        int nw = info.width(), nh = info.height();
	
	// If this is the 2nd or subsequent image	
	if(original_width && original_height && nw != original_width && nh != original_height){		
		// Allow for switching between portrait and landscape	
		if (nh != original_width && nw != original_height){
			cout << "Warning: This image appears to have different dimensions from the previous one." << endl << endl;
		}
	}
	
	original_width = info.width();
	original_height = info.height();

	//int exif_ok = get_exif_data(filename);
		
	if (focal_length && cropFactor){
		calculate_focal_length_pixels();
	}else{
		int exif_ok = get_exif_data(filename);
	}
		
	if (nw > nh){
		min_line_length_squared = (length_threshold*nw)*(length_threshold*nw);
	}else{
		min_line_length_squared = (length_threshold*nh)*(length_threshold*nh);
	}


	// Resize image
	resize_image(in, nw, nh);

	// Convert to greyscale
	BImage grey(nw, nh);
	copyImage(srcImageRange(in, RGBToGrayAccessor<RGBValue<UInt16> >()), destImage(grey));

    BImage image(nw, nh);

	// Run Canny edge detector
	if(detector != 3){
		detect_edge(grey, filename, image);
	}else{
		image = grey;
	}
#if 1	// test find_N8_lines

	bool saveimages = true;	// write debug pix
	int lmin = int(sqrt(min_line_length_squared));
	double flpix = focal_length_pixels;

	cout<<"  edgeMap2linePts(saveimages "<<saveimages<<"): ";
	int nsegs = edgeMap2linePts( image, saveimages );
	cout<<nsegs<<" segments"<<endl;

	cout<<"  linePts2lineList(lmin "<<lmin<<" flpix "<<flpix<<") "<<endl;
	int nlines = linePts2lineList( image, lmin, flpix, lines );
	cout<<"  "<<nlines<<" lines"<<endl;

	if (nlines == 0){
		cout << "No lines found!" << endl << endl;
	}else{
		cout << nlines << " lines found" << endl;
/*
		int llp = lines.size() - 1;
		plot_lines(filename, image, lines, plotted, llp );
		plotted = llp;
*/
		cout << endl;
	}

    int good_lines;
	good_lines = nlines;

#else	// original

	FVector2Image edgeness(nw,nh);
	FImage cornerness(nw, nh);
	vector<Point2D> coords, inliers;

	// Run short straw corner finding algorithm
	if(ss){	
		cout << "Running ShortStraw corner finder..." << endl;
		extract_coords(image, coords);	
		short_straw(image, coords, filename);	
		coords.clear();
	}else{	
		// Generate boundary tensor
		generate_boundary_tensor(image, edgeness, cornerness, filename);	
		// Remove corners
		nuke_corners(image, cornerness, filename);	
	}
	
	// Get coordinates
	extract_coords(image, coords);	
		
	unsigned int intital_coords_size = coords.size();
	
	if(verbose) cout << "Initial 'edge' pixel count:\t" << coords.size() << endl << endl;
	
	cout << "Detecting lines..." << endl << endl;
	
	vector<vector<Point2D> > short_segments;
			
	// Get 10 strongest lines
	unsigned int good_lines = 0;
	while (good_lines < 10 && coords.size() > 1){
		if(verbose) cout << "Searching for line " << good_lines+1 << ":" << endl;
		if(verbose) cout << "=====================" << endl << endl;
		find_ann(coords, edgeness, inliers, good_lines, short_segments);
		if(inliers.size()){
			//sort_inliers(inliers);
			lines.push_back(inliers);
			inliers.clear();		
		}			
		if(verbose) cout << "Remaining coords:\t" << coords.size() << "\t(" << 100*((double)coords.size()/intital_coords_size) << " %)" << endl << endl;
		
	}		

	if(short_segments.size()){
		cout << "Joining segments..." << endl << endl;
		join_short_segments(short_segments,good_lines,image);	
	}
	short_segments.clear();	
		
	// Sort lines by distance between first and last point or number of pixels
	// Problem if we're doing multiple images
	// Sort_lines_by_length();

	if (good_lines == 0){
		cout << "No lines found!" << endl << endl;
	}else{
		// Create image for each line
		for (int i = plotted; i < lines.size(); i++){
			//cout << "plotted = " << plotted << " --- " << " line.size() = " << lines.size() << " --- i = " << i << endl;
			plot_inliers(filename, image, lines[i],i+1);
			plotted++;	
		}
		cout << endl;
	}
#endif	// testing find_N8_lines
}
