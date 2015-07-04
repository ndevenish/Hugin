// -*- c-basic-offset: 4 -*-

/** @file LensDB.h
 *
 *  @brief class to access Hugins camera and lens database
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _LENS_DB_H
#define _LENS_DB_H

#include <hugin_shared.h>
#include <panodata/SrcPanoImage.h>
#include <panodata/Panorama.h>
#include <string>
#include <vector>

namespace HuginBase
{

namespace LensDB
{
/** vector storing a list of lens names */
typedef std::vector<std::string> LensList;

/** main database class */
class IMPEX LensDB
{
public:
    /** constructor */
    LensDB();
    /** destructor */
    virtual ~LensDB();

    // routines to read from database
    /** returns the crop factor for the given camera (maker/model) 
        @param maker maker of the desired camera 
        @param model camera model
        @param cropFactor contains the crop factor
        @return true, if the crop factor could be obtained from the database, otherwise false */
    bool GetCropFactor(const std::string& maker, const std::string& model, double& cropFactor) const;
    /** returns the projection of the lens
        @param lens name of the lens, or for compact camera Maker|Model
        @param projection contains the projection of the lens 
        @return true, if the database has stored information about the lens projection */
    bool GetProjection(const std::string& lens, BaseSrcPanoImage::Projection& projection) const;
    /** returns the crop of the lens
        the information for landscape and portrait images are stored separately
        @param lens name of the lens, or for compact camera Maker|Model
        @param focal focal length, for which the crop should be returned
        @param imageSize size of the image for which the crop information is wanted
        @param cropRect contains the crop information
        @return true, if the database has stored information about the lens crop */
    bool GetCrop(const std::string& lens, const double focal, const vigra::Size2D& imageSize, vigra::Rect2D& cropRect) const;
    /** returns the field of view of the lens 
        the fov is always returned for a landscape image with aspect ratio 3:2
        @param lens name of the lens, or for compact camera Maker|Model
        @param focal focal length, for which the fov should be returned
        @param fov stored the returned field of view
        @return true, if the database has stored information about the field of view */
    bool GetFov(const std::string& lens, const double focal, double& fov) const;
    /** returns the distortion parameters of the lens
        @param lens name of the lens, or for compact camera Maker|Model
        @param focal focal length, for which the distortion parameters should be returned
        @param distortion stored the returned distortion parameters
        @return true, if the database has stored information about the distortion */
    bool GetDistortion(const std::string& lens, const double focal, std::vector<double>& distortion) const;
    /** returns the vignetting parameters of the lens
        @param lens name of the lens, or for compact camera Maker|Model
        @param focal focal length, for which the vignetting parameters should be returned
        @param aperture aperture, for which the vignetting parameters should be returned
        @param distance distance, for which the vignetting parameters should be returned (currently ignored)
        @param vignetting stored the returned vignetting
        @return true, if the database has stored information about the vignetting */
    bool GetVignetting(const std::string& lens, const double focal, const double aperture, const double distance, std::vector<double>& vignetting) const;
    /** returns the tca distortion parameters of the lens
        @param lens name of the lens, or for compact camera Maker|Model
        @param focal focal length, for which the distortion parameters should be returned
        @param tca_red stored the returned tca distortion parameters for red channel
        @param tca_blue stored the returned tca distortion parameters for blue channel
        @return true, if the database has stored information about the distortion */
    bool GetTCA(const std::string& lens, const double focal, std::vector<double>& tca_red, std::vector<double>& tca_blue) const;
    /** return a vector of lenses with selected database entries
        @param distortion vector contains lenses with distortion data
        @param vignetting vector contains lenses with vignetting data
        @param tca vector contains lenses with tca data 
        @param lensList vector containing the lens names
        @return true, if lenses were found */
    bool GetLensNames(const bool distortion, const bool vignetting, const bool tca, LensList& lensList) const;
    /** compress database by remove all entries and insert instead the average values
        @return true, if cleanup was successful */
    bool CleanUpDatabase();
    /** remove all database entry which refers to given lens 
        @return true, if all was successful, false, if there were errors */
    bool RemoveLens(const std::string& lensname);
    /** remove all database entry which refers to given camera
        @return true, if all was successful, false, if there were errors */
    bool RemoveCamera(const std::string& maker, const std::string& model);

