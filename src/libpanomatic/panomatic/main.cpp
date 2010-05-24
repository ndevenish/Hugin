/*
* Copyright (C) 2007-2008 Anael Orlinski
*
* This file is part of Panomatic.
*
* Panomatic is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* 
* Panomatic is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with Panomatic; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <iostream>
#include <vector>
#include <string>
#include <boost/foreach.hpp>
#include "tclap/CmdLine.h"
#include "Utils.h"

using namespace std;
using namespace TCLAP;


#include "PanoDetector.h"

const char* kVersion="0.9.4";


class MyOutput : public StdOutput
{
public:

	virtual void failure(CmdLineInterface& c, ArgException& e)
	{
		std::cerr << "Parse error: " << e.argId() << std::endl << "             " << e.error() << std::endl << std::endl << endl;
		usage(c);
	}

	virtual void usage(CmdLineInterface& c)
	{
		int iML = 30;
		cout << "Basic usage : " << endl;
		cout << "  "<< c.getProgramName() << " -o output IMG1 ... IMGn" << endl;
		
		cout << endl <<"All options : " << endl;
		list<Arg*> args = c.getArgList();
		for (ArgListIterator it = args.begin(); it != args.end(); it++)
		{
			string aL = (*it)->longID();
			string aD = (*it)->getDescription();
			// replace tabs by n spaces.
			size_t p = aD.find_first_of("\t");
			while(p != string::npos)
			{
				string aD1 = aD.substr(0, p);
				string aD2 = aD.substr(p+1, aD.size() - p + 1);

				aD = aD1 + "\n" + string(iML, ' ') + aD2;
				p = aD.find_first_of("\t");
			}
			

			if (aL.size() > iML)
				cout << aL << endl << string(iML, ' ')  << aD << endl;
			else
				cout << aL  << string(iML - aL.size(), ' ')   << aD << endl;
		}
	}

	virtual void version(CmdLineInterface& c)
	{
		cout << "my version message: 0.1" << endl;
	}
};



void parseOptions(int argc, char** argv, PanoDetector& ioPanoDetector)
{
	try {

		CmdLine cmd("Pan-o-matic", ' ', kVersion );

		MyOutput my;
		cmd.setOutput(&my);

/*REMOVING SURF #ifdef SURF_ENABLED		
		SwitchArg aArgGrad ( "","grad", "Use non-patented descriptor   (default:false)", false );
		SwitchArg aArgSurfExtended("","surf128", "Uses extended SURF (128 descriptors)    (default:false)", false);
#endif */
		SwitchArg aArgLoadKeypoints("","loadkeys", "Load keypoints from file instead of detecting them. (default:false)\n", false);
		SwitchArg aArgFullScale("","fullscale", "Uses full scale image to detect keypoints    (default:false)\n", false);
//REMOVING SURF 		ValueArg<int> aArgSurfScoreThreshold("","surfscore", "Detection score threshold    (default : 1000)\n", false, 1000, "int");
		ValueArg<int> aArgSieve1Width("","sieve1width", "Sieve 1 : Number of buckets on width    (default : 10)", false, 10, "int");
		ValueArg<int> aArgSieve1Height("","sieve1height",  "Sieve 1 : Number of buckets on height    (default : 10)", false, 10, "int");
		ValueArg<int> aArgSieve1Size("","sieve1size",	"Sieve 1 : Max points per bucket    (default : 10)\n", false, 10, "int");
		SwitchArg aArgLinearMatch("","linearmatch", "Enable linear images matching (default : all pairs)", false);
		ValueArg<int> aArgLinearMatchLen("","linearmatchlen", "Number of images to match in linear matching (default:1)\n", false, 1 ,"int");
		
		ValueArg<int> aArgKDTreeSearchSteps("","kdtreesteps",   "KDTree : search steps    (default : 40)", false, 40, "int");
		ValueArg<double> aArgKDTreeSecondDist("","kdtreeseconddist", "KDTree : distance of 2nd match    (default : 0.15)\n", false, 0.15, "double");
		ValueArg<int> aArgMinMatches("","minmatches", "Minimum matches    (default : 4)", false, 4, "int");
		ValueArg<int> aArgRansacIter("","ransaciter", "Ransac : iterations    (default : 1000)", false, 1000, "int");
		ValueArg<int> aArgRansacDist("","ransacdist", "Ransac : homography estimation distance threshold (pixels)"
														"\t    (default : 25)", false, 25, "int");
		ValueArg<int> aArgSieve2Width("","sieve2width", "Sieve 2 : Number of buckets on width    (default : 5)", false, 5, "int");
		ValueArg<int> aArgSieve2Height("","sieve2height", "Sieve 2 : Number of buckets on height    (default : 5)", false, 5, "int");
		ValueArg<int> aArgSieve2Size("","sieve2size", "Sieve 2 : Max points per bucket    (default : 1)\n", false, 1 ,"int");
		
		SwitchArg aArgPTGuiCompat("","ptgui", "Activate Autopano compatibility for PTGui\n", false);
