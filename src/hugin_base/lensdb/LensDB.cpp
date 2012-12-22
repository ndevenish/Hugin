/** @file LensDB.cpp
 *
 *  @brief Implementation of wrapper around function for access to lensfun database
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

#include "LensDB.h"
#include <boost/filesystem.hpp>
#include <iostream>
#include <hugin_utils/stl_utils.h>

// minimum version for all features is 0.2.5.1, for earlierer version only a subset of the functions
// is supported
#define MIN_LF_VERSION  0x20501

namespace HuginBase
{

namespace LensDB
{

#if BOOST_FILESYSTEM_VERSION < 3
typedef boost::filesystem::basic_path<std::string, boost::filesystem::path_traits> basic_path;
#define GETPATHSTRING(x) x
#else
typedef boost::filesystem::path basic_path;
#define GETPATHSTRING(x) (x).string()
#endif

LensDB* LensDB::m_instance=NULL;

LensDB::LensDB()
{
    m_db=lf_db_new();
    if(!m_db)
    {
        m_db=NULL;
    }
    else
    {
        m_user_db_path=std::string(m_db->HomeDataDir);
    };
    m_initialized=false;
    m_lenses=NULL;
    m_newDB=NULL;
    m_updatedLenses=NULL;
    m_currentLens=NULL;
    m_updatedMounts=NULL;
    m_needLensCleanup=true;
};

LensDB::~LensDB()
{
    FreeLensList();
    if (m_db!=NULL)
    {
        m_db->Destroy();
    };
    CleanSaveInformation();
};

void LensDB::FreeLensList()
{
    if(m_lenses!=NULL)
    {
        if(!m_needLensCleanup)
        {
            delete [] m_lenses;
        }
        else
        {
            lf_free(m_lenses);
        };
    };
    m_lenses=NULL;
};

LensDB& LensDB::GetSingleton()
{
    if(m_instance==NULL)
    {
        m_instance = new LensDB();
    };
    return *m_instance;
};

void LensDB::Clean()
{
    if(m_instance!=NULL)
    {
        delete m_instance;
    };
    m_instance=NULL;
};

void LensDB::SetMainDBPath(std::string mainPath)
{
    m_main_db_path=mainPath;
};

std::string LensDB::GetMainDBPath()
{
    return m_main_db_path;
};

std::string LensDB::GetUserDBPath()
{
    return m_user_db_path;
};

bool LensDB::InitDB()
{
    if(m_initialized)
        return true;
    if(m_db!=NULL)
    {
#if LF_VERSION < MIN_LF_VERSION
        // workaround a bug in lensfun, that the numeric locale is not correctly set back
        char * old_locale = setlocale(LC_NUMERIC,NULL);
        old_locale = strdup(old_locale);
        setlocale(LC_NUMERIC,"C");
#endif
        if(m_main_db_path.empty())
        {
            lfError e=m_db->Load();
            m_initialized=true;
        }
        else
        {
            LoadFilesInDir(m_main_db_path);
            LoadFilesInDir(m_user_db_path);
            m_initialized=true;
        };
#if LF_VERSION < MIN_LF_VERSION
        // workaround a bug in lensfun, that the numeric locale is not correctly set back
        setlocale(LC_NUMERIC,old_locale);
        free(old_locale);
#endif
    };
    return m_initialized;
};

// this function is a rewrite of a lensfun function, because it can not publically accessed
bool LensDB::LoadFilesInDir(std::string path)
{
    if(m_db==NULL)
        return false;
    lfError e=LF_NO_ERROR;
    basic_path p(path);
    try
    {
        if(exists(p))
        {
            if (is_directory(p))
            {
                typedef std::vector<basic_path> fl_vec;
                fl_vec filelist;
                std::copy(boost::filesystem::directory_iterator(p), boost::filesystem::directory_iterator(), std::back_inserter(filelist));
                for(fl_vec::const_iterator it=filelist.begin();it!=filelist.end();it++)
                {
                    basic_path file=*it;
                    if(GETPATHSTRING(file.extension())==std::string(".xml"))
                    {
                        lfError e2=m_db->Load(file.string().c_str());
                        if(e==LF_NO_ERROR && e2!=LF_NO_ERROR)
                        {
                            e=e2;
                        };
                    };
                };
            }
        }
    }
    catch (const boost::filesystem::filesystem_error& ex)
    {
        std::cout << "Error reading lensfun db. Code: " << ex.what() << '\n';
    }
    return e==LF_NO_ERROR;
};

void LensDB::ReloadUserPart()
{
    if(m_db==NULL)
    {
        return;
    };
    if(!m_initialized)
    {
        InitDB();
    }
    else
    {
        LoadFilesInDir(m_user_db_path);
    };
};

bool LensDB::GetCropFactor(std::string maker, std::string model, double &cropFactor)
{
    InitDB();
    if(!m_initialized)
        return false;
    cropFactor=0;
    const lfCamera **cameras=m_db->FindCameras(maker.c_str(),model.c_str());
    if(cameras)
    {
        cropFactor=cameras[0]->CropFactor;
    }
    lf_free (cameras);
    return cropFactor>0;
};

bool LensDB::GetCameraMount(std::string maker, std::string model, std::string &mount)
{
    InitDB();
    if(!m_initialized)
        return false;
    mount="";
    const lfCamera **cameras=m_db->FindCameras(maker.c_str(),model.c_str());
    if(cameras)
    {
        mount=std::string(cameras[0]->Mount);
    }
    lf_free (cameras);
    return !mount.empty();
};

bool LensDB::FindLens(std::string camMaker, std::string camModel, std::string lens)
{
    FreeLensList();
    m_needLensCleanup=true;
    InitDB();
    if(!m_initialized)
    {
        return false;
    };
    if(camMaker.empty() && camModel.empty() && lens.empty())
    {
        return false;
    };
    if(lens.empty() || hugin_utils::tolower(lens)=="standard")
    {
        const lfCamera** cam=m_db->FindCameras(camMaker.c_str(),camModel.c_str());
        if(cam)
        {
            m_lenses=m_db->FindLenses(cam[0],NULL,"Standard");
            lf_free(cam);
        }
        else
        {
            return false;
        };
    }
    else
    {
        if(!camModel.empty())
        {
            const lfCamera** cam=m_db->FindCameras(camMaker.c_str(),camModel.c_str());
            if(cam)
            {
                m_lenses=m_db->FindLenses(cam[0], NULL, lens.c_str());
                lf_free(cam);
            }
            else
            {
                m_lenses=m_db->FindLenses(NULL, NULL, lens.c_str());
            }
        }
        else
        {
            m_lenses=m_db->FindLenses(NULL, NULL,lens.c_str());
        };
    };
    if(m_lenses)
    {
        return true;
    }
    else
    {
        lf_free(m_lenses);
        m_lenses=NULL;
        return false;
    };
};

void LensDB::SetActiveLens(const lfLens* activeLens)
{
    FreeLensList();
    m_needLensCleanup=false;
    m_lenses=new const lfLens* [1];
    m_lenses[0]=activeLens;
};

bool LensDB::CheckLensFocal(double focal)
{
    if(m_lenses==NULL)
    {
        return false;
    };
    return focal>=m_lenses[0]->MinFocal && focal<=m_lenses[0]->MaxFocal;
};

bool LensDB::CheckLensAperture(double aperture)
{
    if(m_lenses==NULL)
    {
        return false;
    };
    if(m_lenses[0]->MinAperture>0)
    {
        return aperture>=m_lenses[0]->MinAperture;
    }
    else
    {
        //no aperture found, assuming it is correct
        return true;
    };
};

bool LensDB::FindLenses(std::string camMaker, std::string camModel, std::string lensname, LensDBList &foundLenses, bool fuzzy)
{
    InitDB();
    if(!m_initialized)
    {
        return false;
    };
    foundLenses.SetCameraModelMaker(camMaker, camModel);
    if(camMaker.empty() && camModel.empty() && lensname.empty())
    {
        return false;
    };
    const lfLens **lenses;
    int searchFlag=fuzzy ? LF_SEARCH_LOOSE : 0;
    if(lensname.empty() || hugin_utils::tolower(lensname)=="standard")
    {
        const lfCamera** cam=m_db->FindCameras(camMaker.c_str(),camModel.c_str());
        if(cam)
        {
            lenses=m_db->FindLenses(cam[0],NULL,"Standard",searchFlag);
            lf_free(cam);
        }
        else
        {
            return false;
        };
    }
    else
    {
        if(!camModel.empty())
        {
            const lfCamera** cam=m_db->FindCameras(camMaker.c_str(), camModel.c_str());
            if(cam)
            {
                lenses=m_db->FindLenses(cam[0], NULL, lensname.c_str(), searchFlag);
                lf_free(cam);
            }
            else
            {
                lenses=m_db->FindLenses(NULL, NULL, lensname.c_str(), searchFlag);
            }
        }
        else
        {
            lenses=m_db->FindLenses(NULL, NULL, lensname.c_str(), searchFlag);
        };
    };

    foundLenses.SetLenses(lenses);
    return foundLenses.GetLensCount()>0;
};

bool LensDB::GetMounts(std::vector<std::string> &foundMounts)
{
    InitDB();
    if(!m_initialized)
    {
        return false;
    };
    foundMounts.clear();
    const lfMount *const* mounts=m_db->GetMounts();
    if(mounts)
    {
        for(int i=0;mounts[i];i++)
        {
            foundMounts.push_back(m_db->MountName(mounts[i]->Name));
        }
    };
    return foundMounts.size()>0;
};

/** translates the projection names from lensfun form into Hugin form 
    @return true, if conversion was sucessful */