    //routines to save to database
    /** save the camera with the given cropfactor into the database
        @param maker camera maker to save
        @param model camera model to save
        @param cropfactor crop factor for given camera
        @return true, if saving was successful */
    bool SaveCameraCrop(const std::string& maker, const std::string& model, const double cropfactor);
    /** save the camera with the given EMoR parameters into the database
        @param maker camera maker to save
        @param model camera model to save
        @param ISO ISO settings for which the EMoR parameters applies
        @param emor EMoR parameters to save
        @param weight weight factor for given values (0-100, default is 10)
        @return true, if saving was successful */
    bool SaveEMoR(const std::string& maker, const std::string& model, const int iso, const std::vector<float>& emor, const int weight = 10);
    /** saves the projection for the lens in the database
        @param lens name of the lens, or for compact camera Maker|Model
        @param projection projection of the lens
        @return true, if saving was successful */
    bool SaveLensProjection(const std::string& lens, const BaseSrcPanoImage::Projection projection);
    /** saves the crop information of the lens in the database
        the information for landscape and portrait images are stored separately
        @param lens name of the lens, or for compact camera Maker|Model
        @param focal focal length, for which the crop should be saved
        @param imageSize size of the image for which the crop information is saved
        @param cropRect contains the crop information which should be saved
        @return true, if saving was successful */
    bool SaveLensCrop(const std::string& lens, const double focal, const vigra::Size2D& imageSize, const vigra::Rect2D& cropRect);
    /** saves the field of view of the lens
        the fov should always calculated for a landscape image with aspect ratio 3:2
        @param lens name of the lens, or for compact camera Maker|Model
        @param focal focal length, for which the fov should be saved
        @param fov field of view for storing
        @param weight weight factor for given values (0-100, default is 10)
        @return true, if saving was successful */
    bool SaveLensFov(const std::string& lens, const double focal, const double fov, const int weight = 10);
    /** saves the distortion parameters of the lens in the database
        @param lens name of the lens, or for compact camera Maker|Model
        @param focal focal length, for which the distortion parameters should be saved
        @param distortion distortion parameters which should be stored in database
        @param weight weight factor for given values (0-100, default is 10)
        @return true, if saving was successful */
    bool SaveDistortion(const std::string& lens, const double focal, const std::vector<double>& distortion, const int weight = 10);
    /** saves the vignetting parameters of the lens
        @param lens name of the lens, or for compact camera Maker|Model
        @param focal focal length, for which the vignetting parameters should be saved
        @param aperture aperture, for which the vignetting parameters should be saved
        @param distance distance, for which the vignetting parameters should be saved
        @param vignetting vignetting parameters which should be stored
        @param weight weight factor for given values (0-100, default is 10)
        @return true, if saving was successful */
    bool SaveVignetting(const std::string& lens, const double focal, const double aperture, const double distance, const std::vector<double>& vignetting, const int weight = 10);
    /** saves the tca distortion parameters of the lens
        @param lens name of the lens, or for compact camera Maker|Model
        @param focal focal length, for which the distortion parameters should be saved
        @param tca_red tca distortion parameters for red channel
        @param tca_blue tca distortion parameters for blue channel
        @param weight weight factor for given values (0-100, default is 10)
        @return true, if saving was successful */
    bool SaveTCA(const std::string& lens, const double focal, const std::vector<double>& tca_red, const std::vector<double>& tca_blue, const int weight=10);
    /** returns the filename of the lens database */
    std::string GetDBFilename() const;
    // access to single database class
    /** returns the static LensDB instance */
    static LensDB& GetSingleton();
    /** cleanup the static LensDB instance, must be called at the end of the program */
    static void Clean();
    /** export database to file */
    bool ExportToFile(const std::string& filename);
    /** import data from external file */
    bool ImportFromFile(const std::string& filename);
private:
    // prevent copying of class
    LensDB(const LensDB&);
    LensDB& operator=(const LensDB&);
    // private variables
    class Database;
    Database *m_db;
    static LensDB* m_instance;
};

/** routine for automatically saving information from pano into database */
IMPEX bool SaveLensDataFromPano(const HuginBase::Panorama& pano);

}; //namespace LensDB
}; //namespace HuginBase

#endif //_LENS_DB_H
