/*
' PTLens, Copyright (C) 2004, Thomas Niemann
' email: thomasn@epaperpress.com, subject=ptlens
'
' Additional developers:
'  Tim Jacobs twjacobs@gmail.com     
'    conversion to ANSI C
'
'  Pablo d'Angelo pablo.dangelo@web.de
'    code cleanup, removed global variables, standalone .c file
'
' This program is free software; you can redistribute
' it and/or modify it under the terms of the GNU
' General Public License as published by the Free
' Software Foundation; either version 2 of the
' License, or (at your option) any later version.
'
' This program is distributed in the hope that it will
' be useful, but WITHOUT ANY WARRANTY; without even
' the implied warranty of MERCHANTABILITY or FITNESS
' FOR A PARTICULAR PURPOSE. See the GNU General Public
' License for more details.
'
' You should have received a copy of the GNU General
' Public License along with this program; if not,
' write to the Free Software Foundation, Inc., 675
' Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef PTLENSDB_H
#define PTLENSDB_H

#if defined(__cplusplus) && __cplusplus
extern "C" {
#endif

/// version of the API. 
#define PTLDB_VERSION 1

// all these constants are not so nice.
#define PTLDB_MAX_FILES  100
#define PTLDB_MAX_PATH 512
#define PTLDB_MAX_NAME_LEN 100
#define PTLDB_MAX_COEFFS 10000

// define the database structures
typedef struct PTLDB_grp{
    struct PTLDB_grp *nextGrp;
    char name[256];
}PTLDB_GrpNode;

typedef struct PTLDB_cam{
    struct PTLDB_cam *nextCam;
    struct PTLDB_cam *firstModel;
    long numMake;
    long numModel;
    PTLDB_GrpNode *group;
    char menuMake[256];
    char menuModel[256];
    char exifMake[256];
    char exifModel[256];
    float multiplier;
}PTLDB_CamNode;

typedef struct PTLDB_lens{
    struct PTLDB_lens *nextLns;
    struct PTLDB_lens *firstLns;
    long numLens;
    char group[256];
    char menuLens[256];
    float converterFactor;
    long converterDetected;
    long coefLB;
    long coefUB;
    long vigCoefLB;
    long vigCoefUB;
    long tcaCoefLB;
    long tcaCoefUB;
    float multiplier;
}PTLDB_LnsNode;


// define other structures
typedef struct
{
    float f;
    float a;
    float b;
    float c;
}PTLDB_CoefType;

// define other structures
typedef struct
{
    float f;    ///< focal length
    float k;    ///< f stop
    float coef[4];
}PTLDB_VigCoefType;

typedef struct
{
    float f;    ///< focal length
    float coefRed[4];
    float coefBlue[4];
}PTLDB_TCACoefType;


/*
typedef struct {
	char inputDir[256];
	char stitcher[256];
	char viewerPath[256];
	char *profilePath;
	char suffix[32];
	char menuMake[256];
	char menuModel[256];
	char menuLens[256];
	int format;
	int jpegQuality;
	int interpolation;
	int beep;
	int resize;
	int converterDetected;
}prefType;
*/

typedef struct
{
//    double hfov;
    double a;
    double b;
    double c;
    double d;
} PTLDB_RadCoef;

/** \brief This struct hold information about an image, and
 *         is used to retrieve the coefficients for the current setup
 */
typedef struct
{
    PTLDB_CamNode *camera;
    PTLDB_LnsNode *lens;
    double focalLength;
    double aperture;
    unsigned width;
    unsigned height;
    int converterDetected;  ///< 1: a converter has been detected
    int resize;             ///< 1: resize image to avoid black borders
} PTLDB_ImageInfo;

