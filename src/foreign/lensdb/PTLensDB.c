/*
' PTLens, Copyright (C) 2004, Thomas Niemann
' email: thomasn@epaperpress.com, subject=ptlens
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <math.h>

#include "PTLensDB.h"

//#include "string_utils.h"


/** \file modDB.c
 * \brief Functions to read lens profile information from profile
 * files.
 * \author Tim Jacobs (ported from Thoms Niemann's PTLens code)
 * \date 2004.11.13
 *
 * Full description here.
 */


//extern prefType pref;

/** \brief Measurement of 35mm film format diagonal
 *
 * if assuming 23.3 x 35 (phil's web site) then
 *    diag35 = 42.046284
 * if assuming 36 x 24
 *    diag35 = 43.266615
 */
static const double diag35 = 43.266615;


// various static members required during parsing.
/** \brief Current profile file name being read from
 */
static char *fileName;
/** \brief Holds the value of the current line of being read in profile.txt.
 */
static int line;
/** \brief String to hold the right hand side of a parsed line.
 */
static char rhs[256];
/** \brief State value for parsing profile files
 */
static const long PROCESS_VOID = 0;
/** \brief State value for parsing profile files
 */
static const long PROCESS_CAM = 1;
/** \brief State value for parsing profile files
 */
static const long PROCESS_LENS = 2;

/**
 * \brief Copy len characters from srcStr to destStr
 *        starting from offset.
 * \param destStr The destination string
 * \param srcStr The source string
 * \param offset The offset in srcStr from which to start copying
 * \param len The number of characters to copy to destStr
 *
 *****************************************************/
static void substr(char *destStr, const char *srcStr, int offset, int len)
{
	int i;
	for(i = 0; i < len && srcStr[offset + i] != '\0'; i++)
		destStr[i] = srcStr[i + offset];
	destStr[i] = '\0';
}


// DGSW - Unused function
#if 0

#define NUL '\0'

static char *stristr(const char *String, const char *Pattern)
{
      char *pptr, *sptr, *start;

      for (start = (char *)String; *start != NUL; start++)
      {
            /* find start of pattern in string */
            for ( ; ((*start!=NUL) && (toupper(*start) != toupper(*Pattern))); start++)
                  ;
            if (NUL == *start)
                  return NULL;

            pptr = (char *)Pattern;
            sptr = (char *)start;

            while (toupper(*sptr) == toupper(*pptr))
            {
                  sptr++;
                  pptr++;

                  /* if end of pattern then pattern was found */

                  if (NUL == *pptr)
                        return (start);
            }
      }
      return NULL;
}

#endif

/**
 * \brief Find the last occurance of str1 in str2
 * \return The position in str2 of the last occurance of str1.
 *         0 if not found
 * \param str2 The string to search
 * \param str1 The substring to look for in str1
 *
 *****************************************************/
static long instrrev(const char *str2, const char *str1)
{
    int length = (int)strlen(str2);
    if (length > 0)
    {
        const char *start = str2;
        const char *back = &str2[length];
        while((*(back-1) != *str1) && back != start)
        {
            --back;
            length--;
        }
    }
    return(length);
}


/**
 * \brief Trim white space from str
 * \return The input string (str) minus any white space
 *         from either end
 * \param str The input string
 *
 *****************************************************/
static char *trim(char *str)
{
	int length = (int)strlen(str);
	if (length > 0)
	{
		char *data = str;
     	char *front = str;
     	char *back = &str[length];
     	while(isspace(*front))
     		++front;
     	//while(isspace(*(back-1)))
        while( front < back && isspace(*(back-1)))
     		--back;
     	while(front < back)
     		*(data++) = *(front++);
     	*data = 0;
     }
     return(str);
}




/**
 * \brief If key matches the beginning of str, copy the value
 *        (ie the rest of the string minus key) to the global.
 *        variable rhs.
 * \return 0 if key not found, 1 otherwise
 * \param str The string possibly containing key and value
 * \param key The key to look for
 *
 *****************************************************/
static int lhs(const char *str, const char *key)
{
    int ret_value = 0;
    if (strncmp(str, key, strlen(key)) != 0)
    {
        ret_value = 0;
    } else {
        substr(rhs, str, (int)strlen(key), (int)(strlen(str)-strlen(key)));
        ret_value = 1;
    }
    return(ret_value);
}




/**
 * \brief Sort the global array of coefficients, coef[],
 *        within a region defined by lb and ub.
 * \param lb The lower bound (inclusive) for the sort region
 * \param ub The upper bound (inclusive) for the sort region
 *
 *****************************************************/
static void sortCoefs(PTLDB_DB * db, long lb, long ub)
{
    long i, j, t;
    PTLDB_CoefType c;

    // optimized bubble sort
    // t is an index to the last unsorted number
    t = ub;
    while (t > 0)
    {
        j = t;
        t = 0;
        for (i = lb; i < j; i++)
        {
            if (db->coef[i].f > db->coef[i + 1].f)
            {	// out of order, so bubble-down
                c = db->coef[i];
                db->coef[i] = db->coef[i + 1];
                db->coef[i + 1] = c;
                // remember point of last swap
                t = i;
            }
        }
    }
}




/**
 * \brief Insert a camera into the linked list of cameras
 *        such that cameras are alphabetized by make within
 *        models and models are in the same order as in 
 *        "profile.txt".
 * \param pCam The camera to insert
 *
 *****************************************************/
