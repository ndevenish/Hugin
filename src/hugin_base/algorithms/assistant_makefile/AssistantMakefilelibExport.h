/**
 * @file AssistantMakefilelibExport.h
 * @brief
 *  
 * @author Thomas Modes
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

#ifndef ASSISTANTMAKEFILELIBEXPORT_H_
#define ASSISTANTMAKEFILELIBEXPORT_H_

#include <hugin_shared.h>
#include <algorithms/PanoramaAlgorithm.h>
#include <panodata/PanoramaData.h>
#include <iosfwd>
#include <makefilelib/Manager.h>
#include <makefilelib/Makefile.h>
#include <makefilelib/Variable.h>
#include <makefilelib/Rule.h>


/**
 *
 */
namespace HuginBase
{

class IMPEX AssistantMakefilelibExport : public PanoramaAlgorithm
{
public:
    struct AssistantPrograms
    {
        std::string icpfind;
        std::string celeste;
        std::string cpclean;
        std::string autooptimiser;
        std::string pano_modify;
        std::string checkpto;
        std::string linefind;

        AssistantPrograms()
        {
            // default programs
            icpfind="icpfind";
            celeste="celeste_standalone";
            cpclean="cpclean";
            autooptimiser="autooptimiser";
            pano_modify="pano_modify";
            checkpto="checkpto";
            linefind="linefind";
        };
    };

private:
	PanoramaData & pano;
	std::ostream & makefile;
	const std::string& projectFile;
    const AssistantPrograms &progs;
    const bool &runLinefind;
    const bool &runCeleste;
    const double &celesteThreshold;
    const bool &celesteSmallRadius;
    const bool &runCPClean;
    const double &scale;

	makefile::Manager mgr;
	std::ostringstream valuestream;

	bool createItems();
    void echoInfo(makefile::Rule& inforule, const std::string& info);
	bool writeMakefile()
	{
		return makefile::Makefile::getSingleton().writeMakefile(makefile) != 0;
	}
public:
	AssistantMakefilelibExport(PanoramaData & pano_,
            const AssistantPrograms & progs_,
            const bool &runLinefind_,
            const bool &runCeleste_,
            const double &celesteThreshold_,
            const bool &celesteSmallRadius_,
            const bool &runCPClean_,
            const double &scale_,
            std::ostream & makefile_,
            const std::string& projectFile_)
	: PanoramaAlgorithm(pano_),
      pano(pano_), progs(progs_), runLinefind(runLinefind_),
      runCeleste(runCeleste_), celesteThreshold(celesteThreshold_), celesteSmallRadius(celesteSmallRadius_),
      runCPClean(runCPClean_), scale(scale_), makefile(makefile_), projectFile(projectFile_)
	{
        valuestream.imbue(makefile::GetMakefileLocale());
	}

	static void createMakefile(PanoramaData & pano_,
            const AssistantPrograms & progs_,
            const bool &runLinefind_,
            const bool &runCeleste_,
            const double &celesteThreshold_,
            const bool &celesteSmallRadius_,
            const bool &runCPClean_,
            const double &scale_,
            std::ostream & makefile_,
            const std::string& projectFile_)
	{
		AssistantMakefilelibExport* instance = new AssistantMakefilelibExport(
				pano_, progs_, runLinefind_, runCeleste_, celesteThreshold_, celesteSmallRadius_,
                runCPClean_, scale_, makefile_, projectFile_);
		instance->createItems();
		instance->writeMakefile();
		delete instance;
	}

	virtual bool modifiesPanoramaData() const
	{
		return false;
	}
	bool runAlgorithm()
	{
		return createItems() && writeMakefile();
	}

	virtual ~AssistantMakefilelibExport()
	{
		// TODO Auto-generated destructor stub
	}
};

};
#endif
