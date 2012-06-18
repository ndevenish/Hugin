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
#include <fstream>
#include <vector>
#include <string>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

#include <tclap/CmdLine.h>

#include <localfeatures/KeyPointDetector.h>
#include <localfeatures/CircularKeyPointDescriptor.h>
#include <localfeatures/Sieve.h>
#include <localfeatures/KeyPointIO.h>

#include <vigra/impex.hxx>
#include <vigra/stdimage.hxx>
#include <vigra/stdimagefunctions.hxx>
#include <vigra/rgbvalue.hxx>

using namespace std;
using namespace TCLAP;
using namespace lfeat;

const char* kVersion="0.9.5";

#define TRACE_IMG(A) cerr << A << std::endl
#define TRACE_INFO(A) cerr << A << std::endl


//typedef boost::shared_ptr<lfeat::KeyPoint>			KeyPointPtr;
//typedef std::vector<KeyPointPtr>						KeyPointVect_t;
//typedef std::vector<KeyPointPtr>::iterator				KeyPointVectIt_t;


// define a Keypoint insertor
class KeyPointVectInsertor : public lfeat::KeyPointInsertor
{
public:
    KeyPointVectInsertor ( KeyPointVect_t& iVect ) : _v ( iVect ) {};
    inline virtual void operator() ( const lfeat::KeyPoint& k )
    {
        _v.push_back ( KeyPointPtr ( new lfeat::KeyPoint ( k ) ) );
    }

private:
    KeyPointVect_t& _v;
};

// define a sieve extractor
class SieveExtractorKP : public lfeat::SieveExtractor<KeyPointPtr>
{
public:
    SieveExtractorKP ( KeyPointVect_t& iV ) : _v ( iV ) {};
    inline virtual void operator() ( const KeyPointPtr& k )
    {
        _v.push_back ( k );
    }
private:
    KeyPointVect_t& _v;
};

