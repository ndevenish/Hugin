// -*- c-basic-offset: 4 -*-
/** @file Exiv2Helper.cpp
 *
 *  @brief helper functions to work with Exif data via the exiv2 library
 * 
 *
 *  @author Pablo d'Angelo, T. Modes
 *
 */

/*
 *  This is free software; you can redistribute it and/or
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

#include "Exiv2Helper.h"
#include "hugin_math/hugin_math.h"
#include "hugin_utils/utils.h"  
#include "exiv2/easyaccess.hpp"

namespace HuginBase
{
    namespace Exiv2Helper
    {

        bool _getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, long & value)
        {
            Exiv2::ExifData::iterator itr = exifData.findKey(Exiv2::ExifKey(keyName));
            if (itr != exifData.end() && itr->count())
            {
                value = itr->toLong();
                return true;
            }
            else
            {
                return false;
            };
        };

        bool _getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, float & value)
        {
            Exiv2::ExifData::iterator itr = exifData.findKey(Exiv2::ExifKey(keyName));
            if (itr != exifData.end() && itr->count())
            {
                value = itr->toFloat();
                return true;
            }
            else
            {
                return false;
            };
        };

        bool _getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, std::string & value)
        {
            Exiv2::ExifData::iterator itr = exifData.findKey(Exiv2::ExifKey(keyName));
            if (itr != exifData.end() && itr->count())
            {
                value = itr->toString();
                return true;
            }
            else
            {
                return false;
            };
        }

        bool _getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, std::vector<float> & values)
        {
            values.clear();
            Exiv2::ExifData::iterator itr = exifData.findKey(Exiv2::ExifKey(keyName));
            if (itr != exifData.end() && itr->count())
            {
                for(long i=0; i<itr->count(); i++)
                {
                    values.push_back(itr->toFloat(i));
                };
                return true;
            }
            else
            {
                return false;
            }
        }

        bool _getExiv2Value(Exiv2::ExifData& exifData, uint16_t tagID, std::string groupName, std::string & value)
        {
            Exiv2::ExifData::iterator itr = exifData.findKey(Exiv2::ExifKey(tagID, groupName));
            if (itr != exifData.end() && itr->count())
            {
                value = itr->toString();
                return true;
            }
            else
            {
                return false;
            };
        };

        bool _getExiv2Value(Exiv2::ExifData& exifData, uint16_t tagID, std::string groupName, double & value)
        {
            Exiv2::ExifData::iterator itr = exifData.findKey(Exiv2::ExifKey(tagID, groupName));
            if (itr != exifData.end() && itr->count())
            {
                value = itr->toFloat();
                return true;
            }
            else
            {
                return false;
            }
        }

        const double getExiv2ValueDouble(Exiv2::ExifData& exifData, Exiv2::ExifData::const_iterator it)
        {
            if(it!=exifData.end() && it->count())
            {
                return it->toFloat();
            }
            return 0;
        };

        const double getExiv2ValueDouble(Exiv2::ExifData& exifData, std::string keyName)
        {
            float d;
            if(_getExiv2Value(exifData, keyName, d))
            {
                return d;
            }
            return 0;
        };

        const std::string getExiv2ValueString(Exiv2::ExifData& exifData,Exiv2::ExifData::const_iterator it)
        {
            if(it!=exifData.end() && it->count())
            {
                return it->toString();
            };
            return std::string("");
        };

        const std::string getExiv2ValueString(Exiv2::ExifData& exifData, std::string keyName)
        {
            std::string s;
            if(_getExiv2Value(exifData, keyName, s))
            {
                return s;
            }
            return std::string("");
        };

        const long getExiv2ValueLong(Exiv2::ExifData& exifData, Exiv2::ExifData::const_iterator it)
        {
            if(it!=exifData.end() && it->count())
            {
                return it->toLong();
            }
            return 0;
        };

        const long getExiv2ValueLong(Exiv2::ExifData& exifData, std::string keyName)
        {
            long l;
            if(_getExiv2Value(exifData, keyName, l))
            {
                return l;
            }
            return 0;
        };
        
        //for diagnostic
        void PrintTag(Exiv2::ExifData::iterator itr)
        {
            std::cout << itr->value() << " (" << itr->typeName() << ", size: " << itr->count() << ")" << std::endl;
            if(itr->count()>1)
            {
                std::cout << "[";
                for(long i=0; i<itr->count(); i++)
                {
                    std::cout << itr->toFloat(i) << ",";
                }
                std::cout << "]" << std::endl;
            };
        };

        bool readRedBlueBalance(Exiv2::ExifData &exifData, double & redBalance, double & blueBalance)
        {
            redBalance=1.0;
            blueBalance=1.0;
            //Panasonic makernotes (also some Leica cams)
            float val1=0, val2=0, val3=0;
            std::vector<float> values;
            if(_getExiv2Value(exifData, "Exif.Panasonic.WBRedLevel", val1) &&
               _getExiv2Value(exifData, "Exif.Panasonic.WBGreenLevel", val2) &&
               _getExiv2Value(exifData, "Exif.Panasonic.WBBlueLevel", val3))
            {
                if(val1!=0 && val2!=0 && val3!=0)
                {
                    redBalance=val1 / val2;;
                    blueBalance=val3 / val2;
                    return true;
                }
                else
                {
                    return false;
                };
            };
            // Pentax makernotes
            if (_getExiv2Value(exifData, "Exif.Pentax.RedBalance", val1) &&
                _getExiv2Value(exifData, "Exif.Pentax.BlueBalance", val2))
            {
                if(val1!=0 && val2!=0)
                {
                    redBalance=val1 / 8192.0;
                    blueBalance=val2 / 8192.0;
                    return true;
                }
                else
                {
                    return false;
                };
            };
#if EXIV2_TEST_VERSION(0,23,0)
            if (_getExiv2Value(exifData, "Exif.PentaxDng.RedBalance", val1) &&
                _getExiv2Value(exifData, "Exif.PentaxDng.BlueBalance", val2))
            {
                if(val1!=0 && val2!=0)
                {
                    redBalance=val1 / 256.0;
                    blueBalance=val2 / 256.0;
                    return true;
                }
                else
                {
                    return false;
                };
            };
#endif
            //Olympus makernotes
            if (_getExiv2Value(exifData, "Exif.Olympus.RedBalance", val1) &&
                _getExiv2Value(exifData, "Exif.Olympus.BlueBalance", val2))
            {
                if(val1!=0 && val2!=0)
                {
                    redBalance=val1 / 256.0;
                    blueBalance=val2 / 256.0;
                    return true;
                }
                else
                {
                    return false;
                };
            };
            if(_getExiv2Value(exifData, "Exif.OlympusIp.WB_RBLevels", values))
            {
                if(values.size()>=2)
                {
                    if(values[0]!=0 && values[1]!=0)
                    {
                        redBalance=values[0]/256.0;
                        blueBalance=values[1]/256.0;
                        return true;
                    }
                    else
                    {
                        return false;
                    };
                }
                else
                {
                    return false;
                };
            };
            // Nikon makernotes
            if(_getExiv2Value(exifData, "Exif.Nikon3.WB_RBLevels", values))
            {
                if(values.size()>=2)
                {
                    if(values[0]!=0 && values[1]!=0)
                    {
                        redBalance=values[0];
                        blueBalance=values[1];
                        return true;
                    }
                    else
                    {
                        return false;
                    };
                }
                else
                {
                    return false;
                };
            };
            if(_getExiv2Value(exifData, "Exif.NikonCb1.WB_RBGGLevels", values))
            {
                if(values.size()==4)
                {
                    if(values[0]!=0 && values[1]!=0 && values[2]!=0 && values[3]!=0)
                    {
                        redBalance=values[0] / values[2];
                        blueBalance=values[1] / values[2];
                        return true;
                    }
                    else
                    {
                        return false;
                    };
                }
                else
                {
                    return false;
                };
            };
            if(_getExiv2Value(exifData, "Exif.NikonCb2.WB_RGGBLevels", values))
            {
                if(values.size()==4)
                {
                    if(values[0]!=0 && values[1]!=0 && values[2]!=0 && values[3]!=0)
                    {
                        redBalance=values[0] / values[1];
                        blueBalance=values[3] / values[1];
                        return true;
                    }
                    else
                    {
                        return false;
                    };
                }
                else
                {
                    return false;
                };
            };
            if(_getExiv2Value(exifData, "Exif.NikonCb2a.WB_RGGBLevels", values))
            {
                if(values.size()==4)
                {
                    if(values[0]!=0 && values[1]!=0 && values[2]!=0 && values[3]!=0)
                    {
                        redBalance=values[0] / values[1];
                        blueBalance=values[3] / values[1];
                        return true;
                    }
                    else
                    {
                        return false;
                    };
                }
                else
                {
                    return false;
                };
            };
            if(_getExiv2Value(exifData, "Exif.NikonCb2b.WB_RGGBLevels", values))
            {
                if(values.size()==4)
                {
                    if(values[0]!=0 && values[1]!=0 && values[2]!=0 && values[3]!=0)
                    {
                        redBalance=values[0] / values[1];
                        blueBalance=values[3] / values[1];
                        return true;
                    }
                    else
                    {
                        return false;
                    };
                }
                else
                {
                    return false;
                };
            };
            if(_getExiv2Value(exifData, "Exif.NikonCb3.WB_RGBGLevels", values))
            {
                if(values.size()==4)
                {
                    if(values[0]!=0 && values[1]!=0 && values[2]!=0 && values[3]!=0)
                    {
                        redBalance=values[0] / values[1];
                        blueBalance=values[2] / values[3];
                        return true;
                    }
                    else
                    {
                        return false;
                    };
                }
                else
                {
                    return false;
                };
            };

            return false;
        };

        const double getCropFactor(Exiv2::ExifData &exifData, long width, long height)
        {
            double cropFactor=0;
            // some cameras do not provide Exif.Image.ImageWidth / Length
            // notably some Olympus
            long eWidth = 0;
            _getExiv2Value(exifData,"Exif.Image.ImageWidth",eWidth);

            long eLength = 0;
            _getExiv2Value(exifData,"Exif.Image.ImageLength",eLength);

            double sensorPixelWidth = 0;
            double sensorPixelHeight = 0;
            if (eWidth > 0 && eLength > 0)
            {
                sensorPixelHeight = (double)eLength;
                sensorPixelWidth = (double)eWidth;
            }
            else
            {
                // No EXIF information, use number of pixels in image
                sensorPixelWidth = width;
                sensorPixelHeight = height;
            }

            // force landscape sensor orientation
            if (sensorPixelWidth < sensorPixelHeight )
            {
                double t = sensorPixelWidth;
                sensorPixelWidth = sensorPixelHeight;
                sensorPixelHeight = t;
            }

            DEBUG_DEBUG("sensorPixelWidth: " << sensorPixelWidth);
            DEBUG_DEBUG("sensorPixelHeight: " << sensorPixelHeight);

            // some cameras do not provide Exif.Photo.FocalPlaneResolutionUnit
            // notably some Olympus

            long exifResolutionUnits = 0;
            _getExiv2Value(exifData,"Exif.Photo.FocalPlaneResolutionUnit",exifResolutionUnits);

            float resolutionUnits= 0;
            switch (exifResolutionUnits)
            {
                case 3: resolutionUnits = 10.0; break;  //centimeter
                case 4: resolutionUnits = 1.0; break;   //millimeter
                case 5: resolutionUnits = .001; break;  //micrometer
                default: resolutionUnits = 25.4; break; //inches
            }

            DEBUG_DEBUG("Resolution Units: " << resolutionUnits);

            // some cameras do not provide Exif.Photo.FocalPlaneXResolution and
            // Exif.Photo.FocalPlaneYResolution, notably some Olympus
            float fplaneXresolution = 0;
            _getExiv2Value(exifData,"Exif.Photo.FocalPlaneXResolution",fplaneXresolution);

            float fplaneYresolution = 0;
            _getExiv2Value(exifData,"Exif.Photo.FocalPlaneYResolution",fplaneYresolution);

            float CCDWidth = 0;
            if (fplaneXresolution != 0)
            {
                CCDWidth = (float)(sensorPixelWidth / ( fplaneXresolution / resolutionUnits));
            }

            float CCDHeight = 0;
            if (fplaneYresolution != 0)
            {
                CCDHeight = (float)(sensorPixelHeight / ( fplaneYresolution / resolutionUnits));
            }

            DEBUG_DEBUG("CCDHeight:" << CCDHeight);
            DEBUG_DEBUG("CCDWidth: " << CCDWidth);

            // calc sensor dimensions if not set and 35mm focal length is available
            hugin_utils::FDiff2D sensorSize;
            if (CCDHeight > 0 && CCDWidth > 0)
            {
                // read sensor size directly.
                sensorSize.x = CCDWidth;
                sensorSize.y = CCDHeight;
                std::string exifModel;
                if(_getExiv2Value(exifData, "Exif.Image.Model", exifModel))
                {
                    if (exifModel == "Canon EOS 20D")
                    {
                        // special case for buggy 20D camera
                        sensorSize.x = 22.5;
                        sensorSize.y = 15;
                    }
                };
                // check if sensor size ratio and image size fit together
                double rsensor = (double)sensorSize.x / sensorSize.y;
                double rimg = (double) width / height;
                if ( (rsensor > 1 && rimg < 1) || (rsensor < 1 && rimg > 1) )
                {
                    // image and sensor ratio do not match
                    // swap sensor sizes
                    float t;
                    t = sensorSize.y;
                    sensorSize.y = sensorSize.x;
                    sensorSize.x = t;
                }

                DEBUG_DEBUG("sensorSize.y: " << sensorSize.y);
                DEBUG_DEBUG("sensorSize.x: " << sensorSize.x);

                cropFactor = sqrt(36.0*36.0+24.0*24.0) /
                    sqrt(sensorSize.x*sensorSize.x + sensorSize.y*sensorSize.y);
                // FIXME: HACK guard against invalid image focal plane definition in EXIF metadata with arbitrarly chosen limits for the crop factor ( 1/100 < crop < 100)
                if (cropFactor < 0.01 || cropFactor > 100)
                {
                    cropFactor = 0;
                }
            }
            else
            {
                // alternative way to calculate the crop factor for Olympus cameras

                // Windows debug stuff
                // left in as example on how to get "console output"
                // written to a log file    
                // freopen ("oly.log","a",stdout);
                // fprintf (stdout,"Starting Alternative crop determination\n");
        
                float olyFPD = 0;
                _getExiv2Value(exifData,"Exif.Olympus.FocalPlaneDiagonal",olyFPD);
                if (olyFPD > 0.0)
                {
                    // Windows debug stuff
                    // fprintf(stdout,"Oly_FPD:");
                    // fprintf(stdout,"%f",olyFPD);
                    cropFactor = sqrt(36.0*36.0+24.0*24.0) / olyFPD;
                }
                else
                {
                    // for newer Olympus cameras the FocalPlaneDiagonal tag was moved into
                    // equipment (sub?)-directory, so check also there
                    _getExiv2Value(exifData,"Exif.OlympusEq.FocalPlaneDiagonal",olyFPD);
                    if (olyFPD > 0.0)
                    {
                        cropFactor = sqrt(36.0*36.0+24.0*24.0) / olyFPD;
                    };
                };
            };
            return cropFactor;
        };

        const std::string getLensName(Exiv2::ExifData &exifData)
        {
            std::string lensName;
            // first we are reading LensModel in Exif section, this is only available
            // with EXIF >= 2.3
#if EXIV2_TEST_VERSION(0,22,0)
            //the string "Exif.Photo.LensModel" is only defined in exiv2 0.22.0 and above
            if(_getExiv2Value(exifData, "Exif.Photo.LensModel", lensName))
#else
            if(_getExiv2Value(exifData, 0xa434, "Photo", lensName))
#endif
            {
                if(lensName.length()>0)
                {
                    return lensName;
                };
            }
            else
            {
                //no lens in Exif found, now look in makernotes
                Exiv2::ExifData::const_iterator itr2 = Exiv2::lensName(exifData);
                if (itr2!=exifData.end() && itr2->count())
                {
                    //we are using prettyPrint function to get string of lens name
                    //it2->toString returns for many cameras only an ID number
                    lensName=itr2->print(&exifData);
                    //check returned lens name
                    if(lensName.length()>0)
                    {
                        //for Canon it can contain (65535) or (0) for unknown lenses
                        //for Pentax it can contain Unknown (0xHEX)
                        if(lensName.compare(0, 1, "(")!=0 && lensName.compare(0, 7, "Unknown")!=0)
                        {
                            return lensName;
                        }
                    };
                };
            };
            return std::string("");
        };

    }; //namespace Exiv2Helper
}; //namespace HuginBase