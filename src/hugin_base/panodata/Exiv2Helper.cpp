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

namespace HuginBase
{
    namespace Exiv2Helper
    {

        bool getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, long & value)
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

        bool getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, float & value)
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

        bool getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, std::string & value)
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

        bool getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, std::vector<float> & values)
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

        bool getExiv2Value(Exiv2::ExifData& exifData, uint16_t tagID, std::string groupName, std::string & value)
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

        bool getExiv2Value(Exiv2::ExifData& exifData, uint16_t tagID, std::string groupName, double & value)
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
            if(getExiv2Value(exifData, "Exif.Panasonic.WBRedLevel", val1) &&
               getExiv2Value(exifData, "Exif.Panasonic.WBGreenLevel", val2) &&
               getExiv2Value(exifData, "Exif.Panasonic.WBBlueLevel", val3))
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
            if (getExiv2Value(exifData, "Exif.Pentax.RedBalance", val1) &&
                getExiv2Value(exifData, "Exif.Pentax.BlueBalance", val2))
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
            if (getExiv2Value(exifData, "Exif.PentaxDng.RedBalance", val1) &&
                getExiv2Value(exifData, "Exif.PentaxDng.BlueBalance", val2))
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
            if (getExiv2Value(exifData, "Exif.Olympus.RedBalance", val1) &&
                getExiv2Value(exifData, "Exif.Olympus.BlueBalance", val2))
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
            if(getExiv2Value(exifData, "Exif.OlympusIp.WB_RBLevels", values))
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
            if(getExiv2Value(exifData, "Exif.Nikon3.WB_RBLevels", values))
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
            if(getExiv2Value(exifData, "Exif.NikonCb1.WB_RBGGLevels", values))
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
            if(getExiv2Value(exifData, "Exif.NikonCb2.WB_RGGBLevels", values))
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
            if(getExiv2Value(exifData, "Exif.NikonCb2a.WB_RGGBLevels", values))
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
            if(getExiv2Value(exifData, "Exif.NikonCb2b.WB_RGGBLevels", values))
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
            if(getExiv2Value(exifData, "Exif.NikonCb3.WB_RGBGLevels", values))
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

    }; //namespace Exiv2Helper
}; //namespace HuginBase