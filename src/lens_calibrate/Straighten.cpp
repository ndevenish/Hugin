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

#include <map>
#include "Globals.h"
#include "ProcessImage.h"

using namespace std;

double straighten(){

	double invert_size_factor = 1.0/sizefactor;
	double best_angle = 0, best_score = 10000000;
	//map<double,double> best_rotation;

	cout << "Finding optimal rotation angle..." << endl;
	for(double angle = -90; angle < 90; angle += 0.25){
	
		double radians = angle*(M_PI/180);
		//cout << "Degrees:\t" << angle << "\tRadians:\t" << radians << endl;
		double total_x_diff = 0;
	
		for(int i = 0; i < lines.size(); i++){
				
 			// Get indexes of max/min x and y points
			int i_x_max = max_x_index(lines[i]);    
			int i_x_min = min_x_index(lines[i]);
			int i_y_max = max_y_index(lines[i]);
 			int i_y_min = min_y_index(lines[i]);
			int x_diff = lines[i][i_x_max]->x - lines[i][i_x_min]->x;
			int y_diff = lines[i][i_y_max]->y - lines[i][i_y_min]->y;

			// Vertical line
			if(y_diff > x_diff){				
						
				// Index for coords of top of line
				double y_max_y = invert_size_factor * lines[i][i_y_max]->y;
				double y_max_x = invert_size_factor * lines[i][i_y_max]->x;
				// Index for coords of bottom of line
				double y_min_y = invert_size_factor * lines[i][i_y_min]->y;
				double y_min_x = invert_size_factor * lines[i][i_y_min]->x;
				
				// Move so 0,0 is centre (of rotation)
				y_max_y -= original_height/2;	
				y_max_y *= -1;				
				y_max_x -= original_width/2;	
				
				y_min_y -= original_height/2;	
				y_min_y *= -1;				
				y_min_x -= original_width/2;					

				// New position after rotating
				double rotated_y_max_x = (y_max_x * cos(radians)) - (y_max_y * sin(radians));				

				// New position after rotating
				double rotated_y_min_x = (y_min_x * cos(radians)) - (y_min_y * sin(radians));	

				total_x_diff += fabs(rotated_y_max_x-rotated_y_min_x);			
			}
		}
		
		//cout << angle << "\tTotal:\t" << total_x_diff << endl;

		if(total_x_diff < best_score){
			best_score = total_x_diff;
			best_angle = angle;
		}		
		//best_rotation[angle] = total_x_diff;
		total_x_diff = 0;
	}
	//for(double angle = -90; angle < 90; angle += 0.25){
	//	cout << angle << "\tdegrees - Score:\t" << best_rotation[angle] << endl;

	//}
	//cout << endl;
	return(-best_angle);
}
