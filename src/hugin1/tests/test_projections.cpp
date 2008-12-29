#include "PT/PanoToolsInterface.h"

using namespace PT;
using namespace PTools;

int main(int argc, char ** argv)
{
    SrcPanoImage src;
    src.setHFOV(100);
    src.setSize(vigra::Size2D(101,101));

    PanoramaOptions opts;

    bool failed = false;

    double eps = 1e-8;

    for (int srcp=0; srcp < (int) SrcPanoImage::EQUIRECTANGULAR; srcp++) {
        src.setProjection( (SrcPanoImage::Projection) srcp);
        for (int destp=0; destp < (int) PanoramaOptions::PANINI; destp++) 
        {
            opts.setProjection((PanoramaOptions::ProjectionFormat) destp);
            opts.setHFOV(100, false);
            opts.setWidth(101,false);
            opts.setHeight(101);

            PTools::Transform ptTrans;
            ptTrans.createTransform(src, opts);
            PTools::Transform ptInvTrans;
            ptInvTrans.createInvTransform(src, opts);

            for (int x=0; x < 101; x+=25) {
                for (int y=0; y < 101; y+=25) {
                    FDiff2D srcC(x,y);
                    FDiff2D destC;
                    ptTrans.transformImgCoord(destC, srcC);
                    FDiff2D srcC2;
                    ptInvTrans.transformImgCoord(srcC2, destC);
                    double errx = srcC.x - srcC2.x;
                    double erry = srcC.y - srcC2.y;
                    if (fabs(errx) > eps || fabs(erry) > eps) {
                        failed=true;
                        fprintf(stderr, "%d -> %d, failed: %f,%f (%f, %f) != %f,%f, error: %f,%f\n", srcp, destp, srcC.x, srcC.y, destC.x, destC.y, srcC2.x, srcC2.y, errx, erry);
                    }
                }
            }
        }
    }
    if (failed) {
        return 1;
    }
    return 0;
}

