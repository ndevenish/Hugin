/**
 * @file PanoramaMakefilelibExport.h
 * @brief
 *  Created on: Aug 5, 2010
 * @author Florian Achleitner <florian.achleitner.2.6.31@gmail.com>
 */

#ifndef PANORAMAMAKEFILELIBEXPORT_H_
#define PANORAMAMAKEFILELIBEXPORT_H_

#include <hugin_shared.h>
#include <algorithm/PanoramaAlgorithm.h>
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

class PanoramaMakefilelibExport : public PanoramaAlgorithm
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
	const UIntSet & images;
	const std::string & ptofile;
	const std::string & outputPrefix;
	const PTPrograms & progs;
	const std::string & includePath;
	std::vector<std::string> & outputFiles;
	std::ostream & makefile;
	const std::string& tmpDir;

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
			makefile::Variable*& vstacksshell);
	void createexposure(const std::vector<UIntSet> stackdata,
			const std::string stkname,
			const std::string filenamecenter, const std::string inputfilenamecenter, const std::string filenameext,
			std::vector<makefile::Variable*>& stacks,
			std::vector<makefile::Variable*>& stacks_shell,
			std::vector<makefile::Variable*>& stacks_input,
			std::vector<makefile::Variable*>& stacks_input_shell,
			makefile::Variable*& vstacks,
			makefile::Variable*& vstacksshell,
			makefile::Variable*& vstacksrem,
			makefile::Variable*& vstacksremshell,
			std::vector<std::string>& inputs);

	void createcheckProgCmd(makefile::Rule& testrule, const std::string& progName, const std::string& progCommand);

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
            const std::string& tmpDir_)
	: PanoramaAlgorithm(pano),
	  pano(pano_), images(images_), ptofile(ptofile_), outputPrefix(outputPrefix_),
	  progs(progs_), includePath(includePath_), outputFiles(outputFiles_),
	  makefile(makefile_), tmpDir(tmpDir_)
	{
		valuestream.imbue(makefile::Makefile::locale);

	}

	static void createMakefile(PanoramaData & pano_,
            const UIntSet & images_,
            const std::string & ptofile_,
            const std::string & outputPrefix_,
            const PTPrograms & progs_,
            const std::string & includePath_,
            std::vector<std::string> & outputFiles_,
            std::ostream & makefile_,
            const std::string& tmpDir_)
	{
		PanoramaMakefilelibExport* instance = new PanoramaMakefilelibExport(
				pano_, images_, ptofile_, outputPrefix_, progs_, includePath_,
				outputFiles_, makefile_, tmpDir_);
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

}

#endif /* PANORAMAMAKEFILELIBEXPORT_H_ */
