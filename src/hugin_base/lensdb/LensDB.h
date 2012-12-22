// -*- c-basic-offset: 4 -*-

/** @file LensDB.h
 *
 *  @brief Wrapper around function for access to lensfun database
 *
 *  @author T. Modes
 */

/*  This program is free software; you can redistribute it and/or
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

#ifndef _LENS_DB_H
#define _LENS_DB_H

#include <hugin_shared.h>
#include <string>
#ifndef Hugin_shared
#define CONF_LENSFUN_STATIC
#endif
#include <lensfun.h>
#include <panodata/SrcPanoImage.h>
#include <hugin_math/hugin_math.h>

namespace HuginBase
{

namespace LensDB
{

/** class to save a list of lenses */
class IMPEX LensDBList
{
public:
    LensDBList();
    ~LensDBList();
    const size_t GetLensCount() const;
    const lfLens* GetLens(size_t index) const;
    void SetLenses(const lfLens** lenses);
    std::string GetLensName(size_t index) const;
    void SetCameraModelMaker(const std::string camMaker, const std::string camModel);
private:
    const lfLens** m_lenses;
    size_t m_lensCount;
    std::string m_camMaker;
    std::string m_camModel;
};

/** main wrapper class for lensfun database */
class IMPEX LensDB
{
public:
    /** constructor */
    LensDB();
    /** destructor */
    virtual ~LensDB();

    /** sets the main path of the database files, if not set the default location is used */
    void SetMainDBPath(std::string mainPath);
    /** return the path where the database is read */
    std::string GetMainDBPath();
    /** returns the path to the user specific part of the database */
    std::string GetUserDBPath();
    /** reloads the user part of the lensfun db */
    void ReloadUserPart();

    // routines to read from database
    /** returns the crop factor for the given camera (maker/model) 
        @param maker maker of the desired camera 
        @param model camera model
        @param cropFactor contains the crop factor
        @return true, if the crop factor could be obtained from the database, otherwise false */
    bool GetCropFactor(std::string maker, std::string model, double &cropFactor);
    /** returns the mount of the given camera (maker/model) 
        @param maker maker of the desired camera 
        @param model camera model
        @param mount contains the mount
        @return true, if the mount could be obtained from the database, otherwise false */
    bool GetCameraMount(std::string maker, std::string model, std::string &mount);
    /** searches for the given lens and store it parameters inside 
        @param camMaker maker of the camera, for fixed lens cameras
        @param camModel model of the camera, for fixed lens cameras
        @param lens lens for searching
        @return true, if a lens was found */
    bool FindLens(std::string camMaker, std::string camModel, std::string lens);
    /** sets the active lens for the following Check* call */
    void SetActiveLens(const lfLens* activeLens);
    /** checks if the focal length matches the lens spec 
        @return true, if focal length is inside spec*/
    bool CheckLensFocal(double focal);
    /** checks if the aperture matches the lens spec 
        @return true, if aperture is inside spec*/
    bool CheckLensAperture(double aperture);
    /** searches for the given lens and gives back a list of matching lenses 
        @param camMaker maker of the camera, for fixed lens cameras
        @param camModel model of the camera, for fixed lens cameras
        @param lensname lensname to search
        @param foundLenses contains found lenses names
        @param fuzzy set to true, if search should use fuzzy algorithm
        @return true, if lenses were found */
    bool FindLenses(std::string camMaker, std::string camModel, std::string lensname, LensDBList &foundLenses, bool fuzzy=false);
    /** searches for all mounts in the database
        @return true, if mounts were found */
    bool GetMounts(std::vector<std::string> &foundMounts);
    /** returns the projection of the last found lens (you need to call LensDB::FindLens before calling this procedure)
        @param projection contains the projection of the lens 
        @return true, if the database has stored information about the lens projection */
    bool GetProjection(BaseSrcPanoImage::Projection & projection);
    /** returns the crop of the last found lens (you need to call LensDB::FindLens before calling this procedure)
        @param focal focal length, for which the crop should be returned
        @param cropMode contains the crop mode of the lens
        @param cropLeftTop contains the left top border edge of the crop
        @param cropRightBottom contains the right bottom edge of the crop
        @return true, if the database has stored information about the lens crop */
    bool GetCrop(double focal, BaseSrcPanoImage::CropMode &cropMode, hugin_utils::FDiff2D &cropLeftTop, hugin_utils::FDiff2D &cropRightBottom);
    /** returns the field of view of the last found lens (you need to call LensDB::FindLens before calling this procedure)
        @param focal focal length, for which the fov should be returned
        @param fov stored the returned field of view
        @return true, if the database has stored information about the field of view */
    bool GetFov(double focal,double &fov);
    /** returns the distortion parameters of the last found lens (you need to call LensDB::FindLens before calling this procedure)
        @param focal focal length, for which the distortion parameters should be returned
        @param distortion stored the returned distortion parameters
        @return true, if the database has stored information about the distortion */
    bool GetDistortion(double focal, std::vector<double> &distortion);
    /** returns the vignetting parameters of the last found lens (you need to call LensDB::FindLens before calling this procedure)
        @param focal focal length, for which the vignetting parameters should be returned
        @param aperture aperture, for which the vignetting parameters should be returned
        @param distance distance, for which the vignetting parameters should be returned
        @param vignetting stored the returned vignetting
        @return true, if the database has stored information about the vignetting */
    bool GetVignetting(double focal, double aperture, double distance, std::vector<double> &vignetting);