/*REMOVING SURF #ifdef SURF_ENABLED
		cmd.add ( aArgGrad ) ;
		cmd.add(aArgSurfExtended);
#endif */
		cmd.add(aArgPTGuiCompat);
		cmd.add(aArgSieve2Size);
		cmd.add(aArgSieve2Height);
		cmd.add(aArgSieve2Width);
		cmd.add(aArgRansacDist);
		cmd.add(aArgRansacIter);
		cmd.add(aArgMinMatches);
		cmd.add(aArgLinearMatchLen);
		cmd.add(aArgLinearMatch);
		cmd.add(aArgKDTreeSecondDist);
		cmd.add(aArgKDTreeSearchSteps);
		cmd.add(aArgSieve1Size);
		cmd.add(aArgSieve1Height);
		cmd.add(aArgSieve1Width);
//REMOVING SURF 		cmd.add(aArgSurfScoreThreshold);		
		cmd.add(aArgFullScale);
		cmd.add(aArgLoadKeypoints);
		

		//ValueArg<int> aArgNumKeys("k", "keys", "Number of keys per image pair", false, 10, "int");
		//cmd.add( aArgNumKeys );

		SwitchArg aArgTest("t","test", "Enables test mode\n", false);
		cmd.add( aArgTest );

		ValueArg<int> aArgCores("n","ncores", "Number of CPU/Cores    (default:autodetect)", false, utils::getCPUCount(), "int");
		cmd.add( aArgCores );

		ValueArg<string> aArgOutputFile("o","output","Output file",true, "default", "string");
		cmd.add( aArgOutputFile );


		UnlabeledMultiArg<string> aArgFiles("fileName", "Image files", true, "string");
		cmd.add( aArgFiles );


		cmd.parse(argc,argv);

		//
		// Set variables
		//
		
		ioPanoDetector.setOutputFile(aArgOutputFile.getValue());

		ioPanoDetector.setGradientDescriptor(true);

/*REMOVING SURF #ifdef SURF_ENABLED
		if ( aArgGrad.isSet() ) {
			ioPanoDetector.setGradientDescriptor(true);
		} else {
			ioPanoDetector.setGradientDescriptor(false);
		}
		
		ioPanoDetector.setSurfExtended(aArgSurfExtended.isSet());
#endif */
//REMOVING SURF 		if (aArgSurfScoreThreshold.isSet())	ioPanoDetector.setSurfScoreThreshold(aArgSurfScoreThreshold.getValue());
		if (aArgSieve1Width.isSet())		ioPanoDetector.setSieve1Width(aArgSieve1Width.getValue());
		if (aArgSieve1Height.isSet())		ioPanoDetector.setSieve1Height(aArgSieve1Height.getValue());
		if (aArgSieve1Size.isSet())			ioPanoDetector.setSieve1Size(aArgSieve1Size.getValue());
		if (aArgKDTreeSearchSteps.isSet())	ioPanoDetector.setKDTreeSearchSteps(aArgKDTreeSearchSteps.getValue());
		if (aArgKDTreeSecondDist.isSet())	ioPanoDetector.setKDTreeSecondDistance(aArgKDTreeSecondDist.getValue());
		if (aArgMinMatches.isSet())			ioPanoDetector.setMinimumMatches(aArgMinMatches.getValue());
		if (aArgRansacIter.isSet())			ioPanoDetector.setRansacIterations(aArgRansacIter.getValue());
		if (aArgRansacDist.isSet())			ioPanoDetector.setRansacDistanceThreshold(aArgRansacDist.getValue());
		if (aArgSieve2Width.isSet())		ioPanoDetector.setSieve2Width(aArgSieve2Width.getValue());
		if (aArgSieve2Height.isSet())		ioPanoDetector.setSieve2Height(aArgSieve2Height.getValue());
		if (aArgSieve2Size.isSet())			ioPanoDetector.setSieve2Size(aArgSieve2Size.getValue());
		if (aArgLinearMatch.isSet())		ioPanoDetector.setLinearMatch(aArgLinearMatch.getValue());
		if (aArgLinearMatchLen.isSet())		ioPanoDetector.setLinearMatchLen(aArgLinearMatchLen.getValue());
        if (aArgFullScale.isSet())          ioPanoDetector.setDownscale(false);

		if (aArgLoadKeypoints.isSet()) {
			ioPanoDetector.setLoadKeypoints(aArgLoadKeypoints.getValue());
			ioPanoDetector.setDownscale(false);
		}
		
		if (aArgTest.isSet())				ioPanoDetector.setTest(aArgTest.getValue());
		if (aArgCores.isSet())				ioPanoDetector.setCores(aArgCores.getValue());

		vector<string> aFiles = aArgFiles.getValue();
		BOOST_FOREACH(string& aF, aFiles)
		{
			if (aArgPTGuiCompat.getValue())
			{
				if (aF.find("/path:") == 0)
					ioPanoDetector.setOutputFile(aF.substr(6, aF.size() - 6) + "\\panorama0.oto");

				if (aF.find("/") == 0)
					continue;
			}

			ioPanoDetector.addFile(aF);
		}

	} catch ( ArgException& e )
	{ 
		cout << "ERROR: " << e.error() << " " << e.argId() << endl; 
	}
}

int main(int argc, char **argv) 
{
	std::cout << "Pan-o-matic " << kVersion << " by Anael Orlinski - naouel@naouel.org" << endl << endl;

	// create a panodetector object
	PanoDetector aPanoDetector;
	parseOptions(argc, argv, aPanoDetector);
	
	if (!aPanoDetector.checkData())
		return 0;
	aPanoDetector.printDetails();
	
	TIMETRACE("Detection",aPanoDetector.run());
	
	return 0;
	
}