bool TranslateProjectionLF2Hugin(const lfLens* lens, BaseSrcPanoImage::Projection & projection)
{
    switch(lens->Type)
    {
        case LF_RECTILINEAR:
            projection=BaseSrcPanoImage::RECTILINEAR;
            return true;
            break;
        case LF_FISHEYE:
            projection=BaseSrcPanoImage::FULL_FRAME_FISHEYE;
#if LF_VERSION>=MIN_LF_VERSION
            lfLensCalibCrop cropMode;
            if (lens->InterpolateCrop(lens->MinFocal,cropMode))
            {
                if(cropMode.CropMode==LF_CROP_CIRCLE)
                {
                    projection=BaseSrcPanoImage::CIRCULAR_FISHEYE;
                    return true;
                }
            };
#endif
            return true;
            break;
        case LF_PANORAMIC:
            projection=BaseSrcPanoImage::PANORAMIC;
            return true;
            break;
        case LF_EQUIRECTANGULAR:
            projection=BaseSrcPanoImage::EQUIRECTANGULAR;
            return true;
            break;
#if LF_VERSION>=MIN_LF_VERSION
            // the following projection are only defined in lensfun in 0.2.5.1 and later
        case LF_FISHEYE_ORTHOGRAPHIC:
            projection=BaseSrcPanoImage::FISHEYE_ORTHOGRAPHIC;
            return true;
            break;
        case LF_FISHEYE_STEREOGRAPHIC:
            projection=BaseSrcPanoImage::FISHEYE_STEREOGRAPHIC;
            return true;
            break;
        case LF_FISHEYE_EQUISOLID:
            projection=BaseSrcPanoImage::FISHEYE_EQUISOLID;
            return true;
            break;
        case LF_FISHEYE_THOBY:
            projection=BaseSrcPanoImage::FISHEYE_THOBY;
            return true;
            break;
#endif
        case LF_UNKNOWN:
        default:
            return false;
            break;
    };
    return false;
};