/** database with all lenses and cameras */
typedef struct {
//    CamNode *pCurCam;
    PTLDB_CamNode *pCamHdr;
    PTLDB_LnsNode *pLnsHdr;
//    PTLDB_GrpNode *pCurCamGrp;
    unsigned fileIndex;
    // FIXME dangelo: this is not nice... but I'm lazy and not used to write C code.
    char fileList[PTLDB_MAX_FILES][PTLDB_MAX_PATH];
    long coefIndex;
    long vigCoefIndex;
    long tcaCoefIndex;
    PTLDB_CoefType coef[PTLDB_MAX_COEFFS];
    PTLDB_VigCoefType vigCoef[PTLDB_MAX_COEFFS];
    PTLDB_TCACoefType tcaCoef[PTLDB_MAX_COEFFS];
} PTLDB_DB;



/**
 * \brief Read lens calibration data from profile files.
 * \return pointer to database if successfull, NULL otherwise
 * \param profileFile filename of main profile (profile.txt)
 *
 *****************************************************/
PTLDB_DB * PTLDB_readDB(const char * profileFile);


/**
 * \brief Free database
 * \param db  Pointer to database
 *
 *****************************************************/
void PTLDB_freeDB(PTLDB_DB * db);

/**
 * \brief Free a lens node linked list
 * \param lens  Pointer to first lens
 *
 *****************************************************/
void PTLDB_freeLnsList(PTLDB_LnsNode * lens);

 /** \brief Look for a camera in the linked list of supported cameras.
 * \return pointer to camera node if successfull, NULL otherwise
 * \param db        Pointer to database
 * \param exifMake  The camera make as found in the jpeg EXIF data
 * \param exifModel The camera model as found in the jpeg EXIF data
 *
 *****************************************************/
PTLDB_CamNode *PTLDB_findCamera(PTLDB_DB * db, const char *exifMake, const char *exifModel);

/**
 * \brief Look for a lens in the linked list of supported lenses.  
 *        The lens is checked to make sure it has been calibrated
 *        for the camera passed in.
 * \param db        The lens database
 * \param lens      The lens name to look for
 * \param camera    The camera model as found in the jpeg EXIF data
 * \return pointer to the lens node if successful, NULL otherwise
 *
 *****************************************************/
PTLDB_LnsNode *PTLDB_findLens(PTLDB_DB * db, const char *lens, PTLDB_CamNode *camera);
 
/**
 * \brief Create a linked list of all calibrated lenses for
 *        the input camera.
 * \return pointer to the first lens node if lenses found,
 *         NULL otherwise. The list should be freed using
 *         PTLDB_freeLnsList().
 * \param db     Pointer to the database
 * \param camera A pointer to the camera node in the linked list
 *
 *****************************************************/
PTLDB_LnsNode *PTLDB_findLenses(PTLDB_DB * db, PTLDB_CamNode *camera);

/** \brief Display all information read from profile files.
 * \return void.
 */
void PTLDB_printDB(PTLDB_DB * db);

/**
 * \brief Calculate the lens correction coefficients (a,b,c,d) for
 *        a particular focal length of a particular lens by linear
 *        interpolation of known coefficients for the lens.
 * \param image    information about current image
 * \param coef1    pointer to radial distortion coefficients.
 *                 Will contain the coefficients
 * \param 
 * \return 0 if unsuccessful, 1 otherwise.
 *
 *****************************************************/
int PTLDB_getRadCoefs(PTLDB_DB * db, PTLDB_ImageInfo * info, PTLDB_RadCoef * coef1);


/**
 * \brief Calculate the Horizontal Field Of View (HFOV) for
 *        a given focal length, width, height, and multiplier.
 * \return The HFOV.
 * \param thisCamera The current camera
 * \param foc The focal length of the lens when the picture was
 *        taken. Read from EXIF data.
 * \param width width of image
 * \param height height of image
 *
 *****************************************************/
double PTLDB_getHfov(PTLDB_CamNode * thisCamera, double foc, int width, int height);


#if defined(__cplusplus) && __cplusplus
}
#endif

#endif
