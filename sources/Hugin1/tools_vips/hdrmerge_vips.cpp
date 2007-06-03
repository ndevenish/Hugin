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