/** translates the projection names from Hugin form into lensfun form 
    @return true, if conversion was sucessful */
bool TranslateProjectionHugin2LF(BaseSrcPanoImage::Projection projection, lfLens* lens)
{
    switch(projection)
    {
        case BaseSrcPanoImage::RECTILINEAR:
            lens->Type=LF_RECTILINEAR;
            return true;
            break;
        case BaseSrcPanoImage::CIRCULAR_FISHEYE:
            lens->Type=LF_FISHEYE;
#if LF_VERSION>=MIN_LF_VERSION
            lfLensCalibCrop crop;
            if(lens->InterpolateCrop(lens->MinFocal,crop))
            {
                if(crop.CropMode!=LF_CROP_CIRCLE)
                {
                    return false;
                }
            }
            else
            {
                crop.Crop[0]=0;
                crop.Crop[1]=1;
                crop.Crop[2]=0;
                crop.Crop[3]=1;
                crop.CropMode=LF_CROP_CIRCLE;
                crop.Focal=lens->MinFocal;
                lens->AddCalibCrop(&crop);
            };
#endif
            return true;
            break;
        case BaseSrcPanoImage::FULL_FRAME_FISHEYE:
            lens->Type=LF_FISHEYE;
            return true;
            break;
        case BaseSrcPanoImage::PANORAMIC:
            lens->Type=LF_PANORAMIC;
            return true;
            break;
        case BaseSrcPanoImage::EQUIRECTANGULAR:
            lens->Type=LF_EQUIRECTANGULAR;
            return true;
            break;
#if LF_VERSION>=MIN_LF_VERSION
            // the following projection are only defined in lensfun in 0.2.5.1 and later
        case BaseSrcPanoImage::FISHEYE_ORTHOGRAPHIC:
            lens->Type=LF_FISHEYE_ORTHOGRAPHIC;
            return true;
            break;
        case BaseSrcPanoImage::FISHEYE_STEREOGRAPHIC:
            lens->Type=LF_FISHEYE_STEREOGRAPHIC;
            return true;
            break;
        case BaseSrcPanoImage::FISHEYE_EQUISOLID:
            lens->Type=LF_FISHEYE_EQUISOLID;
            return true;
            break;
        case BaseSrcPanoImage::FISHEYE_THOBY:
            lens->Type=LF_FISHEYE_THOBY;
            return true;
            break;
#endif
    };
    return false;
};

