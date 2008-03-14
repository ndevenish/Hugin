/*
 * Main.java
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */
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