bool DetectKeypoints ( const std::string& imgfile, bool downscale,
                       double surfScoreThreshold,
                       KeyPointPtr preKeypoint,
                       bool onlyInterestPoints, int sieveWidth,
                       int sieveHeight, int sieveSize, KeypointWriter& writer )
{
    TRACE_IMG ( "Analyze image..." );
    try

    {
        vigra::ImageImportInfo aImageInfo ( imgfile.c_str() );

        int aNewImgWidth = aImageInfo.width();
        int aNewImgHeight = aImageInfo.height();

        int aOrigImgWidth = aNewImgWidth;
        int aOrigImgHeight = aNewImgHeight;

        int scale = 1;
        if ( downscale )
        {
            aNewImgWidth >>= 1;
            aNewImgHeight >>= 1;
            scale = 2;
        }

        vigra::DImage aImageDouble ( aNewImgWidth, aNewImgHeight );

        if ( aImageInfo.isGrayscale() )
        {
            if ( downscale )
            {
                TRACE_IMG ( "Load greyscale..." );
                vigra::DImage aImageG ( aImageInfo.width(), aImageInfo.height() );
                importImage ( aImageInfo, destImage ( aImageG ) );
                vigra::resizeImageNoInterpolation (
                    aImageG.upperLeft(),
                    aImageG.upperLeft() + vigra::Diff2D ( aNewImgWidth * 2, aNewImgHeight * 2 ),
                    vigra::DImage::Accessor(),
                    aImageDouble.upperLeft(),
                    aImageDouble.lowerRight(),
                    vigra::DImage::Accessor() );
            }
            else
            {
                TRACE_IMG ( "Load greyscale..." );
                importImage ( aImageInfo, destImage ( aImageDouble ) );
            }
        }
        else
        {
            TRACE_IMG ( "Load RGB..." );
            //open the image in RGB
            vigra::DRGBImage aImageRGB ( aImageInfo.width(), aImageInfo.height() );

            if ( aImageInfo.numExtraBands() == 1 )

            {

                TRACE_INFO ( "Image with alpha channels are not supported" );
                return false;

            }

            else if ( aImageInfo.numExtraBands() == 0 )

            {

                vigra::importImage ( aImageInfo, destImage ( aImageRGB ) );
            }
            else
            {
                TRACE_INFO ( "Image with multiple alpha channels are not supported" );
                return false;
            }

            if ( downscale )
            {
                TRACE_IMG ( "Resize to greyscale double..." );
                vigra::resizeImageNoInterpolation (
                    aImageRGB.upperLeft(),
                    aImageRGB.upperLeft() + vigra::Diff2D ( aNewImgWidth * 2, aNewImgHeight * 2 ),
                    vigra::RGBToGrayAccessor<vigra::RGBValue<double> >(),
                    aImageDouble.upperLeft(),
                    aImageDouble.lowerRight(),
                    vigra::DImage::Accessor() );

            }
            else
            {
                // convert to greyscale
                TRACE_IMG ( "Convert to greyscale double..." );
                vigra::copyImage (	aImageRGB.upperLeft(),
                                    aImageRGB.lowerRight(),
                                    vigra::RGBToGrayAccessor<vigra::RGBValue<double> >(),
                                    aImageDouble.upperLeft(),
                                    vigra::DImage::Accessor() );
            }
        }


        ImageInfo imginfo(imgfile, aOrigImgWidth, aOrigImgHeight);

        TRACE_IMG ( "Build integral image..." );
        // create integral image
        lfeat::Image img;
        img.init ( aImageDouble );

        KeyPointVect_t kp;
        KeyPointVectInsertor aInsertor = KeyPointVectInsertor ( kp );
        if ( ! preKeypoint)
        {
            // setup the detector
            lfeat::KeyPointDetector aKP;
            aKP.setScoreThreshold ( surfScoreThreshold );

            // detect the keypoints
            aKP.detectKeypoints ( img, aInsertor );

            TRACE_IMG ( "Found "<< kp.size() << " interest points." );

            TRACE_IMG ( "Filtering keypoints..." );

            lfeat::Sieve<lfeat::KeyPointPtr, lfeat::KeyPointPtrSort > aSieve ( sieveWidth, sieveHeight, sieveSize );
            // insert the points in the Sieve
            double aXF = ( double ) sieveWidth / ( double ) aImageDouble.width();
            double aYF = ( double ) sieveHeight / ( double ) aImageDouble.height();
            BOOST_FOREACH ( KeyPointPtr& aK, kp )
            aSieve.insert ( aK, ( int ) ( aK->_x * aXF ), ( int ) ( aK->_y * aYF ) );

            // pull remaining values from the sieve
            kp.clear();

            // make an extractor and pull the points
            SieveExtractorKP aSieveExt ( kp );
            aSieve.extract ( aSieveExt );

            TRACE_IMG ( "Kept " << kp.size() << " interest points." );
        }
        else
        {
            kp.push_back(boost::shared_ptr<KeyPoint>(preKeypoint));
        }

        lfeat::KeyPointDescriptor* aKPD;
        aKPD = new lfeat::CircularKeyPointDescriptor( img );
        TRACE_IMG ( "Generating descriptors and writing output..." );


        int dims = aKPD->getDescriptorLength();
        if ( onlyInterestPoints )
        {
            dims = 0;
        }

        // compute orientation
        // vector for keypoints with more than one orientation
        KeyPointVect_t kp_new_ori;

        BOOST_FOREACH ( KeyPointPtr& aK, kp )
        {
            if (!( preKeypoint  && preKeypoint->_ori > -10000))
            {
                double angles[4];
                int nAngles = aKPD->assignOrientation ( *aK, angles );
                std::cerr << "Orientations:" << aK->_ori;
                for (int i=0; i < nAngles; i++)
                {
                    // duplicate Keypoint with additional angles
                    KeyPointPtr aKn = KeyPointPtr ( new lfeat::KeyPoint ( *aK ) );
                    aKn->_ori = angles[i];
                    std::cerr << " " << aKn->_ori;
                    kp_new_ori.push_back(aKn);
                }
                std::cerr << std::endl;
            }
        }

        // append new keypoints to kp
        kp.insert(kp.end(), kp_new_ori.begin(), kp_new_ori.end());

        writer.writeHeader ( imginfo, kp.size(), aKPD->getDescriptorLength() );

        BOOST_FOREACH ( KeyPointPtr& aK, kp )
        {
            if ( !onlyInterestPoints )
            {
                aKPD->makeDescriptor ( *aK );
            }
            writer.writeKeypoint ( aK->_x * scale, aK->_y * scale, aK->_scale * scale, aK->_ori,
                                   aK->_score, dims, aK->_vec );
        }
        writer.writeFooter();
        delete aKPD;
    }

    catch ( std::exception& e )

    {

        TRACE_INFO ( "An error happened while computing keypoints : caught exception: " << e.what() << endl );

        return false;

    }

    return true;
}


