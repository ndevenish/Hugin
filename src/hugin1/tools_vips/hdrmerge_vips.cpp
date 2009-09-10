/*
 *  @common Simple test program for experimental vips_hdrmerge (not available
 *          publicly), obseleted by the hugin_hdrmerge tool.
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

#include <unistd.h>

using namespace vips;

int
main (int argc, char **argv)
{
  try
  {
    // 1 is the output image, the other args are input
    if (argc < 2)
      throw VError ("usage: out in1, in2 ...\n");

    std::vector<VImage> ins;
    int nin = argc - 2;
    int w=0, h=0;
    for (int i = 0; i < nin; i++) {
        VImage in (argv[i+2]);
        int nw = in.Xoffset() + in.Xsize();
        int nh = in.Yoffset() + in.Ysize();
        if (w < nw) w = nw;
        if (h < nh) h = nh;
        ins.push_back( in );
    }

    // embed in the output image
    for (int i = 0; i < nin; i++) {
        ins[i] = ins[i].embed (0,
		               ins[i].Xoffset(),
		               ins[i].Yoffset(),
                               w, h);
    }

    // perform the actual merge operations
    VImage out = VImage::hdrmerge(ins);
    out.write (argv[1]);
  }
  catch (VError err)
  {
    err.perror (argv[0]);
  }

  return (0);
}