static void insertCam(PTLDB_DB * db, PTLDB_CamNode *pCam)
{
    PTLDB_CamNode *pCamTmp;

    if (db->pCamHdr == NULL)	//If first camera, set pCamHdr
    {
        db->pCamHdr = pCam;
    } else {				//Else find where to insert the new Camera Node
        pCamTmp = db->pCamHdr;
        while (1)
        {
            if (pCamTmp->nextCam == NULL)	//Only one node so far
            	break;
            if (pCamTmp->nextCam->menuMake > pCam->menuMake) 
            	break;
            pCamTmp = pCamTmp->nextCam;
        }
//Insert the new Camera Node
        if (pCamTmp->menuMake > pCam->menuMake)
        {
            // place before first make on list
            pCam->nextCam = pCamTmp;
            db->pCamHdr = pCam;
        } else {
            // insert after pCamTmp
            pCam->nextCam = pCamTmp->nextCam;
            pCamTmp->nextCam = pCam;
        }
    }
    //printf("Inserted camera: %-35s\n%", pCam->menuModel);
}




/**
 * \brief Insert a lens into the linked list of lenses
 *        such that lenses belonging to the same group
 *        are together.
 * \param pLns The lens to insert
 *
 *****************************************************/
static void insertLns(PTLDB_DB * db, PTLDB_LnsNode *pLns)
{
    PTLDB_LnsNode *pLnsTmp;
    long i;
    
    // adjust focal length so it looks like converter detected
    if (pLns->converterFactor != 1.0 && pLns->converterDetected == 0)
    {
        for (i = pLns->coefLB; i <= pLns->coefUB; i++)
        {
            db->coef[i].f = db->coef[i].f * pLns->converterFactor;
            pLns->converterDetected = 1;
        }
    }
    
    // find group/lens insertion point
    // find last group match and append
    if (db->pLnsHdr == NULL)
    {
        db->pLnsHdr = pLns;
    } else {
        // find first reference to group
        pLnsTmp = db->pLnsHdr;
        while (1)
        {
            if (pLnsTmp->nextLns == NULL)
            	break;
            if (pLnsTmp->group == pLns->group)
            	break;
            pLnsTmp = pLnsTmp->nextLns;
        }

        // find last reference to group
        if (pLnsTmp->group == pLns->group)
        {
            while (1)
            {
                if (pLnsTmp->nextLns == NULL)
                	break;
                if (pLnsTmp->nextLns->group != pLns->group)
                	break;
                pLnsTmp = pLnsTmp->nextLns;
            }
        }

        // insert after pLnsTmp
        pLns->nextLns = pLnsTmp->nextLns;
        pLnsTmp->nextLns = pLns;
    }
    //printf("Inserted Lens: %-40s %-10s\n", pLns->menuLens, pLns->group);
}




/**
 * \brief Utility function to print error messages from parsing
 *        the profile files.
 * \return The value of b passed in
 * \param b Indicates if an error message needs to be printed
 * \param s The string identifying a missing element in the profile file
 *
 *****************************************************/
static int check(int b, char *s)
{
    if (b == 0)
        printf("File %s, line %d, missing: %s\n", fileName, line, s);

    return(b);
}


/**
 * \brief Function to parse profile files. The Lens, Camera, and
 *        group linked lists are created here, and the coef[] array
 *        is populated.
 * \return 1 if line parsed successfully, 0 otherwise
 * \param s A line from a profile file
 *
 *****************************************************/
