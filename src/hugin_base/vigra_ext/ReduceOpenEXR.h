// -*- c-basic-offset: 4 -*-
/** @file ReduceOpenEXR.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
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

#include <vigra/sized_int.hxx>
#include <vigra_ext/HDRUtils.h>
#include <vigra_ext/FileRAII.h>

#include <ImfRgbaFile.h>
#include <ImfArray.h>


// hack to read pgm header
inline bool readPGMHeader(FILE * file, int & w, int &h, int & maxval)
{
    char line[257];
    fgets(line, 256, file);
    if (strncmp(line,"P5", 2)) {
        printf("pgm read: not a pgm file\n");
        return false;
    }

    fgets(line, 256, file);
    while (line[0] == '#')
    fgets(line, 256, file);

    sscanf(line,"%d %d", &w, &h);
    fgets(line, 256, file);
    sscanf(line, "%d", &maxval);
    return true;
}

template<class Functor>
void reduceFilesToHDR(std::vector<std::string> input, std::string output,
                      bool onlyCompleteOverlap, Functor & reduce)
{
    typedef vigra::RGBValue<float> PixelType;
    typedef std::shared_ptr<Imf::RgbaInputFile> InFilePtr;
    typedef std::shared_ptr<vigra_ext::FileRAII> AutoFilePtr;

    // open all input files.
    std::vector<AutoFilePtr> inputGrayFiles;
    std::vector<InFilePtr> inputFiles;
    std::vector<vigra::Rect2D> inputROIs;
    vigra::Rect2D outputROI;
    vigra::Rect2D outputSize;
    for (unsigned i=0; i < input.size(); i++) {
        std::string grayFile = hugin_utils::stripExtension(input[i]) + "_gray.pgm";
        InFilePtr in(new Imf::RgbaInputFile(input[i].c_str()));
        inputFiles.push_back(in);
        Imath::Box2i dw = in->dataWindow();
        vigra::Rect2D roi(dw.min.x, dw.min.y, dw.max.x+1, dw.max.y+1);
        DEBUG_DEBUG("image " << i << "ROI: " << roi);
        dw = in->displayWindow();
        vigra::Rect2D imgSize(dw.min.x, dw.min.y, dw.max.x+1, dw.max.y+1);

        AutoFilePtr inGray(new vigra_ext::FileRAII(grayFile.c_str(), "rb"));
        int w, h, maxval;
        readPGMHeader(inGray->get(), w, h, maxval);
        vigra_precondition(w == roi.width() && h == roi.height(), ".exr and _gray.pgm images not of the same size");
        inputGrayFiles.push_back(inGray);
        if (i==0) {
            outputROI = roi;
            outputSize = imgSize;
        } else {
            outputROI |= roi;
            outputSize |= imgSize;
        }
        inputROIs.push_back(roi);
    }
    DEBUG_DEBUG("output display: " << outputSize);
    DEBUG_DEBUG("output data (ROI): " << outputROI);

    // create output file
    Imath::Box2i displayWindow (Imath::V2i (outputSize.left(), outputSize.top()),
                                Imath::V2i (outputSize.right() - 1, outputSize.bottom() - 1));
    Imath::Box2i dataWindow (Imath::V2i (outputROI.left(), outputROI.top()),
                             Imath::V2i (outputROI.right() - 1, outputROI.bottom() - 1));
    Imf::RgbaOutputFile outputFile (output.c_str(), displayWindow, dataWindow, Imf::WRITE_RGBA);

    int roiWidth = outputROI.right() - outputROI.left();
    // process some 64k of scanlines at a time.
    // ass
    int nScanlines = 64*1024 /2/4/input.size()/roiWidth;
    if (nScanlines < 10) nScanlines = 10;
    DEBUG_DEBUG("processing " << nScanlines << " scanlines in one go");

    typedef std::shared_ptr<vigra::ArrayVector<vigra::UInt8> > Array8Ptr;
    typedef std::shared_ptr<Imf::Array2D<Imf::Rgba> > ArrayPtr;
    std::vector<ArrayPtr> inputArrays;
    std::vector<Array8Ptr> inputGrayArrays;
    std::vector<Imf::Rgba *> inputPtr(input.size());
    std::vector<vigra::UInt8 *> inputGrayPtr(input.size());
    // create frame buffers for the input files
    for (unsigned i=0; i < input.size(); i++) {
        ArrayPtr p(new Imf::Array2D<Imf::Rgba>);
        p->resizeErase(nScanlines, roiWidth);
        inputArrays.push_back(p);
        Array8Ptr pg(new vigra::ArrayVector<vigra::UInt8>(nScanlines*roiWidth, vigra::UInt8(0)));
        inputGrayArrays.push_back(pg);
    }
    // create output framebuffer
    Imf::Array2D<Imf::Rgba> outputArray(nScanlines, roiWidth);

    // main processing loop
    int y = outputROI.top();
    while (y < outputROI.bottom())
    {
        for (unsigned j=0; j < input.size(); j++) {
            Imf::Rgba * pixels = &(*inputArrays[j])[0][0];
            // shift to our buffer origin and apply shift required by readPixels()
            pixels = pixels - outputROI.left() - y * roiWidth;
            inputFiles[j]->setFrameBuffer( pixels, 1, roiWidth);
            // TODO: restrict reading to actual ROI of input image.
            int ys = std::max(y, inputROIs[j].top());
            int ye = std::min(y + nScanlines-1, inputROIs[j].bottom()-1);
            // read if inside roi
            if (ys <=ye)
                inputFiles[j]->readPixels (ys, ye);
            inputPtr[j] = &(*inputArrays[j])[0][0];

            // read data from raw gray level input
            for(int k=0; k < nScanlines; k++) {
                if (k+y >= inputROIs[j].top() && k+y < inputROIs[j].bottom()) {
                    // read scanline from raw image
                    vigra::UInt8 * grayp = inputGrayArrays[j]->data() +
                                             (inputROIs[j].left()-outputROI.left()) + k*roiWidth;
                    int nElem = inputROIs[j].width();
                    size_t n = fread(grayp, 1, nElem, inputGrayFiles[j]->get());
                    assert (n == (size_t)nElem);
                }
            }
            inputGrayPtr[j] = inputGrayArrays[j]->data();
        }
        // reduce content
        Imf::Rgba * outputPtr = &outputArray[0][0];
        Imf::Rgba * outputPtrEnd = outputPtr + nScanlines*roiWidth;
        for (; outputPtr != outputPtrEnd; ++outputPtr)
        {
            reduce.reset();
            bool valid = false;
            bool complete = true;
            for (unsigned int j=0; j< input.size(); j++) {
                Imf::Rgba p = *inputPtr[j];
                bool isValid = p.a > 0;
                valid |= isValid;
                complete &= isValid;
                if (isValid) {
                    reduce(PixelType(p.r, p.g, p.b), *inputGrayPtr[j]);
                }
                ++inputPtr[j];
                ++inputGrayPtr[j];
            }
            // need to properly set the alpha...
            PixelType val = reduce();
            outputPtr->r = val.red();
            outputPtr->g = val.green();
            outputPtr->b = val.blue();
            if (onlyCompleteOverlap) {
                outputPtr->a = complete ? 1 : 0;
            } else {
                outputPtr->a = valid ? 1 : 0;
            }
        }
        // save pixels.
        Imf::Rgba * pixels = &outputArray[0][0];
        pixels = pixels - outputROI.left() - y * roiWidth;

        outputFile.setFrameBuffer (pixels,
                                   1, roiWidth);
        int wh = std::min(outputROI.bottom()-y, nScanlines);
        outputFile.writePixels( wh );
        y += nScanlines;
    }
}

#if 0
        // read in pixels
        for (int j=0; j < input.size(); j++) {
            int yend = y + nScanlines;
            // check if there is something to read from this image
            if (heightLeft[j] > 0) {
                // calculate y-offset in terms of data window of image
                int y_rel_start = inputROIs[j].top() - y;
                if (y_rel_start > 0 && y_rel_start < nScanlines) {
                    // we have something to read, setup correct framebuffer address
                    //
                    inputFiles[j].setFrameBuffer( &(*(inputArrays[j]))[0][0] - outputROI().left()
                                                     - outputROI().top() * roiWidth,
                                                1, roiWidth);
                      
                    int y_read_start = y
                    int y_read_end = std::min(inputRoi
                    
            if (!( inputROIs[j].bottom() <= y || inputROIs[j].top() >= yend )) {
                // inside the ROI. Calculate the number of rows inside the buffer
                
                // calculate position in framebuffer.
                if (y >= inputROIs[j].top()
                int ystart = 
        }
#endif

