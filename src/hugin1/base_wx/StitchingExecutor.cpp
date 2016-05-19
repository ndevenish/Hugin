/**
* @file StitchingExecutor.cpp
* @brief implementation of CommandQueue creating for stitching engine
*
* @author T. Modes
*/

/*  This is free software; you can redistribute it and/or
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

#include "StitchingExecutor.h"

#include <wx/config.h>
#if wxCHECK_VERSION(3,0,0)
#include <wx/translation.h>
#else
#include <wx/intl.h>
#endif
#include <wx/arrstr.h>
#include <wx/filefn.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include "hugin_utils/utils.h"
#include "hugin_base/panotools/PanoToolsUtils.h"
#include "hugin_base/panodata/PanoramaOptions.h"
#include "hugin_base/algorithms/basic/LayerStacks.h"
#include "base_wx/platform.h"
#include "base_wx/wxPlatform.h"
#include "base_wx/LensTools.h"
#include "hugin/config_defaults.h"

#if wxCHECK_VERSION(3,0,0)
#define WXSTRING(s)  s
#else
#define WXSTRING(s) wxString(s.c_str(), wxConvLocal)
#endif

namespace HuginQueue
{
    namespace detail
    {
        // contains some helper functions
        /** returns an array of filenames with numbers */
        wxArrayString GetNumberedFilename(const wxString& prefix, const wxString& postfix, const HuginBase::UIntSet& img)
        {
            wxArrayString filenames;
            for (HuginBase::UIntSet::const_iterator it = img.begin(); it != img.end(); ++it)
            {
                filenames.Add(wxString::Format(wxT("%s%04u%s"), prefix.c_str(), *it, postfix.c_str()));
            };
            return filenames;
        };

        /** append all strings from input array to output array */
        void AddToArray(const wxArrayString& input, wxArrayString& output)
        {
            for (size_t i = 0; i < input.size(); ++i)
            {
                output.Add(input[i]);
            };
        };

        /** return the temp dir from the preferences, ensure that it ends with path separator */
        const wxString GetConfigTempDir(const wxConfigBase* config)
        {
            wxString tempDir = config->Read(wxT("tempDir"), wxT(""));
            if (!tempDir.IsEmpty())
            {
                if (tempDir.Last() != wxFileName::GetPathSeparator())
                {
                    tempDir.Append(wxFileName::GetPathSeparator());
                }
            };
            return tempDir;
        };

        /** generate the final argfile
            @return full name of generated argfile 
        */
        wxString GenerateFinalArgfile(const HuginBase::Panorama & pano, const wxString& projectName, const wxConfigBase* config, const HuginBase::UIntSet& images, const double exifToolVersion)
        {
            wxString argfileInput = config->Read(wxT("/output/FinalArgfile"), wxEmptyString);
            const bool generateGPanoTags = (config->Read(wxT("/output/writeGPano"), HUGIN_EXIFTOOL_CREATE_GPANO) == 1l) && (exifToolVersion >= 9.09);
            pano_projection_features proj;
            const HuginBase::PanoramaOptions &opts = pano.getOptions();
            const bool readProjectionName = panoProjectionFeaturesQuery(opts.getProjection(), &proj) != 0;
            // build placeholder map
            std::map<wxString, wxString> placeholders;
#ifdef _WIN32
            const wxString linebreak(wxT("&#xd;&#xa;"));
#else
            const wxString linebreak(wxT("&#xa;"));
#endif
            if (readProjectionName)
            {
                placeholders.insert(std::make_pair(wxT("%projection"), wxString(proj.name, wxConvLocal)));
                placeholders.insert(std::make_pair(wxT("%projectionNumber"), wxString::Format(wxT("%d"), opts.getProjection())));
            };
            // fill in some placeholders
            placeholders.insert(std::make_pair(wxT("%hfov"), wxString::Format(wxT("%.0f"), opts.getHFOV())));
            placeholders.insert(std::make_pair(wxT("%vfov"), wxString::Format(wxT("%.0f"), opts.getVFOV())));
            placeholders.insert(std::make_pair(wxT("%ev"), wxString::Format(wxT("%.2f"), opts.outputExposureValue)));
            placeholders.insert(std::make_pair(wxT("%nrImages"), wxString::Format(wxT("%lu"), (unsigned long)images.size())));
            placeholders.insert(std::make_pair(wxT("%nrAllImages"), wxString::Format(wxT("%lu"), (unsigned long)pano.getNrOfImages())));
            placeholders.insert(std::make_pair(wxT("%fullwidth"), wxString::Format(wxT("%u"), opts.getWidth())));
            placeholders.insert(std::make_pair(wxT("%fullheight"), wxString::Format(wxT("%u"), opts.getHeight())));
            placeholders.insert(std::make_pair(wxT("%width"), wxString::Format(wxT("%d"), opts.getROI().width())));
            placeholders.insert(std::make_pair(wxT("%height"), wxString::Format(wxT("%d"), opts.getROI().height())));
            wxFileName projectFilename(projectName);
            placeholders.insert(std::make_pair(wxT("%projectname"), projectFilename.GetFullName()));
            // now open the final argfile
            wxFileName tempArgfileFinal(wxFileName::CreateTempFileName(GetConfigTempDir(config) + wxT("he")));
            wxFFileOutputStream outputStream(tempArgfileFinal.GetFullPath());
            wxTextOutputStream outputFile(outputStream);
            // write argfile
            outputFile << wxT("-Software=Hugin ") << wxString(hugin_utils::GetHuginVersion().c_str(), wxConvLocal) << endl;
            outputFile << wxT("-E") << endl;
            outputFile << wxT("-UserComment<${UserComment}") << linebreak;
            if (readProjectionName)
            {
                outputFile << wxT("Projection: ") << placeholders[wxT("%projection")] << wxT(" (") << placeholders[wxT("%projectionNumber")] << wxT(")") << linebreak;
            };
            outputFile << wxT("FOV: ") << placeholders[wxT("%hfov")] << wxT(" x ") << placeholders[wxT("%vfov")] << linebreak;
            outputFile << wxT("Ev: ") << placeholders[wxT("%ev")] << endl;
            outputFile << wxT("-f") << endl;
            if (generateGPanoTags)
            {
                //GPano tags are only indented for equirectangular images
                if (opts.getProjection() == HuginBase::PanoramaOptions::EQUIRECTANGULAR)
                {
                    const vigra::Rect2D roi = opts.getROI();
                    int left = roi.left();
                    int top = roi.top();
                    int width = roi.width();
                    int height = roi.height();

                    int fullWidth = opts.getWidth();
                    if (opts.getHFOV()<360)
                    {
                        fullWidth = static_cast<int>(opts.getWidth() * 360.0 / opts.getHFOV());
                        left += (fullWidth - opts.getWidth()) / 2;
                    };
                    int fullHeight = opts.getHeight();
                    if (opts.getVFOV()<180)
                    {
                        fullHeight = static_cast<int>(opts.getHeight() * 180.0 / opts.getVFOV());
                        top += (fullHeight - opts.getHeight()) / 2;
                    };
                    outputFile << wxT("-UsePanoramaViewer=True") << endl;
                    outputFile << wxT("-StitchingSoftware=Hugin") << endl;
                    outputFile << wxT("-ProjectionType=equirectangular") << endl;
                    outputFile << wxT("-CroppedAreaLeftPixels=") << left << endl;
                    outputFile << wxT("-CroppedAreaTopPixels=") << top << endl;
                    outputFile << wxT("-CroppedAreaImageWidthPixels=") << width << endl;
                    outputFile << wxT("-CroppedAreaImageHeightPixels=") << height << endl;
                    outputFile << wxT("-FullPanoWidthPixels=") << fullWidth << endl;
                    outputFile << wxT("-FullPanoHeightPixels=") << fullHeight << endl;
                    outputFile << wxT("-SourcePhotosCount=") << static_cast<wxUint32>(pano.getNrOfImages()) << endl;
                };
            };
            // now open the input file and append it
            if (!argfileInput.IsEmpty())
            {
                if (wxFileExists(argfileInput))
                {
                    wxFileInputStream inputFileStream(argfileInput);
                    wxTextInputStream input(inputFileStream);
                    while (inputFileStream.IsOk() && !inputFileStream.Eof())
                    {
                        wxString line = input.ReadLine();
                        // replace all placeholders
                        for (std::map<wxString, wxString>::const_iterator it = placeholders.begin(); it != placeholders.end(); ++it)
                        {
                            line.Replace(it->first, it->second, true);
                        };
                        // now append to existing argfile
                        outputFile << line << endl;
                    };
                };
            };
            return tempArgfileFinal.GetFullPath();
        };

        double wxStringToCDouble(const wxString& s)
        {
            double val;
#if wxCHECK_VERSION(2,9,1)
            s.ToCDouble(&val);
#else
            wxString s2(s);
            s2.Replace(wxT("."), wxLocale::GetInfo(wxLOCALE_DECIMAL_POINT, wxLOCALE_CAT_NUMBER));
            s2.ToDouble(&val);
#endif
            return val;
        };

        wxString PrintDetailInfo(const HuginBase::Panorama& pano, const HuginBase::PanoramaOptions& opts, const HuginBase::UIntSet& allActiveImages, const wxString& prefix, const wxString& bindir, wxConfigBase* config, double& exiftoolVersion)
        {
            wxString output;
            const wxString wxEndl(wxT("\n"));
            output
                << wxT("============================================") << wxEndl
                << _("Stitching panorama...") << wxEndl
                << wxT("============================================") << wxEndl
                << wxEndl
                << _("Platform:") << wxT(" ") << wxGetOsDescription() << wxEndl
                << _("Version:") << wxT(" ") << wxString(hugin_utils::GetHuginVersion().c_str(), wxConvLocal) << wxEndl
                << _("Working directory:") << wxT(" ") << wxFileName::GetCwd() << wxEndl
                << _("Output prefix:") << wxT(" ") << prefix << wxEndl
                << wxEndl;
            if (opts.outputLDRBlended || opts.outputLDRExposureBlended || opts.outputLDRExposureLayersFused || 
                opts.outputHDRBlended || opts.outputLDRExposureLayers)
            {
                switch (opts.blendMode)
                {
                    case HuginBase::PanoramaOptions::ENBLEND_BLEND:
                        {
                            wxArrayString version;
                            if (wxExecute(wxEscapeFilename(GetExternalProgram(config, bindir, wxT("enblend"))) + wxT(" --version"), version, wxEXEC_SYNC) == 0l)
                            {
                                output << _("Blender:") << wxT(" ") << version[0] << wxEndl;
                            }
                            else
                            {
                                output << _("Blender:") << wxT(" ") << _("Unknown blender (enblend --version failed)") << wxEndl;
                            };
                        };
                        break;
                    case HuginBase::PanoramaOptions::INTERNAL_BLEND:
                    default:  // switch to internal blender for all other cases, not exposed in GUI
                        output << _("Blender:") << wxT(" ") << _("internal") << wxEndl;
                        break;
                };
            };
            if (opts.outputLDRExposureBlended || opts.outputLDRExposureLayersFused || opts.outputLDRStacks )
            {
                wxArrayString version;
                if (wxExecute(wxEscapeFilename(GetExternalProgram(config, bindir, wxT("enfuse"))) + wxT(" --version"), version, wxEXEC_SYNC) == 0l)
                {
                    output << _("Exposure fusion:") << wxT(" ") << version[0] << wxEndl;
                }
                else
                {
                    output << _("Exposure fusion:") << wxT(" ") << _("Unknown exposure fusion (enfuse --version failed)") << wxEndl;
                };
            };
            if (config->Read(wxT("/output/useExiftool"), HUGIN_USE_EXIFTOOL) == 1l)
            {
                wxArrayString version;
                if (wxExecute(wxEscapeFilename(GetExternalProgram(config, bindir, wxT("exiftool"))) + wxT(" -ver"), version, wxEXEC_SYNC) == 0l)
                {
                    output << _("ExifTool version:") << wxT(" ") << version[0] << wxEndl;
                    exiftoolVersion = wxStringToCDouble(version[0]);
                }
                else
                {
                    output << _("ExifTool:") << wxT(" ") << _("FAILED") << wxEndl;
                    exiftoolVersion = 1;
                };
            };
            output
                << wxEndl
                << _("Number of active images:") << wxT(" ") << allActiveImages.size() << wxEndl
                << wxString::Format(_("Output exposure value: %.1f"), opts.outputExposureValue) << wxEndl
                << wxString::Format(_("Canvas size: %dx%d"), opts.getSize().width(), opts.getSize().height()) << wxEndl
                << wxString::Format(_("ROI: (%d, %d) - (%d, %d)"), opts.getROI().left(), opts.getROI().top(), opts.getROI().right(), opts.getROI().bottom()) << wxT(" ") << wxEndl
                << wxString::Format(_("FOV: %.0fx%.0f"), opts.getHFOV(), opts.getVFOV()) << wxEndl;
            pano_projection_features proj;
            const bool readProjectionName = panoProjectionFeaturesQuery(opts.getProjection(), &proj) != 0;
            if (readProjectionName)
            {
                output
                    << _("Projection:") << wxT(" ") << wxGetTranslation(wxString(proj.name, wxConvLocal))
                    << wxT("(") << opts.getProjection() << wxT(")") << wxEndl;
            }
            else
            {
                output
                    << _("Projection:") << wxT(" ") << opts.getProjection() << wxEndl;
            };
            output
                << _("Using GPU for remapping:") << wxT(" ") << (opts.remapUsingGPU ? _("true") : _("false")) << wxEndl
                << wxEndl;
            if (opts.outputLDRBlended || opts.outputLDRExposureBlended || opts.outputLDRExposureLayersFused || opts.outputHDRBlended)
            {
                output << _("Panorama Outputs:") << wxEndl;
                if (opts.outputLDRBlended)
                {
                    output << wxT("* ") << _("Exposure corrected, low dynamic range") << wxEndl;
                };
                if (opts.outputLDRExposureBlended)
                {
                    output << wxT("* ") << _("Exposure fused from stacks") << wxEndl;
                };
                if (opts.outputLDRExposureLayersFused)
                {
                    output << wxT("* ") << _("Exposure fused from any arrangement") << wxEndl;
                };
                if (opts.outputHDRBlended)
                {
                    output << wxT("* ") << _("High dynamic range") << wxEndl;
                };
                output << wxEndl;
            };
            if (opts.outputLDRLayers || opts.outputLDRExposureRemapped || opts.outputHDRLayers)
            {
                output << _("Remapped Images:") << wxEndl;
                if (opts.outputLDRBlended)
                {
                    output << wxT("* ") << _("Exposure corrected, low dynamic range") << wxEndl;
                };
                if (opts.outputLDRExposureRemapped)
                {
                    output << wxT("* ") << _("No exposure correction, low dynamic range") << wxEndl;
                };
                if (opts.outputHDRLayers)
                {
                    output << wxT("* ") << _("High dynamic range") << wxEndl;
                };
                output << wxEndl;
            };
            if (opts.outputLDRStacks || opts.outputHDRStacks)
            {
                output << _("Combined stacks:") << wxEndl;
                if (opts.outputLDRStacks)
                {
                    output << wxT("* ") << _("Exposure fused stacks") << wxEndl;
                };
                if (opts.outputHDRStacks)
                {
                    output << wxT("* ") << _("High dynamic range") << wxEndl;
                };
                output << wxEndl;
            };
            if (opts.outputLDRExposureLayers)
            {
                output << _("Layers:") << wxEndl
                    << wxT("* ") << _("Blended layers of similar exposure, without exposure correction") << wxEndl
                    << wxEndl;
            };
            const HuginBase::SrcPanoImage img = pano.getImage(*allActiveImages.begin());
            output << _("First input image") << wxEndl
                << _("Number:") << wxT(" ") << *allActiveImages.begin() << wxEndl
                << _("Filename:") << wxT(" ") << WXSTRING(img.getFilename()) << wxEndl
                << wxString::Format(_("Size: %dx%d"), img.getWidth(), img.getHeight()) << wxEndl
                << _("Projection:") << wxT(" ") << getProjectionString(img) << wxEndl
                << _("Response type:") << wxT(" ") << getResponseString(img) << wxEndl
                << wxString::Format(_("HFOV: %.0f"), img.getHFOV()) << wxEndl
                << wxString::Format(_("Exposure value: %.1f"), img.getExposureValue()) << wxEndl
                << wxEndl;
            return output;
        };