static long processDbFile(PTLDB_DB * db, const char *s)
{
    long ret_value=1;
    static long state;
    static PTLDB_CamNode *pCam;
    static PTLDB_LnsNode *pLns;  
    static int lnsMenuLens;
    static int lnsCalABC;
    static int lnsMultiplier;
    static int lnsGroup;
    static int lnsConverterFactor;
    static int lnsConverterDetected;

    static int camMenuMake;
    static int camMenuModel;
    static int camExifMake;
    static int camExifModel;
    static int camMultiplier;
    static int camGroup;

    PTLDB_CoefType * ccoef;
    PTLDB_TCACoefType * tccoef;
    PTLDB_VigCoefType * vccoef;

    int n;

    if (strlen(s) == 0)
    	return(ret_value);
    if (strncmp(s, "#", 1) == 0)
    	return 1;

    if (lhs(s, "begin") == 1)
    {
        if (state != PROCESS_VOID) {
            printf( "File %s line %d invalid statement %s\n", fileName, line, s);
            return 0;
        }
        if (strcmp(trim(rhs), "camera") == 0)
        {
            state = PROCESS_CAM;
            pCam = (PTLDB_CamNode *)malloc(sizeof(PTLDB_CamNode));
    		pCam->nextCam = NULL;
			pCam->group = NULL;
            camMenuMake = 0;
            camMenuModel = 0;
            camExifMake = 0;
            camExifModel = 0;
            camGroup = 0;
            camMultiplier = 0; 
        } else if (strcmp(trim(rhs), "lens") == 0)
        {
            state = PROCESS_LENS;
            pLns = (PTLDB_LnsNode *)malloc(sizeof(PTLDB_LnsNode));
            pLns->nextLns = NULL;
            pLns->coefLB = -1;
            pLns->coefUB = -1;
            pLns->vigCoefLB = -1;
            pLns->vigCoefUB = -1;
            pLns->tcaCoefLB = -1;
            pLns->tcaCoefUB = -1;
            pLns->converterFactor = 1.0;
            lnsMenuLens = 0;
            lnsGroup = 0;
            lnsMultiplier = 0;
            lnsCalABC = 0;
            lnsConverterFactor = 0;
            lnsConverterDetected = 0;
        }
    } else if (lhs(s, "end") == 1){
        if (state == PROCESS_CAM)
//Insert a camera node
        {
            if (check(camMenuMake, "menu_make") == 0) return 0;
            if (check(camMenuModel, "menu_model") == 0) return 0;
            if (check(camExifMake, "exif_make") == 0) return 0;
            if (check(camExifMake, "exif_model") == 0) return 0;
            if (check(camMultiplier, "multiplier") == 0) return 0;
            if (check(camGroup, "group") == 0) return 0;
            insertCam(db, pCam);
//Insert a lens node
        } else if (state == PROCESS_LENS){
            if (check(lnsMenuLens, "menu_lens") == 0) return 0;
            if (check(lnsCalABC, "cal_abc") == 0) return 0;
            if (check(lnsGroup, "group") == 0) return 0;
            if (check(lnsMultiplier, "multiplier") == 0) return 0;
            
            if (lnsConverterFactor == 1)
            {
                if (check(lnsConverterDetected, "converter_detected") == 0)
                    return 0;
            }
            // TODO: dangelo, sort vig and tca coefs ?
            sortCoefs(db, pLns->coefLB, pLns->coefUB);
            insertLns(db, pLns);
        } else {
            printf( "File %s line %d invalid statement %s\n", fileName, line, s);
            return 0;
        }
        
        state = PROCESS_VOID;

    } else if (lhs(s, "menu_make:") == 1) {
        if (state != PROCESS_CAM) {
            printf( "File %s line %d invalid statement %s\n", fileName, line, s);
            return 0;
        }
        strcpy(pCam->menuMake, trim(rhs));
        camMenuMake = 1;
            
    } else if (lhs(s, "menu_model:") == 1) {
        if (state != PROCESS_CAM) {
            printf( "File %s line %d invalid statement %s\n", fileName, line, s);
            return 0;
        }
        strcpy(pCam->menuModel, trim(rhs));
        camMenuModel = 1;

    } else if (lhs(s, "exif_make:") == 1) {
        if (state != PROCESS_CAM) {
            printf( "File %s line %d invalid statement %s\n", fileName, line, s);
            return 0;
        }
        strcpy(pCam->exifMake, trim(rhs));
        camExifMake = 1;
    
    } else if (lhs(s, "exif_model:") == 1) {
        if (state != PROCESS_CAM) {
            printf( "File %s line %d invalid statement %s\n", fileName, line, s);
            return 0;
        }
        strcpy(pCam->exifModel, trim(rhs));
        camExifModel = 1;
    
    } else if (lhs(s, "group:") == 1) {
        PTLDB_GrpNode *pGrp, *p;
        if (state == PROCESS_CAM)
        {
            pGrp = (PTLDB_GrpNode *)malloc(sizeof(PTLDB_GrpNode));
            strcpy(pGrp->name, trim(rhs));
            pGrp->nextGrp = NULL;

            // add new group to end of the groups list
            if (pCam->group == NULL)
            {
                pCam->group = pGrp;
            } else {
                p = pCam->group;
                while (p->nextGrp != NULL)
                    p = p->nextGrp;
                p->nextGrp = pGrp;
            }
            camGroup = 1;
        } else if (state == PROCESS_LENS) {
            strcpy(pLns->group, trim(rhs));
            lnsGroup = 1;
        } else {
            printf( "File %s line %d invalid statement %s\n", fileName, line, s);
            return 0;
        }

    } else if (lhs(s, "multiplier:") == 1) {
        if (state == PROCESS_CAM)
        {
            pCam->multiplier = (float)atof(rhs);
            camMultiplier = 1;
        } else if (state == PROCESS_LENS) {
            pLns->multiplier = (float)atof(rhs);
            lnsMultiplier = 1;
        } else {
            printf( "File %s line %d invalid statement %s\n", fileName, line, s);
            return 0;
        }
        
    } else if (lhs(s, "menu_lens:") == 1) {
        if (state != PROCESS_LENS) {
            printf( "File %s line %d invalid statement %s\n", fileName, line, s);
            return 0;
        }
        strcpy(pLns->menuLens, trim(rhs));
        lnsMenuLens = 1;
    
    } else if (lhs(s, "converter_factor:") == 1) {
        if (state != PROCESS_LENS) {
            printf( "File %s line %d invalid statement %s\n", fileName, line, s);
            return 0;
        }
        pLns->converterFactor = (float)atof(rhs);
        lnsConverterFactor = 1;
    
    } else if (lhs(s, "converter_detected:") == 1) {
        if (state != PROCESS_LENS) {
            printf( "File %s line %d invalid statement %s\n", fileName, line, s);
            return 0;
        }
        pLns->converterDetected = atol(rhs);
        lnsConverterDetected = 1;
    
    } else if (lhs(s, "cal_abc:") == 1) {
        if (state != PROCESS_LENS) {
            printf( "File %s line %d invalid statement %s\n", fileName, line, s);
            return 0;
        }
        if (pLns->coefLB == -1) pLns->coefLB = db->coefIndex;
        pLns->coefUB = db->coefIndex;
        ccoef = &(db->coef[db->coefIndex]);
        n = sscanf(rhs, "%f %f %f %f", &(ccoef->f), &(ccoef->a), &(ccoef->b), &(ccoef->c));
        if ( n != 4) {
            printf( "File %s line %d invalid statement %s\n", fileName, line, s);
            return 0;
        }
        db->coefIndex++;
        if (db->coefIndex == PTLDB_MAX_COEFFS) {
            fprintf(stderr,"FATAL ERROR: too many coefficients read.\n"
                    "Increase PTLDB_MAX_COEFFS in libPTLens.h and recompile program\n");
            exit(1);
        }
        lnsCalABC = 1;
        // vignetting correction parameters
    } else if (lhs(s, "cal_vig:") == 1) {
        if (state != PROCESS_LENS) {
            printf( "File %s line %d invalid statement %s\n", fileName, line, s);
            return 0;
        }
        if (pLns->vigCoefLB == -1) pLns->vigCoefLB = db->vigCoefIndex;
        pLns->vigCoefUB = db->vigCoefIndex;
        vccoef = &(db->vigCoef[db->vigCoefIndex]);
        n = sscanf(rhs, "%f %f %f %f %f %f", &(vccoef->f), &(vccoef->k), &(vccoef->coef[0]), &(vccoef->coef[1]),
                   &(vccoef->coef[2]), &(vccoef->coef[3]));
        if ( n != 6 ) {
            printf( "File %s line %d invalid statement %s\n", fileName, line, s);
            return 0;
        }
        db->vigCoefIndex++;
        if (db->vigCoefIndex == PTLDB_MAX_COEFFS) {
            fprintf(stderr,"FATAL ERROR: too many coefficients read.\n"
                    "Increase PTLDB_MAX_COEFFS in libPTLens.h and recompile program\n");
            exit(1);
        }
    } else if (lhs(s, "cal_tca:") == 1) {
        if (state != PROCESS_LENS) {
            printf( "File %s line %d invalid statement %s\n", fileName, line, s);
            return 0;
        }
        if (pLns->tcaCoefLB == -1) pLns->tcaCoefLB = db->tcaCoefIndex;
        pLns->tcaCoefUB = db->tcaCoefIndex;
        tccoef = &(db->tcaCoef[db->tcaCoefIndex]);
        n = sscanf(rhs, "%f %f %f %f %f %f %f %f %f", &(tccoef->f), 
                   &(tccoef->coefRed[0]), &(tccoef->coefRed[1]), &(tccoef->coefRed[2]), &(tccoef->coefRed[3]),
                   &(tccoef->coefBlue[0]), &(tccoef->coefBlue[1]), &(tccoef->coefBlue[2]), &(tccoef->coefBlue[3]));
        if ( n != 9) {
            printf( "File %s line %d invalid statement %s\n", fileName, line, s);
            return 0;
        }
        db->tcaCoefIndex++;
        if (db->tcaCoefIndex == PTLDB_MAX_COEFFS) {
            fprintf(stderr,"FATAL ERROR: too many coefficients read.\n"
                    "Increase PTLDB_MAX_COEFFS in libPTLens.h and recompile program\n");
            exit(1);
        }
    } else {
        printf( "File %s line %d, ignoring unknown statement: %s\n", fileName, line, s);
    }
    return(ret_value);
}


