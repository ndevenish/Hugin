// -*- c-basic-offset: 4 -*-

/** @file verdandi.cpp
*
*  @brief program to stitch images using the watershed algorithm
*
*  @author T. Modes
*
*/

/*  This program is free software; you can redistribute it and/or
*  modify it under the terms of the GNU General Public
*  License as published by the Free Software Foundation; either
*  version 2 of the License, or (at your option) any later version.
*
*  This software is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  General Public License for more details.
*
*  You should have received a copy of the GNU General Public
*  License along with this software; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/

#include <stdio.h>
#include <iostream>
#include <getopt.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <vigra_ext/impexalpha.hxx>
#include <vigra_ext/StitchWatershed.h>
#include <vigra_ext/utils.h>
#include <hugin_utils/utils.h>
#include <hugin_utils/stl_utils.h>

template <class ImageType, class MaskType>
void LoadAndMergeImages(std::vector<vigra::ImageImportInfo> imageInfos, const std::string& filename, const std::string& compression, const bool wrap)
{
    if (imageInfos.empty())
    {
        return;
    };
    vigra::Size2D imageSize(imageInfos[0].getCanvasSize());
    if (imageSize.area() == 0)
    {
        // not all images contains the canvas size/full image size
        // in this case take also the position into account to get full image size
        imageSize = vigra::Size2D(imageInfos[0].width() + imageInfos[0].getPosition().x,
            imageInfos[0].height() + imageInfos[0].getPosition().y);
    };
    ImageType image(imageSize);
    MaskType mask(imageSize);
    vigra::importImageAlpha(imageInfos[0],
        std::pair<typename ImageType::Iterator, typename ImageType::Accessor>(image.upperLeft() + imageInfos[0].getPosition(), image.accessor()),
        std::pair<typename MaskType::Iterator, typename MaskType::Accessor>(mask.upperLeft() + imageInfos[0].getPosition(), mask.accessor()));
    std::cout << "Loaded " << imageInfos[0].getFileName() << std::endl;
    vigra::Rect2D roi(vigra::Point2D(imageInfos[0].getPosition()), imageInfos[0].size());

    for (size_t i = 1; i < imageInfos.size(); ++i)
    {
        ImageType image2(imageInfos[i].size());
        MaskType mask2(image2.size());
        vigra::importImageAlpha(imageInfos[i], vigra::destImage(image2), vigra::destImage(mask2));
        std::cout << "Loaded " << imageInfos[i].getFileName() << std::endl;
        roi |= vigra::Rect2D(vigra::Point2D(imageInfos[i].getPosition()), imageInfos[i].size());

        vigra_ext::MergeImages(image, mask, image2, mask2, imageInfos[i].getPosition(), wrap);
    };
    // save output
    {
        vigra::ImageExportInfo exportImageInfo(filename.c_str());
        exportImageInfo.setXResolution(imageInfos[0].getXResolution());
        exportImageInfo.setYResolution(imageInfos[0].getYResolution());
        exportImageInfo.setPosition(roi.upperLeft());
        exportImageInfo.setCanvasSize(mask.size());
        exportImageInfo.setPixelType(imageInfos[0].getPixelType());
        exportImageInfo.setICCProfile(imageInfos[0].getICCProfile());
        if (!compression.empty())
        {
            exportImageInfo.setCompression(compression.c_str());
        };
        try
        {
            vigra::exportImageAlpha(vigra::srcImageRange(image, roi), vigra::srcImage(mask, roi.upperLeft()), exportImageInfo);
        }
        catch (...)
        {
            std::cerr << "Warning: Output file format does not support alpha channel. Fall back to export image without alpha channel" << std::endl;
            vigra::exportImage(vigra::srcImageRange(image, roi), exportImageInfo);
        };
    };
};

static void usage(const char* name)
{
    std::cout << name << ": stitch images using watershed algorithm" << std::endl
        << name << " version " << hugin_utils::GetHuginVersion() << std::endl
        << std::endl
        << "Usage:  " << name << " [options] images" << std::endl
        << std::endl
        << "     --output=FILE       Set the filename for the output file." << std::endl
        << "     --compression=value Compression of the output files" << std::endl
        << "                            For jpeg output: 0-100" << std::endl
        << "                            For tiff output: PACKBITS, DEFLATE, LZW" << std::endl
        << "     -w, --wrap          Wraparound 360 deg border." << std::endl
        << "     -h, --help          Shows this help" << std::endl
        << std::endl;
};

template<class ImageType>
bool ResaveImage(const vigra::ImageImportInfo& importInfo, const vigra::ImageExportInfo& exportInfo)
{
    if (importInfo.numExtraBands() == 0)
    {
        ImageType image(importInfo.size());
        vigra::importImage(importInfo, vigra::destImage(image));
        vigra::exportImage(vigra::srcImageRange(image), exportInfo);
        return true;
    }
    if (importInfo.numExtraBands() == 1)
    {
        ImageType image(importInfo.size());
        vigra::BImage mask(importInfo.size());
        vigra::importImageAlpha(importInfo, vigra::destImage(image), vigra::destImage(mask));
        try
        {
            vigra::exportImageAlpha(vigra::srcImageRange(image), vigra::srcImage(mask), exportInfo);
        }
        catch (...)
        {
            std::cerr << "Warning: Output file format does not support alpha channel. Fall back to export image without alpha channel" << std::endl;
            vigra::exportImage(vigra::srcImageRange(image), exportInfo);
        };
        return true;
    };
    std::cerr << "ERROR: Images with several alpha channels are not supported." << std::endl;
    return false;
};

int main(int argc, char* argv[])
{
    // parse arguments
    const char* optstring = "o:hw";

    enum
    {
        OPT_COMPRESSION = 1000,
    };
    static struct option longOptions[] =
    {
        { "output", required_argument, NULL, 'o' },
        { "compression", required_argument, NULL, OPT_COMPRESSION},
        { "wrap", no_argument, NULL, 'w' },
        { "help", no_argument, NULL, 'h' },
        0
    };

    int c;
    int optionIndex = 0;
    std::string output;
    std::string compression;
    bool wraparound = false;
    while ((c = getopt_long(argc, argv, optstring, longOptions, &optionIndex)) != -1)
    {
        switch (c)
        {
        case 'o':
            output = optarg;
            break;
        case 'h':
            usage(hugin_utils::stripPath(argv[0]).c_str());
            return 0;
            break;
        case OPT_COMPRESSION:
            compression = hugin_utils::toupper(optarg);
            break;
        case 'w':
            wraparound = true;
            break;
        case ':':
            std::cerr << "Option " << longOptions[optionIndex].name << " requires a parameter." << std::endl;
            return 1;
            break;
        case '?':
            break;
        default:
            abort();
        }
    };

    unsigned nFiles = argc - optind;
    if (nFiles < 1)
    {
        std::cerr << std::endl << "Error: at least one image need to be specified" << std::endl << std::endl;
        return 1;
    }

    // extract file names
    std::vector<std::string> files;
    for (size_t i = 0; i < nFiles; i++)
    {
        std::string currentFile(argv[optind + i]);
        // check file existence
        if (hugin_utils::FileExists(currentFile))
        {
            files.push_back(currentFile);
        };
    }

    if (files.empty())
    {
        std::cerr << "Error: " << hugin_utils::stripPath(argv[0]) << " needs at least one image." << std::endl;
        return 1;
    };

    if (output.empty())
    {
        output = "final.tif";
    };
    if (files.size() == 1)
    {
        //special case, only one image given
        vigra::ImageImportInfo imageInfo(files[0].c_str());
        vigra::ImageExportInfo exportInfo(output.c_str());
        if (!compression.empty())
        {
            exportInfo.setCompression(compression.c_str());
        };
        exportInfo.setPixelType(imageInfo.getPixelType());
        exportInfo.setXResolution(imageInfo.getXResolution());
        exportInfo.setYResolution(imageInfo.getYResolution());
        exportInfo.setCanvasSize(imageInfo.getCanvasSize());
        exportInfo.setPosition(imageInfo.getPosition());
        exportInfo.setICCProfile(imageInfo.getICCProfile());
        const std::string pixeltype = imageInfo.getPixelType();
        if (imageInfo.isColor())
        {
            if (pixeltype == "UINT8")
            {
                if (!ResaveImage<vigra::BRGBImage>(imageInfo, exportInfo))
                {
                    return 1;
                };
            }
            else if (pixeltype == "INT16")
            {
                if (!ResaveImage<vigra::Int16RGBImage>(imageInfo, exportInfo))
                {
                    return 1;
                };
            }
            else if (pixeltype == "UINT16")
            {
                if (!ResaveImage<vigra::UInt16RGBImage>(imageInfo, exportInfo))
                {
                    return 1;
                };
            }
            else if (pixeltype == "INT32")
            {
                if (!ResaveImage<vigra::Int32RGBImage>(imageInfo, exportInfo))
                {
                    return 1;
                };
            }
            else if (pixeltype == "UINT32")
            {
                if (!ResaveImage<vigra::UInt32RGBImage>(imageInfo, exportInfo))
                {
                    return 1;
                };
            }
            else if (pixeltype == "FLOAT")
            {
                if (!ResaveImage<vigra::FRGBImage>(imageInfo, exportInfo))
                {
                    return 1;
                };
            }
            else
            {
                std::cerr << " ERROR: unsupported pixel type: " << pixeltype << std::endl;
                return 1;
            };
        }
        else
        {
            //grayscale images
            if (pixeltype == "UINT8")
            {
                if (!ResaveImage<vigra::BImage>(imageInfo, exportInfo))
                {
                    return 1;
                };
            }
            else if (pixeltype == "INT16")
            {
                if (!ResaveImage<vigra::Int16Image>(imageInfo, exportInfo))
                {
                    return 1;
                };
            }
            else if (pixeltype == "UINT16")
            {
                if (!ResaveImage<vigra::UInt16Image>(imageInfo, exportInfo))
                {
                    return 1;
                };
            }
            else if (pixeltype == "INT32")
            {
                if (!ResaveImage<vigra::Int32Image>(imageInfo, exportInfo))
                {
                    return 1;
                };
            }
            else if (pixeltype == "UINT32")
            {
                if (!ResaveImage<vigra::UInt32Image>(imageInfo, exportInfo))
                {
                    return 1;
                };
            }
            else if (pixeltype == "FLOAT")
            {
                if (!ResaveImage<vigra::FImage>(imageInfo, exportInfo))
                {
                    return 1;
                };
            }
            else
            {
                std::cerr << " ERROR: unsupported pixel type: " << pixeltype << std::endl;
                return 1;
            };
        };
    }
    else
    {
        std::vector<vigra::ImageImportInfo> imageInfos;
        for (size_t i = 0; i < files.size(); ++i)
        {
            vigra::ImageImportInfo imageInfo(files[i].c_str());
            imageInfos.push_back(imageInfo);
        };
        const std::string pixeltype(imageInfos[0].getPixelType());
        if (imageInfos[0].numExtraBands() != 1)
        {
            std::cerr << "ERROR: Image does not contain alpha channel." << std::endl;
            return 1;
        }
        //check, that image information matches
        for (size_t i = 1; i < files.size(); ++i)
        {
            if (imageInfos[0].isColor() != imageInfos[i].isColor())
            {
                std::cerr << "ERROR: You can't merge color and grayscale images." << std::endl;
                return 1;
            };
            if (imageInfos[0].numBands() != imageInfos[i].numBands())
            {
                std::cerr << "ERROR: You can't merge image with different number of channels." << std::endl
                    << "       Image \"" << imageInfos[0].getFileName() << "\" has " << imageInfos[0].numBands() << " channels," << std::endl
                    << "       but image \"" << imageInfos[i].getFileName() << "\" has " << imageInfos[i].numBands() << " channels." << std::endl;
                return 1;
            };
            if (strcmp(pixeltype.c_str(), imageInfos[i].getPixelType()) != 0)
            {
                std::cerr << "ERROR: You can't merge images with different pixel types." << std::endl
                    << "       Image \"" << imageInfos[0].getFileName() << "\" has pixel type " << imageInfos[0].getPixelType() << "," << std::endl
                    << "       but image \"" << imageInfos[i].getFileName() << "\" has pixel type " << imageInfos[i].getPixelType() << "." << std::endl;
                return 1;
            };
            /*if (GetCanvasSize(imageInfos[0]) != GetCanvasSize(imageInfos[i]))
            {
                std::cerr << "ERROR: Dimension of images does not match." << std::endl
                    << "       Image \"" << imageInfos[0].getFileName() << "\" has dimension " << GetCanvasSize(imageInfos[0]) << "," << std::endl
                    << "       but image \"" << imageInfos[i].getFileName() << "\" has dimension " << GetCanvasSize(imageInfos[i]) << "." << std::endl;
                return 1;
            };*/
        };

        if (imageInfos[0].isColor())
        {
            if (pixeltype == "UINT8")
            {
                LoadAndMergeImages<vigra::BRGBImage, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "INT16")
            {
                LoadAndMergeImages<vigra::Int16RGBImage, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "UINT16")
            {
                LoadAndMergeImages<vigra::UInt16RGBImage, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "INT32")
            {
                LoadAndMergeImages<vigra::Int32RGBImage, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "UINT32")
            {
                LoadAndMergeImages<vigra::UInt32RGBImage, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "FLOAT")
            {
                LoadAndMergeImages<vigra::FRGBImage, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else
            {
                std::cerr << " ERROR: unsupported pixel type: " << pixeltype << std::endl;
            };
        }
        else
        {
            //grayscale images
            if (pixeltype == "UINT8")
            {
                LoadAndMergeImages<vigra::BImage, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "INT16")
            {
                LoadAndMergeImages<vigra::Int16Image, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "UINT16")
            {
                LoadAndMergeImages<vigra::UInt16Image, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "INT32")
            {
                LoadAndMergeImages<vigra::Int32Image, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "UINT32")
            {
                LoadAndMergeImages<vigra::UInt32Image, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "FLOAT")
            {
                LoadAndMergeImages<vigra::FImage, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else
            {
                std::cerr << " ERROR: unsupported pixel type: " << pixeltype << std::endl;
            };
        };
    };

    std::cout << "Written result to " << output << std::endl;
    return 0;
}