bool LensDB::GetProjection(BaseSrcPanoImage::Projection & projection)
{
    if(m_lenses==NULL)
    {
        return false;
    };
    return TranslateProjectionLF2Hugin(m_lenses[0],projection);
};

bool LensDB::GetCrop(double focal,BaseSrcPanoImage::CropMode &cropMode,hugin_utils::FDiff2D &cropLeftTop,hugin_utils::FDiff2D &cropRightBottom)
{
    if(m_lenses==NULL)
    {
        return false;
    };
#if LF_VERSION>=MIN_LF_VERSION
    struct lfLensCalibCrop crop;
    if(m_lenses[0]->InterpolateCrop(focal,crop))
    {
        switch (crop.CropMode)
        {
            case LF_CROP_RECTANGLE:
                cropMode=BaseSrcPanoImage::CROP_RECTANGLE;
                break;
            case LF_CROP_CIRCLE:
                cropMode=BaseSrcPanoImage::CROP_CIRCLE;
                break;
            default:
                cropMode=BaseSrcPanoImage::NO_CROP;
                break;
        };
        if(cropMode!=BaseSrcPanoImage::NO_CROP)
        {
            cropLeftTop.x=crop.Crop[0];
            cropLeftTop.y=crop.Crop[2];
            cropRightBottom.x=crop.Crop[1];
            cropRightBottom.y=crop.Crop[3];
        };
        return true;
    }
    else
    {
        return false;
    };
#else
    return false;
#endif
};

bool LensDB::GetFov(double focal,double &fov)
{
    if(m_lenses==NULL)
    {
        return false;
    };
#if LF_VERSION>=MIN_LF_VERSION
    lfLensCalibFov calibFov;
    if(m_lenses[0]->InterpolateFov(focal,calibFov))
    {
        fov=calibFov.FieldOfView;
        return true;
    }
#endif
    return false;
};

bool LensDB::GetDistortion(double focal, std::vector<double> &distortion)
{
    distortion.clear();
    if(m_lenses==NULL)
    {
        return false;
    };
    lfLensCalibDistortion calibDist;
    if(m_lenses[0]->InterpolateDistortion(focal,calibDist))
    {
        switch(calibDist.Model)
        {
            //only a part of the distortion models are supported by Hugin
            //convert all into PTLens model where possible
            case LF_DIST_MODEL_POLY3:
                distortion.push_back(0);
                distortion.push_back(calibDist.Terms[0]);
                distortion.push_back(0);
                return true;
                break;
            case LF_DIST_MODEL_PTLENS:
                distortion.push_back(calibDist.Terms[0]);
                distortion.push_back(calibDist.Terms[1]);
                distortion.push_back(calibDist.Terms[2]);
                return true;
                break;
            case LF_DIST_MODEL_NONE:
            case LF_DIST_MODEL_POLY5:
            case LF_DIST_MODEL_FOV1:
            default:
                return false;
                break;
        };
    }
    else
    {
        return false;
    };
};

bool LensDB::GetVignetting(double focal, double aperture, double distance, std::vector<double> &vignetting)
{
    vignetting.clear();
    if(m_lenses==NULL)
    {
        return false;
    };
    double checkedAperture=0;
    if(m_lenses[0]->MinAperture>0)
    {
        checkedAperture=aperture;
    };
    lfLensCalibVignetting calibVig;
    if(m_lenses[0]->InterpolateVignetting(focal,checkedAperture,distance,calibVig))
    {
        switch(calibVig.Model)
        {
            case LF_VIGNETTING_MODEL_PA:
                vignetting.push_back(1.0);
                vignetting.push_back(calibVig.Terms[0]);
                vignetting.push_back(calibVig.Terms[1]);
                vignetting.push_back(calibVig.Terms[2]);
                return true;
                break;
            case LF_VIGNETTING_MODEL_NONE:
            default:
                return false;
                break;
        };
    }
    else
    {
        return false;
    };
};