    //routines to save to database
    /** save the camera with the given cropfactor into the given file 
        @param filename in which file the db should be saved, existing entries will be 
        @param maker camera maker to save
        @param model camera model to save
        @param mount mount of the used camera model
        @param cropfactor crop factor for given camera
        @return true, if saving was successful */
    bool SaveCameraCrop(std::string filename, std::string maker, std::string model, std::string mount, double cropfactor);

    /** starts saving a lens to database, call LensDB::SaveHFOV, LensDB::SaveCrop, LensDB::SaveDistortion and/or LensDB::SaveVignetting
        to add information to database, finally save information to file with LensDB::EndSaveLens
        @param filename filename to which the information should be saved
        @param maker maker of the lens
        @param lensname lens name
        @param mount mount of the lens (if empty string "Generic" is written to database )
        @param projection projection of the lens
        @param cropfactor crop factor of the lens
        @return 0 - if all okay, 1 - if database could not created, 2 - if projection does not match type saved in database, 3 - if crop factor does not match crop factor in database */
    int BeginSaveLens(std::string filename, std::string maker, std::string lensname, std::string mount, BaseSrcPanoImage::Projection projection, double cropfactor);
    /** updated the hfov for the given focal length
        @param focal focal length
        @param hfov horizontal field of view to save */
    void SaveHFOV(double focal, double hfov);
    /** updated the crop for the given focal length
        @param focal focal length
        @param hfov horizontal field of view to save */
    void SaveCrop(double focal, BaseSrcPanoImage::CropMode cropMode, hugin_utils::FDiff2D cropLeftTop, hugin_utils::FDiff2D cropRightBottom);
    /** updated the distortion for the given focal length
        @param focal focal length
        @param distortion distortion to save, vector must have 3 elements */
    void SaveDistortion(double focal, std::vector<double> distortion);
    /** updated the vignetting for the given focal length
        @param focal focal length, for which vignetting was determined
        @param aperture aperture, for which vignetting was determined
        @param distance distance for which vignetting was determined
        @param vignetting vignetting to save, vector must have 3 elements */
    void SaveVignetting(double focal, double aperture, double distance, std::vector<double> vignetting);
    /** finally saves the new information to file, see also LensDB::BeginSaveLens */
    bool EndSaveLens();

    // access to single database class
    /** returns the static LensDB instance */
    static LensDB& GetSingleton();
    /** cleanup the static LensDB instance, must be called at the end of the program */
    static void Clean();
private:
    /** initialize db */
    bool InitDB();
    /** load all xml files in given path into database */
    bool LoadFilesInDir(std::string path);
    /** free all ressources used for saving lens */
    void CleanSaveInformation();
    /** check, if mount is already in database, if not it populates the LensDB::m_updatedMounts with the mounts
        @return true, if new mount is found */
    bool IsNewMount(std::string newMount);
    /** deletes the lens list */
    void FreeLensList();
    /** cleans up the information stored for mounts (variable LensDB::m_updatedMounts) */
    void CleanUpdatedMounts();
    /** the main database */
    struct lfDatabase *m_db;
    /** database for saving */ 
    struct lfDatabase *m_newDB;
    /** found lenses for LensDB::GetProjection, LensDB::GetCrop */
    const struct lfLens **m_lenses;
    /** variable used for cleanup of lensfun points */
    bool m_needLensCleanup;
    /** list of lenses for saving */
    struct lfLens **m_updatedLenses;
    /** list of new mounts for saving */
    struct lfMount **m_updatedMounts;
    /** struct of lens currently is saved */
    struct lfLens *m_currentLens;
    /** current filename for saving lens */
    std::string m_lensFilename;
    /** true, if database was successful initialized */
    bool m_initialized;
    // stores the pathes
    std::string m_main_db_path;
    std::string m_user_db_path;
    static LensDB* m_instance;
};

}; //namespace LensDB
}; //namespace HuginBase

#endif //_LENS_DB_H