/**
 * \brief Parse the lines in the master profile file, "profile.txt".
 * \return 1 if line parsed successfully, 0 otherwise
 * \param s A line from "profile.txt"
 *
 *****************************************************/
static long processDbIndex(PTLDB_DB * db, const char *s)
{
    if (strlen(s) == 0)
    	return 1;
    if (strncmp(s, "#", 1) == 0)
    	return 1;

    if (lhs(s, "version:") == 1)
    {
        // rhs contains version
        if (strcmp(trim(rhs), "4.2") != 0)
        {
            printf("Profile version 4.2 required, found version %s\n", rhs);
            return 0;
        }
    } else if (lhs(s, "file:") == 1) {
//    	db.fileList[fileIndex] = (char *)malloc((strlen(rhs) + 1) * sizeof(char));
        strcpy(db->fileList[db->fileIndex], trim(rhs));
        db->fileIndex++;
    } else {
        fprintf(stderr, "File %s, line %d, invalid statement: %s\n", fileName, line, s);
        return 0;
    }
    return 1;
}




/**
 * \brief Read lens calibration data from profile files.
 * \return pointer to database if successfull, NULL otherwise
 * \param profileFile filename of main profile (profile.txt)
 *
 *****************************************************/
PTLDB_DB * PTLDB_readDB(const char * profileFile)
{
    PTLDB_CamNode *pCam, *firstModel;
    PTLDB_LnsNode *pLns, *firstLns;
    char prevMake[PTLDB_MAX_NAME_LEN], prevModel[PTLDB_MAX_NAME_LEN], prevGroup[PTLDB_MAX_NAME_LEN];
    FILE *file;
    char * fileName;
    char dir[PTLDB_MAX_PATH];			//directory path to profile files
    char nextFile[PTLDB_MAX_PATH];
    char s[1000];			//holds input lines from profile files
    unsigned long i;
    long ret_value;
    int pathlen;
    unsigned line;
    struct stat file_info;

    PTLDB_DB * db = (PTLDB_DB *) malloc(sizeof(PTLDB_DB));
    if (db == NULL) {
        return db;
    }
    db->fileIndex = 0;
    db->pCamHdr = NULL;
    db->pLnsHdr = NULL;
    db->coefIndex = 0;

    //Open profile.txt and read in the list of profile files
    // verify presence of profile.txt
    if (stat(profileFile, &file_info) == -1)
    {
        fprintf(stderr, "Could not stat profile file %s: %s", profileFile, strerror(errno));
        free(db);
        return NULL;
    }
    file = fopen(profileFile, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Could not open profile file %s: %s", profileFile, strerror(errno));
        free(db);
        return NULL;
    }

    // get profile files from profile.txt
    line = 0;
    while (fgets(s,1000,file)!=NULL)
    {
        line = line + 1;
        ret_value = processDbIndex(db, s);
        if (!ret_value)
        {
            fclose(file);
            free(db);
            return NULL;
        }
    }
    fclose(file);

//Read in each profile file
    /* Save the directory part of the profilePath in dir
        * First look for the last '/'. If not found, look for
        * the last '\'. If that's not found assume no path part
        * in the profilePath specification (ie it is in the 
        * current directory)
    */
    pathlen = instrrev(profileFile, "/");
    if (pathlen == 0)
        pathlen = instrrev(profileFile, "\\");
    if (pathlen != 0)
    {
        strncpy(dir, profileFile, pathlen);
        dir[pathlen] = 0;
    } else {
        dir[0] = 0;
    }

    for (i=0; i < db->fileIndex; i++)
    {
        strcpy(nextFile, dir);
        strcat(nextFile, db->fileList[i]);
        fileName = nextFile;
        file = fopen(fileName, "r");
        if (file == NULL)
        {
            fprintf(stderr, "Unable to open profile file: %s\n", fileName);
            free(db);
            return NULL;
        }
        line = 0;
        while (fgets((s),1000,file)!=NULL)
        {
            line = line + 1;
            ret_value = processDbFile(db, trim(s));
            if (! ret_value)
            {
                PTLDB_freeDB(db);
                fclose(file);
                return NULL;
            }
        }
        fclose(file);
    }

//find first camera model
    prevMake[0] = 0;			//Set prevMake to NULL string
    pCam = db->pCamHdr;
	firstModel = NULL;
    while (pCam != NULL)
    {
        if (pCam->menuMake != prevMake)
        {
            strcpy(prevMake, pCam->menuMake);
            prevModel[0] = 0;	//Set prevModel to NULL string
            firstModel = pCam;
        }
        if (strcmp(pCam->menuModel, prevModel) != 0)
            strcpy(prevModel, pCam->menuModel);

        pCam->firstModel = firstModel;
        pCam = pCam->nextCam;
    }

// find first lens
    pLns = db->pLnsHdr;
    prevGroup[0] = 0;			//Set prevGroup to NULL string
	firstLns = NULL;
    while (pLns != NULL)
    {
        if (strcmp(pLns->group, prevGroup) != 0)
        {
            strcpy(prevGroup, pLns->group);
            firstLns = pLns;
        }
        pLns->firstLns = firstLns;
        pLns = pLns->nextLns;
    }

    return db;
}

