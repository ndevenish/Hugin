/**********************************************************************
Reads the feature table from "features.txt", copies the features from
the second frame to those of the third frame, writes the features to
"feat2.txt", and writes the new feature table to "ft2.txt".  Then the
eighth feature is overwritten with the fifth feature, and the resulting
table is saved to "ft3.txt".
**********************************************************************/

#include <stdio.h>
#include "klt/klt.h"

int main(int argc, char * argv[])
{
  KLT_FeatureList fl;
  KLT_FeatureHistory fh;
  KLT_FeatureTable ft;
  int i;

  assert(argc == 3);
  ft = KLTReadFeatureTable(NULL, argv[1]);
  fl = KLTCreateFeatureList(ft->nFeatures);
  KLTExtractFeatureList(fl, ft, 1);

  tc = KLTCreateTrackingContext();
  tc->window_width=windowSize;
  tc->window_height=windowSize;
  KLTUpdateTCBorder(tc);
  
  KLTPrintTrackingContext(tc);

  img1 = pgmReadFile(argv[2], NULL, &ncols, &nrows);


}