class MyOutput : public StdOutput
{
public:

    virtual void failure ( CmdLineInterface& c, ArgException& e )
    {
        std::cerr << "Parse error: " << e.argId() << std::endl << "             " << e.error() << std::endl << std::endl << endl;
        usage ( c );
    }

    virtual void usage ( CmdLineInterface& c )
    {
        int iML = 30;
        cout << "Basic usage : " << endl;
        cout << "  "<< c.getProgramName() << " [options ] IMG" << endl;

        cout << endl <<"All options : " << endl;
        list<Arg*> args = c.getArgList();
        for ( ArgListIterator it = args.begin(); it != args.end(); it++ )
        {
            string aL = ( *it )->longID();
            string aD = ( *it )->getDescription();
            // replace tabs by n spaces.
            size_t p = aD.find_first_of ( "\t" );
            while ( p != string::npos )
            {
                string aD1 = aD.substr ( 0, p );
                string aD2 = aD.substr ( p+1, aD.size() - p + 1 );

                aD = aD1 + "\n" + string ( iML, ' ' ) + aD2;
                p = aD.find_first_of ( "\t" );
            }


            if ( (int)aL.size() > iML )
            {
                cout << aL << endl << string ( iML, ' ' )  << aD << endl;
            }
            else
            {
                cout << aL  << string ( iML - aL.size(), ' ' )   << aD << endl;
            }
        }
    }

    virtual void version ( CmdLineInterface& c )
    {
        cout << "my version message: 0.1" << endl;
    }
};



