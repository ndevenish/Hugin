/**
 * @file PanoramaMakefilelibExport.cpp
 * @brief
 *  Created on: Aug 5, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#include "PanoramaMakefilelibExport.h"

#include <makefilelib/char_type.h>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <locale>
#include <makefilelib/Comment.h>
#include <makefilelib/Variable.h>
#include <makefilelib/VariableDef.h>
#include <makefilelib/VariableRef.h>
#include <makefilelib/MakefileItem.h>
#include <makefilelib/Makefile.h>
#include <makefilelib/AutoVariable.h>
#include <makefilelib/Newline.h>
#include <makefilelib/Rule.h>
#include <makefilelib/Conditional.h>
#include <makefilelib/Manager.h>

#include <panodata/PanoramaData.h>
#include <hugin_utils/utils.h>

#include <boost/smart_ptr/scoped_ptr.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

namespace HuginBase
{
using namespace makefile;
namespace mf = makefile;


bool PanoramaMakefilelibExport::create()
{
	mgr.own_add((new Comment(
		cstr("makefile for panorama stitching, created by hugin using the new makefilelib"))));

	//----------
	// set temporary dir if defined
	if(!tmpDir.empty())
	{
#ifdef UNIX_LIKE
		mgr.own_add(new Comment(cstr("set temporary directory for UNIX_LIKE")));
		mf::Variable* vtmpdir = mgr.own(new mf::Variable(cstr("TMPDIR"), tmpDir));
		vtmpdir->setExport(true); vtmpdir->getDef().add();
#else
		mgr.own_add(new Comment(cstr("set temporary directory for not UNIX_LIKE")));
		mf::Variable* vtmpdir = mgr.own(new mf::Variable(cstr("TEMP"), tmpDir));
		vtmpdir->setExport(true); vtmpdir->getDef().add();
		mf::Variable* vtmpdir2 = mgr.own(new mf::Variable(cstr("TMP"), tmpDir));
		vtmpdir2->setExport(true); vtmpdir2->getDef().add();
#endif
	}

#ifdef _WINDOWS
	mgr.own_add(new Comment(cstr("Force using cmd.exe")); cwinshell.add());
	mf::Variable* winshell = mgr_own(new Variable(cstr("SHELL"), getenv("ComSpec")));
	winshell->getDef().add();
#endif

	//----------
	// set the tool commands
	mgr.own_add(new Comment(cstr("Tool configuration")));
	mf::Variable* vnona = mgr.own(new mf::Variable(cstr("NONA"), progs.nona)); vnona->getDef().add();
	mf::Variable* vPTStitcher = mgr.own(new mf::Variable(cstr("PTSTITCHER"), progs.PTStitcher)); vPTStitcher->getDef().add();
	mf::Variable* vPTmender = mgr.own(new mf::Variable(cstr("PTMENDER"), progs.PTmender)); vPTmender->getDef().add();
	mf::Variable* vPTblender = mgr.own(new mf::Variable(cstr("PTBLENDER"), progs.PTblender)); vPTblender->getDef().add();
	mf::Variable* vPTmasker = mgr.own(new mf::Variable(cstr("PTMASKER"), progs.PTmasker)); vPTmasker->getDef().add();
	mf::Variable* vPTroller = mgr.own(new mf::Variable(cstr("PTROLLER"), progs.PTroller)); vPTroller->getDef().add();
	mf::Variable* venblend = mgr.own(new mf::Variable(cstr("ENBLEND"), progs.enblend)); venblend->getDef().add();
	mf::Variable* venfuse = mgr.own(new mf::Variable(cstr("ENFUSE"), progs.enfuse)); venfuse->getDef().add();
	mf::Variable* vsmartblend = mgr.own(new mf::Variable(cstr("SMARTBLEND"), progs.smartblend)); vsmartblend->getDef().add();
	mf::Variable* vhdrmerge = mgr.own(new mf::Variable(cstr("HDRMERGE"), progs.hdrmerge)); vhdrmerge->getDef().add();
#ifdef _WINDOWS
	mf::Variable* vrm = mgr.own(new mf::Variable(cstr("RM"), cstr("del")));
#else
	mf::Variable* vrm = mgr.own(new mf::Variable(cstr("RM"), cstr("rm")));
#endif
	vrm->getDef().add();

	//----------
	// if this is defined and we have .app in the exiftool command, we execute it with perl by prepending perl -w
	// to the command name.
#ifdef MAC_SELF_CONTAINED_BUNDLE
	mf::Variable* vexiftool = mgr.own(new mf::Variable(cstr("EXIFTOOL"),
			progs.exiftool.find(".app") != std::string::npos ?
			cstr("perl -w ") + progs.exiftool :
			progs.exiftool));
#else
	mf::Variable* vexiftool = mgr.own(new mf::Variable(cstr("EXIFTOOL"), progs.exiftool));
#endif
	vexiftool->getDef().add();

	//----------
	// Project parameters
	mgr.own_add(new Comment(cstr("Project parameters")));
	PanoramaOptions opts = pano.getOptions();
	mf::Variable* vhugin_projection = mgr.own(new mf::Variable(cstr("HUGIN_PROJECTION"),
			opts.getProjection()));
	vhugin_projection->getDef().add();

	mf::Variable* vhugin_hfov = mgr.own(new mf::Variable(cstr("HUGIN_HFOV"), opts.getHFOV()));
	vhugin_hfov->getDef().add();
	mf::Variable* vhugin_width = mgr.own(new mf::Variable(cstr("HUGIN_WIDTH"), opts.getWidth()));
	vhugin_width->getDef().add();
	mf::Variable* vhugin_height = mgr.own(new mf::Variable(cstr("HUGIN_HEIGHT"), opts.getHeight()));
	vhugin_width->getDef().add();

	//----------
    // options for the programs
    mgr.own_add(new Comment(cstr("options for the programs")));
    // set remapper specific settings
    mf::Variable* vnonaldr = NULL;
    mf::Variable* vnonaopts = NULL;
	if(opts.remapper == PanoramaOptions::NONA)
	{
		string val;
		if (opts.outputImageType == "tif" && opts.outputLayersCompression.size() != 0)
			val = "-z " + opts.outputLayersCompression;
		else if (opts.outputImageType == "jpg")
			val = "-z PACKBITS ";

		vnonaldr = mgr.own(new mf::Variable(cstr("NONA_LDR_REMAPPED_COMP"), val, Makefile::NONE));
		vnonaldr->getDef().add();

		vnonaopts = mgr.own(new mf::Variable(cstr("NONA_OPTS"),
				opts.remapUsingGPU ? "-g " : "", Makefile::NONE));
		vnonaopts->getDef().add();
	}

    // set blender specific settings
	mf::Variable* venblendopts = NULL;
	mf::Variable* venblendldrcomp = NULL;
	mf::Variable* venblendhdrcomp = NULL;

	if(opts.blendMode == PanoramaOptions::ENBLEND_BLEND)
	{
		{
			valuestream.str(opts.enblendOptions);
			if (opts.getHFOV() == 360.0)
				// blend over the border
				valuestream << " -w";

			vigra::Rect2D roi = opts.getROI();
			if (roi.top() != 0 || roi.left() != 0 )
				valuestream << " -f" << roi.width() << "x" << roi.height() << "+" << roi.left() << "+" << roi.top();
			else
				valuestream << " -f" << roi.width() << "x" << roi.height();
			venblendopts = mgr.own(new mf::Variable(cstr("ENBLEND_OPTS"), valuestream.str(), Makefile::NONE));
			venblendopts->getDef().add();
		}

		{
			valuestream.str("");
			if (opts.outputImageType == "tif" && opts.outputImageTypeCompression.size() != 0)
				valuestream << "--compression " << opts.outputImageTypeCompression;
			else if (opts.outputImageType == "jpg")
				valuestream << "--compression " << opts.quality;

			venblendldrcomp = mgr.own(new mf::Variable(cstr("ENBLEND_LDR_COMP"), valuestream.str(), Makefile::NONE));
			venblendldrcomp->getDef().add();
		}

		{
			string val;
			if (opts.outputImageTypeHDR == "tif" && opts.outputImageTypeHDRCompression.size() != 0) {
				val += "--compression " + opts.outputImageTypeHDRCompression;
			}
			venblendhdrcomp = mgr.own(new mf::Variable(cstr("ENBLEND_HDR_COMP"), val, Makefile::NONE));
			venblendhdrcomp->getDef().add();
		}
	}

	mf::Variable* vptblenderopts = NULL;
	if(opts.blendMode == PanoramaOptions::PTBLENDER_BLEND)
	{
		valuestream.str("");
		switch (opts.colorCorrection)
		{
			case PanoramaOptions::NONE:
				break;
			case PanoramaOptions::BRIGHTNESS_COLOR:
				valuestream << " -k " << opts.colorReferenceImage;
				break;
			case PanoramaOptions::BRIGHTNESS:
				valuestream << " -k " << opts.colorReferenceImage;
				break;
			case PanoramaOptions::COLOR:
				valuestream << " -k " << opts.colorReferenceImage;
				break;
		}
		vptblenderopts = mgr.own(new mf::Variable(cstr("PTBLENDER_OPTS"), valuestream.str(), Makefile::NONE));
		vptblenderopts->getDef().add();
	}

	mf::Variable* vsmartblendopts = NULL;
	if(opts.blendMode == PanoramaOptions::SMARTBLEND_BLEND)
	{
		vsmartblendopts = mgr.own(new mf::Variable(cstr("SMARTBLEND_OPTS"),
			opts.getHFOV() == 360.0 ? cstr(" -w") : cstr("")));
		// TODO: build smartblend command line from given images. (requires additional program)
	}
	//----------
	//----------
	//----------


	//----------

	makefile.imbue(Makefile::locale);

	Makefile::getSingleton().writeMakefile(makefile);
	Makefile::getSingleton().clean();
	return true;
}


}
