// -*- c-basic-offset: 4 -*-
/** @file CalculateOptimalROI.cpp
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "CalculateOptimalROI.h"
#include <algorithm>

namespace HuginBase {

///
bool CalculateOptimalROI::calcOptimalROI(PanoramaData& panorama)
{
    activeImages=panorama.getActiveImages();
    if (activeImages.empty())
    {
        return false;
    };

    PanoramaOptions opt = panorama.getOptions();
    o_optimalSize=opt.getSize();
    if (o_optimalSize.x == 0 || o_optimalSize.y == 0)
    {
        return false;
    };
    m_bestRect = vigra::Rect2D();
    try
    {
        testedPixels.resize(o_optimalSize.x*o_optimalSize.y,false);
        pixels.resize(o_optimalSize.x*o_optimalSize.y,false);
    }
    catch(std::bad_alloc&)
    {
        //could not allocate enough memory
        return false;
    };

    for (UIntSet::const_iterator it=activeImages.begin(); it!=activeImages.end(); ++it)
    {
        const SrcPanoImage &img=panorama.getImage(*it);
        PTools::Transform *transf=new PTools::Transform();
        transf->createTransform(img,opt);
        transfMap.insert(std::pair<unsigned int,PTools::Transform*>(*it,transf));
    }
    
    if (!getProgressDisplay()->updateDisplay("Calculate the cropping region"))
    {
        CleanUp();
        return false;
    };
    if (!autocrop())
    {
        CleanUp();
        return false;
    };
    
    //clean up on demand
    CleanUp();
    return true;
};

void CalculateOptimalROI::CleanUp()
{
    for (std::map<unsigned int, PTools::Transform*>::iterator it = transfMap.begin(); it != transfMap.end(); ++it)
    {
        delete (*it).second;
    };
};

//now you can do dynamic programming, look thinks up on fly
bool CalculateOptimalROI::stackPixel(int i, int j, UIntSet &stack)
{
    bool inside = intersection; // start with true for intersection mode and with false for union mode
    //check that pixel at each place
    for(UIntSet::const_iterator it=stack.begin();it!=stack.end();++it)
    {
        double xd,yd;
        if(transfMap[*it]->transformImgCoord(xd,yd,(double)i,(double)j))
        {
            if(o_panorama.getImage(*it).isInside(vigra::Point2D(xd,yd)))
            {
                if (!intersection) {
                    //if found in a single image, short cut out
                    inside=true;
                    break;
                }
            }
            else {
                if (intersection) {
                    //outside of at least one image - return false
                    inside=false;
                    break;
                }
            }
        }
    }

    return inside;
}

bool CalculateOptimalROI::imgPixel(int i, int j)
{
    if(!testedPixels[j*o_optimalSize.x+i])
    {
        bool inside;
        
        if (stacks.empty())
        {
            // no stacks - test all images on union or intersection
            inside = stackPixel(i, j, activeImages);
        }
        else
        {
            inside = false;
            // pixel must be inside of at least one stack
            for (unsigned s=0; s < stacks.size(); s++)
            {
                // images in each stack are tested on intersection
                if (stackPixel(i, j, stacks[s]))
                {
                    inside = true;
                    break;
                }
            }
        }

        testedPixels[j*o_optimalSize.x + i] = true;
        pixels[j*o_optimalSize.x + i] = inside;
        
        return inside;
    }
    //else it is know if this pixel is covered by at least one image
    else
    {
        return pixels[j*o_optimalSize.x+i];
    }
}

/** add new rect to list of rects to be check, do some checks before */
void CalculateOptimalROI::AddCheckingRects(std::list<vigra::Rect2D>& testingRects, const vigra::Rect2D& rect, const long maxvalue)
{
    if(rect.left()<0 || rect.top()<0 || rect.right()>o_optimalSize.x || rect.bottom()>o_optimalSize.y)
    {
        return;
    }
    
    if (rect.left() < rect.right() && rect.top() < rect.bottom())
    {
        //not big enough
        if(maxvalue>0 && rect.area()<maxvalue)
        {
            return;
        }
        // check if rect is already in list
        std::list<vigra::Rect2D>::iterator it=std::find(testingRects.begin(), testingRects.end(), rect);
        if (it == testingRects.end())
        {
            testingRects.push_back(rect);
        };
    }
}