bool LensDB::SaveCameraCrop(std::string filename, std::string maker, std::string model, std::string mount, double cropfactor)
{
#if LF_VERSION < MIN_LF_VERSION
    // workaround a bug in lensfun, that the numeric locale is not correctly set for saving
    char * old_locale = setlocale(LC_NUMERIC,NULL);
    old_locale = strdup(old_locale);
    setlocale(LC_NUMERIC,"C");
#endif

    if(m_newDB!=NULL)
    {
        CleanSaveInformation();
    };
    m_newDB=lf_db_new();
    if(!m_newDB)
    {
        m_newDB=NULL;
        return false;
    };
    // load if file exists already
    basic_path p(filename);
    if(boost::filesystem::exists(p))
    {
        if(boost::filesystem::is_regular_file(p))
        {
            m_newDB->Load(filename.c_str());
            //ignore errors
        };
    };
    // check if camera is already in db
    const lfCamera **oldCameras=m_newDB->FindCameras(maker.c_str(),model.c_str());
    bool updateCam=false;
    if(oldCameras)
    {
        for(size_t i=0;oldCameras[i];i++)
        {
            updateCam=true;
        };
    };

    //check, if we have a new mount
    bool newMount=false;
    if(!updateCam)
    {
        newMount=IsNewMount(mount);
    };

    //count, how many cameras are in the db
    const lfCamera *const* allCameras=m_newDB->GetCameras();
    size_t nrCam=0;
    if(allCameras)
    {
        for(size_t i=0;allCameras[i];i++)
        {
            nrCam++;
        };
    };

    if(!updateCam)
    {
        nrCam++;
    };

    //build the updated list
    //lensfun does not provide an easy way to add a new camera, so we are using this ugly workaround
    lfCamera** updatedCams=new lfCamera*[nrCam+1];
    for(size_t i=0;i<nrCam;i++)
    {
        updatedCams[i]=new lfCamera();
    };
    updatedCams[nrCam]=NULL;

    for(size_t i=0;allCameras[i];i++)
    {
        lf_camera_copy(updatedCams[i],allCameras[i]);
        if(updateCam)
        {
            if(strcmp(allCameras[i]->Maker,oldCameras[0]->Maker)==0 && 
               strcmp(allCameras[i]->Model,oldCameras[0]->Model)==0)
            {
                updatedCams[i]->CropFactor=cropfactor;
            };
        };
    };
    lf_free(oldCameras);
    if(!updateCam)
    {
        updatedCams[nrCam-1]->SetMaker(maker.c_str());
        updatedCams[nrCam-1]->SetModel(model.c_str());
        if(mount.empty())
        {
            updatedCams[nrCam-1]->SetMount("Generic");
        }
        else
        {
            updatedCams[nrCam-1]->SetMount(mount.c_str());
        };
        updatedCams[nrCam-1]->CropFactor=cropfactor;
    };

    //finally save
    lfError e;
    if(newMount)
    {
        e=m_newDB->Save(filename.c_str(), m_updatedMounts, updatedCams, m_newDB->GetLenses());
    }
    else
    {
        e=m_newDB->Save(filename.c_str(),m_newDB->GetMounts(),updatedCams,m_newDB->GetLenses());
    };
    //cleanup
    if (updatedCams)
    {
        for (int i = 0; updatedCams[i]; i++)
        {
            delete updatedCams[i];
        };
    };
    delete []updatedCams;
#if LF_VERSION < MIN_LF_VERSION
    setlocale(LC_NUMERIC, old_locale);
    free(old_locale);
#endif
    m_newDB->Destroy();
    m_newDB=NULL;
    return e==LF_NO_ERROR;
};

