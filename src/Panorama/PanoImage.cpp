// -*- c-basic-offset: 4 -*-
// PanoImage.cpp
//
// Pablo d'Angelo <pablo@mathematik.uni-ulm.de>
// Last change: Time-stamp: <10-Aug-2003 02:17:50 pablo@island.wh-wurm.uni-ulm.de>
//
//

#include <stdio.h>
#include <math.h>
//#include <setjmp.h>

#include <stdexcept>

#include <jhead/jhead.h>
#include <PT/PanoImage.h>
#include <PT/Panorama.h>
#include <common/utils.h>
#include "hugin/ImageCache.h"

using namespace PT;
using namespace std;

#if 0
QDomElement ImageOptions::toXML(QDomDocument & doc)
{
    QDomElement root = doc.createElement("options");
    root.setAttribute("feather_width", featherWidth);
    root.setAttribute("ignore_frame_width", ignoreFrameWidth);
    root.setAttribute("morph", morph);
    root.setAttribute("optimize_yaw", optimizeYaw);
    root.setAttribute("optimize_roll", optimizeRoll);
    root.setAttribute("optimize_pitch", optimizePitch);
    root.setAttribute("optimize_HFOV", optimizeFOV);
    root.setAttribute("optimize_a", optimizeA);
    root.setAttribute("optimize_b", optimizeB);
    root.setAttribute("optimize_c", optimizeC);
    root.setAttribute("optimize_d", optimizeD);
    root.setAttribute("optimize_e", optimizeE);

    return root;
}


void ImageOptions::setFromXML(const QDomNode & node)
{
    DEBUG_DEBUG("ImageOptions::setFromXML");
    Q_ASSERT(node.nodeName() == "options");
    QDomNamedNodeMap attrs = node.attributes();
    featherWidth = getAttrib(attrs, "feather_width").toUInt();
    ignoreFrameWidth = getAttrib(attrs, "ignore_frame_width").toUInt();
    morph = getAttrib(attrs, "morph").toUInt() != 0;
}

#endif

PanoImage::PanoImage(const std::string &filename, int width, int height,
                     int lens)
    : filename(filename),
      height(height),
      width(width)
{
    options.lensNr = lens;
}

#if 0
PanoImage::PanoImage(const std::string &filename)
    : filename(filename)
{
    DEBUG_DEBUG("PanoImage ctor for " << filename);
    init();
    readImageInformation();
}
#endif

void PanoImage::init()
{
    height = 0;
    width = 0;
}


#if 0
PanoImage::PanoImage(QDomNode & node)
{
    setFromXML(node);
}
#endif


PanoImage::~PanoImage()
{
    DEBUG_DEBUG("PanoImage dtor, image= " << filename);
}

#if 0
void PanoImage::setFromXML(QDomNode & elem)
{
    assert(elem.nodeName() == "image");
    QDomNamedNodeMap attrs = elem.attributes();
    filename = getAttrib(attrs, "file");
    isLandscape = getAttrib(attrs, "landscape").toUInt() != 0;
    height = getAttrib(attrs, "height").toUInt();
    width = getAttrib(attrs, "width").toUInt();

    qDebug("PanoImage::setFromXML, file %s", filename.ascii());

    options.setFromXML(elem.namedItem("position"));
    lens.setFromXML(elem.namedItem("lens"));
    options.setFromXML(elem.namedItem("options"));
}


QDomElement PanoImage::toXML(QDomDocument & doc)
{
    QDomElement root = doc.createElement("image");
    root.setAttribute("file", filename);
    root.setAttribute("landscape", isLandscape);
    root.setAttribute("height",height);
    root.setAttribute("width",width);
    root.appendChild(lens.toXML(doc));
    root.appendChild(position.toXML(doc));
    root.appendChild(options.toXML(doc));
    return root;
}
#endif

bool PanoImage::readImageInformation()
{
  std::string::size_type idx = filename.rfind('.');
  if (idx == std::string::npos) {
      DEBUG_DEBUG("could not find extension in filename");
      return false;
  }
  std::string ext = filename.substr( idx+1 );

  // this depends on wxWindow
  wxImage * image = ImageCache::getInstance().getImage(filename);
  width = image->GetWidth();
  height = image->GetHeight();

  /*
  try {
      // find another platform independant way to read the image info..
      Magick::Image img;
      img.ping(filename.c_str());
      width = img.baseColumns();
      height = img.baseRows();
  } catch( Magick::Exception &e ) {
      DEBUG_NOTICE("error while reading file information for " << filename
                   << ": " << e.what());
      return false;
  }
  */
//  isLandscape = (width > height);
  return true;
}