/** check if given rect covers the whole pano */
bool CalculateOptimalROI::CheckRectCoversPano(const vigra::Rect2D& rect)
{
    for (int i = rect.left(); i<rect.right(); i++)
    {
        if (imgPixel(i, rect.top()) == 0 || imgPixel(i, rect.bottom() - 1) == 0)
        {
            return false;
        }
    }

    for (int j = rect.top(); j<rect.bottom(); j++)
    {
        if (imgPixel(rect.left(), j) == 0 || imgPixel(rect.right() - 1, j) == 0)
        {
            return false;
        }
    }
    return true;
};

vigra::Rect2D ModifyRect(const vigra::Rect2D& rect, long deltaLeft, long deltaTop, long deltaRight, long deltaBottom)
{
    vigra::Rect2D newRect(rect);
    newRect.moveBy(deltaLeft, deltaTop);
    newRect.addSize(vigra::Size2D(deltaRight - deltaLeft, deltaBottom - deltaTop));
    return newRect;
};

void CalculateOptimalROI::nonreccheck(const vigra::Rect2D& rect, int acc, int searchStrategy, long& maxvalue)
{
    std::list<vigra::Rect2D> testRects;
    testRects.push_back(rect);
    
    while(!testRects.empty())
    {
        vigra::Rect2D testingRect = *testRects.begin();
        const bool rectCovers = CheckRectCoversPano(testingRect);
        switch(searchStrategy)
        {
            case 1:
                if(!rectCovers)
                {
                    //all directions (shrink only)
                    AddCheckingRects(testRects, ModifyRect(testingRect,   0, acc,    0,    0), maxvalue);
                    AddCheckingRects(testRects, ModifyRect(testingRect,   0,   0,    0, -acc), maxvalue);
                    AddCheckingRects(testRects, ModifyRect(testingRect, acc,   0,    0,    0), maxvalue);
                    AddCheckingRects(testRects, ModifyRect(testingRect,   0,   0, -acc,    0), maxvalue);
                }
                //it was good, stop recursing
                else
                {
                    if(maxvalue<testingRect.area())
                    {
                        maxvalue=testingRect.area();
                        m_bestRect = testingRect;
                    }
                }
                break;
            case 2:
                if(!rectCovers)
                {
                    //all directions (shrink only)
                    AddCheckingRects(testRects, ModifyRect(testingRect, acc >> 1, 0, -(acc >> 1), 0), maxvalue);
                    AddCheckingRects(testRects, ModifyRect(testingRect, 0, acc >> 1, 0, -(acc >> 1)), maxvalue);
                }
                //it was good, stop recursing
                else
                {
                    if(maxvalue<testingRect.area())
                    {
                        maxvalue = testingRect.area();
                        m_bestRect = testingRect;
                    }
                }
                break;
            case 0:
            default:
                if(rectCovers)
                {
                    //check growth in all 4 directions
                    AddCheckingRects(testRects, ModifyRect(testingRect, -acc, 0, 0, 0), maxvalue);
                    AddCheckingRects(testRects, ModifyRect(testingRect, 0, 0, acc, 0), maxvalue);
                    AddCheckingRects(testRects, ModifyRect(testingRect, 0, -acc, 0, 0), maxvalue);
                    AddCheckingRects(testRects, ModifyRect(testingRect, 0, 0, 0, acc), maxvalue);
                    //check if shrinking in one direction will allow more growth in other direction
                    AddCheckingRects(testRects, ModifyRect(testingRect, -2*acc, acc, 0, 0), maxvalue);
                    AddCheckingRects(testRects, ModifyRect(testingRect, -2*acc, 0, 0, -acc), maxvalue);
                    AddCheckingRects(testRects, ModifyRect(testingRect, 0, acc, 2*acc, 0), maxvalue);
                    AddCheckingRects(testRects, ModifyRect(testingRect, 0, 0, 2*acc, -acc), maxvalue);
                    AddCheckingRects(testRects, ModifyRect(testingRect, acc, -2 * acc, 0, 0), maxvalue);
                    AddCheckingRects(testRects, ModifyRect(testingRect, 0, -2*acc, -acc, 0), maxvalue);
                    AddCheckingRects(testRects, ModifyRect(testingRect, acc, 0, 0, 2*acc), maxvalue);
                    AddCheckingRects(testRects, ModifyRect(testingRect, 0, 0, -acc, 2*acc), maxvalue);
                    AddCheckingRects(testRects, ModifyRect(testingRect, -acc, acc, acc, 0), maxvalue);
                    AddCheckingRects(testRects, ModifyRect(testingRect, -acc, 0, acc, -acc), maxvalue);
                    AddCheckingRects(testRects, ModifyRect(testingRect, acc, -acc, 0, acc), maxvalue);
                    AddCheckingRects(testRects, ModifyRect(testingRect, 0, -acc, -acc, acc), maxvalue);
                    if(maxvalue<testingRect.area())
                    {
                        maxvalue = testingRect.area();
                        m_bestRect = testingRect;
                    }
                }
        };
        testRects.pop_front();
    }
}

