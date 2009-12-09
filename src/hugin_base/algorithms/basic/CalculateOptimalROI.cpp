// -*- c-basic-offset: 4 -*-
/** @file PanoramaDataLegacySupport.h
 *
 *  @author <cnidarian>
 *
 *  $Id: CalculateOptimalROI.cpp 2510 2009-9-9 cnidarian $
 *
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "CalculateOptimalROI.h"

namespace HuginBase {

using namespace hugin_utils;

//uses the area as that major searching factor
//else uses width+heigh like the perimeter
#define USEAREA

//new feature, allow some backtracking, slower but less greedy
//allow to shrink the image this much before re-expanding
//#define BACKTRACKPERCENT 0.0025
//increased time too much in several cases (for now zero
#define BACKTRACKPERCENT 0.0

//Used for initial inner search, should be base 2
//the larger, the longer the initial search will take
//TODO: if done with shifts, it would be faster? or at least force base 2
#define MINACC 8


///
bool CalculateOptimalROI::calcOptimalROI(PanoramaData& panorama)
{
    if (panorama.getNrOfImages() == 0)
        return false;

    printf("Down to Algorithm\n");
    PanoramaOptions opt = panorama.getOptions();
    
    totImages=panorama.getNrOfImages();

    //Calculate the very extreme, ignoring the FOV or original settings
    /*for (unsigned i = 0; i < panorama.getNrOfImages(); i++) {
        o_optimalROI|=calcOutsideBox(i,panorama.getSrcImage(i), opt);
        printf("outside  %d %d - %d %d\n",o_optimalROI.left(),o_optimalROI.top(),o_optimalROI.right(),o_optimalROI.bottom());
    }*/
    
    //assume centered?
    //o_optimalROI=vigra::Rect2D(-o_optimalSize.x/2,-o_optimalSize.y/2,o_optimalSize.x/2,o_optimalSize.y/2);
    vigra::Rect2D origROI=opt.getROI();
    vigra::Size2D origSize;
    origSize.x=opt.getWidth();
    origSize.y=opt.getHeight();
    
    printf("Orig Size %dx%d\n",origSize.x,origSize.y);
    printf("Orig ROI %dx%d - %dx%d\n",origROI.left(),origROI.top(),origROI.right(),origROI.bottom());
    
    o_optimalSize=origSize;
    o_optimalROI=vigra::Rect2D(0,0,o_optimalSize.x,o_optimalSize.y);
    //o_optimalROI=origROI;
    
    /*double scalex,scaley;
    
    o_optimalSize.x=o_optimalROI.width();
    o_optimalSize.y=o_optimalROI.height();
    
    printf("Calc Scale: %lfx%lf\n",(double)opt.getWidth()/o_optimalSize.x,(double)opt.getHeight()/o_optimalSize.y);

    printf("Calc Size %dx%d\n",o_optimalSize.x,o_optimalSize.y);
    printf("Calc ROI %dx%d - %dx%d\n",o_optimalROI.left(),o_optimalROI.top(),o_optimalROI.right(),o_optimalROI.bottom());
    */    
    //make memory
    unsigned char *tmp=new unsigned char[o_optimalSize.x*o_optimalSize.y];
    //zero out, unseen 127
    memset(tmp,127,o_optimalSize.x*o_optimalSize.y);
    
    //new on demand version does not draw every pixel
    transfList=new PTools::Transform[totImages];
    imgSizeList=new vigra::Size2D[totImages];
    
    printf("Drawing output regions\n");
    for (unsigned int i = 0; i < panorama.getNrOfImages(); i++) {
        drawOutputRegion(i,tmp,panorama.getSrcImage(i), opt);
    }
    
    printf("Calculate the cropping region\n");
    autocrop(tmp);
    
    o_optimalROI=vigra::Rect2D(best.left,best.top,best.right,best.bottom);
    printf("Crop %dx%d - %dx%d\n",o_optimalROI.left(),o_optimalROI.top(),o_optimalROI.right(),o_optimalROI.bottom());
    printf("Crop Size %dx%d\n",o_optimalROI.width(),o_optimalROI.height());
    

//debug image of the region calculated
#if 0
    FILE *outstream=fopen("/tmp/test.pgm","wb");
    fprintf(outstream,"P5\n");
    fprintf(outstream,"%d %d\n",o_optimalSize.x,o_optimalSize.y);
    fprintf(outstream,"255\n");
    fwrite(tmp,o_optimalSize.x*o_optimalSize.y,1,outstream);
    fclose(outstream);
#endif

    //clean up on demand
    /*for(unsigned int i=0;i<totImages;i++)
    {
        delete transfList[i];
        delete timgList[i];
    }*/
    delete [] tmp;
    delete [] transfList;
    delete [] imgSizeList;

    return true;
}


