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

/** save image, when possible with alpha channel, take care of formats which does not support alpha channels */
template <class ImageType, class MaskType>
bool SaveImage(ImageType& image, MaskType& mask, vigra::ImageExportInfo& exportImageInfo, std::string filetype, std::string pixelType, const vigra::Rect2D& roi, const int inputNumberBands)
{
    exportImageInfo.setPixelType(pixelType.c_str());
    if (vigra::isBandNumberSupported(filetype, inputNumberBands))
    {
        vigra::exportImageAlpha(vigra::srcImageRange(image, roi), vigra::srcImage(mask, roi.upperLeft()), exportImageInfo);
        return true;
    }
    else
    {
        if (vigra::isBandNumberSupported(filetype, inputNumberBands - 1))
        {
            std::cout << "Warning: Filetype " << filetype << " does not support alpha channels." << std::endl
                << "Saving image without alpha channel." << std::endl;
            vigra::exportImage(vigra::srcImageRange(image, roi), exportImageInfo);
            return true;
        }
        else
        {
            std::cerr << "Error: Output filetype " << filetype << " does not support " << inputNumberBands << " channels." << std::endl
                << "Can't save image." << std::endl;
        };
    };
    return false;
};

/** save final image, take care of some supported pixel types and convert when necessary to smaller pixel type */
template <class ImageType, class MaskType>
bool SaveFinalImage(ImageType& image, MaskType& mask, const vigra::ImageImportInfo& input, vigra::ImageExportInfo& output, const vigra::Rect2D& roi)
{
    VIGRA_UNIQUE_PTR<vigra::Encoder> encoder(vigra::encoder(output));
    if (vigra::isPixelTypeSupported(encoder->getFileType(), input.getPixelType()))
    {
        return SaveImage(image, mask, output, encoder->getFileType(), input.getPixelType(), roi, input.numBands());
    }
    else
    {
        if (vigra::isPixelTypeSupported(encoder->getFileType(), "UINT16"))
        {
            // transform to UINT16
            vigra::omp::transformImage(vigra::srcImageRange(image), vigra::destImage(image),
                vigra::linearIntensityTransform<typename ImageType::PixelType>(65535.0 / vigra::NumericTraits<typename vigra::NumericTraits<typename ImageType::PixelType>::ValueType>::max()));
            return SaveImage(image, mask, output, encoder->getFileType(), "UINT16", roi, input.numBands());
        }
        else
        {
            if (vigra::isPixelTypeSupported(encoder->getFileType(), "UINT8"))
            {
                // transform to UINT8
                vigra_ext::ConvertTo8Bit(image);
                return SaveImage(image, mask, output, encoder->getFileType(), "UINT8", roi, input.numBands());
            }
            else
            {
                std::cerr << "ERROR: Output file type " << encoder->getFileType() << " does not support" << std::endl
                    << "requested pixeltype " << input.getPixelType() << "." << std::endl
                    << "Save output in other file format." << std::endl;
            };
        };
    };
    return false;
};

/** set compression for jpeg or tiff */
void SetCompression(vigra::ImageExportInfo& output, const std::string& compression)
{
    const std::string ext(hugin_utils::toupper(hugin_utils::getExtension(output.getFileName())));
    if (!compression.empty())
    {
        if (ext == "JPEG" || ext == "JPG")
        {
            output.setCompression(std::string("JPEG QUALITY=" + compression).c_str());
        }
        else
        {
            output.setCompression(compression.c_str());
        };
    };
};

/** loads image one by one and merge with all previouly loaded images, saves the final results */
template <class ImageType, class MaskType>
bool LoadAndMergeImages(std::vector<vigra::ImageImportInfo> imageInfos, const std::string& filename, const std::string& compression, const bool wrap)
{
    if (imageInfos.empty())
    {
        return false;
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
        exportImageInfo.setICCProfile(imageInfos[0].getICCProfile());
        SetCompression(exportImageInfo, compression);
        return SaveFinalImage(image, mask, imageInfos[0], exportImageInfo, roi);
    };
};

/** prints help screen */
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

/** resave a single image
 *  LoadAndMergeImage would require the full canvas size for loading, so using this specialized version
 *  which is using the cropped intermediates images */