#if !wxCHECK_VERSION(3,0,0)
        /** create a temporary response with all images in given array, used by enblend, enfuse and exiftool 
            only needed for wxWidgets 2.8.x */
        const wxArrayString CreateResponsefile(const wxArrayString images, const size_t maxFileNumber=20000U)
        {
            wxArrayString responseFiles;
            wxFFileOutputStream* outputStream = NULL;
            wxTextOutputStream* outputFile = NULL;
            wxString workingDir = wxFileName::GetCwd();
            if (workingDir.Last() != wxFileName::GetPathSeparator())
            {
                workingDir.Append(wxFileName::GetPathSeparator());
            };
            // write response file
            for (size_t i = 0; i < images.size(); ++i)
            {
                if (i % maxFileNumber == 0)
                {
                    wxFileName tempResponsefile(wxFileName::CreateTempFileName(GetConfigTempDir(wxConfigBase::Get()) + wxT("hb")));
                    responseFiles.Add(tempResponsefile.GetFullPath());
                    if (outputFile != NULL)
                    {
                        outputStream->Close();
                        delete outputFile;
                        delete outputStream;
                    };
                    outputStream = new wxFFileOutputStream(tempResponsefile.GetFullPath());
                    outputFile = new wxTextOutputStream(*outputStream);
                };
                (*outputFile) << workingDir.c_str() << images[i].c_str() << endl;
            };
            if (outputFile != NULL)
            {
                outputStream->Close();
                delete outputFile;
                delete outputStream;
            }
            return responseFiles;
        };