bool CalculateOptimalROI::autocrop()
{
    long maxvalue=0;

    //put backwards at the start
    int startacc = pow(2.0, std::min((int)log2(o_optimalSize.x / 2 - 1), (int)log2(o_optimalSize.y / 2 - 1)) - 1);
    if (startacc < 1)
    {
        startacc = 1;
    };

    //start smaller to get biggest initial position
    if(startacc>64)
    {
        //we start searching with a symmetric decrease
        for(int acc=startacc;acc>=64;acc/=2)
        {
            nonreccheck(vigra::Rect2D(vigra::Point2D(), o_optimalSize), acc, 2, maxvalue);
            if (!getProgressDisplay()->updateDisplayValue())
            {
                return false;
            };
            if(maxvalue>0)
            {
                printf("Inner %d %ld: %d %d - %d %d\n", acc, maxvalue, m_bestRect.left(), m_bestRect.right(), m_bestRect.top(), m_bestRect.bottom());
                break;
            }
        }
    };

    if(maxvalue==0)
    {
        // if the rough symmetric search failed we are using also an asymmetric strategy
        for(int acc=startacc;acc>=1;acc/=2)
        {
            nonreccheck(vigra::Rect2D(vigra::Point2D(), o_optimalSize), acc, 1, maxvalue);
            if (!getProgressDisplay()->updateDisplayValue())
            {
                return false;
            };
            if (maxvalue>0)
            {
                printf("Inner %d %ld: %d %d - %d %d\n", acc, maxvalue, m_bestRect.left(), m_bestRect.right(), m_bestRect.top(), m_bestRect.bottom());
                break;
            }
        }
    };
    
    for(int acc=startacc;acc>=1;acc/=2)
    {
        printf("Starting %d: %d %d - %d %d\n", acc, m_bestRect.left(), m_bestRect.right(), m_bestRect.top(), m_bestRect.bottom());
        nonreccheck(m_bestRect, acc, 0, maxvalue);
        if (!getProgressDisplay()->updateDisplayValue())
        {
            return false;
        };
    }

    return true;
}

void CalculateOptimalROI::setStacks(std::vector<UIntSet> hdr_stacks)
{
    stacks=hdr_stacks;
    intersection=true;
};

} //namespace