template<class ImageType>
bool ResaveImage(const vigra::ImageImportInfo& importInfo, vigra::ImageExportInfo& exportInfo)
{
    ImageType image(importInfo.size());
    vigra::BImage mask(image.size());
    if (importInfo.numExtraBands() == 0)
    {
        vigra::importImage(importInfo, vigra::destImage(image));
        // init mask
        vigra::initImage(vigra::destImageRange(mask), 255);
    }
    else
    {
        if (importInfo.numExtraBands() == 1)
        {
            vigra::importImageAlpha(importInfo, vigra::destImage(image), vigra::destImage(mask));
        }
        else
        {
            std::cerr << "ERROR: Images with several alpha channels are not supported." << std::endl;
            return false;
        };
    };

    const vigra::Rect2D roi(vigra::Point2D(0, 0), image.size());
    return SaveFinalImage(image, mask, importInfo, exportInfo, roi);
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

    bool success = false;
    if (files.size() == 1)
    {
        //special case, only one image given
        vigra::ImageImportInfo imageInfo(files[0].c_str());
        vigra::ImageExportInfo exportInfo(output.c_str());
        exportInfo.setXResolution(imageInfo.getXResolution());
        exportInfo.setYResolution(imageInfo.getYResolution());
        exportInfo.setCanvasSize(imageInfo.getCanvasSize());
        exportInfo.setPosition(imageInfo.getPosition());
        exportInfo.setICCProfile(imageInfo.getICCProfile());
        SetCompression(exportInfo, compression);
        const std::string pixeltype = imageInfo.getPixelType();
        if (imageInfo.isColor())
        {
            if (pixeltype == "UINT8")
            {
                success = ResaveImage<vigra::BRGBImage>(imageInfo, exportInfo);
            }
            else if (pixeltype == "INT16")
            {
                success = ResaveImage<vigra::Int16RGBImage>(imageInfo, exportInfo);
            }
            else if (pixeltype == "UINT16")
            {
                success = ResaveImage<vigra::UInt16RGBImage>(imageInfo, exportInfo);
            }
            else if (pixeltype == "INT32")
            {
                success = ResaveImage<vigra::Int32RGBImage>(imageInfo, exportInfo);
            }
            else if (pixeltype == "UINT32")
            {
                success = ResaveImage<vigra::UInt32RGBImage>(imageInfo, exportInfo);
            }
            else if (pixeltype == "FLOAT")
            {
                success = ResaveImage<vigra::FRGBImage>(imageInfo, exportInfo);
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
                success = ResaveImage<vigra::BImage>(imageInfo, exportInfo);
            }
            else if (pixeltype == "INT16")
            {
                success = ResaveImage<vigra::Int16Image>(imageInfo, exportInfo);
            }
            else if (pixeltype == "UINT16")
            {
                success = ResaveImage<vigra::UInt16Image>(imageInfo, exportInfo);
            }
            else if (pixeltype == "INT32")
            {
                success = ResaveImage<vigra::Int32Image>(imageInfo, exportInfo);
            }
            else if (pixeltype == "UINT32")
            {
                success = ResaveImage<vigra::UInt32Image>(imageInfo, exportInfo);
            }
            else if (pixeltype == "FLOAT")
            {
                success = ResaveImage<vigra::FImage>(imageInfo, exportInfo);
            }
            else
            {
                std::cerr << " ERROR: unsupported pixel type: " << pixeltype << std::endl;
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
        };

        if (imageInfos[0].isColor())
        {
            if (pixeltype == "UINT8")
            {
                success = LoadAndMergeImages<vigra::BRGBImage, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "INT16")
            {
                success = LoadAndMergeImages<vigra::Int16RGBImage, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "UINT16")
            {
                success = LoadAndMergeImages<vigra::UInt16RGBImage, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "INT32")
            {
                success = LoadAndMergeImages<vigra::Int32RGBImage, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "UINT32")
            {
                success = LoadAndMergeImages<vigra::UInt32RGBImage, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "FLOAT")
            {
                success = LoadAndMergeImages<vigra::FRGBImage, vigra::BImage>(imageInfos, output, compression, wraparound);
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
                success = LoadAndMergeImages<vigra::BImage, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "INT16")
            {
                success = LoadAndMergeImages<vigra::Int16Image, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "UINT16")
            {
                success = LoadAndMergeImages<vigra::UInt16Image, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "INT32")
            {
                success = LoadAndMergeImages<vigra::Int32Image, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "UINT32")
            {
                success = LoadAndMergeImages<vigra::UInt32Image, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else if (pixeltype == "FLOAT")
            {
                success = LoadAndMergeImages<vigra::FImage, vigra::BImage>(imageInfos, output, compression, wraparound);
            }
            else
            {
                std::cerr << " ERROR: unsupported pixel type: " << pixeltype << std::endl;
            };
        };
    };

    if (success)
    {
        std::cout << "Written result to " << output << std::endl;
        return 0;
    };
    return 1;
}
