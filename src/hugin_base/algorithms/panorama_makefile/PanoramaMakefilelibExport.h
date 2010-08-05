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

	bool create();

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
		// TODO Auto-generated constructor stub

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
		instance->create();
		delete instance;
	}

	virtual bool modifiesPanoramaData() const
	{
		return false;
	}
	bool runAlgorithm()
	{
		return create();
	}

	virtual ~PanoramaMakefilelibExport()
	{
		// TODO Auto-generated destructor stub
	}


};

}

#endif /* PANORAMAMAKEFILELIBEXPORT_H_ */