int LensDB::BeginSaveLens(std::string filename, std::string maker, std::string lensname, std::string mount, BaseSrcPanoImage::Projection projection, double cropfactor)
{
    if(m_newDB!=NULL)
    {
        CleanSaveInformation();
    };
    m_newDB=lf_db_new();
    if(!m_newDB)
    {
        m_newDB=NULL;
        return 1;
    };
    // load if file exists already
    basic_path p(filename);
    m_lensFilename=p.string();
    if(boost::filesystem::exists(p))
    {
        if(boost::filesystem::is_regular_file(p))
        {
#if LF_VERSION < MIN_LF_VERSION
            char * old_locale = setlocale(LC_NUMERIC,NULL);
            old_locale = strdup(old_locale);
            setlocale(LC_NUMERIC,"C");
#endif
            m_newDB->Load(m_lensFilename.c_str());
            //ignore errors
#if LF_VERSION < MIN_LF_VERSION
            setlocale(LC_NUMERIC, old_locale);
            free(old_locale);
#endif
        };
    };

    // check if lens is already in db
    lfCamera* cam=NULL;
    if(!mount.empty())
    {
        cam=new lfCamera();
        cam->SetMount(mount.c_str());
        cam->CropFactor=cropfactor;
    };
    const lfLens **oldLenses=m_newDB->FindLenses(cam,NULL,lensname.c_str());
    if(cam!=NULL)
    {
        delete cam;
    };
    bool updateLens=false;
    if(oldLenses)
    {
        for(size_t i=0;oldLenses[i];i++)
        {
            updateLens=true;
        };
    };

    if(updateLens)
    {
        BaseSrcPanoImage::Projection oldProjection;
        if(TranslateProjectionLF2Hugin(oldLenses[0],oldProjection))
        {
            if(oldProjection!=projection)
            {
                //if projection does not match, reject to save lens
                lf_free(oldLenses);
                m_newDB->Destroy();
                return 2;
            };
        };
        if(abs(oldLenses[0]->CropFactor-cropfactor)>0.05)
        {
            //if crop factor does not match, reject to save lens
            lf_free(oldLenses);
            m_newDB->Destroy();
            return 3;
        };
    }
    else
    {
        IsNewMount(mount);
    };

    //count, how many lenses are in the db
    const lfLens *const* allLenses=m_newDB->GetLenses();
    size_t nrLens=0;
    if(allLenses)
    {
        for(size_t i=0;allLenses[i];i++)
        {
            nrLens++;
        };
    };

    if(!updateLens)
    {
        nrLens++;
    };

    //build the updated list
    //lensfun does not provide an easy way to add a new camera, so we are using this ugly workaround
    m_updatedLenses=new lfLens*[nrLens+1];
    for(size_t i=0;i<nrLens;i++)
    {
        m_updatedLenses[i]=new lfLens();
    };
    m_updatedLenses[nrLens]=NULL;

    for(size_t i=0;allLenses[i];i++)
    {
        lf_lens_copy(m_updatedLenses[i],allLenses[i]);
        if(updateLens)
        {
            if(strcmp(m_updatedLenses[i]->Model,oldLenses[0]->Model)==0 && 
               strcmp(m_updatedLenses[i]->Maker,oldLenses[0]->Maker)==0)
            {
                bool compareMounts=false;
                if(oldLenses[0]->Mounts)
                {
                    for(int j=0; oldLenses[0]->Mounts[j] && !compareMounts; j++)
                    {
                        for(int k=0; m_updatedLenses[i]->Mounts[k] && !compareMounts; k++)
                        {
                            if(strcmp(m_updatedLenses[i]->Mounts[k], oldLenses[0]->Mounts[j])==0)
                            {
                                compareMounts=true;
                            };
                        };
                    };
                };
                if(compareMounts)
                {
                    m_currentLens=m_updatedLenses[i];
                };
            };
        };
    };
    lf_free(oldLenses);
    if(!updateLens)
    {
        m_currentLens=m_updatedLenses[nrLens-1];
        m_currentLens->SetModel(lensname.c_str());
        m_currentLens->SetMaker(maker.c_str());
        if(mount.empty())
        {
            m_currentLens->AddMount("Generic");
        }
        else
        {
            m_currentLens->AddMount(mount.c_str());
        };
        TranslateProjectionHugin2LF(projection,m_currentLens);
        m_currentLens->CropFactor=cropfactor;
        m_currentLens->GuessParameters();
    };
    m_currentLens->Check();
    return 0;
};

void LensDB::SaveHFOV(double focal, double hfov)
{
#if LF_VERSION>=MIN_LF_VERSION
    if(m_currentLens)
    {
        int index=-1;
        if(m_currentLens->CalibFov)
        {
            for(int i=0;m_currentLens->CalibFov[i];i++)
            {
                if(abs(m_currentLens->CalibFov[i]->Focal-focal)<0.05)
                {
                    index=i;
                    break;
                };
            };
        };
        if(index>-1)
        {
            m_currentLens->RemoveCalibFov(index);
        };
        lfLensCalibFov lcf;
        lcf.Focal=focal;
        lcf.FieldOfView=hfov;
        m_currentLens->AddCalibFov(&lcf);
    };
#endif
};

