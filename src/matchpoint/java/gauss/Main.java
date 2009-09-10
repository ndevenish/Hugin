/***************************************************************************
 *   Copyright (C) 2007 by Zoran Mesec   *
 *   zoran.mesec@gmail.com   *
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
package gauss;

import java.lang.Math;



/**
 *
 * @author zoran
 */
public class Main {
    
    

    
    /** Creates a new instance of Main */
    public Main() {
        
        
    }
    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        // TODO code application logic here
        //stuff for calculating the second order derivatives gaussian kernels
        double dev=1.2; //standard deviation
        int size=26;
        //
        double border = 3*dev;
        double step = border/size*2;
        double d;
        System.out.println("gxx");
        for(double i=-border;i<=border;i+=step) {
            //for(double i=-3.6;i<=3.6;i+=1.8) {
             for(double j=-border;j<=border;j+=step) {
                d=((Math.pow(i,2)*Math.pow(2*Math.PI*Math.pow(dev,6),-1))*Math.exp(-(Math.pow(i,2)+Math.pow(j,2))/(2*Math.pow(dev,2))))-(Math.pow(2*Math.PI*Math.pow(dev,4),-1)*Math.exp(-(Math.pow(i,2)+Math.pow(j,2))/(2*Math.pow(dev,2))));
                System.out.print(Math.round(100000*d)/8+" ");
                //System.out.print(d+" ");
             }
             System.out.println();
        }
        System.out.println("gxy");
        for(double i=-border;i<=border;i+=step) {
             for(double j=-border;j<=border;j+=step) {
                 d=i*j*Math.pow(Math.PI*2*Math.pow(dev,6),-1)*Math.exp(-(Math.pow(i,2)+Math.pow(j,2))/(2*Math.pow(dev,2)));
                 System.out.print(Math.round(1.173*(10000*d))+" ");
             }
             System.out.println();
        }        
    }
    
}