/* ==================================================================== */

void PTLDB_freeDB(PTLDB_DB * db)
{
    PTLDB_LnsNode *tLns, *pCurLns;
    PTLDB_CamNode *tCam, *pCurCam;
    PTLDB_GrpNode *tGrp, *pCurGrp;

    pCurCam = db->pCamHdr;
    while(pCurCam != NULL) {
        pCurGrp = pCurCam->group;
        while(pCurGrp != NULL) {
            tGrp = pCurGrp;
            pCurGrp = pCurGrp->nextGrp;
            free(tGrp);
        };
        tCam = pCurCam;
        pCurCam = pCurCam->nextCam;
        free(tCam);
    };

    pCurLns = db->pLnsHdr;
    while(pCurLns != NULL) {
        tLns = pCurLns;
        pCurLns = pCurLns->nextLns;
        free(tLns);
    };
}

/* ==================================================================== */

void PTLDB_freeLnsList(PTLDB_LnsNode * pCurLns)
{
    PTLDB_LnsNode *tLns;

    // free lenses
    while(pCurLns != NULL) {
        tLns = pCurLns;
        pCurLns = pCurLns->nextLns;
        free(tLns);
    };
}

/* ==================================================================== */

PTLDB_CamNode *PTLDB_findCamera(PTLDB_DB * db, const char *exifMakeStr, const char *exifModelStr)
{
    PTLDB_CamNode *pCurCam;
    char * exifMake;
    char * exifModel;

    // make a copy, because trim() might modify the strings.
    exifMake = strdup(exifMakeStr);
    exifModel = strdup(exifModelStr);

    pCurCam = db->pCamHdr;
    /*
    while((pCurCam != NULL) && ((strcmp(pCurCam->exifModel, exifModel) != 0) ||
                                        (strcmp(pCurCam->exifMake, exifMake) != 0)))
                                        */
    while((pCurCam != NULL) && ((strcmp(pCurCam->exifModel, trim(exifModel)) != 0) ||
                (strcmp(pCurCam->exifMake, trim(exifMake)) != 0)))
        pCurCam = pCurCam->nextCam;

    free(exifMake);
    free(exifModel);
    return(pCurCam);
}


/**
 * \brief Look for a lens in the list of lens groups compatable with
 *        the camera.
 * \return 0 if unseccessful, 1 otherwise
 * \param thisLens The lens to look for
 * \param thisCamera The camera model as found in the jpeg EXIF data
 *
 *****************************************************/
static int inGroups(PTLDB_LnsNode *thisLens, PTLDB_CamNode *thisCamera)
{
    PTLDB_GrpNode *pCurGroup;

    pCurGroup = thisCamera->group;
    while(pCurGroup != NULL && (strcmp(pCurGroup->name, thisLens->group) != 0))
        pCurGroup = pCurGroup->nextGrp;
    if(pCurGroup == NULL)
        return 0;
    else
        return 1;
}