void LensDB::SaveCrop(double focal, BaseSrcPanoImage::CropMode cropMode, hugin_utils::FDiff2D cropLeftTop, hugin_utils::FDiff2D cropRightBottom)
{
#if LF_VERSION>=MIN_LF_VERSION
    if(m_currentLens)
    {
        //first run, we search and remove crop with focal length 0
        //this can happen with circular fisheye lenses
        int index=-1;
        if(m_currentLens->CalibCrop)
        {
            for(int i=0;m_currentLens->CalibCrop[i];i++)
            {
                if(m_currentLens->CalibCrop[i]->Focal==0)
                {
                    index=i;
                    break;
                };
            };
        };
        if(index>-1)
        {
            m_currentLens->RemoveCalibCrop(index);
        };
        //now searching for existing crop setting
        index=-1;
        if(m_currentLens->CalibCrop)
        {
            for(int i=0;m_currentLens->CalibCrop[i];i++)
            {
                if(abs(m_currentLens->CalibCrop[i]->Focal-focal)<0.05)
                {
                    index=i;
                    break;
                };
            };
        };
        if(index>-1)
        {
            m_currentLens->RemoveCalibCrop(index);
        };
        lfLensCalibCrop lcc;
        lcc.Focal=focal;
        switch(cropMode)
        {
            case BaseSrcPanoImage::CROP_CIRCLE:
                lcc.CropMode=LF_CROP_CIRCLE;
                break;
            case BaseSrcPanoImage::CROP_RECTANGLE:
                lcc.CropMode=LF_CROP_RECTANGLE;
                break;
            case BaseSrcPanoImage::NO_CROP:
                return;
        };
        lcc.Crop[0]=cropLeftTop.x;
        lcc.Crop[1]=cropRightBottom.x;
        lcc.Crop[2]=cropLeftTop.y;
        lcc.Crop[3]=cropRightBottom.y;
        m_currentLens->AddCalibCrop(&lcc);
    };
#endif
};

void LensDB::SaveDistortion(double focal, std::vector<double> distortion)
{
    if(m_currentLens)
    {
        if(distortion.size()!=4)
        {
            //skip invalid distortion data
            return;
        };
        int index=-1;
        if(m_currentLens->CalibDistortion)
        {
            for(int i=0;m_currentLens->CalibDistortion[i];i++)
            {
                if(abs(m_currentLens->CalibDistortion[i]->Focal-focal)<0.05)
                {
                    index=i;
                    break;
                };
            };
        };
        if(index>-1)
        {
            m_currentLens->RemoveCalibDistortion(index);
        };
        lfLensCalibDistortion lcd;
        lcd.Focal=focal;
        lcd.Model=LF_DIST_MODEL_PTLENS;
        lcd.Terms[0]=distortion[0];
        lcd.Terms[1]=distortion[1];
        lcd.Terms[2]=distortion[2];
        m_currentLens->AddCalibDistortion(&lcd);
    };
};

void LensDB::SaveVignetting(double focal, double aperture, double distance, std::vector<double> vignetting)
{
    if(m_currentLens)
    {
        if(vignetting.size()!=4)
        {
            //skip invalid vignetting data
            return;
        };
        int index=-1;
        if(m_currentLens->CalibVignetting)
        {
            for(int i=0;m_currentLens->CalibVignetting[i];i++)
            {
                if(abs(m_currentLens->CalibVignetting[i]->Focal-focal)<0.05 &&
                   abs(m_currentLens->CalibVignetting[i]->Aperture-aperture)<0.05 &&
                   abs(m_currentLens->CalibVignetting[i]->Distance-distance)<0.05)
                {
                    index=i;
                    break;
                };
            };
        };
        if(index>-1)
        {
            m_currentLens->RemoveCalibVignetting(index);
        };
        lfLensCalibVignetting lcv;
        lcv.Focal=focal;
        lcv.Aperture=aperture;
        lcv.Distance=distance;
        lcv.Model=LF_VIGNETTING_MODEL_PA;
        lcv.Terms[0]=vignetting[1];
        lcv.Terms[1]=vignetting[2];
        lcv.Terms[2]=vignetting[3];
        m_currentLens->AddCalibVignetting(&lcv);
    };
};

