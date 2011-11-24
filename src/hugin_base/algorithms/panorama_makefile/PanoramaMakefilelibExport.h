/*
This file is part of hugin.

hugin is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

hugin is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with hugin.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file PanoramaMakefilelibExport.h
 * @brief
 *  Created on: Aug 5, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef PANORAMAMAKEFILELIBEXPORT_H_
#define PANORAMAMAKEFILELIBEXPORT_H_

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
class IMPEX PanoramaMakefilelibExport : public PanoramaAlgorithm
{
public:
    struct PTPrograms
    {
        std::string nona;
        std::string hdrmerge;
        std::string PTStitcher;
        std::string PTmender;
        std::string PTblender;
        std::string PTmasker;
        std::string PTroller;
        std::string enblend;
        std::string enblend_opts;
        std::string enfuse;
        std::string enfuse_opts;
        std::string smartblend;
        std::string smartblend_opts;
        std::string exiftool;
        std::string exiftool_opts;

        PTPrograms()
        {
                // default programs
                nona = "nona";
                PTStitcher = "PTStitcher";
                PTmender = "PTmender";
                PTblender = "PTblender";
                PTmasker = "PTmasker";
                PTroller = "PTroller";
                enblend = "enblend";
                enfuse = "enfuse";
                smartblend = "smartblend.exe";
                hdrmerge = "hugin_hdrmerge";
                exiftool = "exiftool";
        }
    };
private:
	PanoramaData & pano;
	UIntSet images;
	const std::string & ptofile;
	const std::string & outputPrefix;
	const PTPrograms & progs;
	const std::string & includePath;
	std::vector<std::string> & outputFiles;
	std::ostream & makefile;
	const std::string& tmpDir;
    const bool copyMetadata;
    const int nrThreads;

	makefile::Manager mgr;
	std::ostringstream valuestream;

	bool createItems();

	void createstacks(const std::vector<UIntSet> stackdata,
			const std::string stkname,
			const std::string filenamecenter, const std::string inputfilenamecenter, const std::string filenameext,
			std::vector<makefile::Variable*>& stacks,
			std::vector<makefile::Variable*>& stacks_shell,
			std::vector<makefile::Variable*>& stacks_input,
			std::vector<makefile::Variable*>& stacks_input_shell,
			makefile::Variable*& vstacks,
			makefile::Variable*& vstacksshell,
			std::vector<std::string>& allfiles);
	void createexposure(const std::vector<UIntSet> stackdata,
			const std::string stkname,
			const std::string filenamecenter, const std::string inputfilenamecenter, const std::string filenameext,
			std::vector<makefile::Variable*>& stacks,
			std::vector<makefile::Variable*>& stacks_shell,
			std::vector<makefile::Variable*>& stacks_input,
			std::vector<makefile::Variable*>& stacks_input_shell,
			std::vector<makefile::Variable*>& stacks_input_pt,
			std::vector<makefile::Variable*>& stacks_input_pt_shell,
			makefile::Variable*& vstacks,
			makefile::Variable*& vstacksshell,
			makefile::Variable*& vstacksrem,
			makefile::Variable*& vstacksremshell,
			std::vector<std::string>& inputs);

	void createcheckProgCmd(makefile::Rule& testrule, const std::string& progName, const std::string& progCommand);
    void echoInfo(makefile::Rule& inforule, const std::string& info);
    void printSystemInfo(makefile::Rule& inforule);
	bool writeMakefile()
	{
		return makefile::Makefile::getSingleton().writeMakefile(makefile) != 0;
	}
public:
    PanoramaMakefilelibExport(PanoramaData & pano_,
            const UIntSet & images_,
            const std::string & ptofile_,
            const std::string & outputPrefix_,
            const PTPrograms & progs_,
            const std::string & includePath_,
            std::vector<std::string> & outputFiles_,
            std::ostream & makefile_,
            const std::string& tmpDir_,
            const bool copyMetadata_,
            const int nrThreads_);

	static void createMakefile(PanoramaData & pano_,
            const UIntSet & images_,
            const std::string & ptofile_,
            const std::string & outputPrefix_,
            const PTPrograms & progs_,
            const std::string & includePath_,
            std::vector<std::string> & outputFiles_,
            std::ostream & makefile_,
            const std::string& tmpDir_,
            const bool copyMetadata_,
            const int nrThreads_)
	{
		PanoramaMakefilelibExport* instance = new PanoramaMakefilelibExport(
				pano_, images_, ptofile_, outputPrefix_, progs_, includePath_,
                outputFiles_, makefile_, tmpDir_, copyMetadata_, nrThreads_);
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

	virtual ~PanoramaMakefilelibExport()
	{
		// TODO Auto-generated destructor stub
	}


};
/**
 * Simple helper to output stacks for debugging.
 * @param stackdata
 */
void IMPEX printstacks(const std::vector<UIntSet>& stackdata);
/**
 * Simply calls push_back, convinience function.
 * @param vec
 * @param element
 */
template<typename T>
void append(std::vector<T>& vec, const T& element);
/**
 * Copys all of src to the end of test. Just calls insert.
 * Convinience function.
 * @param dst
 * @param src
 */
template<typename T>
void append(std::vector<T>& dst, const std::vector<T>& src);
}

#endif /* PANORAMAMAKEFILELIBEXPORT_H_ */