/* ==================================================================== */

 PTLDB_LnsNode *PTLDB_findLens(PTLDB_DB * db, const char *lens, PTLDB_CamNode *camera)
 {
    PTLDB_LnsNode *pCurLens;

    pCurLens = db->pLnsHdr;
    while(pCurLens != NULL)
    {
            if(inGroups(pCurLens, camera))			//If this lens supports the current camera
                    if(strcmp(pCurLens->menuLens, lens) == 0)	//See if this lens is the one the user used
                            return(pCurLens);
            pCurLens = pCurLens->nextLns;
    }
    return(NULL);	//If here, lens not found (pCurLens is NULL)
 }

 /* ==================================================================== */

 PTLDB_LnsNode *PTLDB_findLenses(PTLDB_DB * db, PTLDB_CamNode *camera)
 {
    PTLDB_LnsNode *pDBLens;
    PTLDB_LnsNode *pCamLens;
    PTLDB_LnsNode *pFirstLens = NULL;

    pCamLens = NULL;

    pDBLens = db->pLnsHdr;
    while(pDBLens != NULL)
    {
        if(inGroups(pDBLens, camera) == 1)			//If this lens supports the current camera
        {
            if(pFirstLens == NULL)
            {
                pCamLens = (PTLDB_LnsNode *)malloc(sizeof(PTLDB_LnsNode));
                pCamLens->nextLns = NULL;
                pCamLens->firstLns = pDBLens->firstLns;
                pCamLens->numLens = pDBLens->numLens;
                strcpy(pCamLens->group, pDBLens->group);
                strcpy(pCamLens->menuLens, pDBLens->menuLens);
                pCamLens->converterFactor = pDBLens->converterFactor;
                pCamLens->converterDetected = pDBLens->converterDetected;
                pCamLens->coefLB = pDBLens->coefLB;
                pCamLens->coefUB = pDBLens->coefUB;
                pCamLens->vigCoefLB = pDBLens->vigCoefLB;
                pCamLens->vigCoefUB = pDBLens->vigCoefUB;
                pCamLens->tcaCoefLB = pDBLens->tcaCoefLB;
                pCamLens->tcaCoefUB = pDBLens->tcaCoefUB;
                pCamLens->multiplier = pDBLens->multiplier;
                pFirstLens = pCamLens;
            } else {
                pCamLens->nextLns = (PTLDB_LnsNode *)malloc(sizeof(PTLDB_LnsNode));
                pCamLens = pCamLens->nextLns;
                pCamLens->nextLns = NULL;
                pCamLens->firstLns = pDBLens->firstLns;
                pCamLens->numLens = pDBLens->numLens;
                strcpy(pCamLens->group, pDBLens->group);
                strcpy(pCamLens->menuLens, pDBLens->menuLens);
                pCamLens->converterFactor = pDBLens->converterFactor;
                pCamLens->converterDetected = pDBLens->converterDetected;
                pCamLens->coefLB = pDBLens->coefLB;
                pCamLens->coefUB = pDBLens->coefUB;
                pCamLens->vigCoefLB = pDBLens->vigCoefLB;
                pCamLens->vigCoefUB = pDBLens->vigCoefUB;
                pCamLens->tcaCoefLB = pDBLens->tcaCoefLB;
                pCamLens->tcaCoefUB = pDBLens->tcaCoefUB;
                pCamLens->multiplier = pDBLens->multiplier;
            }
        }
        pDBLens = pDBLens->nextLns;
    }
    return(pFirstLens);
}

/* ==================================================================== */

void PTLDB_printDB(PTLDB_DB * db)
{
    PTLDB_CamNode *pCurCam;
    PTLDB_LnsNode *pCurLns;
    PTLDB_GrpNode *pCurCamGrp;
    long i;
    //Traverse Camera linked list
    pCurCam = db->pCamHdr;
    printf("\nCamera Information\n");
    while (pCurCam != NULL)
    {
            printf("%-35s %-25s", pCurCam->exifMake, pCurCam->exifModel);
            pCurCamGrp = pCurCam->group;
            while (pCurCamGrp != NULL)
            {
                    printf("%-10s", pCurCamGrp->name);
                    pCurCamGrp = pCurCamGrp->nextGrp;
            }
            printf("\n");
            pCurCam = pCurCam->nextCam;
    }


    //Traverse Lens linked list
    printf("\nLens Information\n");
    printf("%-50s%-10s\n", "Lens", "Group");
    pCurLns = db->pLnsHdr;
    while (pCurLns != NULL)
    {
            printf("\n%-50s%-10s\n", pCurLns->menuLens, pCurLns->group);
            //Traverse coefficients array
            printf("%-10s%-10s%-10s%-10s\n", "f", "a", "b", "c");
            for (i=pCurLns->coefLB; i<=pCurLns->coefUB; i++)
            {
                printf("%5.2f %9.6f %9.6f %9.6f\n", db->coef[i].f, db->coef[i].a,
                       db->coef[i].b, db->coef[i].c);
            }
            printf("%-10s%-10s%-10s%-10s%-10s\n", "f", "k", "a", "b", "c");
            for (i=pCurLns->vigCoefLB; i<=pCurLns->vigCoefUB; i++)
            {
                printf("%5.2f %5.2f %9.6f %9.6f %9.6f %9.6f\n", db->vigCoef[i].f, db->vigCoef[i].k,
                       db->vigCoef[i].coef[0], db->vigCoef[i].coef[1], db->vigCoef[i].coef[2], db->vigCoef[i].coef[3]);
            }
            printf("%-10s%-10s%-10s%-10s%-10s   %-10s%-10s%-10s%-10s\n", "f", "red a", "red b", "red c", "red d",
                   "blue a", "blue b", "blue c", "blue d");
            for (i=pCurLns->tcaCoefLB; i<=pCurLns->tcaCoefUB; i++)
            {
                printf("%5.2f %9.6f %9.6f %9.6f %9.6f   %9.6f %9.6f %9.6f %9.6f\n", db->tcaCoef[i].f,
                       db->tcaCoef[i].coefRed[0], db->tcaCoef[i].coefRed[1], db->tcaCoef[i].coefRed[2], db->tcaCoef[i].coefRed[3],
                       db->tcaCoef[i].coefBlue[0], db->tcaCoef[i].coefBlue[1], db->tcaCoef[i].coefBlue[2], db->tcaCoef[i].coefBlue[3]);
            }

            pCurLns = pCurLns->nextLns;
    }
}