bool LensDB::EndSaveLens()
{
    lfError e=LF_WRONG_FORMAT;
    if(m_newDB!=NULL)
    {
#if LF_VERSION < MIN_LF_VERSION
        // lensfun does not set correctly the numeric locale for saving
        char * old_locale = setlocale(LC_NUMERIC,NULL);
        old_locale = strdup(old_locale);
        setlocale(LC_NUMERIC,"C");
#endif
        m_currentLens->Check();
        if(m_updatedMounts!=NULL)
        {
            e=m_newDB->Save(m_lensFilename.c_str(),m_updatedMounts,m_newDB->GetCameras(),m_updatedLenses);
        }
        else
        {
            e=m_newDB->Save(m_lensFilename.c_str(),m_newDB->GetMounts(),m_newDB->GetCameras(),m_updatedLenses);
        };
        m_lensFilename.clear();
#if LF_VERSION < MIN_LF_VERSION
        setlocale(LC_NUMERIC,old_locale);
        free(old_locale);
#endif
        CleanSaveInformation();
    };
    return e==LF_NO_ERROR;
};

void LensDB::CleanSaveInformation()
{
    CleanUpdatedMounts();
    if (m_updatedLenses!=NULL)
    {
        for (int i = 0; m_updatedLenses[i]; i++)
        {
            delete m_updatedLenses[i];
        };
        delete []m_updatedLenses;
        m_updatedLenses=NULL;
    };
    if(m_newDB!=NULL)
    {
        m_newDB->Destroy();
        m_newDB=NULL;
    };
};

bool LensDB::IsNewMount(std::string newMount)
{
    CleanUpdatedMounts();
    //check, if mount is already in main database
    const lfMount* mount=m_db->FindMount(newMount.c_str());
    if(mount)
    {
        return false;
    };
    //check, if mount is already in new file
    mount=NULL;
    mount=m_newDB->FindMount(newMount.c_str());
    if(mount)
    {
        return false;
    };
    //we have a new mount
    const lfMount *const* allMounts=m_newDB->GetMounts();
    size_t nrMounts=0;
    if(allMounts)
    {
        for(size_t i=0;allMounts[i];i++)
        {
            nrMounts++;
        };
    };
    nrMounts++;

    //build the updated list
    //lensfun does not provide an easy way to add a new camera, so we are using this ugly workaround
    m_updatedMounts=new lfMount*[nrMounts+1];
    m_updatedMounts[nrMounts]=NULL;

    for(size_t i=0;allMounts[i];i++)
    {
        //lf_mount_copy(m_updatedMounts[i],allMounts[i]);
        m_updatedMounts[i]=new lfMount(*allMounts[i]);
    };

    m_updatedMounts[nrMounts-1]=new lfMount();
    m_updatedMounts[nrMounts-1]->SetName(newMount.c_str());
    m_updatedMounts[nrMounts-1]->AddCompat("Generic");
    return true;
};

void LensDB::CleanUpdatedMounts()
{
    if (m_updatedMounts!=NULL)
    {
        for (int i = 0; m_updatedMounts[i]; i++)
        {
            delete m_updatedMounts[i];
        };
        delete []m_updatedMounts;
        m_updatedMounts=NULL;
    };
};

LensDBList::LensDBList()
{
    m_lenses=NULL;
};

LensDBList::~LensDBList()
{
    if(m_lenses)
    {
        lf_free(m_lenses);
    };
};

const size_t LensDBList::GetLensCount() const
{
    return m_lensCount;
};

const lfLens* LensDBList::GetLens(size_t index) const
{
    if(index<m_lensCount)
    {
        return m_lenses[index];
    }
    else
    {
        return NULL;
    };
};

void LensDBList::SetLenses(const lfLens** lenses)
{
    if(m_lenses)
    {
        lf_free(m_lenses);
    };
    m_lenses=lenses;
    m_lensCount=0;
    if(m_lenses)
    {
        for(size_t i=0;m_lenses[i];i++)
        {
            m_lensCount++;
        };
    };
};

std::string LensDBList::GetLensName(size_t index) const
{
    if(index>=m_lensCount)
    {
        return "";
    };
    std::string lensname=m_lenses[index]->Model;
    if(lensname=="Standard")
    {
        return m_camModel + " (" + m_camMaker + ")";
    }
    else
    {
        return lensname + " (Focal length multiplier: " + hugin_utils::doubleToString(m_lenses[index]->CropFactor,1) +")";
    };
};

void LensDBList::SetCameraModelMaker(const std::string camMaker, const std::string camModel)
{
    m_camMaker=camMaker;
    m_camModel=camModel;
};

} //namespace LensDB
} //namespace HuginBase
