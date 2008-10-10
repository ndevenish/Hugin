#include "CelesteGlobals.h"
#include <stdlib.h>

// GLOBAL GLOBALS

bool kSaveFilter = true;  // in case of multiple files, we save GFs only once
bool kVerbosity  = false; // whether to output any messages or not

// LOCAL GLOBALS

// -r : Radius of filter
int gRadius = 20;

// -s : Sigma modulator			
float gS = 18.0;

// -a : Number of angles				
int gA = 8;

// -f : Number of frequencies				
int gF = 6;

// -l : Lower bound of frequency				
float gL = 0.1;

// -u : Upper bound of frequency
float gU = 1.8;	

// Number of fiducials			
int gNumLocs = 0;

// Co-ordinates of fiducials			
int **gLocations = NULL;

// Spacing between fiducial points
int spacing = (gRadius * 2) + 1;

int resize_dimension = 820;
