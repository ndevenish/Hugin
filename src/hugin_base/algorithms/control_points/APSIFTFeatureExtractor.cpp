// -*- c-basic-offset: 4 -*-
/** @file APSIFTFeatureExtractor.cpp
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: Panorama.h 1947 2007-04-15 20:46:00Z dangelo $
 *
 *
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

#include "FeatureExtractor.h"

#include <panodata/PanoramaData.h>
#include <huginapp/ImageCache.h>
#include "APSIFTFeatureExtractor.h"

namespace HuginBase {

CPVector & APSIFTFeatureExtractor::extract(const PanoramaData& pano, int imageNr)
{
    std::vector<Keypoints> keypoints;

    PanoImage img = pano.getImage(imageNr);

    // prepare images and save to disk (maybe for 16bit and hdr files?)
#ifdef unix
    // TODO: use a safe way to create temporary files.
    std::string tmpfilename("/tmp/apskey.tmp");
    // run generatekeys on image
    std::string cmd("generatekeys ");
    cmd = cmd + pano.getImage(imageNr).getFilename() + " " + tmpfilename + " 800";
    if (system(cmd.c_str()) == -1) {
        // fatal error
        perror("could not execute keypoint creation program");
        return keypoints;
;
    }

    // load keypoints from xml data
#else
#warning APSIFTFeatureExtractor not supported on this platform yet
#endif
    // load from disk & add to image
    Size2D imageSize;
    std::string imgFN;
    readAPSIFTXML(tmpfilename, keypoints, imageSize, imgFN);
    img.setKeypoints(keypoints);
    pano.setImage(imageNr, img);
}

void readAPSIFTXML(const char * filename, 
                   std::vector<Keypoints> & keypoints,
                   vigra::Size2D & imageSize,
                   std::string & imageFilename)
{
  xmlTextReaderPtr reader;
  reader = xmlReaderForFile(filename, NULL, 0);

  Keypoint kp;

  if (reader != NULL) {
    int ret;
    const xmlChar *name;

    ret = xmlTextReaderRead(reader);
    while (ret == 1) {
        name = xmlTextReaderConstName(reader);
        if (xmlStrcmp(name, "KeypointXMLList")==0) {
            ret = xmlTextReaderRead(reader);
            while (ret == 1) {
                name = xmlTextReaderConstName(reader);

                if (xmlStrcmp(name, "XDim")==0 && xmlTextReaderNodeType(reader)==1) {
                    ret = xmlTextReaderRead(reader);
                    if (xmlTextReaderHasValue(reader))
                        imageSize.x = atoi(xmlTextReaderConstValue(reader));
                } else if (xmlStrcmp(name, "YDim")==0 && xmlTextReaderNodeType(reader)==1) {
                    ret = xmlTextReaderRead(reader);
                    if (xmlTextReaderHasValue(reader))
                        imageSize.y = atoi(xmlTextReaderConstValue(reader));
                } else if (xmlStrcmp(name, "ImageFile")==0 && xmlTextReaderNodeType(reader)==1) {
                    ret = xmlTextReaderRead(reader);
                    if (xmlTextReaderHasValue(reader) && xmlTextReaderNodeType(reader)==3)
                        imageFilename = String(xmlTextReaderConstValue(reader));

                } else
                if (xmlStrcmp(name, "Arr")==0 && xmlTextReaderNodeType(reader)==1) {
                    ret = xmlTextReaderRead(reader);
                    while (ret == 1) {
                        name = xmlTextReaderConstName(reader);
                        if (xmlStrcmp(name, "KeypointN")==0 && xmlTextReaderNodeType(reader)==1) {
                            KeypointN* kp = KeypointN_new0();
                            ret = xmlTextReaderRead(reader);
                            while (ret == 1) {
                                name = xmlTextReaderConstName(reader);
                                if (xmlStrcmp(name, "X")==0 && xmlTextReaderNodeType(reader)==1) {
                                    ret = xmlTextReaderRead(reader);
                                    if (xmlTextReaderHasValue(reader))
                                        kp.pos.x = atof(xmlTextReaderConstValue(reader));
                                } else if (xmlStrcmp(name, "Y")==0 && xmlTextReaderNodeType(reader)==1) {
                                    ret = xmlTextReaderRead(reader);
                                    if (xmlTextReaderHasValue(reader))
                                        kp.pos.y = atof(xmlTextReaderConstValue(reader));
                                } else if (xmlStrcmp(name, "Scale")==0 && xmlTextReaderNodeType(reader)==1) {
                                    ret = xmlTextReaderRead(reader);
                                    if (xmlTextReaderHasValue(reader))
                                        kp.scale = atof(xmlTextReaderConstValue(reader));
                                } else if (xmlStrcmp(name, "Orientation")==0 && xmlTextReaderNodeType(reader)==1) {
                                    ret = xmlTextReaderRead(reader);
                                    if (xmlTextReaderHasValue(reader))
                                        kp.orientation = atof(xmlTextReaderConstValue(reader));
                                } else if (xmlStrcmp(name, "Dim")==0 && xmlTextReaderNodeType(reader)==1) {
                                    ret = xmlTextReaderRead(reader);
                                    if (xmlTextReaderHasValue(reader))
                                        dim = (atoi(xmlTextReaderConstValue(reader)));
                                } else if (xmlStrcmp(name, "Descriptor")==0 && xmlTextReaderNodeType(reader)==1) {
//                                    KeypointN_CreateDescriptor(kp);
                                    int i=0;
                                    kp.descriptor.resize(dim);
                                    while (ret == 1) {
                                        name = xmlTextReaderConstName(reader);
                                        if (xmlStrcmp(name, "int")==0 && xmlTextReaderNodeType(reader)==1) {
                                            if (xmlTextReaderNodeType(reader)==1) {
                                                ret = xmlTextReaderRead(reader);
                                                if (xmlTextReaderHasValue(reader))
                                                    kp.descriptor[i++] = atof(xmlTextReaderConstValue(reader));
                                            }
                                        }
                                        if (strcmp(name, "Descriptor")==0 && xmlTextReaderNodeType(reader)==15) {
                                            break;
                                        }
                                        ret = xmlTextReaderRead(reader);
                                    }
                                }
                                if (strcmp(name, "Descriptor")==0 && xmlTextReaderNodeType(reader)==15) {
                                    break;
                                }
                                ret = xmlTextReaderRead(reader);
                            }
                            keypoints.push_back(kp);
                            // KeypointXMLList_Add(kl, kp);
                            if (strcmp(name, "KeypointN")==0 && xmlTextReaderNodeType(reader)==15) {
                                break;
                            }
                        }
                        ret = xmlTextReaderRead(reader);
                    }
                    if (strcmp(name, "Arr")==0 && xmlTextReaderNodeType(reader)==15) {
                        break;
                    }
                }
                ret = xmlTextReaderRead(reader);
            }
            if (strcmp(name, "KeypointXMLList")==0 && xmlTextReaderNodeType(reader)==15) {
                break;
            }
        }
        ret = xmlTextReaderRead(reader);
    }
    xmlFreeTextReader(reader);
  }
  xmlCleanupParser();
  return;
}

} // namespace