void CalculateOptimalROI::drawOutputRegion(int imgnum, unsigned char *tmp, const SrcPanoImage & src,
                                           const PanoramaOptions & dest)
{
    vigra::Size2D srcsize=src.getSize();
    
    PTools::Transform transf;
    SrcPanoImage timg = src;
    
//on demand version of inverse transform
#if 1
    //transfList[imgnum]=transf;
    //śtimgList[imgnum]=timg;
    transfList[imgnum].createTransform(timg, dest);
    imgSizeList[imgnum] = srcsize;
#endif


    //sub pixel, one direction
#if 0
    double x,y;
    double xd,yd;
    
    double ox,oy;
    ox=o_optimalROI.left();
    oy=o_optimalROI.top();
    
    int sx,sy;
    sx=o_optimalSize.x;
    sy=o_optimalSize.y;
    
    
       
    //I know it should be an inverse look up, and check if it falls
    //in the image, but, this is easier to do right now
    
    //subpixel scaling - very slow
    //normal scaling with dialate would miss some if not just single spaces
    //for(y=0.0;y<srcsize.y;y+=0.1)
    //{
    //    for(x=0.0;x<srcsize.x;x+=0.1)
    //    {
    transf.createTransform(timg, dest);
    for(y=0.0;y<srcsize.y;y+=0.5)
    {
        for(x=0.0;x<srcsize.x;x+=0.5)
        {
            transf.transformImgCoord(xd,yd,x,y);
            //tmp[(int)floor(yd)*sx+(int)floor(xd)]=255;
            int tx=(int)floor(xd-ox);
            int ty=(int)floor(yd-oy);
            if(tx<0 && ty<0)
            {
                printf("Problem Too Small: %d %d %d\n",tx,ty,ty*sx+tx);
            }
            else if(tx>=sx || ty>=sy)
            {
                printf("Problem Too Big: %d %d %d\n",tx,ty,ty*sx+tx);
            }
            else
            {
                tmp[ty*sx+tx]=255;
            }
        }
    }
#endif

    //inverse transform, run through all pixels
#if 0

    //inverse transform
    //integer, check if in images, scales with image size
    //transf.createInvTransform(timg, dest);
    transf.createTransform(timg, dest);
    int x,y;
    int tx,ty;
    double xd,yd;
    //range between positive and negative
    for(y=0;y<o_optimalSize.y;y++)
    {
        printf("%6.02lf%%\r",100.0*((double)imgnum+(double)y/o_optimalSize.y)/totImages);
        for(x=0;x<o_optimalSize.x;x++)
        {
            //hopefully this is to image
            transf.transformImgCoord(xd,yd,(double)x,(double)y);
            //if in image
            //printf("%lf %lf\n",xd,yd);
            if(xd>=0 && yd>=0 && xd<srcsize.x && yd<srcsize.y)
            {
                //printf("%lf %lf - %d %d\n",xd,yd,(x-o_optimalROI.left()),(y-o_optimalROI.top()));
                //tmp[(y-o_optimalROI.top())*o_optimalROI.width()+(x-o_optimalROI.left())]=127;
                tmp[y*o_optimalSize.x+x]=127;
            }
        }
    }
#endif
    
    return;
}


vigra::Rect2D CalculateOptimalROI::calcOutsideBox(int imgnum, const SrcPanoImage & src,
                                                    const PanoramaOptions & dest)
{
    vigra::Size2D srcsize=src.getSize();
    printf("Input Image %d: %dx%d\n",imgnum,srcsize.x,srcsize.y);
    
    PTools::Transform transf;
    SrcPanoImage timg = src;
    //transf.createTransform(timg, dest);
    transf.createInvTransform(timg, dest);
    
    
    double x,y;
    double xd,yd;
        
    vigra::Point2D pt;
    vigra::Rect2D box;

    for(y=0.0;y<srcsize.y;y+=1.0)
    {
        for(x=0.0;x<srcsize.x;x+=1.0)
        {
            transf.transformImgCoord(xd,yd,x,y);
            pt.x=(int)floor(xd);
            pt.y=(int)floor(yd);
            box|=pt;
        }
    }
    printf("box  %d %d - %d %d\n",box.left(),box.top(),box.right(),box.bottom());
    return box;
}





//actual cropping code, shoehorn this in
#if 1

//replace simple macro with function
//#define imgPixel(img,i,j) (img[(j)*o_optimalSize.x+(i)])