void parseOptions ( int argc, char** argv )
{
    try
    {

        CmdLine cmd ( "keypoints", ' ', kVersion );

        MyOutput my;
        cmd.setOutput ( &my );

        SwitchArg aArgFullScale ( "","fullscale", "Uses full scale image to detect keypoints    (default:false)\n", false );
        // SURF has a better performance than the other descriptors, use it by default, if it is enabled
        ValueArg<int> aArgSurfScoreThreshold ( "","surfscore", "Detection score threshold    (default : 1000)\n", false, 1000, "int" );
        ValueArg<int> aArgSieve1Width ( "","sievewidth", "Interest point sieve: Number of buckets on width    (default : 10)", false, 10, "int" );
        ValueArg<int> aArgSieve1Height ( "","sieveheight",  "Interest point sieve : Number of buckets on height    (default : 10)", false, 10, "int" );
        ValueArg<int> aArgSieve1Size ( "","sievesize",	"Interest point sieve : Max points per bucket    (default : 10)\n", false, 10, "int" );
        ValueArg<std::string> aArgOutputFormat ( "","format", "Output format (text, autopano-xml, descperf), default text\n", false, "text", "string" );
        ValueArg<std::string> aArgOutputFile ( "o","output", "Output file. If not specified, print to standard out\n", false, "", "string" );
        SwitchArg aArgInterestPoints ( "","interestpoints", "output only the interest points and the scale (default:false)\n", false );
        ValueArg<std::string> aArgFixedInterestPoint ( "","ip", "Compute descriptor at x,y,scale,ori \n", false, "", "string" );

        cmd.add ( aArgSurfScoreThreshold );
        cmd.add ( aArgFullScale );
        cmd.add ( aArgSieve1Width );
        cmd.add ( aArgSieve1Height );
        cmd.add ( aArgSieve1Size );
        cmd.add ( aArgOutputFormat );
        cmd.add ( aArgOutputFile );
        cmd.add ( aArgInterestPoints );
        cmd.add ( aArgFixedInterestPoint );

        /*
        	SwitchArg aArgTest("t","test", "Enables test mode\n", false);
        	cmd.add( aArgTest );
        */

        UnlabeledMultiArg<string> aArgFiles ( "fileName", "Image files", true, "string" );
        cmd.add ( aArgFiles );

        cmd.parse ( argc,argv );

        //
        // Set variables
        //
        vector<string> aFiles = aArgFiles.getValue();
        if ( aFiles.size() != 1 )
        {
            exit ( 1 );
        }

        double surfScoreThreshold=1000;
        if ( aArgSurfScoreThreshold.isSet() )
        {
            surfScoreThreshold = ( aArgSurfScoreThreshold.getValue() );
        }

        bool downscale = true;
        if ( aArgFullScale.isSet() )
        {
            downscale = false;
        }

        int sieveWidth = 10;
        if ( aArgSieve1Width.isSet() )
        {
            sieveWidth = aArgSieve1Width.getValue();
        }
        int sieveHeight = 10;
        if ( aArgSieve1Height.isSet() )
        {
            sieveHeight = aArgSieve1Height.getValue();
        }
        int sieveSize = 10;
        if ( aArgSieve1Size.isSet() )
        {
            sieveSize = aArgSieve1Size.getValue();
        }

        bool onlyInterestPoints = false;
        if ( aArgInterestPoints.isSet() )
        {
            onlyInterestPoints = true;
        }

        std::ostream* outstream;
        if ( aArgOutputFile.isSet() )
        {
            outstream = new std::ofstream(aArgOutputFile.getValue().c_str());
        }
        else
        {
            outstream = & std::cout;
        }

        KeypointWriter* writer = 0;
        std::string outputformat = "text";
        if ( aArgOutputFormat.isSet() )
        {
            outputformat = aArgOutputFormat.getValue();
        }
        if (outputformat == "text")
        {
            writer = new SIFTFormatWriter(*outstream);
        }
        else if (outputformat == "autopano-sift-xml")
        {
            writer = new AutopanoSIFTWriter(*outstream);
        }
        else if (outputformat == "descperf")
        {
            writer = new DescPerfFormatWriter(*outstream);
        }
        else
        {
            std::cerr << "Unknown output format, valid values are text, autopano-sift-xml, descperf" << std::endl;
            exit(1);
        }


        KeyPointPtr preKPPtr;
        if ( aArgFixedInterestPoint.isSet() )
        {
            preKPPtr = KeyPointPtr(new KeyPoint());
            preKPPtr->_x = -10001;
            preKPPtr->_ori = -10001;
            int nf = sscanf(aArgFixedInterestPoint.getValue().c_str(), "%lf:%lf:%lf:%lf",
                            &(preKPPtr->_x), &(preKPPtr->_y), &(preKPPtr->_scale), &(preKPPtr->_ori));
            std::cerr << "passed orientation: " << preKPPtr->_ori << std::endl;
            if (nf < 3)
            {
                std::cerr << "Invalid value for --ip option, expected --ip x:y:scale:ori" << std::endl;
                exit(1);
            }
        }

        DetectKeypoints ( aFiles[0], downscale, surfScoreThreshold, preKPPtr, onlyInterestPoints, sieveWidth, sieveHeight, sieveSize, *writer );

        if ( aArgOutputFile.isSet() )
        {
            delete outstream;
        }

    }
    catch ( ArgException& e )
    {
        cout << "ERROR: " << e.error() << " " << e.argId() << endl;
    }
}

int main ( int argc, char** argv )
{
    std::cerr << "keypoints " << kVersion << " by Anael Orlinski - naouel@naouel.org" << endl << endl;

    // create a panodetector object
    parseOptions ( argc, argv );

    return 0;

}
