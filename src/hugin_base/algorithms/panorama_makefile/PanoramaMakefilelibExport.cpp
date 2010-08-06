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

#include <boost/smart_ptr/scoped_ptr.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

namespace HuginBase
{
using namespace makefile;
namespace mf = makefile;
bool PanoramaMakefilelibExport::create()
{
	Manager mgr;

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
	mf::Variable* vexiftool = mgr.own(new mf::Variable(cstr("EXIFTOOL"), progs.exiftool.find(".app") != std::string::npos ?
			cstr("perl -w ") + progs.exiftool :
			progs.exiftool));
#else
	mf::Variable* vexiftool = mgr.own(new mf::Variable(cstr("EXIFTOOL"), progs.exiftool));
#endif
	vexiftool->getDef().add();

	//----------
	// Project parameters
	//----------
	//----------
	//----------
	//----------


	//----------
	// prepare output stream to get number output right.
	// We use the "C" locale for NUMERIC and the system's for everything else.
	// Note: C++ locales are different from C. Calling setlocale doesn't influence C++ streams!
	std::locale makefilelocale(std::locale(""), "C", std::locale::numeric);
	makefile.imbue(makefilelocale);

	Makefile::getSingleton().writeMakefile(makefile);
	Makefile::getSingleton().clean();
	return true;
}

}