//now you can do dynamic programming, look thinks up on fly
int CalculateOptimalROI::imgPixel(unsigned char *img,int i, int j)
{
    //if unknown (127) then do a look up
    if(img[(j)*o_optimalSize.x+(i)]==127)
    {
        //check that pixel at each place
        for(unsigned int c=0;c<totImages;c++)
        {
            double xd,yd;
            transfList[c].transformImgCoord(xd,yd,(double)i,(double)j);
            if(xd>=0 && yd>=0 && xd<imgSizeList[c].x && yd<imgSizeList[c].y)
            {
                //printf("%lf %lf - %d %d\n",xd,yd,(x-o_optimalROI.left()),(y-o_optimalROI.top()));
                //if found in a single image, short cut out
                img[j*o_optimalSize.x+i]=191;
                return img[j*o_optimalSize.x+i];
            }
        }
        
        //if made it through the for loop without a success, mark as bad
        img[j*o_optimalSize.x+i]=0;
        return img[j*o_optimalSize.x+i];
    }
    //else it is know to be bad (0) or good enough (191)
    else
    {
        return img[(j)*o_optimalSize.x+(i)];
    }
    
    
}


void CalculateOptimalROI::makecheck(int left,int top,int right,int bottom)
{
    if(left<max.left || top<max.top || right>max.right || bottom>max.bottom)
    {
        return;
    }
    
    
    if(left<right && top<bottom)
    {
        /*if(left>min.left*(2-BACKTRACKPERCENT) || right<min.right*BACKTRACKPERCENT || top>min.top*(2-BACKTRACKPERCENT) || bottom<min.bottom*BACKTRACKPERCENT)
        {
            return;
        }*/

        //big enough
#ifdef USEAREA
        if(maxvalue>0 && (right-left)*(bottom-top)<maxvalue*(1.0-BACKTRACKPERCENT))
#else
        if(maxvalue>0 && (right-left)+(bottom-top)<maxvalue*(1.0-BACKTRACKPERCENT))
#endif
        {
            return;
        }
        
        nonrec *tmp=head;
        //no delete test
        //nonrec *tmp=begin;
        //check if exists
        while(tmp!=NULL)
        {
            if(tmp->left==left && tmp->right==right && tmp->top==top && tmp->bottom==bottom)
            {
                //printf("found dupe\n");
                return;
            }
            tmp=tmp->next;
        }
        
        //make
        tmp=new nonrec;
        tmp->left=left;
        tmp->top=top;
        tmp->right=right;
        tmp->bottom=bottom;
        tmp->next=NULL;
        
        count++;
        tail->next=tmp;
        tail=tmp;
    }
    return;
}

//15 7 819 394
//31 17 672 370

void CalculateOptimalROI::nonreccheck(unsigned char *img,int left,int top,int right,int bottom,int acc,int dodouble)
{
    nonrec *tmp;
    tmp=new nonrec;
    tmp->left=left;
    tmp->top=top;
    tmp->right=right;
    tmp->bottom=bottom;
    tmp->next=NULL;
    
    begin=tmp;
    head=tmp;
    tail=tmp;
    count=0;
    total=0;
    
    count++;
    int ocount=0;
    
    while(count>0)
    {
        total++;
        if(total%1000==0)
        {
            printf("Total: %d Count: %d Diff: %d\n",total,count,count-ocount);
            ocount=count;
            fflush(stdout);
        }
        //printf("Try: %d %d - %d %d\n",left,right,top,bottom);
        count--;
        left=head->left;
        top=head->top;
        right=head->right;
        bottom=head->bottom;
        
        //do lasso, check if failed
        int i,j,flag;
        flag=0;
        for(i=left;i<right && flag==0;i++)
        {
            if(imgPixel(img,i,top)==0 || imgPixel(img,i,bottom-1)==0)
            {
                flag=1;
            }
        }
        
        for(j=top;j<bottom && flag==0;j++)
        {
            if(imgPixel(img,left,j)==0 || imgPixel(img,right-1,j)==0)
            {
                flag=1;
            }
        }

        //if failed, then recurse
        
        if(dodouble==1)
        {
            if(flag==1)
            {
                //45 degree angel
                //makecheck(left+acc,top+acc,right,bottom);
                //makecheck(left,top,right-acc,bottom-acc);
                
                //all directions (shrink only)
                makecheck(left,top+acc,right,bottom);
                makecheck(left,top,right,bottom-acc);
                makecheck(left+acc,top,right,bottom);
                makecheck(left,top,right-acc,bottom);
                
            }
            //it was good, stop recursing
            else
            {
                //printf("\nGood\n");
#ifdef USEAREA
                if(maxvalue<(right-left)*(bottom-top))
#else
                if(maxvalue<(right-left)+(bottom-top))
#endif
                {
#ifdef USEAREA
                    maxvalue=(right-left)*(bottom-top);
#else
                    maxvalue=(right-left)+(bottom-top);
#endif
                    best.right=right;
                    best.left=left;
                    best.top=top;
                    best.bottom=bottom;
                    printf("MaxValue: %d: %d %d - %d %d\n",maxvalue,left,right,top,bottom);
                    fflush(stdout);
                }
            }
        }
        else
        {
            if(flag==0)
            {
                makecheck(left-acc,top,right,bottom);
                makecheck(left,top,right+acc,bottom);
                makecheck(left,top-acc,right,bottom);
                makecheck(left,top,right,bottom+acc);
                
                //From above - testing
                //makecheck(left+acc,top,right,bottom);
                //makecheck(left,top,right-acc,bottom);
                //makecheck(left,top+acc,right,bottom);
                //makecheck(left,top,right,bottom-acc);
                
                
#ifdef USEAREA
                if(maxvalue<(right-left)*(bottom-top))
#else
                if(maxvalue<(right-left)+(bottom-top))
#endif
                {
#ifdef USEAREA
                    maxvalue=(right-left)*(bottom-top);
#else
                    maxvalue=(right-left)+(bottom-top);
#endif
                    best.right=right;
                    best.left=left;
                    best.top=top;
                    best.bottom=bottom;
                    printf("MaxValue: %d: %d %d - %d %d\n",maxvalue,left,right,top,bottom);
                    fflush(stdout);
                }
            }
        }
        
        tmp=head->next;
        if(tmp!=NULL)
        {
            //experiment with no deletion
            delete head;
            head=tmp;
        }
    }
    
    //no delete test 
    //head=begin;
    while(head!=NULL)
    {
        tmp=head->next;
        delete head;
        head=tmp;
    }
    
    if(maxvalue>0 && acc==1 && dodouble==0)
    {
        printf("Found Solution: %d %d %d %d\n",best.left,best.top,best.right,best.bottom);
        /*int i,j;
        for(i=left;i<right;i++)
        {
            imgPixel(img,i,best.top)=255;
            imgPixel(img,i,best.bottom-1)=255;
        }
        
        for(j=top;j<bottom;j++)
        {
            imgPixel(img,best.left,j)=255;
            imgPixel(img,best.right-1,j)=255;
        }*/
        //writeimage(inpano);
    }
}

