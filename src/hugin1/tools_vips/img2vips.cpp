/*
 *  @common Convert an image to a vips file, keeping the canvas position.
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
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

/* compile with

	g++ -g -Wall blend.cc `pkg-config vipsCC-7.11 --cflags --libs`

 */

#include <vips/vips>
#include <vips/vips.h>

#include <vigra/impex.hxx>

#include <string>
#include <common/utils.h>
#include <unistd.h>

using namespace vips;
using namespace std;
using namespace utils;
using namespace vigra;

void
convert_file (const char *filename)
{
  ImageImportInfo inFile(filename);
  int bands = inFile.numBands();
  if (inFile.getPixelType() == std::string("FLOAT") && bands == 4) 
  {
      cout << "Reading " << filename << " ... " << std::flush;
      BasicImage<TinyVector<float,4> > img(inFile.size());
      importImage(inFile, destImage(img));
      cout << "  done. " << std::flush;
      void *data = &(img(0,0));
      VImage viout (data, img.size().x, img.size().y, bands, VImage::FMTFLOAT);
      // copy position field
      viout.image()->Xoffset = inFile.getPosition().x;
      viout.image()->Yoffset = inFile.getPosition().y;
      std::string outfile(utils::stripExtension(filename));
      outfile += ".v";
      cout << " Writing " << outfile << std::endl;
      viout.write (outfile.c_str());
  } else {
      throw VError("Fatal error: unsupported input file format (only RGB float with alpha supported right now)");
  }
}

int
main (int argc, char **argv)
{
  try
  {
    // 1 is the output image, the other args are input
    if (argc < 1)
      throw VError ("usage: in1 (in2) ...\n");

    int nin = argc - 1;
    for (int i = 0; i < nin; i++) {
        convert_file (argv[i + 1]);
    }
  }
  catch (VError err)
  {
    err.perror (argv[0]);
  }

  return (0);
}