/* ==================================================================== */

double PTLDB_getHfov(PTLDB_CamNode * thisCamera, double foc, int width, int height)
{
    double rad;
    double widthmm;
    double diagpx;
    double diagmm;
    double ret_val;

    // compute sensor width
    diagpx = sqrt(width * width + height * height);
    diagmm = diag35 / thisCamera->multiplier;
    widthmm = width * diagmm / diagpx;

    rad = 2 * atan(widthmm / 2 / foc);
    ret_val = 180 * rad / (4 * atan(1));
    return(ret_val);
}


/**
 * \brief Calculate coefficient "d".
 * \param coef1  lens distortion coefficients, coef1.d is recalculated
 * \param width  image width
 * \param height image height
 * \param resize 0: keep black borders, 1: resize image, stretch to avoid black
 *               borders.
 *
 *****************************************************/
static void coefD(PTLDB_RadCoef *coef1, int width, int height, int resize)
{
    double r_test[2];
    double p, r, r_fixed, fixx, test;
    int test_points, i;
    double a, b, c;

    if (resize == 0)
    {
        coef1->d = 1 - (coef1->a + coef1->b + coef1->c);
        return;
    }

    // truncate black edges
    // algorithm courtesy of Paul Wilkinson, paul.wilkinson@ntlworld.com
    a = coef1->a;
    b = coef1->b;
    c = coef1->c;
    if (width > height)
        p = (double)(width) / (double)(height);
    else
        p = (double)(height) / (double)(width);

    //***************************************************
    //* Set the test point for the far corner.          *
    //***************************************************
    r_test[0] = sqrt(1 + p * p);
    test_points = 1;

    //***************************************************
    //* For non-zero values of a, there are two other   *
    //* possible test points.                           *
    //***************************************************
    if (a != 0.0)
    {
        r = (-b + sqrt(b * b - 3 * a * c)) / (3 * a);
        if (r >= 1 && r <= r_test[0])
        {
            r_test[test_points] = r;
            test_points = test_points + 1;
        }
        r = (-b - sqrt(b * b - 3 * a * c)) / (3 * a);
        if (r >= 1 && r <= r_test[0])
        {
            r_test[test_points] = r;
            test_points = test_points + 1;
        }
    }
    
    //***************************************************
    //* For zero a and non-zero b, there is one other   *
    //* possible test point.                            *
    //***************************************************
    if (a == 0 && b != 0)
    {
        r = -c / (2 * b);
        if (r >= 1 && r <= r_test[0])
        {
            r_test[test_points] = r;
            test_points = test_points + 1;
        }
    }

    //***************************************************
    //* Set the fixed point to Helmut's orginal point.  *
    //***************************************************
    r_fixed = 1.0;
    fixx = a + b + c;

    //***************************************************
    //* Test other possible fixed points.               *
    //***************************************************
    for (i = 0; i <= test_points - 1; i++)
    {
        r = r_test[i];
        test = r * (c + r * (b + r * a));
        if (test > fixx)
        {
            fixx = test;
            r_fixed = r;
        }
    }
    coef1->d = 1 - fixx;
}

/* ==================================================================== */

int PTLDB_getRadCoefs(PTLDB_DB * db, PTLDB_ImageInfo * info, PTLDB_RadCoef * coef1)
{
    long lb;
    long ub;
    long i;
    double ratio;
    double f;

    f = info->focalLength;
    if (info->converterDetected)
        f = f * info->lens->converterFactor;

    lb = info->lens->coefLB;
    ub = info->lens->coefUB;

    // force focal length to be legal
    if (f < db->coef[lb].f)
    {
        if (f < 0.9 * db->coef[lb].f)
            return 0;
        f = db->coef[lb].f;
    }
    if (f > db->coef[ub].f)
    {
        if (f > 1.1 * db->coef[ub].f)
            return 0;
        f = db->coef[ub].f;
    }

    // calculate hfov
//    coef1.hfov = getHfov(f);

    // calculate a,b,c
    if (lb == ub)
    {
        coef1->a = db->coef[lb].a;
        coef1->b = db->coef[lb].b;
        coef1->c = db->coef[lb].c;
    } else {
        i = lb;
        while (db->coef[i].f < f)
        {
            i = i + 1;
        }
        if (f == db->coef[i].f)
        {
            coef1->a = db->coef[i].a;
            coef1->b = db->coef[i].b;
            coef1->c = db->coef[i].c;
        } else {
            ratio = (f - db->coef[i - 1].f) /
                    (db->coef[i].f - db->coef[i - 1].f);
            coef1->a = db->coef[i - 1].a + ratio *
                    (db->coef[i].a - db->coef[i - 1].a);
            coef1->b = db->coef[i - 1].b + ratio *
                    (db->coef[i].b - db->coef[i - 1].b);
            coef1->c = db->coef[i - 1].c + ratio *
                    (db->coef[i].c - db->coef[i - 1].c);
        }
    }

    // multiplier correction
    if (info->lens->multiplier != info->camera->multiplier)
    {
        double factor;
        factor = info->lens->multiplier / info->camera->multiplier;
        coef1->a = coef1->a * factor * factor * factor;
        coef1->b = coef1->b * factor * factor;
        coef1->c = coef1->c * factor;
    }

    coefD(coef1, info->width, info->height, info->resize);
	return 1;
}

