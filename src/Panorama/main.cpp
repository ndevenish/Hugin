// main.cpp
//
// Purpose:
//
//
//
// Pablo d'Angelo <pablo@mathematik.uni-ulm.de>
// Last change: Time-stamp: <13-Mar-2003 01:41:05 pablo@island>
//
//

#include "Panorama.h"

using namespace std;
using namespace PT;

int main()
{
  Panorama pano;
  
  pano.addImage("blubb.jpg");
  pano.addImage("blubb2.jpg");
  
  pano.addControlPoint(1,100.0,100.0,
                       2,200.0,200.0);
  pano.addControlPoint(1,300.0,300.0,
                       2,200.0,200.0);
  pano.createOptimizerScript(cout);
  
  
  return 0;
}
