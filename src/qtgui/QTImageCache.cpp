// -*- c-basic-offset: 4 -*-

/** @file QTImageCache.cpp
 *
 *  @brief implementation of QTImageCache Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <string>
#include "utils.h"
#include "QTImageCache.h"

QTImageCache * QTImageCache::instance = 0;


QTImageCache::QTImageCache()
{
}

QTImageCache::~QTImageCache()
{
    delete instance;
    instance = 0;
}


QTImageCache & QTImageCache::getInstance()
{
  if (!instance) {
    instance = new QTImageCache();
  }
  return *instance;

}


QImage & QTImageCache::getImage(const std::string & filename)
{
  std::map<std::string, QImage>::iterator it;
  it = images.find(filename);
  if (it != images.end()) {
    return it->second;
  } else {
    QImage image;
    if (! image.load(filename.c_str())) {
      DEBUG_ERROR("Can't load image: " << filename);
      throw 0;
    }
    images[filename] = image;
    return images[filename];
  }
}