int CalculateOptimalROI::autocrop(unsigned char *img)
{
    printf("Original Image: %dx%d\n",o_optimalSize.x,o_optimalSize.y);
    int i,j;
    int flag;
    
    maxvalue=0;
    count=0;
    total=0;
    begin=NULL;
    head=NULL;
    tail=NULL;

    //visited=new unsigned char[inpano->width*inpano->height*inpano->width*inpano->height];
    //memset(visited,0,inpano->width*inpano->height*inpano->width*inpano->height);
    //do a lasso
    /*min.left=o_optimalSize.x/2-1;
    min.right=o_optimalSize.x/2+1;
    min.top=o_optimalSize.y/2-1;
    min.bottom=o_optimalSize.y/2+1;*/

    //put backwards at the start
    min.right=0;
    min.bottom=0;
    min.left=o_optimalSize.x;
    min.top=o_optimalSize.y;
    
    
    max.left=0;
    max.top=0;
    max.right=o_optimalSize.x;
    max.bottom=o_optimalSize.y;
    //best=min;
    
    int startacc;
    for(startacc=1;startacc<o_optimalSize.x/2 && startacc<o_optimalSize.y/2;startacc*=2);
    startacc/=2;
    
    printf("Start Acc: %d\n",startacc);
    int acc=startacc;
    
//doesn't always work if image is sloped
//#define QUICKCEN
#ifdef QUICKCEN
    //assume center of image contains the image
    int centroidy=0;
    int sum=0;
    i=o_optimalSize.x/2;
    for(j=0;j<o_optimalSize.y;j++)
    {
        if(imgPixel(img,i,j)!=0)
        {
            sum++;
            centroidy+=j;
        }
    }
    //found pixels, calc centroid
    if(sum>0)
    {
        centroidy/=sum;
        maxvalue=4;
        min.top=centroidy-1;
        min.bottom=centroidy+1;
        printf("Found Easy Centroid Y: %d\n",centroidy);
        best=min;
    }
    //else do harder method
    else
    {
#endif
        //start smaller to get biggest initial position
        for(acc=(startacc>=MINACC)?startacc/MINACC:1;acc>=1;acc/=2)
        {
            nonreccheck(img,0,0,o_optimalSize.x,o_optimalSize.y,acc,1);
            if(maxvalue>0)
            {
                printf("Inner %d %d: %d %d - %d %d\n",acc,maxvalue,best.left,best.right,best.top,best.bottom);
                min=best;
                break;
            }
        }
#ifdef QUICKCEN
    }
#endif
    
    
    for(acc=startacc;acc>=1;acc/=2)
    {
        printf("Starting %d: %d %d - %d %d\n",acc,best.left,best.right,best.top,best.bottom);
        nonreccheck(img,best.left,best.top,best.right,best.bottom,acc,0);
        min=best;
    }

    //printf("Final Image: %dx%d\n",outpano->width,outpano->height);
    return 0;
}


#endif



} //namespace