/* ==================================================================== */
#if 0
int PTLDB_getRadCoefs(PTLDB_DB * db, PTLDB_ImageInfo * info, PTLDB_RadCoef * coef1)
{
    long lb;
    long ub;
    long i;
    double ratio;
    double f;

    f = info->focalLength;
    if (info->converterDetected)
        f = f * info->lens->converterFactor;

    lb = info->lens->coefLB;
    ub = info->lens->coefUB;

    // force focal length to be legal
    if (f < db->coef[lb].f)
    {
        if (f < 0.9 * db->coef[lb].f)
            return 0;
        f = db->coef[lb].f;
    }
    if (f > db->coef[ub].f)
    {
        if (f > 1.1 * db->coef[ub].f)
            return 0;
        f = db->coef[ub].f;
    }

    // calculate hfov
//    coef1.hfov = getHfov(f);

    // calculate a,b,c
    if (lb == ub)
    {
        coef1->a = db->coef[lb].a;
        coef1->b = db->coef[lb].b;
        coef1->c = db->coef[lb].c;
    } else {
        i = lb;
        while (db->coef[i].f < f)
        {
            i = i + 1;
        }
        if (f == db->coef[i].f)
        {
            coef1->a = db->coef[i].a;
            coef1->b = db->coef[i].b;
            coef1->c = db->coef[i].c;
        } else {
            ratio = (f - db->coef[i - 1].f) /
                    (db->coef[i].f - db->coef[i - 1].f);
            coef1->a = db->coef[i - 1].a + ratio *
                    (db->coef[i].a - db->coef[i - 1].a);
            coef1->b = db->coef[i - 1].b + ratio *
                    (db->coef[i].b - db->coef[i - 1].b);
            coef1->c = db->coef[i - 1].c + ratio *
                    (db->coef[i].c - db->coef[i - 1].c);
        }
    }

    // multiplier correction
    if (info->lens->multiplier != info->camera->multiplier)
    {
        double factor;
        factor = info->lens->multiplier / info->camera->multiplier;
        coef1->a = coef1->a * factor * factor * factor;
        coef1->b = coef1->b * factor * factor;
        coef1->c = coef1->c * factor;
    }

    coefD(coef1, info->width, info->height, info->resize);
    return 1;
}

#endif

/* ==================================================================== */

#if 0

int PTLDB_getVigCoefs(PTLDB_DB * db, PTLDB_ImageInfo * info, PTLDB_VigCoefType * coef1)
{
    long lb;
    long ub;
    long i,j;
    double ratio;
    double f;
    double k;
    long i00, i01, i10, i11

    f = info->focalLength;
    k = info->aperture;
    if (info->converterDetected)
        f = f * info->lens->converterFactor;

    lb = info->lens->vigCoefLB;
    ub = info->lens->vigCoefUB;

    // force focal length to be legal
    if (f < db->coef[lb].f)
    {
        if (f < 0.9 * db->coef[lb].f)
            return 0;
        f = db->coef[lb].f;
    }
    if (f > db->coef[ub].f)
    {
        if (f > 1.1 * db->coef[ub].f)
            return 0;
        f = db->coef[ub].f;
    }

    // calculate hfov
//    coef1.hfov = getHfov(f);

    // calculate a,b,c
    if (lb == ub)
    {
        // just one setting.. use, independently of the parameters.
        *coef1 = db->vigCoef[lb];
    } else {
        
        for (i=lb; i<=ub; i++) {
            if (db->vigCoef[i].f >= f) {
                // try to step back one focal length
                for (
                f1 = db->vigCoef[i].f;
                // search for right aperture
                while(i<=ub && db->vigCoef[i].k < k && db->vigCoef[i].f == f1) 
                {
                    i++;
                }
                if (i>ub || db->vigCoef[i].f > f
        // need to interpolate between focal length and aperture
        i = lb;


        while (db->vigCoef[i].f < f && i < ub)
        {
            i = i + 1;
        }
        //
        if (f == db->coef[i].f)
        {
            // exact hit in f
            // search for right index.
            // linear interpolation in aperture
            j = i;
            while (db->vigCoef[j].f == f && db->vigCoef[j].k < k && j < ub) {
                
            coef1->a = db->coef[i].a;
            coef1->b = db->coef[i].b;
            coef1->c = db->coef[i].c;
        } else {
            ratio = (f - db->coef[i - 1].f) /
                    (db->coef[i].f - db->coef[i - 1].f);
            coef1->a = db->coef[i - 1].a + ratio *
                    (db->coef[i].a - db->coef[i - 1].a);
            coef1->b = db->coef[i - 1].b + ratio *
                    (db->coef[i].b - db->coef[i - 1].b);
            coef1->c = db->coef[i - 1].c + ratio *
                    (db->coef[i].c - db->coef[i - 1].c);
        }
    }

    // multiplier correction
    if (info->lens->multiplier != info->camera->multiplier)
    {
        double factor;
        factor = info->lens->multiplier / info->camera->multiplier;
        coef1->a = coef1->a * factor * factor * factor;
        coef1->b = coef1->b * factor * factor;
        coef1->c = coef1->c * factor;
    }

    coefD(coef1, info->width, info->height, info->resize);
    return 1;
}

#endif
