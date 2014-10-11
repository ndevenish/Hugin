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
*  License along with this software; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include "hugin_version.h"

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
                filenames.Add(wxString::Format(wxT("%s%04d%s"), prefix.c_str(), *it, postfix.c_str()));
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

        /** generate the final argfile 
            @return full name of generated argfile 
        */
        wxString GenerateFinalArgfile(const HuginBase::Panorama & pano, const wxConfigBase* config, const HuginBase::UIntSet& images)
        {
            wxString argfileInput = config->Read(wxT("/output/FinalArgfile"), wxEmptyString);
            const bool generateGPanoTags = config->Read(wxT("/output/writeGPano"), HUGIN_EXIFTOOL_CREATE_GPANO) == 1l;
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
            placeholders.insert(std::make_pair(wxT("%nrImages"), wxString::Format(wxT("%d"), images.size())));
            placeholders.insert(std::make_pair(wxT("%nrAllImages"), wxString::Format(wxT("%d"), pano.getNrOfImages())));
            placeholders.insert(std::make_pair(wxT("%fullwidth"), wxString::Format(wxT("%d"), opts.getWidth())));
            placeholders.insert(std::make_pair(wxT("%fullheight"), wxString::Format(wxT("%d"), opts.getHeight())));
            placeholders.insert(std::make_pair(wxT("%width"), wxString::Format(wxT("%d"), opts.getROI().width())));
            placeholders.insert(std::make_pair(wxT("%height"), wxString::Format(wxT("%d"), opts.getROI().height())));
            // now open the final argfile
            wxString tempDir = config->Read(wxT("tempDir"), wxT(""));
            if (!tempDir.IsEmpty())
            {
                if (tempDir.Last() != wxFileName::GetPathSeparator())
                {
                    tempDir.Append(wxFileName::GetPathSeparator());
                }
            };
            wxFileName tempArgfileFinal(wxFileName::CreateTempFileName(tempDir + wxT("he")));
            wxFFileOutputStream outputStream(tempArgfileFinal.GetFullPath());
            wxTextOutputStream outputFile(outputStream);
            // write argfile
            outputFile << wxT("-Software=Hugin ") << wxString(DISPLAY_VERSION, wxConvLocal) << endl;
            outputFile << wxT("-E") << endl;
            outputFile << wxT("-UserComment<${UserComment}") << linebreak;
            if (readProjectionName)
            {
                outputFile << wxT("Projection: ") << placeholders[wxT("%projection")] << wxT(" (") << placeholders[wxT("%projectionNumber")] << wxT(")") << linebreak;
            };
            outputFile << wxT("FOV: ") << placeholders[wxT("%hfov")] << wxT(" x ") << placeholders[wxT("%vfov")] << linebreak;
            outputFile << wxT("Ev: ") << placeholders[wxT("%ev")] << endl;
            outputFile << wxT("-f") << endl;
#ifdef EXIFTOOL_GPANO_SUPPORT
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
#endif
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

        wxString PrintDetailInfo(const HuginBase::Panorama& pano, const HuginBase::PanoramaOptions& opts, const HuginBase::UIntSet& allActiveImages, const wxString& prefix, const wxString& bindir, wxConfigBase* config)
        {
            wxString output;
            const wxString wxEndl(wxT("\n"));
            output
                << wxT("============================================") << wxEndl
                << _("Stitching panorama...") << wxEndl
                << wxT("============================================") << wxEndl
                << wxEndl
                << _("Platform:") << wxT(" ") << wxGetOsDescription() << wxEndl
                << _("Version:") << wxT(" ") << wxString(DISPLAY_VERSION, wxConvLocal) << wxEndl
                << _("Working directory:") << wxT(" ") << wxFileName::GetCwd() << wxEndl
                << _("Output prefix:") << wxT(" ") << prefix << wxEndl
                << wxEndl;
            if (opts.outputLDRBlended || opts.outputLDRExposureBlended || opts.outputLDRExposureLayersFused || 
                opts.outputHDRBlended || opts.outputLDRExposureLayers)
            {
                if (opts.blendMode == HuginBase::PanoramaOptions::ENBLEND_BLEND)
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
                }
                else
                {
                    output << _("ExifTool:") << wxT(" ") << _("FAILED") << wxEndl;
                };
            };
            output
                << wxEndl
                << _("Number of active images:") << wxT(" ") << allActiveImages.size() << wxEndl
                << wxString::Format(_("Output exposure value: %.1f"), opts.outputExposureValue) << wxEndl
                << wxString::Format(_("Canvas size: %ldx%ld"), opts.getSize().width(), opts.getSize().height()) << wxEndl
                << wxString::Format(_("ROI: (%ld, %ld) - (%ld, %ld)"), opts.getROI().left(), opts.getROI().top(), opts.getROI().right(), opts.getROI().bottom()) << wxT(" ") << wxEndl
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
                output << _("Remapped Images :") << wxEndl;
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
                << wxString::Format(_("Size: %ldx%ld"), img.getWidth(), img.getHeight()) << wxEndl
                << _("Projection:") << wxT(" ") << getProjectionString(img) << wxEndl
                << _("Response type:") << wxT(" ") << getResponseString(img) << wxEndl
                << wxString::Format(_("HFOV: %.0f"), img.getHFOV()) << wxEndl
                << wxString::Format(_("Exposure value: %.1f"), img.getExposureValue()) << wxEndl
                << wxEndl;
            return output;
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
        if (opts.blendMode != HuginBase::PanoramaOptions::ENBLEND_BLEND)
        {
            std::cerr << "ERROR: Only enblend remappper is currently supported by hugin_executor." << std::endl;
            return commands;
        };
        if (opts.hdrMergeMode != HuginBase::PanoramaOptions::HDRMERGE_AVERAGE)
        {
            std::cerr << "ERROR: Only hdr merger HDRMERGE_AVERAGE is currently supported by hugin_executor." << std::endl;
            return commands;
        };
        statusText=detail::PrintDetailInfo(pano, opts, allActiveImages, prefix, ExePath, config);
        // prepare some often needed variables
        const wxString quotedProject(wxEscapeFilename(project));
        // prepare nona arguments
        wxString nonaArgs;
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
        wxString enblendArgsGeneral;
        wxString enblendArgsLDR;
        if (opts.blendMode == HuginBase::PanoramaOptions::ENBLEND_BLEND)
        {
            enblendArgsGeneral.Append(WXSTRING(opts.enblendOptions));
            if (opts.getHFOV() == 360.0)
            {
                enblendArgsGeneral.Append(wxT(" -w"));
            };
            vigra::Rect2D roi = opts.getROI();
            if (roi.top() != 0 || roi.left() != 0)
            {
                enblendArgsGeneral << wxT(" -f") << roi.width() << wxT("x") << roi.height() << wxT("+") << roi.left() << wxT("+") << roi.top();
            }
            else
            {
                enblendArgsGeneral << wxT(" -f") << roi.width() << wxT("x") << roi.height();
            };
            enblendArgsGeneral.Append(wxT(" "));

            if (opts.outputImageType == "tif" && !opts.outputImageTypeCompression.empty())
            {
                enblendArgsLDR << wxT(" --compression=") << WXSTRING(opts.outputImageTypeCompression);
            }
            else
            {
                if (opts.outputImageType == "jpg")
                {
                    enblendArgsLDR << wxT(" --compression=") << opts.quality;
                };
            };
            enblendArgsLDR.Append(wxT(" "));
        };
        // prepare enfuse arguments
        wxString enfuseArgsGeneral(WXSTRING(opts.enfuseOptions) + wxT(" "));
        if (opts.getHFOV() == 360.0)
        {
            enfuseArgsGeneral.Append(wxT("-w "));
        };

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
            wxString finalArgfile = detail::GenerateFinalArgfile(pano, config, allActiveImages);
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
            commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("nona")),
                nonaArgs + wxT("-r ldr -m TIFF_m -o ") + wxEscapeFilename(prefix) + wxT(" ") + quotedProject,
                _("Remapping LDR images...")));
            const wxArrayString remappedImages(detail::GetNumberedFilename(prefix, wxT(".tif"), allActiveImages));
            detail::AddToArray(remappedImages, outputFiles);
            if (opts.outputLDRBlended)
            {
                if (opts.blendMode == HuginBase::PanoramaOptions::ENBLEND_BLEND)
                {
                    wxString finalEnblendArgs(enblendArgsGeneral + enblendArgsLDR);
                    const wxString finalFilename(prefix + wxT(".") + WXSTRING(opts.outputImageType));
                    finalEnblendArgs.Append(wxT(" -o ") + wxEscapeFilename(finalFilename));
                    finalEnblendArgs.Append(wxT(" -- ") + detail::GetQuotedFilenamesString(remappedImages));
                    commands->push_back(new NormalCommand(GetExternalProgram(config, ExePath, wxT("enblend")),
                        finalEnblendArgs, _("Blending images...")));
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
        // exposure fusion output
        if (opts.outputLDRExposureRemapped || opts.outputLDRStacks || opts.outputLDRExposureLayers ||
            opts.outputLDRExposureBlended || opts.outputLDRExposureLayersFused)
        {
            commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("nona")),
                nonaArgs + wxT("-r ldr -m TIFF_m --ignore-exposure -o ") + wxEscapeFilename(prefix + wxT("_exposure_layers_")) + wxT(" ") + quotedProject,
                _("Remapping LDR images without exposure correction...")));
            const wxArrayString remappedImages = detail::GetNumberedFilename(prefix + wxT("_exposure_layers_"), wxT(".tif"), allActiveImages);
            detail::AddToArray(remappedImages, outputFiles);
            if (opts.outputLDRStacks || opts.outputLDRExposureBlended)
            {
                // fusing stacks, then blending
                stacks = getHDRStacks(pano, allActiveImages, opts);
                wxArrayString stackedImages;
                // fuse all stacks
                for (size_t stackNr = 0; stackNr < stacks.size(); ++stackNr)
                {
                    const wxArrayString stackImgs = detail::GetNumberedFilename(prefix + wxT("_exposure_layers_"), wxT(".tif"), stacks[stackNr]);
                    const wxString stackImgName = wxString::Format(wxT("%s_stack_ldr_%04d%s"), prefix.c_str(), stackNr, wxT(".tif"));
                    outputFiles.Add(stackImgName);
                    stackedImages.Add(stackImgName);
                    commands->push_back(new NormalCommand(GetExternalProgram(config, ExePath, wxT("enfuse")),
                        enfuseArgsGeneral + enLayersCompressionArgs + wxT(" -o ") + wxEscapeFilename(stackImgName) + wxT(" -- ") + detail::GetQuotedFilenamesString(stackImgs),
                        wxString::Format(_("Fusing stack number %d"), stackNr)));
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
                    wxString finalEnblendArgs(enblendArgsGeneral + enblendArgsLDR);
                    const wxString fusedStacksFilename(prefix + wxT("_fused.") + WXSTRING(opts.outputImageType));
                    finalEnblendArgs.Append(wxT(" -o ") + wxEscapeFilename(fusedStacksFilename));
                    finalEnblendArgs.Append(wxT(" -- ") + detail::GetQuotedFilenamesString(stackedImages));
                    commands->push_back(new NormalCommand(GetExternalProgram(config, ExePath, wxT("enblend")),
                        finalEnblendArgs, _("Blending all stacks...")));
                    outputFiles.Add(fusedStacksFilename);
                    if (copyMetadata)
                    {
                        filesForFullExiftool.Add(fusedStacksFilename);
                    };
                };
            };
            if (opts.outputLDRExposureLayers || opts.outputLDRExposureLayersFused)
            {
                // blending exposure layers, then fusing
                std::vector<HuginBase::UIntSet> exposureLayers = getExposureLayers(pano, allActiveImages, opts);
                wxArrayString exposureLayersImages;
                // fuse all exposure layers
                for (size_t exposureLayer = 0; exposureLayer < exposureLayers.size(); ++exposureLayer)
                {
                    const wxArrayString exposureLayersImgs = detail::GetNumberedFilename(prefix + wxT("_exposure_layers_"), wxT(".tif"), exposureLayers[exposureLayer]);
                    const wxString exposureLayerImgName = wxString::Format(wxT("%s_exposure_%04d%s"), prefix.c_str(), exposureLayer, wxT(".tif"));
                    exposureLayersImages.Add(exposureLayerImgName);
                    outputFiles.Add(exposureLayerImgName);
                    commands->push_back(new NormalCommand(GetExternalProgram(config, ExePath, wxT("enblend")),
                        enblendArgsGeneral + enLayersCompressionArgs + wxT(" -o ") + wxEscapeFilename(exposureLayerImgName) + wxT(" -- ") + detail::GetQuotedFilenamesString(exposureLayersImgs),
                        wxString::Format(_("Blending exposure layer  %d"), exposureLayer))); 
                    if (copyMetadata && opts.outputLDRExposureLayers)
                    {
                        filesForCopyTagsExiftool.Add(exposureLayerImgName);
                    };
                    if (!opts.outputLDRExposureLayers)
                    {
                        tempFilesDelete.Add(exposureLayerImgName);
                    };
                };
                if (opts.outputLDRExposureLayersFused)
                {
                    wxString finalEnfuseArgs(enfuseArgsGeneral + enblendArgsLDR);
                    const wxString fusedExposureLayersFilename(prefix + wxT("_blended_fused.") + WXSTRING(opts.outputImageType));
                    finalEnfuseArgs.Append(wxT(" -o ") + wxEscapeFilename(fusedExposureLayersFilename));
                    finalEnfuseArgs.Append(wxT(" -- ") + detail::GetQuotedFilenamesString(exposureLayersImages));
                    commands->push_back(new NormalCommand(GetExternalProgram(config, ExePath, wxT("enfuse")),
                        finalEnfuseArgs, _("Fusing all exposure layers...")));
                    outputFiles.Add(fusedExposureLayersFilename);
                    if (copyMetadata)
                    {
                        filesForFullExiftool.Add(fusedExposureLayersFilename);
                    };
                };
            };
            if (!opts.outputLDRExposureRemapped)
            {
                detail::AddToArray(remappedImages, tempFilesDelete);
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
                for (size_t stackNr = 0; stackNr < stacks.size(); ++stackNr)
                {
                    const wxArrayString stackImgs = detail::GetNumberedFilename(prefix + wxT("_hdr_"), wxT(".exr"), stacks[stackNr]);
                    const wxString stackImgName = wxString::Format(wxT("%s_stack_hdr_%04d%s"), prefix.c_str(), stackNr, wxT(".exr"));
                    stackedImages.Add(stackImgName);
                    outputFiles.Add(stackImgName);
                    commands->push_back(new NormalCommand(GetInternalProgram(ExePath, wxT("hugin_hdrmerge")),
                        WXSTRING(opts.hdrmergeOptions) + wxT(" -o ") + wxEscapeFilename(stackImgName) + wxT(" -- ") + detail::GetQuotedFilenamesString(stackImgs),
                        wxString::Format(_("Merging hdr stack number %d"), stackNr)));
                    if (!opts.outputHDRStacks)
                    {
                        tempFilesDelete.Add(stackImgName);
                    };
                };
                if (opts.outputHDRBlended)
                {
                    wxString finalEnblendArgs(enblendArgsGeneral);
                    const wxString mergedStacksFilename(wxEscapeFilename(prefix + wxT("_hdr.") + WXSTRING(opts.outputImageTypeHDR)));
                    finalEnblendArgs.Append(wxT(" -o ") + mergedStacksFilename);
                    finalEnblendArgs.Append(wxT(" -- ") + detail::GetQuotedFilenamesString(stackedImages));
                    commands->push_back(new NormalCommand(GetExternalProgram(config, ExePath, wxT("enblend")),
                        finalEnblendArgs, _("Blending hdr stacks...")));
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
            commands->push_back(new OptionalCommand(GetExternalProgram(config, ExePath, wxT("exiftool")),
                exiftoolArgs + detail::GetQuotedFilenamesString(filesForCopyTagsExiftool),
                _("Updating metadata...")));
        };
        if (!filesForFullExiftool.IsEmpty())
        {
            commands->push_back(new OptionalCommand(GetExternalProgram(config, ExePath, wxT("exiftool")),
                exiftoolArgs + exiftoolArgsFinal + detail::GetQuotedFilenamesString(filesForFullExiftool),
                _("Updating metadata...")));
        };
        return commands;
    };

}; // namespace