#endif
        /** workaround for limitation of wxExecute of wxWidgets 2.8 */
        HuginQueue::NormalCommand* GetEnblendFuseCommand(const wxString& command, const wxString& args, const wxString& comment, const wxArrayString& images, wxArrayString& tempFilesToDelete)
        {
#if wxCHECK_VERSION(3,0,0)
            // on wxWidgets 3.0 this workaround is not necessary, wxExecute process also big number of arguments
            return new NormalCommand(command, args + wxT(" ") + GetQuotedFilenamesString(images), comment);
#else
            if (images.size() < 100)
            {
                return new NormalCommand(command, args + wxT(" ") + GetQuotedFilenamesString(images), comment);
            }
            // wxWidgets 2.8 accept only 127 argument for wxExecute
            // workaround for this case
            // use response file for enblend/enfuse
            // but one response file can only contain 63 image files, so we need
            // to split the response files in several parts
            const wxArrayString tempResponsefiles = CreateResponsefile(images, 60);
            wxString finalArgs(args);
            for (size_t i = 0; i < tempResponsefiles.size(); ++i)
            {
                finalArgs.Append(wxT(" @") + wxEscapeFilename(tempResponsefiles[i]));
                tempFilesToDelete.Add(tempResponsefiles[i]);
            };
            return new NormalCommand(command, finalArgs, comment);
#endif
        }

        /** build quoted filename list for verdandi */
        wxString GetQuotedFilenamesStringForVerdandi(const wxArrayString& files, const HuginBase::Panorama& pano, const HuginBase::UIntSetVector& stacks, const int referenceImage, const bool hardSeam)
        {
            // if output is hard seam we keep the order
            if (hardSeam)
            {
                return GetQuotedFilenamesString(files);
            };
            // user wants a blended seam, we need to figure out the correct order
            int refImage = 0;
            // first build a subpano which contains only one image per stack of the original pano
            HuginBase::UIntSet stackImgs;
            for (size_t i = 0; i < stacks.size(); ++i)
            {
                if (set_contains(stacks[i], referenceImage))
                {
                    refImage = i;
                };
                stackImgs.insert(*(stacks[i].begin()));
            };
            // now create the subpano, don't forget to delete at end
            HuginBase::PanoramaData* subpano = pano.getNewSubset(stackImgs);
            HuginBase::UIntSet subpanoImgs;
            fill_set(subpanoImgs, 0, stackImgs.size() - 1);
            // find the blend order
            HuginBase::UIntVector blendOrder = HuginBase::getEstimatedBlendingOrder(*subpano, subpanoImgs, refImage);
            delete subpano;
            // now build the string in the correct order
            wxString s;
            for (size_t i = 0; i < blendOrder.size();++i)
            {
                s.Append(wxEscapeFilename(files[blendOrder[i]]) + wxT(" "));
            };
            return s;
        };
    } // namespace detail

    CommandQueue* GetStitchingCommandQueue(const HuginBase::Panorama & pano, const wxString& ExePath, const wxString& project, const wxString& prefix, wxString& statusText, wxArrayString& outputFiles, wxArrayString& tempFilesDelete)
    {
        CommandQueue* commands = new CommandQueue;
        const HuginBase::UIntSet allActiveImages = getImagesinROI(pano, pano.getActiveImages());
        if (allActiveImages.empty())
        {
            std::cerr << "ERROR: No active images in ROI. Nothing to do." << std::endl;
            return commands;
        }
        std::vector<HuginBase::UIntSet> stacks;

        // check options, not all are currently supported
        HuginBase::PanoramaOptions opts = pano.getOptions();
        wxConfigBase* config = wxConfigBase::Get();
        opts.remapUsingGPU = config->Read(wxT("/Nona/UseGPU"), HUGIN_NONA_USEGPU) == 1;
        if (opts.remapper != HuginBase::PanoramaOptions::NONA)
        {
            std::cerr << "ERROR: Only nona remappper is supported by hugin_executor." << std::endl;
            return commands;
        };
        if (opts.blendMode != HuginBase::PanoramaOptions::ENBLEND_BLEND && opts.blendMode != HuginBase::PanoramaOptions::INTERNAL_BLEND)
        {
            std::cerr << "ERROR: Only enblend and internal remappper are currently supported by hugin_executor." << std::endl;
            return commands;
        };
        if (opts.hdrMergeMode != HuginBase::PanoramaOptions::HDRMERGE_AVERAGE)
        {
            std::cerr << "ERROR: Only hdr merger HDRMERGE_AVERAGE is currently supported by hugin_executor." << std::endl;
            return commands;
        };
        double exiftoolVersion;
        statusText=detail::PrintDetailInfo(pano, opts, allActiveImages, prefix, ExePath, config, exiftoolVersion);
        // prepare some often needed variables
        const wxString quotedProject(wxEscapeFilename(project));
        // prepare nona arguments
        wxString nonaArgs(wxT("-v "));
        wxString enLayersCompressionArgs;
        if (!opts.outputLayersCompression.empty())
        {
            nonaArgs.Append(wxT("-z ") + WXSTRING(opts.outputLayersCompression) + wxT(" "));
            enLayersCompressionArgs.Append(wxT(" --compression=") + WXSTRING(opts.outputLayersCompression) + wxT(" "));
        }
        else
        {
            if (opts.outputImageType == "jpg")
            {
                nonaArgs.Append(wxT("-z LZW "));
            }
        };
        if (opts.remapUsingGPU)
        {
            nonaArgs.Append(wxT("-g "));
        };
        // prepare enblend arguments
        wxString enblendArgs;
        if (opts.blendMode == HuginBase::PanoramaOptions::ENBLEND_BLEND)
        {
            enblendArgs.Append(WXSTRING(opts.enblendOptions));
            if ((opts.getHFOV() == 360.0) && (opts.getWidth()==opts.getROI().width()))
            {
                enblendArgs.Append(wxT(" -w"));
            };
            const vigra::Rect2D roi (opts.getROI());
            if (roi.top() != 0 || roi.left() != 0)
            {
                enblendArgs << wxT(" -f") << roi.width() << wxT("x") << roi.height() << wxT("+") << roi.left() << wxT("+") << roi.top();
            }
            else
            {
                enblendArgs << wxT(" -f") << roi.width() << wxT("x") << roi.height();
            };
            enblendArgs.Append(wxT(" "));
        };
        // prepare internal blending arguments
        wxString verdandiArgs;
        if (opts.blendMode == HuginBase::PanoramaOptions::INTERNAL_BLEND)
        {
            verdandiArgs.Append(WXSTRING(opts.verdandiOptions));
            if ((opts.getHFOV() == 360.0) && (opts.getWidth() == opts.getROI().width()))
            {
                verdandiArgs.Append(wxT(" -w"));
            };
        };
        // prepare the compression switches
        wxString finalCompressionArgs;
        if (opts.outputImageType == "tif" && !opts.outputImageTypeCompression.empty())
        {
            finalCompressionArgs << wxT(" --compression=") << WXSTRING(opts.outputImageTypeCompression);
        }
        else
        {
            if (opts.outputImageType == "jpg")
            {
                finalCompressionArgs << wxT(" --compression=") << opts.quality;
            };
        };
        finalCompressionArgs.Append(wxT(" "));
        // prepare enfuse arguments
        wxString enfuseArgs(WXSTRING(opts.enfuseOptions) + wxT(" "));
        if ((opts.getHFOV() == 360.0) && (opts.getWidth() == opts.getROI().width()))
        {
            enfuseArgs.Append(wxT(" -w"));
        };
        const vigra::Rect2D roi (opts.getROI());
        if (roi.top() != 0 || roi.left() != 0)
        {
            enfuseArgs << wxT(" -f") << roi.width() << wxT("x") << roi.height() << wxT("+") << roi.left() << wxT("+") << roi.top();
        }
        else
        {
            enfuseArgs << wxT(" -f") << roi.width() << wxT("x") << roi.height();
        };
        enfuseArgs.Append(wxT(" "));

        // prepare exiftool args
        const bool copyMetadata = config->Read(wxT("/output/useExiftool"), HUGIN_USE_EXIFTOOL) == 1l;
        wxString exiftoolArgs;
        wxString exiftoolArgsFinal;
        if (copyMetadata)
        {
            exiftoolArgs = wxT("-overwrite_original_in_place -TagsFromFile ");
            exiftoolArgs.Append(wxEscapeFilename(wxString(pano.getImage(0).getFilename().c_str(), HUGIN_CONV_FILENAME)));
            // required tags, can not be overwritten
            exiftoolArgs.Append(wxT(" -WhitePoint -ColorSpace"));
            wxString exiftoolArgfile = config->Read(wxT("/output/CopyArgfile"), wxEmptyString);
            if (exiftoolArgfile.IsEmpty())
            {
                exiftoolArgfile = wxString(std::string(hugin_utils::GetDataDir() + "hugin_exiftool_copy.arg").c_str(), HUGIN_CONV_FILENAME);
            };
            wxFileName argfile(exiftoolArgfile);
            argfile.Normalize();
            exiftoolArgs.Append(wxT(" -@ ") + wxEscapeFilename(argfile.GetFullPath()) + wxT(" "));
            wxString finalArgfile = detail::GenerateFinalArgfile(pano, project, config, allActiveImages, exiftoolVersion);
            if (!finalArgfile.IsEmpty())
            {
                exiftoolArgsFinal.Append(wxT(" -@ ") + wxEscapeFilename(finalArgfile) + wxT(" "));
                tempFilesDelete.Add(finalArgfile);
            };
        };
        wxArrayString filesForFullExiftool;
        wxArrayString filesForCopyTagsExiftool;

        // normal output
        if (opts.outputLDRBlended || opts.outputLDRLayers)
        {
            const wxArrayString remappedImages(detail::GetNumberedFilename(prefix, wxT(".tif"), allActiveImages));
            const wxString finalFilename(prefix + wxT(".") + WXSTRING(opts.outputImageType));
            if (opts.blendMode == HuginBase::PanoramaOptions::INTERNAL_BLEND && opts.outputLDRBlended)
            {
                wxString finalNonaArgs(wxT("-v -r ldr "));
                if (opts.remapUsingGPU)
                {
                    finalNonaArgs.Append(wxT("-g "));
                }
                if (!opts.verdandiOptions.empty())
                {
                    finalNonaArgs.Append(WXSTRING(opts.verdandiOptions));
                    finalNonaArgs.Append(wxT(" "));
                };
                if (opts.outputImageType == "tif")
                {
                    finalNonaArgs.Append(wxT("-m TIFF "));
                    if (!opts.outputImageTypeCompression.empty())
                    {
                        finalNonaArgs.Append(wxT("-z ") + WXSTRING(opts.outputImageTypeCompression) + wxT(" "));
                    };
                }
                else
                {
                    if (opts.outputImageType == "jpg")
                    {
                        finalNonaArgs.Append(wxT("-m JPEG -z "));
                        finalNonaArgs << opts.quality << wxT(" ");
                    }
                    else
                    {
                        if (opts.outputImageType == "png")
                        {
                            finalNonaArgs.Append(wxT("-m PNG "));
                        }
                        else
                        {
                            std::cerr << "ERROR: Invalid output image type found." << std::endl;
                            return commands;
                        };
                    };
                };
                if (opts.outputLDRLayers)
                {
                    finalNonaArgs.Append(wxT("--save-intermediate-images "));
                    detail::AddToArray(remappedImages, outputFiles);
                }
                finalNonaArgs.Append(wxT("-o ") + wxEscapeFilename(prefix) + wxT(" ") + quotedProject);
                commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("nona")),
                    finalNonaArgs, _("Remapping and blending LDR images...")));
                outputFiles.Add(finalFilename);
                if (copyMetadata)
                {
                    filesForFullExiftool.Add(finalFilename);
                };
            }
            else
            {
                commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("nona")),
                    nonaArgs + wxT("-r ldr -m TIFF_m -o ") + wxEscapeFilename(prefix) + wxT(" ") + quotedProject,
                    _("Remapping LDR images...")));
                detail::AddToArray(remappedImages, outputFiles);
                if (opts.outputLDRBlended)
                {
                    if (opts.blendMode == HuginBase::PanoramaOptions::ENBLEND_BLEND)
                    {
                        wxString finalEnblendArgs(enblendArgs + finalCompressionArgs);
                        finalEnblendArgs.Append(wxT(" -o ") + wxEscapeFilename(finalFilename) + wxT(" -- "));
                        commands->push_back(detail::GetEnblendFuseCommand(GetExternalProgram(config, ExePath, wxT("enblend")),
                            finalEnblendArgs, _("Blending images..."), remappedImages, tempFilesDelete));
                        outputFiles.Add(finalFilename);
                        if (copyMetadata)
                        {
                            filesForFullExiftool.Add(finalFilename);
                        };
                    };
                    if (!opts.outputLDRLayers)
                    {
                        detail::AddToArray(remappedImages, tempFilesDelete);
                    };
                };
            };
        };
        // exposure fusion output
        if (opts.outputLDRExposureRemapped || opts.outputLDRStacks || opts.outputLDRExposureLayers ||
            opts.outputLDRExposureBlended || opts.outputLDRExposureLayersFused)
        {
            const wxArrayString remappedImages = detail::GetNumberedFilename(prefix + wxT("_exposure_layers_"), wxT(".tif"), allActiveImages);
            std::vector<HuginBase::UIntSet> exposureLayers;
            wxArrayString exposureLayersFiles;
            if (opts.outputLDRExposureLayers || opts.outputLDRExposureLayersFused)
            {
                exposureLayers = getExposureLayers(pano, allActiveImages, opts);
            };
            if (opts.blendMode == HuginBase::PanoramaOptions::INTERNAL_BLEND &&
                (opts.outputLDRExposureLayers || opts.outputLDRExposureLayersFused))
            {
                // directly export exposure layers by nona
                wxString finalNonaArgs(nonaArgs);
                if (!opts.verdandiOptions.empty())
                {
                    finalNonaArgs.Append(WXSTRING(opts.verdandiOptions));
                    finalNonaArgs.Append(wxT(" "));
                };
                finalNonaArgs.append(wxT("-r ldr --create-exposure-layers --ignore-exposure -o ") + wxEscapeFilename(prefix + wxT("_exposure_")));
                if (opts.outputLDRExposureRemapped || opts.outputLDRStacks || opts.outputLDRExposureBlended)
                {
                    finalNonaArgs.append(wxT(" --save-intermediate-images --intermediate-suffix=layers_"));
                    detail::AddToArray(remappedImages, outputFiles);
                    if (!opts.outputLDRExposureRemapped)
                    {
                        detail::AddToArray(remappedImages, tempFilesDelete);
                    }
                };
                finalNonaArgs.append(wxT(" "));
                finalNonaArgs.append(quotedProject);
                commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("nona")),
                    finalNonaArgs, _("Remapping LDR images and blending exposure layers...")));
                HuginBase::UIntSet exposureLayersNumber;
                fill_set(exposureLayersNumber, 0, exposureLayers.size() - 1);
                exposureLayersFiles = detail::GetNumberedFilename(prefix + wxT("_exposure_"), wxT(".tif"), exposureLayersNumber);
                detail::AddToArray(exposureLayersFiles, outputFiles);
                if (!opts.outputLDRExposureLayers)
                {
                    detail::AddToArray(exposureLayersFiles, tempFilesDelete);
                };
                if (copyMetadata && opts.outputLDRExposureLayers)
                {
                    detail::AddToArray(exposureLayersFiles, filesForCopyTagsExiftool);
                };
            }
            else
            {
                commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("nona")),
                    nonaArgs + wxT("-r ldr -m TIFF_m --ignore-exposure -o ") + wxEscapeFilename(prefix + wxT("_exposure_layers_")) + wxT(" ") + quotedProject,
                    _("Remapping LDR images without exposure correction...")));
                detail::AddToArray(remappedImages, outputFiles);
                if (!opts.outputLDRExposureRemapped)
                {
                    detail::AddToArray(remappedImages, tempFilesDelete);
                };
                if (opts.outputLDRExposureLayers || opts.outputLDRExposureLayersFused)
                {
                    // blending exposure layers, then fusing
                    // fuse all exposure layers
                    for (unsigned exposureLayer = 0; exposureLayer < exposureLayers.size(); ++exposureLayer)
                    {
                        const wxArrayString exposureLayersImgs = detail::GetNumberedFilename(prefix + wxT("_exposure_layers_"), wxT(".tif"), exposureLayers[exposureLayer]);
                        const wxString exposureLayerImgName = wxString::Format(wxT("%s_exposure_%04u%s"), prefix.c_str(), exposureLayer, wxT(".tif"));
                        exposureLayersFiles.Add(exposureLayerImgName);
                        outputFiles.Add(exposureLayerImgName);
                        // variant with internal blender is handled before, so we need only enblend
                        commands->push_back(detail::GetEnblendFuseCommand(GetExternalProgram(config, ExePath, wxT("enblend")),
                            enblendArgs + enLayersCompressionArgs + wxT(" -o ") + wxEscapeFilename(exposureLayerImgName) + wxT(" -- "),
                            wxString::Format(_("Blending exposure layer %u..."), exposureLayer), exposureLayersImgs, tempFilesDelete));
                        if (copyMetadata && opts.outputLDRExposureLayers)
                        {
                            filesForCopyTagsExiftool.Add(exposureLayerImgName);
                        };
                        if (!opts.outputLDRExposureLayers)
                        {
                            tempFilesDelete.Add(exposureLayerImgName);
                        };
                    };
                };
            };
            if (opts.outputLDRExposureLayersFused)
            {
                wxString finalEnfuseArgs(enfuseArgs + finalCompressionArgs);
                const wxString fusedExposureLayersFilename(prefix + wxT("_blended_fused.") + WXSTRING(opts.outputImageType));
                finalEnfuseArgs.Append(wxT(" -o ") + wxEscapeFilename(fusedExposureLayersFilename) + wxT(" -- "));
                commands->push_back(detail::GetEnblendFuseCommand(GetExternalProgram(config, ExePath, wxT("enfuse")),
                    finalEnfuseArgs, _("Fusing all exposure layers..."), exposureLayersFiles, tempFilesDelete));
                outputFiles.Add(fusedExposureLayersFilename);
                if (copyMetadata)
                {
                    filesForFullExiftool.Add(fusedExposureLayersFilename);
                };
            };
            if (opts.outputLDRStacks || opts.outputLDRExposureBlended)
            {
                // fusing stacks, then blending
                stacks = getHDRStacks(pano, allActiveImages, opts);
                wxArrayString stackedImages;
                // fuse all stacks
                for (unsigned stackNr = 0; stackNr < stacks.size(); ++stackNr)
                {
                    const wxArrayString stackImgs = detail::GetNumberedFilename(prefix + wxT("_exposure_layers_"), wxT(".tif"), stacks[stackNr]);
                    const wxString stackImgName = wxString::Format(wxT("%s_stack_ldr_%04u%s"), prefix.c_str(), stackNr, wxT(".tif"));
                    outputFiles.Add(stackImgName);
                    stackedImages.Add(stackImgName);
                    commands->push_back(detail::GetEnblendFuseCommand(GetExternalProgram(config, ExePath, wxT("enfuse")),
                        enfuseArgs + enLayersCompressionArgs + wxT(" -o ") + wxEscapeFilename(stackImgName) + wxT(" -- "),
                        wxString::Format(_("Fusing stack number %u..."), stackNr), stackImgs, tempFilesDelete));
                    if (copyMetadata && opts.outputLDRStacks)
                    {
                        filesForCopyTagsExiftool.Add(stackImgName);
                    };
                    if (!opts.outputLDRStacks)
                    {
                        tempFilesDelete.Add(stackImgName);
                    };
                };
                if (opts.outputLDRExposureBlended)
                {
                    const wxString fusedStacksFilename(prefix + wxT("_fused.") + WXSTRING(opts.outputImageType));
                    switch (opts.blendMode)
                    {
                    case HuginBase::PanoramaOptions::ENBLEND_BLEND:
                        {
                            wxString finalEnblendArgs(enblendArgs + finalCompressionArgs);
                            finalEnblendArgs.Append(wxT(" -o ") + wxEscapeFilename(fusedStacksFilename) + wxT(" -- "));
                            commands->push_back(detail::GetEnblendFuseCommand(GetExternalProgram(config, ExePath, wxT("enblend")),
                                finalEnblendArgs, _("Blending all stacks..."), stackedImages, tempFilesDelete));
                        };
                        break;
                    case HuginBase::PanoramaOptions::INTERNAL_BLEND:
                    default:  // switch to internal blender for all other cases, not exposed in GUI
                        {
                            wxString finalVerdandiArgs(verdandiArgs + finalCompressionArgs);
                            finalVerdandiArgs.Append(wxT(" -o ") + wxEscapeFilename(fusedStacksFilename));
                            finalVerdandiArgs.Append(wxT(" -- ") + detail::GetQuotedFilenamesStringForVerdandi(stackedImages, pano, stacks, opts.colorReferenceImage, opts.verdandiOptions.find("--seam=blend") == std::string::npos));
                            commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("verdandi")),
                                finalVerdandiArgs, _("Blending all stacks...")));
                        };
                        break;
                    };
                    outputFiles.Add(fusedStacksFilename);
                    if (copyMetadata)
                    {
                        filesForFullExiftool.Add(fusedStacksFilename);
                    };
                };
            };
        };
        // hdr output
        if (opts.outputHDRLayers || opts.outputHDRStacks || opts.outputHDRBlended)
        {
            commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("nona")),
                nonaArgs + wxT("-r hdr -m EXR_m  -o ") + wxEscapeFilename(prefix + wxT("_hdr_")) + wxT(" ") + quotedProject,
                _("Remapping HDR images...")));
            const wxArrayString remappedHDR = detail::GetNumberedFilename(prefix + wxT("_hdr_"), wxT(".exr"), allActiveImages);
            const wxArrayString remappedHDRComp = detail::GetNumberedFilename(prefix + wxT("_hdr_"), wxT("_gray.pgm"), allActiveImages);
            detail::AddToArray(remappedHDR, outputFiles);
            detail::AddToArray(remappedHDRComp, outputFiles);
            if (opts.outputHDRStacks || opts.outputHDRBlended)
            {
                // merging stacks, then blending
                if (stacks.empty())
                {
                    stacks = getHDRStacks(pano, allActiveImages, opts);
                };
                wxArrayString stackedImages;
                // merge all stacks
                for (unsigned stackNr = 0; stackNr < stacks.size(); ++stackNr)
                {
                    const wxArrayString stackImgs = detail::GetNumberedFilename(prefix + wxT("_hdr_"), wxT(".exr"), stacks[stackNr]);
                    const wxString stackImgName = wxString::Format(wxT("%s_stack_hdr_%04u%s"), prefix.c_str(), stackNr, wxT(".exr"));
                    stackedImages.Add(stackImgName);
                    outputFiles.Add(stackImgName);
                    commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("hugin_hdrmerge")),
                        WXSTRING(opts.hdrmergeOptions) + wxT(" -o ") + wxEscapeFilename(stackImgName) + wxT(" -- ") + GetQuotedFilenamesString(stackImgs),
                        wxString::Format(_("Merging HDR stack number %u..."), stackNr)));
                    if (!opts.outputHDRStacks)
                    {
                        tempFilesDelete.Add(stackImgName);
                    };
                };
                if (opts.outputHDRBlended)
                {
                    const wxString mergedStacksFilename(wxEscapeFilename(prefix + wxT("_hdr.") + WXSTRING(opts.outputImageTypeHDR)));
                    wxString finalBlendArgs(wxT(" -o ") + mergedStacksFilename + wxT(" -- "));
                    switch (opts.blendMode)
                    {
                        case HuginBase::PanoramaOptions::ENBLEND_BLEND:
                            commands->push_back(detail::GetEnblendFuseCommand(GetExternalProgram(config, ExePath, wxT("enblend")),
                                enblendArgs + finalBlendArgs, _("Blending HDR stacks..."), stackedImages, tempFilesDelete));
                            break;
                        case HuginBase::PanoramaOptions::INTERNAL_BLEND:
                        default:  // switch to internal blender for all other cases, not exposed in GUI
                            commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("verdandi")),
                                verdandiArgs + finalBlendArgs + detail::GetQuotedFilenamesStringForVerdandi(stackedImages, pano, stacks, opts.colorReferenceImage, opts.verdandiOptions.find("--seam=blend") == std::string::npos),
                                _("Blending HDR stacks...")));
                            break;
                    };
                    outputFiles.Add(mergedStacksFilename);
                };
            };
            if (!opts.outputHDRLayers)
            {
                detail::AddToArray(remappedHDR, tempFilesDelete);
                detail::AddToArray(remappedHDRComp, tempFilesDelete);
            };
        };
        // update metadata
        if (!filesForCopyTagsExiftool.IsEmpty())
        {
#if !wxCHECK_VERSION(3,0,0)
            if (filesForCopyTagsExiftool.size() > 100)
            {
                // workaround for wxExecute limitation
                const wxArrayString responseFiles = detail::CreateResponsefile(filesForCopyTagsExiftool);
                wxString finalExiftoolArgs(exiftoolArgs);
                for (size_t i = 0; i < responseFiles.size(); ++i)
                {
                    finalExiftoolArgs.Append(wxT(" -@ ") + wxEscapeFilename(responseFiles[i]));
                };
                commands->push_back(new OptionalCommand(GetExternalProgram(config, ExePath, wxT("exiftool")),
                    finalExiftoolArgs, _("Updating metadata...")));
                detail::AddToArray(responseFiles, tempFilesDelete);
            }
            else
#endif
            {
                commands->push_back(new OptionalCommand(GetExternalProgram(config, ExePath, wxT("exiftool")),
                    exiftoolArgs + GetQuotedFilenamesString(filesForCopyTagsExiftool),
                    _("Updating metadata...")));
            };
        };
        if (!filesForFullExiftool.IsEmpty())
        {
#if !wxCHECK_VERSION(3,0,0)
            if (filesForFullExiftool.size()>100)
            {
                // workaround for wxExecute limitation
                const wxArrayString responseFiles = detail::CreateResponsefile(filesForCopyTagsExiftool);
                wxString finalExiftoolArgs(exiftoolArgs + exiftoolArgsFinal);
                for (size_t i = 0; i < responseFiles.size(); ++i)
                {
                    finalExiftoolArgs.Append(wxT(" -@ ") + wxEscapeFilename(responseFiles[i]));
                };
                commands->push_back(new OptionalCommand(GetExternalProgram(config, ExePath, wxT("exiftool")),
                    finalExiftoolArgs, _("Updating metadata...")));
                detail::AddToArray(responseFiles, tempFilesDelete);
            }
            else
#endif
            {
                commands->push_back(new OptionalCommand(GetExternalProgram(config, ExePath, wxT("exiftool")),
                    exiftoolArgs + exiftoolArgsFinal + GetQuotedFilenamesString(filesForFullExiftool),
                    _("Updating metadata...")));
            };
        };
        return commands;
    };

    // now the user defined stitching engine
    // we read the settings from an ini file and construct our CommandQueue
    namespace detail
    {
        // add program with argment to queue
        // do some checks on the way
        // if an error occurs or the input is not valid, the queue is cleared and the function returns false
        bool AddBlenderCommand(CommandQueue* queue, const wxString& ExePath, const wxString& prog, 
            const int& stepNr, const wxString& arguments, const wxString& description)
        {
            if (prog.IsEmpty())
            {
                std::cerr << "ERROR: Step " << stepNr << " has no program name specified." << std::endl;
                CleanQueue(queue);
                return false;
            }
            // check program name
            // get full path for some internal commands
            wxString program;
            if (prog.CmpNoCase(wxT("verdandi")) == 0)
            { 
                program = GetInternalProgram(ExePath, wxT("verdandi"));
            }
            else
            {
                if (prog.CmpNoCase(wxT("hugin_hdrmerge")) == 0)
                {
                    program = GetInternalProgram(ExePath, wxT("hugin_hdrmerge"));
                }
                else
                {
                    program = prog;
                }
            };
            // now add to queue
            queue->push_back(new NormalCommand(program, arguments, description));
            return true;
        };

        // read a string from setting and remove all whitespaces
        const wxString GetSettingString(const wxFileConfig& setting, const wxString& name, const wxString defaultValue = wxEmptyString)
        {
            wxString s = setting.Read(name, defaultValue);
            s = s.Trim(true).Trim(false);
            return s;
        };
    }

    CommandQueue* GetStitchingCommandQueueUserOutput(const HuginBase::Panorama & pano, const wxString& ExePath, const wxString& project, const wxString& prefix, const wxString& outputSettings, wxString& statusText, wxArrayString& outputFiles, wxArrayString& tempFilesDelete)
    {
        CommandQueue* commands = new CommandQueue;
        const HuginBase::UIntSet allActiveImages = getImagesinROI(pano, pano.getActiveImages());
        if (allActiveImages.empty())
        {
            std::cerr << "ERROR: No active images in ROI. Nothing to do." << std::endl;
            return commands;
        }
        wxFileInputStream input(outputSettings);
        if (!input.IsOk())
        {
            std::cerr << "ERROR: Can not open file \"" << outputSettings.mb_str(wxConvLocal) << "\"." << std::endl;
            return commands;
        }
        wxFileConfig settings(input);
        long stepCount;
        settings.Read(wxT("/General/StepCount"), &stepCount, 0);
        if (stepCount == 0)
        {
            std::cerr << "ERROR: User-setting does not define any output steps." << std::endl;
            return commands;
        }
        const wxString desc = detail::GetSettingString(settings, wxT("/General/Description"), wxEmptyString);
        if (desc.IsEmpty())
        {
            statusText = wxString::Format(_("Stitching using \"%s\""), outputSettings.c_str());
        }
        else
        {
            statusText = wxString::Format(_("Stitching using \"%s\""), desc.c_str());
        };
        wxString intermediateImageType = detail::GetSettingString(settings, wxT("/General/IntermediateImageType"), wxT(".tif"));
        // add point if missing
        if (intermediateImageType.Left(1).Cmp(wxT("."))!=0)
        {
            intermediateImageType.Prepend(wxT("."));
        }
        // prepare some often needed variables/strings
        const HuginBase::PanoramaOptions opts = pano.getOptions();
        const bool needsWrapSwitch = (opts.getHFOV() == 360.0) && (opts.getWidth() == opts.getROI().width());
        const vigra::Rect2D roi(opts.getROI());
        wxString sizeString;
        if (roi.top() != 0 || roi.left() != 0)
        {
            sizeString << roi.width() << wxT("x") << roi.height() << wxT("+") << roi.left() << wxT("+") << roi.top();
        }
        else
        {
            sizeString << roi.width() << wxT("x") << roi.height();
        };
        const wxArrayString remappedImages(detail::GetNumberedFilename(prefix, intermediateImageType, allActiveImages));

        std::vector<HuginBase::UIntSet> exposureLayers;
        wxArrayString exposureLayersFiles;

        std::vector<HuginBase::UIntSet> stacks;
        wxArrayString stacksFiles;

        // now iterate all steps
        for (size_t i = 0; i < stepCount; ++i)
        {
            wxString stepString(wxT("/Step"));
            stepString << i;
            if (!settings.HasGroup(stepString))
            {
                std::cerr << "ERROR: Output specifies " << stepCount << " steps, but step " << i << " is missing in configuration." << std::endl;
                CleanQueue(commands);
                return commands;
            }
            settings.SetPath(stepString);
            const wxString stepType=detail::GetSettingString(settings, wxT("Type"));
            if (stepType.IsEmpty())
            {
                std::cerr << "ERROR: \"" << stepString.mb_str(wxConvLocal) << "\" has no type defined." << std::endl;
                CleanQueue(commands);
                return commands;
            };
            wxString args = detail::GetSettingString(settings, wxT("Arguments"));
            if (args.IsEmpty())
            {
                std::cerr << "ERROR: Step " << i << " has no arguments given." << std::endl;
                CleanQueue(commands);
                return commands;
            }
            const wxString description = detail::GetSettingString(settings, wxT("Description"));
            if (stepType.CmpNoCase(wxT("remap")) == 0)
            {
                // build nona command
                const bool outputLayers = (settings.Read(wxT("OutputExposureLayers"), 0l) == 1l);
                if (outputLayers)
                {
                    args.Append(wxT(" --create-exposure-layers -o ") + wxEscapeFilename(prefix + wxT("_layer")));
                }
                else
                {
                    args.Append(wxT(" -o ") + wxEscapeFilename(prefix));
                };
                args.Append(wxT(" ") + wxEscapeFilename(project));
                commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("nona")),
                    args, description));
                if (outputLayers)
                {
                    if (exposureLayers.empty())
                    {
                        exposureLayers = getExposureLayers(pano, allActiveImages, opts);
                        HuginBase::UIntSet exposureLayersNumber;
                        fill_set(exposureLayersNumber, 0, exposureLayers.size() - 1);
                        exposureLayersFiles = detail::GetNumberedFilename(prefix + wxT("_layer"), intermediateImageType, exposureLayersNumber);
                    };
                    detail::AddToArray(exposureLayersFiles, outputFiles);
                    if (settings.Read(wxT("Keep"), 0l) == 0l)
                    {
                        detail::AddToArray(exposureLayersFiles, tempFilesDelete);
                    };
                }
                else
                {
                    const wxArrayString remappedHDRComp = detail::GetNumberedFilename(prefix, wxT("_gray.pgm"), allActiveImages);
                    const bool hdrOutput = args.MakeLower().Find(wxT("-r hdr")) != wxNOT_FOUND;
                    detail::AddToArray(remappedImages, outputFiles);
                    if (hdrOutput)
                    {
                        detail::AddToArray(remappedHDRComp, outputFiles);
                    };
                    if (settings.Read(wxT("Keep"), 0l) == 0l)
                    {
                        detail::AddToArray(remappedImages, tempFilesDelete);
                        if (hdrOutput)
                        {
                            detail::AddToArray(remappedHDRComp, tempFilesDelete);
                        };
                    };
                };
            }
            else
            {
                if (stepType.CmpNoCase(wxT("merge")) == 0)
                {
                    // build a merge command
                    wxString resultFile = detail::GetSettingString(settings, wxT("Result"));
                    if (resultFile.IsEmpty())
                    {
                        std::cerr << "ERROR: Step " << i << " has no result file specified." << std::endl;
                        CleanQueue(commands);
                        return commands;
                    };
                    resultFile.Replace(wxT("%prefix%"), prefix, true);
                    if (args.Replace(wxT("%result%"), wxEscapeFilename(resultFile), true) == 0)
                    {
                        std::cerr << "ERROR: Step " << i << " has missing %result% placeholder in arguments." << std::endl;
                        CleanQueue(commands);
                        return commands;
                    };
                    const wxString BlenderInput = detail::GetSettingString(settings, wxT("Input"), wxT("all"));
                    // set the input images depending on the input
                    if (BlenderInput.CmpNoCase(wxT("all")) == 0)
                    {
                        if (args.Replace(wxT("%input%"), GetQuotedFilenamesString(remappedImages), true) == 0)
                        {
                            std::cerr << "ERROR: Step " << i << " has missing %input% placeholder in arguments." << std::endl;
                            CleanQueue(commands);
                            return commands;
                        };
                    }
                    else
                    {
                        if (BlenderInput.CmpNoCase(wxT("stacks")) == 0)
                        {
                            if (stacks.empty())
                            {
                                stacks= HuginBase::getHDRStacks(pano, allActiveImages, opts);
                                HuginBase::UIntSet stackNumbers;
                                fill_set(stackNumbers, 0, stacks.size() - 1);
                                stacksFiles = detail::GetNumberedFilename(prefix + wxT("_stack"), intermediateImageType, stackNumbers);
                            };
                            if (args.Replace(wxT("%input%"), GetQuotedFilenamesString(stacksFiles), true) == 0)
                            {
                                std::cerr << "ERROR: Step " << i << " has missing %input% placeholder in arguments." << std::endl;
                                CleanQueue(commands);
                                return commands;
                            };
                        }
                        else
                        {
                            if (BlenderInput.CmpNoCase(wxT("layers")) == 0)
                            {
                                if (exposureLayers.empty())
                                { 
                                    exposureLayers = getExposureLayers(pano, allActiveImages, opts);
                                    HuginBase::UIntSet exposureLayersNumber;
                                    fill_set(exposureLayersNumber, 0, exposureLayers.size() - 1);
                                    exposureLayersFiles = detail::GetNumberedFilename(prefix + wxT("_layer"), intermediateImageType, exposureLayersNumber);
                                };
                                if (args.Replace(wxT("%input%"), GetQuotedFilenamesString(exposureLayersFiles), true) == 0)
                                {
                                    std::cerr << "ERROR: Step " << i << " has missing %input% placeholder in arguments." << std::endl;
                                    CleanQueue(commands);
                                    return commands;
                                };
                            }
                            else
                            {
                                std::cerr << "ERROR: Step " << i << " has invalid input type: \"" << BlenderInput.mb_str(wxConvLocal) << "\"." << std::endl;
                                CleanQueue(commands);
                                return commands;
                            };
                        };
                    };
                    args.Replace(wxT("%size%"), sizeString, true);
                    wxString wrapSwitch = detail::GetSettingString(settings, wxT("WrapArgument"));
                    if (needsWrapSwitch && !wrapSwitch.IsEmpty())
                    {
                        args.Prepend(wrapSwitch + wxT(" "));
                    }
                    if (!detail::AddBlenderCommand(commands, ExePath, detail::GetSettingString(settings, wxT("Program")), i,
                        args, description))
                    {
                        return commands;
                    };
                    outputFiles.Add(resultFile);
                    if (settings.Read(wxT("Keep"), 1l) == 0l)
                    {
                        tempFilesDelete.Add(resultFile);
                    };
                }
                else
                {
                    if (stepType.CmpNoCase(wxT("stack")) == 0)
                    {
                        // build command for each stack
                        if (stacks.empty())
                        {
                            stacks = HuginBase::getHDRStacks(pano, allActiveImages, opts);
                            HuginBase::UIntSet stackNumbers;
                            fill_set(stackNumbers, 0, stacks.size() - 1);
                            stacksFiles = detail::GetNumberedFilename(prefix + wxT("_stack"), intermediateImageType, stackNumbers);
                        };
                        const bool clean = (settings.Read(wxT("Keep"), 0l) == 0l);
                        args.Replace(wxT("%size%"), sizeString, true);
                        // now iterate each stack
                        for (size_t stackNr = 0; stackNr < stacks.size(); ++stackNr)
                        {
                            wxString finalArgs(args);
                            wxArrayString remappedStackImages = detail::GetNumberedFilename(prefix, intermediateImageType, stacks[stackNr]);
                            if (finalArgs.Replace(wxT("%input%"), GetQuotedFilenamesString(remappedStackImages), true) == 0)
                            {
                                std::cerr << "ERROR: Step " << i << " has missing %input% placeholder in arguments." << std::endl;
                                CleanQueue(commands);
                                return commands;
                            };
                            if (finalArgs.Replace(wxT("%output%"), wxEscapeFilename(stacksFiles[stackNr]), true) == 0)
                            {
                                std::cerr << "ERROR: Step " << i << " has missing %output% placeholder in arguments." << std::endl;
                                CleanQueue(commands);
                                return commands;
                            };
                            if (!detail::AddBlenderCommand(commands, ExePath, detail::GetSettingString(settings, wxT("Program")), i,
                                finalArgs, description))
                            {
                                return commands;
                            };
                            outputFiles.Add(stacksFiles[stackNr]);
                            if (clean)
                            {
                                tempFilesDelete.Add(stacksFiles[stackNr]);
                            };
                        }
                    }
                    else
                    {
                        if (stepType.CmpNoCase(wxT("layer")) == 0)
                        {
                            // build command for each exposure layer
                            if (exposureLayers.empty())
                            {
                                exposureLayers = HuginBase::getExposureLayers(pano, allActiveImages, opts);
                                HuginBase::UIntSet exposureLayersNumber;
                                fill_set(exposureLayersNumber, 0, exposureLayers.size() - 1);
                                exposureLayersFiles = detail::GetNumberedFilename(prefix + wxT("_layer"), intermediateImageType, exposureLayersNumber);
                            };
                            const bool clean = (settings.Read(wxT("Keep"), 0l) == 0l);
                            args.Replace(wxT("%size%"), sizeString, true);
                            // iterate all exposure layers
                            for (size_t exposureLayerNr = 0; exposureLayerNr < exposureLayers.size(); ++exposureLayerNr)
                            {
                                wxString finalArgs(args);
                                wxArrayString remappedLayerImages = detail::GetNumberedFilename(prefix, intermediateImageType, exposureLayers[exposureLayerNr]);
                                if (finalArgs.Replace(wxT("%input%"), GetQuotedFilenamesString(remappedLayerImages), true) == 0)
                                {
                                    std::cerr << "ERROR: Step " << i << " has missing %input% placeholder in arguments." << std::endl;
                                    CleanQueue(commands);
                                    return commands;
                                };
                                if (finalArgs.Replace(wxT("%output%"), wxEscapeFilename(exposureLayersFiles[exposureLayerNr]), true) == 0)
                                {
                                    std::cerr << "ERROR: Step " << i << " has missing %output% placeholder in arguments." << std::endl;
                                    CleanQueue(commands);
                                    return commands;
                                };
                                if (!detail::AddBlenderCommand(commands, ExePath, detail::GetSettingString(settings, wxT("Program")), i,
                                    finalArgs, description))
                                {
                                    return commands;
                                };
                                outputFiles.Add(exposureLayersFiles[exposureLayerNr]);
                                if (clean)
                                {
                                    tempFilesDelete.Add(exposureLayersFiles[exposureLayerNr]);
                                };
                            }
                        }
                        else
                        {
                            if (stepType.CmpNoCase(wxT("modify")) == 0)
                            {
                                // build a modify command
                                wxString inputFiles = detail::GetSettingString(settings, wxT("File"));
                                if (inputFiles.IsEmpty())
                                {
                                    std::cerr << "ERROR: Step " << i << " has no input/output file specified." << std::endl;
                                    CleanQueue(commands);
                                    return commands;
                                };
                                if (args.Find(wxT("%file%")) == wxNOT_FOUND)
                                {
                                    std::cerr << "ERROR: Step " << i << " has missing %file% placeholder in arguments." << std::endl;
                                    CleanQueue(commands);
                                    return commands;
                                };
                                args.Replace(wxT("%project%"), wxEscapeFilename(project), true);
                                const wxString prog = detail::GetSettingString(settings, wxT("Program"));
                                if (prog.IsEmpty())
                                {
                                    std::cerr << "ERROR: Step " << i << " has no program name specified." << std::endl;
                                    CleanQueue(commands);
                                    return commands;
                                };
                                if (inputFiles.CmpNoCase(wxT("all")) == 0)
                                {
                                    for (size_t imgNr = 0; imgNr < remappedImages.size(); ++imgNr)
                                    {
                                        wxString finalArgs(args);
                                        finalArgs.Replace(wxT("%file%"), wxEscapeFilename(remappedImages[imgNr]), true);
                                        commands->push_back(new NormalCommand(prog, finalArgs, description));
                                    };
                                }
                                else
                                {
                                    if (inputFiles.CmpNoCase(wxT("stacks")) == 0)
                                    {
                                        if (stacks.empty())
                                        {
                                            std::cerr << "ERROR: Step " << i << " requests to modify stacks, but no stack was created before." << std::endl;
                                            CleanQueue(commands);
                                            return commands;
                                        };
                                        for (size_t stackNr = 0; stackNr < stacksFiles.size(); ++stackNr)
                                        {
                                            wxString finalArgs(args);
                                            finalArgs.Replace(wxT("%file%"), wxEscapeFilename(stacksFiles[stackNr]), true);
                                            commands->push_back(new NormalCommand(prog, finalArgs, description));
                                        };
                                    }
                                    else
                                    {
                                        if (inputFiles.CmpNoCase(wxT("layers")) == 0)
                                        {
                                            if (exposureLayers.empty())
                                            {
                                                std::cerr << "ERROR: Step " << i << " requests to modify exposure layers, but no exposure layer was created before." << std::endl;
                                                CleanQueue(commands);
                                                return commands;
                                            };
                                            for (size_t layerNr = 0; layerNr < exposureLayersFiles.size(); ++layerNr)
                                            {
                                                wxString finalArgs(args);
                                                finalArgs.Replace(wxT("%file%"), wxEscapeFilename(exposureLayersFiles[layerNr]), true);
                                                commands->push_back(new NormalCommand(prog, finalArgs, description));
                                            };
                                        }
                                        else
                                        {
                                            inputFiles.Replace(wxT("%prefix%"), prefix, true);
                                            args.Replace(wxT("%file%"), wxEscapeFilename(inputFiles), true);
                                            commands->push_back(new NormalCommand(prog , args, description));
                                        };
                                    };
                                };
                            }
                            else
                            {
                                if (stepType.CmpNoCase(wxT("exiftool")) == 0)
                                {
                                    wxString resultFile = detail::GetSettingString(settings, wxT("Result"));
                                    if (resultFile.IsEmpty())
                                    {
                                        std::cerr << "ERROR: Step " << i << " has no result file specified." << std::endl;
                                        CleanQueue(commands);
                                        return commands;
                                    };
                                    resultFile.Replace(wxT("%prefix%"), prefix, true);
                                    if (args.Replace(wxT("%result%"), wxEscapeFilename(resultFile), true) == 0)
                                    {
                                        std::cerr << "ERROR: Step " << i << " has missing %result% placeholder in arguments." << std::endl;
                                        CleanQueue(commands);
                                        return commands;
                                    };
                                    args.Replace(wxT("%image0%"), wxEscapeFilename(wxString(pano.getImage(0).getFilename().c_str(), HUGIN_CONV_FILENAME)), true);
                                    commands->push_back(new OptionalCommand(GetExternalProgram(wxConfigBase::Get(), ExePath, wxT("exiftool")),
                                        args, description));
                                }
                                else
                                {
                                    std::cerr << "ERROR: Step " << i << " has unknown Type \"" << stepType.mb_str(wxConvLocal) << "\"." << std::endl;
                                    CleanQueue(commands);
                                    return commands;
                                };
                            };
                        };
                    };
                };
            };
        };
        return commands;
    }

    /** return a wxString with all files in files quoted */
    wxString GetQuotedFilenamesString(const wxArrayString& files)
    {
        wxString s;
        for (size_t i = 0; i < files.size(); ++i)
        {
            s.Append(wxEscapeFilename(files[i]) + wxT(" "));
        };
        return s;
    };

}; // namespace
