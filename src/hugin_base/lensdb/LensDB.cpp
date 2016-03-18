/** @file LensDB.cpp
 *
 *  @brief Implementation of wrapper around function for access to lens database
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

#include "LensDB.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <hugin_utils/stl_utils.h>
#include <hugin_utils/utils.h>
#include <sqlite3.h>
#include <panodata/StandardImageVariableGroups.h>
#include <algorithms/basic/CalculateCPStatistics.h>

namespace HuginBase
{

namespace LensDB
{

LensDB* LensDB::m_instance=NULL;

// internal class to handle all transfer from/to database
// to prevent including of sqlite3.h into hugin_base header
class LensDB::Database
{
public:
    //struct for retrieving lens data
    struct HFOVData
    {
        double focallength;
        double HFOV;
    };
    struct CropData
    {
        double focallength;
        int left, right, top, bottom;
    };
    struct Distortiondata
    {
        double focallength;
        double a, b, c;
    };
    struct Vignettingdata
    {
        double focallength;
        double aperture;
        double distance;
        double Vb, Vc, Vd;
    };
    struct TCAdata
    {
        double focallength;
        double ra, rb, rc, rd;
        double ba, bb, bc, bd;
    };
    //constructor, open database
    explicit Database(const std::string& filename) : m_filename(filename), m_runningTransaction(false)
    {
        bool newDB = (!hugin_utils::FileExists(m_filename));
        int error = sqlite3_open(m_filename.c_str(), &m_db);
        if (error)
        {
            std::cerr << "Can't open database: " << sqlite3_errmsg(m_db) << std::endl;
            m_db = NULL;
            m_filename = std::string();
        };
        if (newDB)
        {
            if (!CreateTables())
            {
                //something went wrong with the generation of the database structure
                sqlite3_close(m_db);
                m_db = NULL;
                m_filename = std::string();
            };
        }
        else
        {
            // check version and update database when necessary
            // not yet implemented
            // if (GetDBVersion() < 2) UpdateDatabase();
        };
    };
    // destructor, destroy database
    ~Database()
    {
        if (m_db)
        {
            if (m_runningTransaction)
            {
                EndTransaction();
            };
            sqlite3_close(m_db);
        };
    };
    // create the tables for the database
    bool CreateTables()
    {
        const char* createDB = "PRAGMA user_version=1;"
            "CREATE TABLE CameraCropTable (Maker TEXT, Model TEXT, Cropfactor REAL, PRIMARY KEY (Maker, Model));"
            "CREATE TABLE LensProjectionTable (Lens TEXT PRIMARY KEY, Projection INTEGER);"
            "CREATE TABLE LensHFOVTable (Lens TEXT, Focallength REAL, HFOV REAL, Weight INTEGER);"
            "CREATE INDEX HFOV_IndexLens ON LensHFOVTable (Lens);"
            "CREATE INDEX HFOV_IndexLens2 ON LensHFOVTable (Lens, Focallength);"
            "CREATE TABLE LensCropTable (Lens TEXT, Focallength REAL, Width INTEGER, Height INTEGER, CropLeft INTEGER, CropRight INTEGER, CropTop INTEGER, CropBottom INTEGER, PRIMARY KEY (Lens, Focallength, Width, Height));"
            "CREATE TABLE DistortionTable(Lens TEXT, Focallength REAL, a REAL, b REAL, c REAL, Weight INTEGER);"
            "CREATE INDEX Dist_IndexLens ON DistortionTable (Lens);"
            "CREATE INDEX Dist_IndexLensFocal ON DistortionTable (Lens, Focallength);"
            "CREATE TABLE VignettingTable (Lens TEXT, Focallength REAL, Aperture REAL, Distance REAL, Vb REAL, Vc REAL, Vd REAL, Weight INTEGER);"
            "CREATE INDEX Vig_IndexLens ON VignettingTable (Lens);"
            "CREATE INDEX Vig_IndexLensFocal ON VignettingTable (Lens, Focallength);"
            "CREATE INDEX Vig_IndexLensFocalApertureDistance ON VignettingTable (Lens, Focallength, Aperture, Distance);"
            "CREATE TABLE TCATable (Lens TEXT, Focallength REAL, ra REAL, rb REAL, rc REAL, rd REAL, ba REAL, bb REAL, bc REAL, bd REAL, Weight INTEGER);"
            "CREATE INDEX TCA_IndexLens ON TCATable (Lens);"
            "CREATE INDEX TCA_IndexLensFocal ON TCATable (Lens, Focallength);"
            "CREATE TABLE EMORTable (Maker TEXT, Model TEXT, ISO INTEGER, Ra REAL, Rb REAL, Rc REAL, Rd REAL, Re REAL, Weight INTEGER);"
            "CREATE INDEX EMOR_Index_Cam ON EMORTable (Maker, Model);"
            "CREATE INDEX EMOR_Index_CamISO ON EMORTable (Maker, Model, ISO);";
        if (m_db == NULL)
        {
            return false;
        };
        if (sqlite3_exec(m_db, createDB, NULL, NULL, NULL) == SQLITE_OK)
        {
            return true;
        }
        else
        {
            std::cerr << "Could not create database structure." << std::endl;
            return false;
        };
    };
    // report the database version, implemented by PRAGMA user_version
    // currently not used, for further extensions of the database structure
    // @return db version, or -1 if db could not opened/initialized
    int GetDBVersion() const
    {
        if (m_db == NULL)
        {
            return -1;
        };
        sqlite3_stmt *statement;
        const char *tail;
        int version = 0;
        if (sqlite3_prepare_v2(m_db, "PRAGMA user_version;", -1, &statement, &tail) == SQLITE_OK)
        {
            if (sqlite3_step(statement) == SQLITE_ROW)
            {
                version = sqlite3_column_int(statement, 0);
            };
        };
        sqlite3_finalize(statement);
        return version;
    };
    // returns the filename of the database
    std::string GetDBFilename() const
    {
        return m_filename;
    };
    // search for the crop factor of the given camera in the database
    // returns true of camera was found, otherwise false
    bool GetCropFactor(const std::string& maker, const std::string& model, double &cropFactor) const
    {
        cropFactor = 0;
        if (m_db == NULL)
        {
            return false;
        };
        sqlite3_stmt *statement;
        const char *tail;
        if (sqlite3_prepare_v2(m_db, "SELECT Cropfactor FROM CameraCropTable WHERE Maker=?1 AND Model=?2;", -1, &statement, &tail) == SQLITE_OK)
        {
            sqlite3_bind_text(statement, 1, maker.c_str(), -1, NULL);
            sqlite3_bind_text(statement, 2, model.c_str(), -1, NULL);
            if (sqlite3_step(statement) == SQLITE_ROW)
            {
                cropFactor = sqlite3_column_double(statement, 0);
            };
        };
        sqlite3_finalize(statement);
        if (cropFactor < 0.1 || cropFactor>100)
        {
            cropFactor = 0;
        };
        return cropFactor > 0.1;
    };
    // saves the crop factor for the given camera in the database
    // returns true, if data were successful saved into db, false if errors occurred during saving
    bool SaveCropFactor(const std::string& maker, const std::string& model, const double cropFactor)
    {
        if (m_db == NULL)
        {
            return false;
        };
        // do some range checking
        if (cropFactor < 0.1 || cropFactor > 100)
        {
            return false;
        };
        sqlite3_stmt *statement;
        const char *tail;
        int returnValue = 0;
        BeginTransaction();
        if (sqlite3_prepare_v2(m_db, "INSERT OR FAIL INTO CameraCropTable (Maker, Model, Cropfactor) VALUES(?1,?2,?3);", -1, &statement, &tail) == SQLITE_OK)
        {
            sqlite3_bind_text(statement, 1, maker.c_str(), -1, NULL);
            sqlite3_bind_text(statement, 2, model.c_str(), -1, NULL);
            sqlite3_bind_double(statement, 3, cropFactor);
            returnValue = sqlite3_step(statement);
            if (returnValue == SQLITE_CONSTRAINT)
            {
                sqlite3_finalize(statement);
                if (sqlite3_prepare_v2(m_db, "UPDATE CameraCropTable SET Cropfactor=?3 WHERE Maker=?1 AND Model=?2;", -1, &statement, &tail) == SQLITE_OK)
                {
                    sqlite3_bind_text(statement, 1, maker.c_str(), -1, NULL);
                    sqlite3_bind_text(statement, 2, model.c_str(), -1, NULL);
                    sqlite3_bind_double(statement, 3, cropFactor);
                    returnValue = sqlite3_step(statement);
                };
            };
        };
        sqlite3_finalize(statement);
        EndTransaction();
        return returnValue == SQLITE_DONE;
    };
    // search for the projection of the given lens in the database
    // returns true if lens information were was found, otherwise false
    bool GetLensProjection(const std::string& lens, int &projection) const
    {
        projection = -1;
        if (m_db == NULL)
        {
            return false;
        };
        sqlite3_stmt *statement;
        const char *tail;
        if (sqlite3_prepare_v2(m_db, "SELECT Projection FROM LensProjectionTable WHERE Lens=?1;", -1, &statement, &tail) == SQLITE_OK)
        {
            sqlite3_bind_text(statement, 1, lens.c_str(), -1, NULL);
            if (sqlite3_step(statement) == SQLITE_ROW)
            {
                projection = sqlite3_column_int(statement, 0);
            };
        };
        sqlite3_finalize(statement);
        return projection != -1;
    };
    // saves the projection for the given lens in the database
    // returns true, if data were successful saved into db, false if errors occurred during saving
    bool SaveLensProjection(const std::string& lens, const int projection)
    {
        if (m_db == NULL)
        {
            return false;
        };
        sqlite3_stmt *statement;
        const char *tail;
        int returnValue = 0;
        BeginTransaction();
        if (sqlite3_prepare_v2(m_db, "INSERT OR FAIL INTO LensProjectionTable (Lens, Projection) VALUES(?1,?2);", -1, &statement, &tail) == SQLITE_OK)
        {
            sqlite3_bind_text(statement, 1, lens.c_str(), -1, NULL);
            sqlite3_bind_int(statement, 2, projection);
            returnValue = sqlite3_step(statement);
            if (returnValue == SQLITE_CONSTRAINT)
            {
                sqlite3_finalize(statement);
                if (sqlite3_prepare_v2(m_db, "UPDATE LensProjectionTable SET Projection=?2 WHERE Lens=?1;", -1, &statement, &tail) == SQLITE_OK)
                {
                    sqlite3_bind_text(statement, 1, lens.c_str(), -1, NULL);
                    sqlite3_bind_int(statement, 2, projection);
                    returnValue = sqlite3_step(statement);
                };
            };
        };
        sqlite3_finalize(statement);
        EndTransaction();
        return returnValue == SQLITE_DONE;
    };
    // search for the HFOV for the given lens in the database
    // returns true of data for lens was found, otherwise false
    // returns 2 datasets with the values for the 2 focal lengths
    bool GetHFOV(const std::string& lens, const double focallength, std::vector<HFOVData>& hfovData) const
    {
        hfovData.clear();
        if (m_db == NULL)
        {
            return false;
        };
        sqlite3_stmt *statement;
        const char *tail;
        if (sqlite3_prepare_v2(m_db, "SELECT Focallength, SUM(HFOV*Weight)/SUM(Weight) FROM LensHFOVTable WHERE Lens=?1 GROUP BY Focallength ORDER BY ABS(Focallength-?2) ASC LIMIT 2;", -1, &statement, &tail) == SQLITE_OK)
        {
            sqlite3_bind_text(statement, 1, lens.c_str(), -1, NULL);
            sqlite3_bind_double(statement, 2, focallength);
            while (sqlite3_step(statement) == SQLITE_ROW)
            {
                HFOVData newhfovData;
                newhfovData.focallength = sqlite3_column_double(statement, 0);
                newhfovData.HFOV = sqlite3_column_double(statement, 1);
                hfovData.push_back(newhfovData);
            };
        };
        sqlite3_finalize(statement);
        return !hfovData.empty();
    };
    // saves the HFOV data in the database
    // returns true, if data were successful saved into db, false if errors occurred during saving
    bool SaveHFOV(const std::string& lens, const double focallength, const double HFOV, const int weight = 10)
    {
        if (m_db == NULL)
        {
            return false;
        };
        // range checking
        if (HFOV < 0.1 || HFOV>360)
        {
            return false;
        };
        sqlite3_stmt *statement;
        const char *tail;
        int returnValue = 0;
        if (sqlite3_prepare_v2(m_db, "INSERT INTO LensHFOVTable(Lens, Focallength, HFOV, Weight) VALUES(?1,?2,?3,?4);", -1, &statement, &tail) == SQLITE_OK)
        {
            sqlite3_bind_text(statement, 1, lens.c_str(), -1, NULL);
            sqlite3_bind_double(statement, 2, focallength);
            sqlite3_bind_double(statement, 3, HFOV);
            sqlite3_bind_int(statement, 4, weight);
            returnValue = sqlite3_step(statement);
        };
        sqlite3_finalize(statement);
        return returnValue == SQLITE_DONE;
    };
    // search for the crop of the given lens in the database
    // returns true if lens information were was found, otherwise false
    bool GetLensCrop(const std::string& lens, const double focal, const int width, const int height, std::vector<CropData> &cropData) const
    {
        cropData.clear();
        if (m_db == NULL)
        {
            return false;
        };
        sqlite3_stmt *statement;
        const char *tail;
        if (sqlite3_prepare_v2(m_db, "SELECT Focallength, CropLeft, CropRight, CropTop, CropBottom FROM LensCropTable WHERE Lens=?1 AND Width=?2 AND Height=?3 ORDER BY ABS(Focallength-?4) ASC LIMIT 2;", -1, &statement, &tail) == SQLITE_OK)
        {
            sqlite3_bind_text(statement, 1, lens.c_str(), -1, NULL);
            sqlite3_bind_int(statement, 2, width);
            sqlite3_bind_int(statement, 3, height);
            sqlite3_bind_double(statement, 4, focal);
            while (sqlite3_step(statement) == SQLITE_ROW)
            {
                CropData newCropData;
                newCropData.focallength = sqlite3_column_double(statement, 0);
                newCropData.left = sqlite3_column_int(statement, 1);
                newCropData.right = sqlite3_column_int(statement, 2);
                newCropData.top = sqlite3_column_int(statement, 3);
                newCropData.bottom = sqlite3_column_int(statement, 4);
                cropData.push_back(newCropData);
            };
        };
        sqlite3_finalize(statement);
        return !cropData.empty();
    };
    // saves the crop for the given lens in the database
    // returns true, if data were successful saved into db, false if errors occurred during saving
    bool SaveLensCrop(const std::string& lens, const double focal, const int width, const int height, const int left, const int right, const int top, const int bottom)
    {
        if (m_db == NULL)
        {
            return false;
        };
        sqlite3_stmt *statement;
        const char *tail;
        int returnValue = 0;
        BeginTransaction();
        if (sqlite3_prepare_v2(m_db, "INSERT OR FAIL INTO LensCropTable (Lens, Focallength, Width, Height, CropLeft, CropRight, CropTop, CropBottom) VALUES(?1,?2,?3,?4,?5,?6,?7,?8);", -1, &statement, &tail) == SQLITE_OK)
        {
            sqlite3_bind_text(statement, 1, lens.c_str(), -1, NULL);
            sqlite3_bind_double(statement, 2, focal);
            sqlite3_bind_int(statement, 3, width);
            sqlite3_bind_int(statement, 4, height);
            sqlite3_bind_int(statement, 5, left);
            sqlite3_bind_int(statement, 6, right);
            sqlite3_bind_int(statement, 7, top);
            sqlite3_bind_int(statement, 8, bottom);
            returnValue = sqlite3_step(statement);
            if (returnValue == SQLITE_CONSTRAINT)
            {
                sqlite3_finalize(statement);
                if (sqlite3_prepare_v2(m_db, "UPDATE LensCropTable SET CropLeft=?5, CropRight=?6, CropTop=?7, CropBottom=?8 WHERE Lens=?1 AND Focallength=?2 AND Width=?3 AND Height=?4;", -1, &statement, &tail) == SQLITE_OK)
                {
                    sqlite3_bind_text(statement, 1, lens.c_str(), -1, NULL);
                    sqlite3_bind_double(statement, 2, focal);
                    sqlite3_bind_int(statement, 3, width);
                    sqlite3_bind_int(statement, 4, height);
                    sqlite3_bind_int(statement, 5, left);
                    sqlite3_bind_int(statement, 6, right);
                    sqlite3_bind_int(statement, 7, top);
                    sqlite3_bind_int(statement, 8, bottom);
                    returnValue = sqlite3_step(statement);
                };
            };
        };
        sqlite3_finalize(statement);
        EndTransaction();
        return returnValue == SQLITE_DONE;
    };
    // removes the crop information for the given focallength and imagesize
    bool RemoveLensCrop(const std::string& lens, const double focal, const int width, const int height)
    {
        if (m_db == NULL)
        {
            return false;
        };
        sqlite3_stmt *statement;
        const char *tail;
        int returnValue = 0;
        if (sqlite3_prepare_v2(m_db, "DELETE FROM LensCropTable WHERE Lens=?1 AND Focallength=?2 AND Width=?3 AND Height=?4;", -1, &statement, &tail) == SQLITE_OK)
        {
            sqlite3_bind_text(statement, 1, lens.c_str(), -1, NULL);
            sqlite3_bind_double(statement, 2, focal);
            sqlite3_bind_int(statement, 3, width);
            sqlite3_bind_int(statement, 4, height);
            returnValue = sqlite3_step(statement);
        };
        sqlite3_finalize(statement);
        EndTransaction();
        return returnValue == SQLITE_DONE;
    };

    // search for the distortion data for the given lens in the database
    // returns true of data for lens was found, otherwise false
    // returns 2 datasets with the values for the 2 nearest focal lengths
    bool GetDistortionData(const std::string& lens, const double focallength, std::vector<Distortiondata>& distData) const
    {
        distData.clear();
        if (m_db == NULL)
        {
            return false;
        };
        sqlite3_stmt *statement;
        const char *tail;
        if (sqlite3_prepare_v2(m_db, "SELECT Focallength, SUM(a*Weight)/SUM(Weight), SUM(b*Weight)/SUM(Weight), SUM(c*Weight)/SUM(Weight) FROM DistortionTable WHERE Lens=?1 GROUP BY Focallength ORDER BY ABS(Focallength-?2) ASC LIMIT 2;", -1, &statement, &tail) == SQLITE_OK)
        {
            sqlite3_bind_text(statement, 1, lens.c_str(), -1, NULL);
            sqlite3_bind_double(statement, 2, focallength);
            while (sqlite3_step(statement) == SQLITE_ROW)
            {
                Distortiondata newDistData;
                newDistData.focallength = sqlite3_column_double(statement, 0);
                newDistData.a = sqlite3_column_double(statement, 1);
                newDistData.b = sqlite3_column_double(statement, 2);
                newDistData.c = sqlite3_column_double(statement, 3);
                distData.push_back(newDistData);
            };
        };
        sqlite3_finalize(statement);
        return !distData.empty();
    };
    // saves the distortion data in the database
    // returns true, if data were successful saved into db, false if errors occurred during saving
    bool SaveDistortion(const std::string& lens, const double focallength, const double a, const double b, const double c, const int weight = 10)
    {
        if (m_db == NULL)
        {
            return false;
        };
        sqlite3_stmt *statement;
        const char *tail;
        int returnValue = 0;
        if (sqlite3_prepare_v2(m_db, "INSERT INTO DistortionTable(Lens, Focallength, a, b, c, Weight) VALUES(?1,?2,?3,?4,?5,?6);", -1, &statement, &tail) == SQLITE_OK)
        {
            sqlite3_bind_text(statement, 1, lens.c_str(), -1, NULL);
            sqlite3_bind_double(statement, 2, focallength);
            sqlite3_bind_double(statement, 3, a);
            sqlite3_bind_double(statement, 4, b);
            sqlite3_bind_double(statement, 5, c);
            sqlite3_bind_int(statement, 6, weight);
            returnValue = sqlite3_step(statement);
        };
        sqlite3_finalize(statement);
        return returnValue == SQLITE_DONE;
    };
    // search for the vignetting data for the given lens in the database
    // returns true of data for lens was found, otherwise false
    // returns maximal 4 datasets: datasets of the 2 nearest focallengths and for each focallength the 2 nearest apertures
    bool GetVignettingData(const std::string& lens, const double focallength, const double aperture, std::vector<Vignettingdata>& vigData) const
    {
        vigData.clear();
        if (m_db == NULL)
        {
            return false;
        };
        sqlite3_stmt *statement;
        const char *tail;
        if (sqlite3_prepare_v2(m_db,
            "SELECT Focallength, Aperture, SUM(Vb*Weight)/SUM(Weight), SUM(Vc*Weight)/SUM(Weight), SUM(Vd*Weight)/SUM(Weight) FROM VignettingTable "
            "WHERE Lens = ?1 AND ("
            "("
            "Focallength IN "
            "(SELECT Focallength FROM VignettingTable WHERE Lens=?1 GROUP BY Focallength ORDER BY ABS(Focallength-?2) LIMIT 1) "
            "AND Aperture IN "
            "(SELECT Aperture FROM VignettingTable WHERE Lens=?1 AND "
            "Focallength IN "
            "(SELECT Focallength from VignettingTable WHERE Lens=?1 GROUP BY Focallength ORDER BY ABS(Focallength-?2) LIMIT 1) "
            "GROUP BY Aperture ORDER BY ABS(Aperture-?3) LIMIT 2)"
            ") OR ("
            "Focallength IN "
            "(SELECT Focallength FROM VignettingTable WHERE Lens=?1 GROUP BY Focallength ORDER BY ABS(Focallength-?2) LIMIT 1 OFFSET 1) "
            "AND Aperture IN "
            "(SELECT Aperture FROM VignettingTable WHERE Lens=?1 AND "
            "Focallength IN "
            "(SELECT Focallength FROM VignettingTable WHERE Lens=?1 GROUP BY Focallength ORDER BY ABS(Focallength-?2) LIMIT 1 OFFSET 1) "
            "GROUP BY Aperture ORDER BY ABS(Aperture-?3) LIMIT 2)"
            ")"
            ")"
            "GROUP BY Focallength, Aperture ORDER BY Focallength, Aperture;",
            -1, &statement, &tail) == SQLITE_OK)
        {
            sqlite3_bind_text(statement, 1, lens.c_str(), -1, NULL);
            sqlite3_bind_double(statement, 2, focallength);
            sqlite3_bind_double(statement, 3, aperture);
            while (sqlite3_step(statement) == SQLITE_ROW)
            {
                Vignettingdata newVigData;
                newVigData.focallength = sqlite3_column_double(statement, 0);
                newVigData.aperture = sqlite3_column_double(statement, 1);
                newVigData.Vb = sqlite3_column_double(statement, 2);
                newVigData.Vc = sqlite3_column_double(statement, 3);
                newVigData.Vd = sqlite3_column_double(statement, 4);
                vigData.push_back(newVigData);
            };
        };
        sqlite3_finalize(statement);
        return !vigData.empty();
    };
    // saves the vignetting data in the database
    // returns true, if data were successful saved into db, false if errors occurred during saving
    bool SaveVignetting(const std::string& lens, const double focallength, const double aperture, const double distance, const double Vb, const double Vc, const double Vd, const int weight = 10)
    {
        if (m_db == NULL)
        {
            return false;
        };
        sqlite3_stmt *statement;
        const char *tail;
        int returnValue = 0;
        if (sqlite3_prepare_v2(m_db, "INSERT INTO VignettingTable(Lens, Focallength, Aperture, Distance, Vb, Vc, Vd, Weight) VALUES(?1,?2,?3,?4,?5,?6,?7,?8);", -1, &statement, &tail) == SQLITE_OK)
        {
            sqlite3_bind_text(statement, 1, lens.c_str(), -1, NULL);
            sqlite3_bind_double(statement, 2, focallength);
            sqlite3_bind_double(statement, 3, aperture);
            sqlite3_bind_double(statement, 4, distance);
            sqlite3_bind_double(statement, 5, Vb);
            sqlite3_bind_double(statement, 6, Vc);
            sqlite3_bind_double(statement, 7, Vd);
            sqlite3_bind_int(statement, 8, weight);
            returnValue = sqlite3_step(statement);
        };
        sqlite3_finalize(statement);
        return returnValue == SQLITE_DONE;
    };
    // search for the tca data for the given lens in the database
    // returns true of data for lens was found, otherwise false
    // returns 2 datasets with the values for the 2 nearest focal lengths
    bool GetTCAData(const std::string& lens, const double focallength, std::vector<TCAdata>& tcaData) const
    {
        tcaData.clear();
        if (m_db == NULL)
        {
            return false;
        };
        sqlite3_stmt *statement;
        const char *tail;
        if (sqlite3_prepare_v2(m_db, "SELECT Focallength, SUM(ra*Weight)/SUM(Weight), SUM(rb*Weight)/SUM(Weight), SUM(rc*Weight)/SUM(Weight), SUM(rd*Weight)/SUM(Weight), SUM(ba*Weight)/SUM(Weight), SUM(bb*Weight)/SUM(Weight), SUM(bc*Weight)/SUM(Weight), SUM(bd*Weight)/SUM(Weight) FROM TCATable WHERE Lens=?1 GROUP BY Focallength ORDER BY ABS(Focallength-?2) ASC LIMIT 2;", -1, &statement, &tail) == SQLITE_OK)
        {
            sqlite3_bind_text(statement, 1, lens.c_str(), -1, NULL);
            sqlite3_bind_double(statement, 2, focallength);
            while (sqlite3_step(statement) == SQLITE_ROW)
            {
                TCAdata newTCAData;
                newTCAData.focallength = sqlite3_column_double(statement, 0);
                newTCAData.ra = sqlite3_column_double(statement, 1);
                newTCAData.rb = sqlite3_column_double(statement, 2);
                newTCAData.rc = sqlite3_column_double(statement, 3);
                newTCAData.rd = sqlite3_column_double(statement, 4);
                newTCAData.ba = sqlite3_column_double(statement, 5);
                newTCAData.bb = sqlite3_column_double(statement, 6);
                newTCAData.bc = sqlite3_column_double(statement, 7);
                newTCAData.bd = sqlite3_column_double(statement, 8);
                tcaData.push_back(newTCAData);
            };
        };
        sqlite3_finalize(statement);
        return !tcaData.empty();
    };
    // saves the tca data in the database
    // returns true, if data were successful saved into db, false if errors occurred during saving
    bool SaveTCAData(const std::string& lens, const double focallength, const double ra, const double rb, const double rc, const double rd,
        const double ba, const double bb, const double bc, const double bd, const int weight = 10)
    {
        if (m_db == NULL)
        {
            return false;
        };
        sqlite3_stmt *statement;
        const char *tail;
        int returnValue = 0;
        if (sqlite3_prepare_v2(m_db, "INSERT INTO TCATable(Lens, Focallength, ra, rb, rc, rd, ba, bb, bc, bd, Weight) VALUES(?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11);", -1, &statement, &tail) == SQLITE_OK)
        {
            sqlite3_bind_text(statement, 1, lens.c_str(), -1, NULL);
            sqlite3_bind_double(statement, 2, focallength);
            sqlite3_bind_double(statement, 3, ra);
            sqlite3_bind_double(statement, 4, rb);
            sqlite3_bind_double(statement, 5, rc);
            sqlite3_bind_double(statement, 6, rd);
            sqlite3_bind_double(statement, 7, ba);
            sqlite3_bind_double(statement, 8, bb);
            sqlite3_bind_double(statement, 9, bc);
            sqlite3_bind_double(statement, 10, bd);
            sqlite3_bind_int(statement, 11, weight);
            returnValue = sqlite3_step(statement);
        };
        sqlite3_finalize(statement);
        return returnValue == SQLITE_DONE;
    };
    // saves the EMoR data in the database
    // returns true, if data were successful saved into db, false if errors occurred during saving
    bool SaveEMoR(const std::string& maker, const std::string& model, const int iso, const double Ra, const double Rb, const double Rc, const double Rd, const double Re, const int weight = 10)
    {
        if (m_db == NULL)
        {
            return false;
        };
        sqlite3_stmt *statement;
        const char *tail;
        int returnValue = 0;
        if (sqlite3_prepare_v2(m_db, "INSERT INTO EMORTable(Maker, Model, ISO, Ra, Rb, Rc, Rd, Re, Weight) VALUES(?1,?2,?3,?4,?5,?6,?7,?8,?9);", -1, &statement, &tail) == SQLITE_OK)
        {
            sqlite3_bind_text(statement, 1, maker.c_str(), -1, NULL);
            sqlite3_bind_text(statement, 2, model.c_str(), -1, NULL);
            sqlite3_bind_int(statement, 3, iso);
            sqlite3_bind_double(statement, 4, Ra);
            sqlite3_bind_double(statement, 5, Rb);
            sqlite3_bind_double(statement, 6, Rc);
            sqlite3_bind_double(statement, 7, Rd);
            sqlite3_bind_double(statement, 8, Re);
            sqlite3_bind_int(statement, 9, weight);
            returnValue = sqlite3_step(statement);
        };
        sqlite3_finalize(statement);
        return returnValue == SQLITE_DONE;
    };
    // return a list of lens names which has certain information
    bool GetLensNames(const bool distortion, const bool vignetting, const bool tca, LensList& lensList) const
    {
        lensList.clear();
        if (m_db == NULL)
        {
            return false;
        };
        sqlite3_stmt *statement;
        const std::string statement_distortion("SELECT DISTINCT Lens FROM DistortionTable");
        const std::string statement_vignetting("SELECT DISTINCT Lens FROM VignettingTable");
        const std::string statement_tca("SELECT DISTINCT Lens FROM TCATable");
        std::string statementString;
        if (distortion)
        {
            statementString = statement_distortion;
        };
        if (vignetting)
        {
            if (!statementString.empty())
            {
                statementString.append(" UNION ");
            };
            statementString.append(statement_vignetting);
        };
        if (tca)
        {
            if (!statementString.empty())
            {
                statementString.append(" UNION ");
            };
            statementString.append(statement_tca);
        };
        if (statementString.empty())
        {
            return false;
        };
        const char *tail;
        if (sqlite3_prepare_v2(m_db, statementString.c_str(), -1, &statement, &tail) == SQLITE_OK)
        {
            while (sqlite3_step(statement) == SQLITE_ROW)
            {
                std::stringstream stream;
                stream << sqlite3_column_text(statement, 0);
                lensList.push_back(stream.str());
            };
        };
        sqlite3_finalize(statement);
        return !lensList.empty();
    };
    // clean up data base
    bool CleanUp()
    {
        if (m_db == NULL)
        {
            return false;
        };
        BeginTransaction();
        sqlite3_exec(m_db,
            "INSERT INTO DistortionTable(Lens, Focallength, a, b, c, Weight) "
            "SELECT Lens, Focallength, SUM(a*Weight)/SUM(Weight), SUM(b*Weight)/SUM(Weight), SUM(c*Weight)/SUM(Weight), SUM(Weight*Weight)/SUM(Weight)*-1 FROM DistortionTable GROUP By Lens, Focallength;"
            "DELETE FROM DistortionTable WHERE Weight>=0;"
            "UPDATE DistortionTable SET Weight=-Weight WHERE Weight<0;"
            "INSERT INTO LensHFOVTable(Lens, Focallength, HFOV, Weight) "
              "SELECT Lens, Focallength, SUM(HFOV*Weight)/SUM(Weight), SUM(Weight*Weight)/SUM(Weight)*-1 FROM LensHFOVTable GROUP By Lens, Focallength;"
            "DELETE FROM LensHFOVTable WHERE Weight>=0;"
            "UPDATE LensHFOVTable SET Weight=-Weight WHERE Weight<0;"
            "INSERT INTO TCATable(Lens, Focallength, ra, rb, rc, rd, ba, bb, bc, bd, Weight) "
              "SELECT Lens, Focallength, SUM(ra*Weight)/SUM(Weight), SUM(rb*Weight)/SUM(Weight), SUM(rc*Weight)/SUM(Weight), SUM(rd*Weight)/SUM(Weight), SUM(ba*Weight)/SUM(Weight), SUM(bb*Weight)/SUM(Weight), SUM(bc*Weight)/SUM(Weight), SUM(bd*Weight)/SUM(Weight), SUM(Weight*Weight)/SUM(Weight)*-1 FROM TCATable GROUP By Lens, Focallength;"
            "DELETE FROM TCATable WHERE Weight>=0;"
            "UPDATE TCATable SET Weight=-Weight WHERE Weight<0;"
            "INSERT INTO VignettingTable(Lens, Focallength, Aperture, Distance, Vb, Vc, Vd, Weight) "
              "SELECT Lens, Focallength, Aperture, Distance, SUM(Vb*Weight)/SUM(Weight), SUM(Vc*Weight)/SUM(Weight), SUM(Vd*Weight)/SUM(Weight), SUM(Weight*Weight)/SUM(Weight)*-1 FROM VignettingTable GROUP By Lens, Focallength, Aperture, Distance;"
            "DELETE FROM VignettingTable WHERE Weight>=0;"
            "UPDATE VignettingTable SET Weight=-Weight WHERE Weight<0;"
            "INSERT INTO EMORTable(Maker, Model, ISO, Ra, Rb, Rc, Rd, Re, Weight) "
            "SELECT Maker, Model, ISO, SUM(Ra*Weight)/SUM(Weight), SUM(Rb*Weight)/SUM(Weight), SUM(Rc*Weight)/SUM(Weight), SUM(Rd*Weight)/SUM(Weight), SUM(Re*Weight)/SUM(Weight), SUM(Weight*Weight)/SUM(Weight)*-1 FROM EMORTable GROUP By Maker, Model, ISO;"
            "DELETE FROM EMORTable WHERE Weight>=0;"
            "UPDATE EMORTable SET Weight=-Weight WHERE Weight<0;",
            NULL, NULL, NULL);
        EndTransaction();
        return sqlite3_exec(m_db, "VACUUM;", NULL, NULL, NULL) == SQLITE_OK;
    };
    // remove lens from all tables
    bool RemoveLens(const std::string& lensname)
    {
        if (m_db == NULL)
        {
            return false;
        };
        BeginTransaction();
        bool result = RemoveLensFromTable("LensProjectionTable", lensname);
        result &= RemoveLensFromTable("LensHFOVTable", lensname);
        result &= RemoveLensFromTable("LensCropTable", lensname);
        result &= RemoveLensFromTable("DistortionTable", lensname);
        result &= RemoveLensFromTable("VignettingTable", lensname);
        result &= RemoveLensFromTable("TCATable", lensname);
        EndTransaction();
        return result;
    };
    // remove camera from database
    bool RemoveCamera(const std::string& maker, const std::string& model)
    {
        if (m_db == NULL)
        {
            return false;
        };
        BeginTransaction();
        bool result = RemoveCameraFromTable("CameraCropTable", maker, model);
        result &= RemoveCameraFromTable("EMORTable", maker, model);
        EndTransaction();
        return result;
    };
    // export to file
    bool ExportToFile(const std::string& filename)
    {
        if (m_db == NULL)
        {
            return false;
        };
        CleanUp();
        std::ofstream output(filename.c_str());
        if (output.is_open())
        {
            output << "TABLE=CameraCropTable" << std::endl
                << "COLUMNS=Maker;Model;Cropfactor" << std::endl;
            OutputSQLToStream("SELECT Maker, Model, Cropfactor FROM CameraCropTable;", output);
            output << "ENDTABLE" << std::endl
                << "TABLE=LensProjectionTable" << std::endl
                << "COLUMNS=Lens;Projection" << std::endl;
            OutputSQLToStream("SELECT Lens, Projection FROM LensProjectionTable;", output);
            output << "ENDTABLE" << std::endl
                << "TABLE=LensHFOVTable" << std::endl
                << "COLUMNS=Lens;Focallength;HFOV;Weight" << std::endl;
            OutputSQLToStream("SELECT Lens, Focallength, HFOV, Weight FROM LensHFOVTable;", output);
            output << "ENDTABLE" << std::endl
                << "TABLE=LensCropTable" << std::endl
                << "COLUMNS=Lens;Focallength;Width;Height;CropLeft;CropRight;CropTop;CropBottom" << std::endl;
            OutputSQLToStream("SELECT Lens, Focallength, Width, Height, CropLeft, CropRight, CropTop, CropBottom FROM TABLE LensCropTable;", output);
            output << "ENDTABLE" << std::endl
                << "TABLE=DistortionTable" << std::endl
                << "COLUMNS=Lens;Focallength;a;b;c;Weight" << std::endl;
            OutputSQLToStream("SELECT Lens, Focallength, a, b, c, Weight FROM DistortionTable;", output);
            output << "ENDTABLE" << std::endl
                << "TABLE=VignettingTable" << std::endl
                << "COLUMNS=Lens;Focallength;Aperture;Distance;Vb;Vc;Vd;Weight" << std::endl;
            OutputSQLToStream("SELECT Lens, Focallength, Aperture, Distance, Vb, Vc, Vd, Weight FROM VignettingTable;", output);
            output << "ENDTABLE" << std::endl
                << "TABLE=TCATable" << std::endl
                << "COLUMNS=Lens;Focallength;ra;rb;rc;rd;ba;bb;bc;bd;Weight" << std::endl;
            OutputSQLToStream("SELECT Lens, Focallength, ra, rb, rc, rd, ba, bb, bc, bd, Weight FROM TCATable;", output);
            output << "ENDTABLE" << std::endl
                << "TABLE=EMORTable" << std::endl
                << "COLUMNS=Maker;Model;ISO;Ra;Rb;Rc;Rd;Re;Weight" << std::endl;
            OutputSQLToStream("SELECT Maker, Model, ISO, Ra, Rb, Rc, Rd, Re, Weight FROM EMORTable;", output);
            output << "ENDTABLE" << std::endl;
            output.close();
            return true;
        }
        else
        {
            std::cerr << "Could not open file \"" << filename << "\"." << std::endl;
            return false;
        };
    };
    // import data from external file 
    bool ImportFromFile(const std::string& filename)
    {
        if (m_db == NULL)
        {
            return false;
        };
        std::ifstream input(filename);
        if (input.is_open())
        {
            while (!input.eof())
            {
                std::string line;
                std::getline(input, line);
                if (line.empty())
                {
                    continue;
                };
                if (line.compare(0, 6, "TABLE=") == 0)
                {
                    std::vector<std::string> substring = hugin_utils::SplitString(line, "=");
                    if (substring.size() == 2)
                    {
                        if (substring[1] == "CameraCropTable")
                        {
                            std::cout << "\tImporting CameraCropTable..." << std::endl;
                            if (!ImportCropFactor(input))
                            {
                                input.close();
                                std::cerr << "Error in input file." << std::endl;
                                return false;
                            };
                        }
                        else
                        {
                            if (substring[1] == "LensProjectionTable")
                            {
                                std::cout << "\tImporting LensProjectionTable..." << std::endl;
                                if (!ImportProjection(input))
                                {
                                    input.close();
                                    std::cerr << "Error in input file." << std::endl;
                                    return false;
                                };
                            }
                            else
                            {
                                if (substring[1] == "LensHFOVTable")
                                {
                                    std::cout << "\tImporting LensHFOVTable..." << std::endl;
                                    if (!ImportHFOV(input))
                                    {
                                        input.close();
                                        std::cerr << "Error in input file." << std::endl;
                                        return false;
                                    };
                                }
                                else
                                {
                                    if (substring[1] == "LensCropTable")
                                    {
                                        std::cout << "\tImporting LensCropTable..." << std::endl;
                                        if (!ImportLensCrop(input))
                                        {
                                            input.close();
                                            std::cerr << "Error in input file." << std::endl;
                                            return false;
                                        };
                                    }
                                    else
                                    {
                                        if (substring[1] == "DistortionTable")
                                        {
                                            std::cout << "\tImporting DistortionTable..." << std::endl;
                                            if (!ImportDistortion(input))
                                            {
                                                input.close();
                                                std::cerr << "Error in input file." << std::endl;
                                                return false;
                                            };
                                        }
                                        else
                                        {
                                            if (substring[1] == "VignettingTable")
                                            {
                                                std::cout << "\tImporting VignettingTable..." << std::endl;
                                                if (!ImportVignetting(input))
                                                {
                                                    input.close();
                                                    std::cerr << "Error in input file." << std::endl;
                                                    return false;
                                                };
                                            }
                                            else
                                            {
                                                if (substring[1] == "TCATable")
                                                {
                                                    std::cout << "\tImporting TCATable..." << std::endl;
                                                    if (!ImportTCA(input))
                                                    {
                                                        input.close();
                                                        std::cerr << "Error in input file." << std::endl;
                                                        return false;
                                                    };
                                                }
                                                else
                                                {
                                                    if (substring[1] == "EMORTable")
                                                    {
                                                        std::cout << "\tImporting EMORTable..." << std::endl;
                                                        if (!ImportEMOR(input))
                                                        {
                                                            input.close();
                                                            std::cerr << "Error in input file." << std::endl;
                                                            return false;
                                                        };
                                                    }
                                                    else
                                                    {
                                                        input.close();
                                                        std::cerr << "Error in input file (Unknown table \"" << substring[1] << "\")." << std::endl;
                                                        return false;
                                                    };
                                                };
                                            };
                                        };
                                    };
                                };
                            };
                        };
                    }
                    else
                    {
                        std::cerr << "Error in input file (Could not parse table name)." << std::endl;
                        input.close();
                        return false;
                    };
                }
                else
                {
                    std::cerr << "Error in input file (Could not find TABLE section)." << std::endl;
                    input.close();
                    return false;
                };
            };
            input.close();
            CleanUp();
            return true;
        }
        else
        {
            std::cerr << "Could not open file \"" << filename << "\"." << std::endl;
            return false;
        };
    };
private:
    // helper functions for BEGIN/COMMIT TRANSACTION
    void BeginTransaction()
    {
        if (!m_runningTransaction)
        {
            m_runningTransaction = (sqlite3_exec(m_db, "BEGIN TRANSACTION;", NULL, NULL, NULL) == SQLITE_OK);
        };
    };
    void EndTransaction()
    {
        if (m_runningTransaction)
        {
            sqlite3_exec(m_db, "COMMIT TRANSACTION;", NULL, NULL, NULL);
            m_runningTransaction = false;
        };
    };

    // removes the given lens from the selected table
    bool RemoveLensFromTable(const std::string& table, const std::string& lens)
    {
        sqlite3_stmt *statement;
        const char *tail;
        int returnValue = 0;
        std::string sqlStatement("DELETE FROM ");
        sqlStatement.append(table);
        sqlStatement.append(" WHERE Lens=?;");
        if (sqlite3_prepare_v2(m_db, sqlStatement.c_str(), -1, &statement, &tail) == SQLITE_OK)
        {
            sqlite3_bind_text(statement, 1, lens.c_str(), -1, NULL);
            returnValue = sqlite3_step(statement);
        };
        sqlite3_finalize(statement);
        return returnValue == SQLITE_DONE;
    };
    // remove given camera from selected table
    bool RemoveCameraFromTable(const std::string& table, const std::string& maker, const std::string& model)
    {
        sqlite3_stmt *statement;
        const char *tail;
        int returnValue = 0;
        std::string sqlStatement("DELETE FROM ");
        sqlStatement.append(table);
        sqlStatement.append(" WHERE Maker=?1 AND Model=?2;");
        if (sqlite3_prepare_v2(m_db, sqlStatement.c_str(), -1, &statement, &tail) == SQLITE_OK)
        {
            sqlite3_bind_text(statement, 1, maker.c_str(), -1, NULL);
            sqlite3_bind_text(statement, 2, model.c_str(), -1, NULL);
            returnValue = sqlite3_step(statement);
        };
        sqlite3_finalize(statement);
        return returnValue == SQLITE_DONE;
    };
    // write result of sql statement to stream, the columns are separated by ;
    void OutputSQLToStream(const std::string& sqlstatement, std::ostream& stream)
    {
        sqlite3_stmt *statement;
        const char *tail;
        if (sqlite3_prepare_v2(m_db, sqlstatement.c_str(), -1, &statement, &tail) == SQLITE_OK)
        {
            while (sqlite3_step(statement) == SQLITE_ROW)
            {
                const int count = sqlite3_column_count(statement);
                if (count > 0)
                {
                    for (int i = 0; i < count; ++i)
                    {
                        stream << sqlite3_column_text(statement, i);
                        if (i + 1 < count)
                        {
                            stream << ";";
                        };
                    };
                };
                stream << std::endl;
            };
        };
        sqlite3_finalize(statement);
    }
    // import cropfactors from stream
    bool ImportCropFactor(std::istream& input)
    {
        std::string s;
        std::getline(input, s);
        // first line should contains the column list
        if (s.compare(0, 8, "COLUMNS=") != 0)
        {
            return false;
        };
        std::vector<std::string> columns = hugin_utils::SplitString(s.substr(8), ";");
        int indexMaker = -1;
        int indexModel = -1;
        int indexCropfactor = -1;
        for (size_t i = 0; i < columns.size(); ++i)
        {
            if (columns[i] == "Maker")
            {
                indexMaker = i;
            };
            if (columns[i] == "Model")
            {
                indexModel = i;
            };
            if (columns[i] == "Cropfactor")
            {
                indexCropfactor = i;
            };
        };
        if (indexMaker == -1)
        {
            std::cerr << "ERROR: Missing column \"Maker\"." << std::endl;
            return false;
        };
        if (indexModel == -1)
        {
            std::cerr << "ERROR: Missing column \"Model\"." << std::endl;
            return false;
        };
        if (indexCropfactor == -1)
        {
            std::cerr << "ERROR: Missing column \"Cropfactor\"." << std::endl;
            return false;
        };
        if (input.eof())
        {
            return false;
        };
        std::getline(input, s);
        while (!input.eof())
        {
            if (s == "ENDTABLE")
            {
                return true;
            }
            std::vector<std::string> items = hugin_utils::SplitString(s, ";");
            if (items.size() == columns.size())
            {
                //ignore lines with not matching count of items
                double cropfactor;
                if (hugin_utils::stringToDouble(items[indexCropfactor], cropfactor))
                {
                    SaveCropFactor(items[indexMaker], items[indexModel], cropfactor);
                };
            };
            std::getline(input, s);
        };
        return false;
    };
    // import projection settings from stream
    bool ImportProjection(std::istream& input)
    {
        std::string s;
        std::getline(input, s);
        // first line should contains the column list
        if (s.compare(0, 8, "COLUMNS=") != 0)
        {
            return false;
        };
        std::vector<std::string> columns = hugin_utils::SplitString(s.substr(8), ";");
        int indexLens = -1;
        int indexProjection = -1;
        for (size_t i = 0; i < columns.size(); ++i)
        {
            if (columns[i] == "Lens")
            {
                indexLens = i;
            };
            if (columns[i] == "Projection")
            {
                indexProjection = i;
            };
        };
        if (indexLens == -1)
        {
            std::cerr << "ERROR: Missing column \"Lens\"." << std::endl;
            return false;
        };
        if (indexProjection == -1)
        {
            std::cerr << "ERROR: Missing column \"Projection\"." << std::endl;
            return false;
        };
        if (input.eof())
        {
            return false;
        };
        std::getline(input, s);
        while (!input.eof())
        {
            if (s == "ENDTABLE")
            {
                return true;
            }
            std::vector<std::string> items = hugin_utils::SplitString(s, ";");
            if (items.size() == columns.size())
            {
                //ignore lines with not matching count of items
                int projection;
                if (hugin_utils::stringToInt(items[indexProjection], projection))
                {
                    SaveLensProjection(items[indexLens], projection);
                };
            };
            std::getline(input, s);
        };
        return false;
    };
    // import hfov values from stream
    bool ImportHFOV(std::istream& input)
    {
        std::string s;
        std::getline(input, s);
        // first line should contains the column list
        if (s.compare(0, 8, "COLUMNS=") != 0)
        {
            return false;
        };
        std::vector<std::string> columns = hugin_utils::SplitString(s.substr(8), ";");
        int indexLens = -1;
        int indexFocallength = -1;
        int indexHFOV = -1;
        int indexWeight = -1;
        for (size_t i = 0; i < columns.size(); ++i)
        {
            if (columns[i] == "Lens")
            {
                indexLens = i;
            };
            if (columns[i] == "Focallength")
            {
                indexFocallength = i;
            };
            if (columns[i] == "HFOV")
            {
                indexHFOV = i;
            };
            if (columns[i] == "Weight")
            {
                indexWeight = i;
            };
        };
        if (indexLens == -1)
        {
            std::cerr << "ERROR: Missing column \"Lens\"." << std::endl;
            return false;
        };
        if (indexFocallength == -1)
        {
            std::cerr << "ERROR: Missing column \"Focallength\"." << std::endl;
            return false;
        };
        if (indexHFOV == -1)
        {
            std::cerr << "ERROR: Missing column \"HFOV\"." << std::endl;
            return false;
        };
        if (indexWeight == -1)
        {
            std::cerr << "ERROR: Missing column \"Weight\"." << std::endl;
            return false;
        };
        if (input.eof())
        {
            return false;
        };
        std::getline(input, s);
        while (!input.eof())
        {
            if (s == "ENDTABLE")
            {
                return true;
            }
            std::vector<std::string> items = hugin_utils::SplitString(s, ";");
            if (items.size() == columns.size())
            {
                //ignore lines with not matching count of items
                double focallength;
                double hfov;
                int weight;
                bool valid = hugin_utils::stringToDouble(items[indexFocallength], focallength);
                valid &= hugin_utils::stringToDouble(items[indexHFOV], hfov);
                valid &= hugin_utils::stringToInt(items[indexWeight], weight);
                if (valid)
                {
                    SaveHFOV(items[indexLens], focallength, hfov, weight);
                };
            };
            std::getline(input, s);
        };
        return false;
    };
    // import crop values from stream
    bool ImportLensCrop(std::istream& input)
    {
        std::string s;
        std::getline(input, s);
        // first line should contains the column list
        if (s.compare(0, 8, "COLUMNS=") != 0)
        {
            return false;
        };
        std::vector<std::string> columns = hugin_utils::SplitString(s.substr(8), ";");
        int indexLens = -1;
        int indexFocallength = -1;
        int indexWidth = -1;
        int indexHeight = -1;
        int indexCropLeft = -1;
        int indexCropRight = -1;
        int indexCropTop = -1;
        int indexCropBottom = -1;
        for (size_t i = 0; i < columns.size(); ++i)
        {
            if (columns[i] == "Lens")
            {
                indexLens = i;
            };
            if (columns[i] == "Focallength")
            {
                indexFocallength = i;
            };
            if (columns[i] == "Width")
            {
                indexWidth = i;
            };
            if (columns[i] == "Height")
            {
                indexHeight = i;
            };
            if (columns[i] == "CropLeft")
            {
                indexCropLeft = i;
            };
            if (columns[i] == "CropRight")
            {
                indexCropRight = i;
            };
            if (columns[i] == "CropTop")
            {
                indexCropTop = i;
            };
            if (columns[i] == "CropBottom")
            {
                indexCropBottom = i;
            };
        };
        if (indexLens == -1)
        {
            std::cerr << "ERROR: Missing column \"Lens\"." << std::endl;
            return false;
        };
        if (indexFocallength == -1)
        {
            std::cerr << "ERROR: Missing column \"Focallength\"." << std::endl;
            return false;
        };
        if (indexWidth == -1)
        {
            std::cerr << "ERROR: Missing column \"Width\"." << std::endl;
            return false;
        };
        if (indexHeight == -1)
        {
            std::cerr << "ERROR: Missing column \"Height\"." << std::endl;
            return false;
        };
        if (indexCropLeft == -1)
        {
            std::cerr << "ERROR: Missing column \"CropLeft\"." << std::endl;
            return false;
        };
        if (indexCropRight == -1)
        {
            std::cerr << "ERROR: Missing column \"CropRight\"." << std::endl;
            return false;
        };
        if (indexCropTop == -1)
        {
            std::cerr << "ERROR: Missing column \"CropTop\"." << std::endl;
            return false;
        };
        if (indexCropBottom == -1)
        {
            std::cerr << "ERROR: Missing column \"CropBottom\"." << std::endl;
            return false;
        };
        if (input.eof())
        {
            return false;
        };
        std::getline(input, s);
        while (!input.eof())
        {
            if (s == "ENDTABLE")
            {
                return true;
            }
            std::vector<std::string> items = hugin_utils::SplitString(s, ";");
            if (items.size() == columns.size())
            {
                //ignore lines with not matching count of items
                double focallength;
                int width, height, cropLeft, cropRight, cropTop, cropBottom;
                bool valid = hugin_utils::stringToDouble(items[indexFocallength], focallength);
                valid &= hugin_utils::stringToInt(items[indexWidth], width);
                valid &= hugin_utils::stringToInt(items[indexHeight], height);
                valid &= hugin_utils::stringToInt(items[indexCropLeft], cropLeft);
                valid &= hugin_utils::stringToInt(items[indexCropRight], cropRight);
                valid &= hugin_utils::stringToInt(items[indexCropTop], cropTop);
                valid &= hugin_utils::stringToInt(items[indexCropBottom], cropBottom);
                if (valid)
                {
                    SaveLensCrop(items[indexLens], focallength, width, height, cropLeft, cropRight, cropTop, cropBottom);
                };
            };
            std::getline(input, s);
        };
        return false;
    };
    // import distortion values from stream
    bool ImportDistortion(std::istream& input)
    {
        std::string s;
        std::getline(input, s);
        // first line should contains the column list
        if (s.compare(0, 8, "COLUMNS=") != 0)
        {
            return false;
        };
        std::vector<std::string> columns = hugin_utils::SplitString(s.substr(8), ";");
        int indexLens = -1;
        int indexFocallength = -1;
        int indexA = -1;
        int indexB = -1;
        int indexC = -1;
        int indexWeight = -1;
        for (size_t i = 0; i < columns.size(); ++i)
        {
            if (columns[i] == "Lens")
            {
                indexLens = i;
            };
            if (columns[i] == "Focallength")
            {
                indexFocallength = i;
            };
            if (columns[i] == "a")
            {
                indexA = i;
            };
            if (columns[i] == "b")
            {
                indexB = i;
            };
            if (columns[i] == "c")
            {
                indexC = i;
            };
            if (columns[i] == "Weight")
            {
                indexWeight = i;
            };
        };
        if (indexLens == -1)
        {
            std::cerr << "ERROR: Missing column \"Lens\"." << std::endl;
            return false;
        };
        if (indexFocallength == -1)
        {
            std::cerr << "ERROR: Missing column \"Focallength\"." << std::endl;
            return false;
        };
        if (indexA == -1)
        {
            std::cerr << "ERROR: Missing column \"a\"." << std::endl;
            return false;
        };
        if (indexB == -1)
        {
            std::cerr << "ERROR: Missing column \"b\"." << std::endl;
            return false;
        };
        if (indexC == -1)
        {
            std::cerr << "ERROR: Missing column \"c\"." << std::endl;
            return false;
        };
        if (indexWeight == -1)
        {
            std::cerr << "ERROR: Missing column \"Weight\"." << std::endl;
            return false;
        };
        if (input.eof())
        {
            return false;
        };
        std::getline(input, s);
        while (!input.eof())
        {
            if (s == "ENDTABLE")
            {
                return true;
            }
            std::vector<std::string> items = hugin_utils::SplitString(s, ";");
            if (items.size() == columns.size())
            {
                //ignore lines with not matching count of items
                double focallength, a, b, c;
                int weight;
                bool valid = hugin_utils::stringToDouble(items[indexFocallength], focallength);
                valid &= hugin_utils::stringToDouble(items[indexA], a);
                valid &= hugin_utils::stringToDouble(items[indexB], b);
                valid &= hugin_utils::stringToDouble(items[indexC], c);
                valid &= hugin_utils::stringToInt(items[indexWeight], weight);
                if (valid)
                {
                    SaveDistortion(items[indexLens], focallength, a, b, c, weight);
                };
            };
            std::getline(input, s);
        };
        return false;
    };
    // import vignetting values from stream
    bool ImportVignetting(std::istream& input)
    {
        std::string s;
        std::getline(input, s);
        // first line should contains the column list
        if (s.compare(0, 8, "COLUMNS=") != 0)
        {
            return false;
        };
        std::vector<std::string> columns = hugin_utils::SplitString(s.substr(8), ";");
        int indexLens = -1;
        int indexFocallength = -1;
        int indexAperture = -1;
        int indexDistance = -1;
        int indexVb = -1;
        int indexVc = -1;
        int indexVd = -1;
        int indexWeight = -1;
        for (size_t i = 0; i < columns.size(); ++i)
        {
            if (columns[i] == "Lens")
            {
                indexLens = i;
            };
            if (columns[i] == "Focallength")
            {
                indexFocallength = i;
            };
            if (columns[i] == "Aperture")
            {
                indexAperture = i;
            };
            if (columns[i] == "Distance")
            {
                indexDistance = i;
            };
            if (columns[i] == "Vb")
            {
                indexVb = i;
            };
            if (columns[i] == "Vc")
            {
                indexVc = i;
            };
            if (columns[i] == "Vd")
            {
                indexVd = i;
            };
            if (columns[i] == "Weight")
            {
                indexWeight = i;
            };
        };
        if (indexLens == -1)
        {
            std::cerr << "ERROR: Missing column \"Lens\"." << std::endl;
            return false;
        };
        if (indexFocallength == -1)
        {
            std::cerr << "ERROR: Missing column \"Focallength\"." << std::endl;
            return false;
        };
        if (indexAperture == -1)
        {
            std::cerr << "ERROR: Missing column \"Aperture\"." << std::endl;
            return false;
        };
        if (indexDistance == -1)
        {
            std::cerr << "ERROR: Missing column \"Distance\"." << std::endl;
            return false;
        };
        if (indexVb == -1)
        {
            std::cerr << "ERROR: Missing column \"Vb\"." << std::endl;
            return false;
        };
        if (indexVc == -1)
        {
            std::cerr << "ERROR: Missing column \"Vc\"." << std::endl;
            return false;
        };
        if (indexVd == -1)
        {
            std::cerr << "ERROR: Missing column \"Vd\"." << std::endl;
            return false;
        };
        if (indexWeight == -1)
        {
            std::cerr << "ERROR: Missing column \"Weight\"." << std::endl;
            return false;
        };
        if (input.eof())
        {
            return false;
        };
        std::getline(input, s);
        while (!input.eof())
        {
            if (s == "ENDTABLE")
            {
                return true;
            }
            std::vector<std::string> items = hugin_utils::SplitString(s, ";");
            if (items.size() == columns.size())
            {
                //ignore lines with not matching count of items
                double focallength, aperture, distance, Vb, Vc, Vd;
                int weight;
                bool valid = hugin_utils::stringToDouble(items[indexFocallength], focallength);
                valid &= hugin_utils::stringToDouble(items[indexAperture], aperture);
                valid &= hugin_utils::stringToDouble(items[indexDistance], distance);
                valid &= hugin_utils::stringToDouble(items[indexVb], Vb);
                valid &= hugin_utils::stringToDouble(items[indexVc], Vc);
                valid &= hugin_utils::stringToDouble(items[indexVd], Vd);
                valid &= hugin_utils::stringToInt(items[indexWeight], weight);
                if (valid)
                {
                    SaveVignetting(items[indexLens], focallength, aperture, distance, Vb, Vc, Vd, weight);
                };
            };
            std::getline(input, s);
        };
        return false;
    };
    // import tca values from stream
    bool ImportTCA(std::istream& input)
    {
        std::string s;
        std::getline(input, s);
        // first line should contains the column list
        if (s.compare(0, 8, "COLUMNS=") != 0)
        {
            return false;
        };
        std::vector<std::string> columns = hugin_utils::SplitString(s.substr(8), ";");
        int  indexLens = -1;
        int indexFocallength = -1;
        int indexRa = -1;
        int indexRb = -1;
        int indexRc = -1;
        int indexRd = -1;
        int indexBa = -1;
        int indexBb = -1;
        int indexBc = -1;
        int indexBd = -1;
        int indexWeight = -1;
        for (size_t i = 0; i < columns.size(); ++i)
        {
            if (columns[i] == "Lens")
            {
                indexLens = i;
            };
            if (columns[i] == "Focallength")
            {
                indexFocallength = i;
            };
            if (columns[i] == "ra")
            {
                indexRa = i;
            };
            if (columns[i] == "rb")
            {
                indexRb = i;
            };
            if (columns[i] == "rc")
            {
                indexRc = i;
            };
            if (columns[i] == "rd")
            {
                indexRd = i;
            };
            if (columns[i] == "ba")
            {
                indexBa = i;
            };
            if (columns[i] == "bb")
            {
                indexBb = i;
            };
            if (columns[i] == "bc")
            {
                indexBc = i;
            };
            if (columns[i] == "bd")
            {
                indexBd = i;
            };
            if (columns[i] == "Weight")
            {
                indexWeight = i;
            };
        };
        if (indexLens == -1)
        {
            std::cerr << "ERROR: Missing column \"Lens\"." << std::endl;
            return false;
        };
        if (indexFocallength == -1)
        {
            std::cerr << "ERROR: Missing column \"Focallength\"." << std::endl;
            return false;
        };
        if (indexRa == -1)
        {
            std::cerr << "ERROR: Missing column \"ra\"." << std::endl;
            return false;
        };
        if (indexRb == -1)
        {
            std::cerr << "ERROR: Missing column \"rb\"." << std::endl;
            return false;
        };
        if (indexRc == -1)
        {
            std::cerr << "ERROR: Missing column \"rc\"." << std::endl;
            return false;
        };
        if (indexRd == -1)
        {
            std::cerr << "ERROR: Missing column \"rd\"." << std::endl;
            return false;
        };
        if (indexBa == -1)
        {
            std::cerr << "ERROR: Missing column \"ba\"." << std::endl;
            return false;
        };
        if (indexBb == -1)
        {
            std::cerr << "ERROR: Missing column \"bb\"." << std::endl;
            return false;
        };
        if (indexBc == -1)
        {
            std::cerr << "ERROR: Missing column \"bc\"." << std::endl;
            return false;
        };
        if (indexBd == -1)
        {
            std::cerr << "ERROR: Missing column \"bd\"." << std::endl;
            return false;
        };
        if (indexWeight == -1)
        {
            std::cerr << "ERROR: Missing column \"Weight\"." << std::endl;
            return false;
        };
        if (input.eof())
        {
            return false;
        };
        std::getline(input, s);
        while (!input.eof())
        {
            if (s == "ENDTABLE")
            {
                return true;
            }
            std::vector<std::string> items = hugin_utils::SplitString(s, ";");
            if (items.size() == columns.size())
            {
                //ignore lines with not matching count of items
                double focallength, ra, rb, rc, rd, ba, bb, bc, bd;
                int weight;
                bool valid = hugin_utils::stringToDouble(items[indexFocallength], focallength);
                valid &= hugin_utils::stringToDouble(items[indexRa], ra);
                valid &= hugin_utils::stringToDouble(items[indexRb], rb);
                valid &= hugin_utils::stringToDouble(items[indexRc], rc);
                valid &= hugin_utils::stringToDouble(items[indexRd], rd);
                valid &= hugin_utils::stringToDouble(items[indexBa], ba);
                valid &= hugin_utils::stringToDouble(items[indexBb], bb);
                valid &= hugin_utils::stringToDouble(items[indexBc], bc);
                valid &= hugin_utils::stringToDouble(items[indexBd], bd);
                valid &= hugin_utils::stringToInt(items[indexWeight], weight);
                if (valid)
                {
                    SaveTCAData(items[indexLens], focallength, ra, rb, rc, rd, ba, bb, bc, bd, weight);
                };
            };
            std::getline(input, s);
        };
        return false;
    };
    // import emor values from stream
    bool ImportEMOR(std::istream& input)
    {
        std::string s;
        std::getline(input, s);
        // first line should contains the column list
        if (s.compare(0, 8, "COLUMNS=") != 0)
        {
            return false;
        };
        std::vector<std::string> columns = hugin_utils::SplitString(s.substr(8), ";");
        int indexMaker = -1;
        int indexModel = -1;
        int indexISO = -1;
        int indexRa = -1;
        int indexRb = -1;
        int indexRc = -1;
        int indexRd = -1;
        int indexRe = -1;
        int indexWeight = -1;
        for (size_t i = 0; i < columns.size(); ++i)
        {
            if (columns[i] == "Maker")
            {
                indexMaker = i;
            };
            if (columns[i] == "Model")
            {
                indexModel = i;
            };
            if (columns[i] == "ISO")
            {
                indexISO = i;
            };
            if (columns[i] == "Ra")
            {
                indexRa = i;
            };
            if (columns[i] == "Rb")
            {
                indexRb = i;
            };
            if (columns[i] == "Rc")
            {
                indexRc = i;
            };
            if (columns[i] == "Rd")
            {
                indexRd = i;
            };
            if (columns[i] == "Re")
            {
                indexRe = i;
            };
            if (columns[i] == "Weight")
            {
                indexWeight = i;
            };
        };
        if (indexMaker == -1)
        {
            std::cerr << "ERROR: Missing column \"Maker\"." << std::endl;
            return false;
        };
        if (indexModel == -1)
        {
            std::cerr << "ERROR: Missing column \"Model\"." << std::endl;
            return false;
        };
        if (indexISO == -1)
        {
            std::cerr << "ERROR: Missing column \"ISO\"." << std::endl;
            return false;
        };
        if (indexRa == -1)
        {
            std::cerr << "ERROR: Missing column \"Ra\"." << std::endl;
            return false;
        };
        if (indexRb == -1)
        {
            std::cerr << "ERROR: Missing column \"Rb\"." << std::endl;
            return false;
        };
        if (indexRc == -1)
        {
            std::cerr << "ERROR: Missing column \"Rc\"." << std::endl;
            return false;
        };
        if (indexRd == -1)
        {
            std::cerr << "ERROR: Missing column \"Rd\"." << std::endl;
            return false;
        };
        if (indexRe == -1)
        {
            std::cerr << "ERROR: Missing column \"Re\"." << std::endl;
            return false;
        };
        if (indexWeight == -1)
        {
            std::cerr << "ERROR: Missing column \"Weight\"." << std::endl;
            return false;
        };
        if (input.eof())
        {
            return false;
        };
        std::getline(input, s);
        while (!input.eof())
        {
            if (s == "ENDTABLE")
            {
                return true;
            }
            std::vector<std::string> items = hugin_utils::SplitString(s, ";");
            if (items.size() == columns.size())
            {
                //ignore lines with not matching count of items
                double Ra, Rb, Rc, Rd, Re;
                int iso, weight;
                bool valid = hugin_utils::stringToInt(items[indexISO], iso);
                valid &= hugin_utils::stringToDouble(items[indexRa], Ra);
                valid &= hugin_utils::stringToDouble(items[indexRb], Rb);
                valid &= hugin_utils::stringToDouble(items[indexRc], Rc);
                valid &= hugin_utils::stringToDouble(items[indexRd], Rd);
                valid &= hugin_utils::stringToDouble(items[indexRe], Re);
                valid &= hugin_utils::stringToInt(items[indexWeight], weight);
                if (valid)
                {
                    SaveEMoR(items[indexMaker], items[indexModel], iso, Ra, Rb, Rc, Rd, Re, weight);
                };
            };
            std::getline(input, s);
        };
        return false;
    };

    std::string m_filename;
    sqlite3 *m_db;
    bool m_runningTransaction;
};

double InterpolateValue(double x, double x0, double y0, double x1, double y1)
{
    if (fabs(x1 - x0) < 1e-4)
    {
        // prevent division through 0, should normally not happens
        return y0;
    };
    return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
};

double InterpolateValueTriangle(double x, double y,
    double x1, double y1, double z1,
    double x2, double y2, double z2,
    double x3, double y3, double z3)
{
    const double a = (x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1);
    if (fabs(a) < 1e-6)
    {
        // this should never happens
        return z1;
    };
    return z1 + ((x - x1) * ((z2 - z1) * (y3 - y1) - (y2 - y1) * (z3 - z1)) + (y - y1) * ((x2 - x1) * (z3 - z1) - (z2 - z1) * (x3 - x1))) / a;
};

LensDB::LensDB()
{
    std::string filename = hugin_utils::GetUserAppDataDir();
    if (filename.length() == 0)
    {
        m_db = NULL;
    }
    else
    {
#if _WIN32
        filename.append("\\");
#else
        filename.append("/");
#endif
        filename.append("camlens.db");
        m_db = new LensDB::Database(filename);
        if (!m_db)
        {
            m_db = NULL;
        };
    };
};

LensDB::~LensDB()
{
    if (m_db)
        delete m_db;
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
    if (m_instance != NULL)
    {
        delete m_instance;
    };
    m_instance = NULL;
};

std::string LensDB::GetDBFilename() const
{
    if (m_db)
    {
        return m_db->GetDBFilename();
    }
    else
    {
        return std::string();
    };
};

bool LensDB::GetCropFactor(const std::string& maker, const std::string& model, double& cropFactor) const
{
    if (m_db == NULL)
    {
        return false;
    };
    return m_db->GetCropFactor(maker, model, cropFactor);
};

bool LensDB::GetProjection(const std::string& lens, BaseSrcPanoImage::Projection& projection) const
{
    if (m_db == NULL)
    {
        return false;
    };
    int proj;
    if (m_db->GetLensProjection(lens, proj))
    {
        projection = static_cast<BaseSrcPanoImage::Projection>(proj);
        return true;
    }
    else
    {
        return false;
    };
};

inline int fsign(double a)
{
    return (a > 0) ? 1 : ((a < 0) ? -1 : 0);
}

/** check if value is inside limit1...limit2 or it is nearer to limit1 than value*tol */
bool IsFocallengthNearRange(const double focal, const double limit1, const double limit2, const double tol)
{
    if (fsign(focal - limit1) != fsign(focal - limit2))
    {
        return true;
    };
    return fabs(focal - limit1) < tol * focal;
}

bool LensDB::GetCrop(const std::string& lens, const double focal, const vigra::Size2D& imageSize, vigra::Rect2D& cropRect) const
{
    if(m_db == NULL)
    {
        return false;
    };
    std::vector<Database::CropData> cropData;
    if (!m_db->GetLensCrop(lens, focal, imageSize.width() , imageSize.height(), cropData))
    {
        return false;
    };
    int left, right, top, bottom;
    if (cropData.size() == 1)
    {
        // only one entry found
        // check focal length
        if (fabs(cropData[0].focallength - focal) < 0.075f * focal)
        {
            // focal length matches
            left = cropData[0].left;
            right = cropData[0].right;
            top = cropData[0].top;
            bottom = cropData[0].bottom;
        }
        else
        {
            // if focal length does not match we ignore crop 
            return false;
        }
    }
    else
    {
        if (!IsFocallengthNearRange(focal, cropData[0].focallength, cropData[1].focallength, 0.15f))
        {
            return false;
        };
        left = hugin_utils::roundi(InterpolateValue(focal, cropData[0].focallength, cropData[0].left, cropData[1].focallength, cropData[1].left));
        right = hugin_utils::roundi(InterpolateValue(focal, cropData[0].focallength, cropData[0].right, cropData[1].focallength, cropData[1].right));
        top = hugin_utils::roundi(InterpolateValue(focal, cropData[0].focallength, cropData[0].top, cropData[1].focallength, cropData[1].top));
        bottom = hugin_utils::roundi(InterpolateValue(focal, cropData[0].focallength, cropData[0].bottom, cropData[1].focallength, cropData[1].bottom));
    };
    cropRect.setUpperLeft(vigra::Point2D(left, top));
    cropRect.setLowerRight(vigra::Point2D(right, bottom));
    return true;
};

bool LensDB::GetFov(const std::string& lens, const double focal, double& fov) const
{
    if (m_db == NULL)
    {
        return false;
    };
    std::vector<Database::HFOVData> hfovdata;
    if (!m_db->GetHFOV(lens, focal, hfovdata))
    {
        return false;
    };
    fov = 0;
    if (hfovdata.size() == 1)
    {
        // only one entry found
        // check focal length
        if (fabs(hfovdata[0].focallength - focal) <= 0.075f * focal)
        {
            // focal length matches
            fov = hfovdata[0].HFOV;
        }
        else
        {
            // if focal length does not match we ignore HFOV 
            return false;
        }
    }
    else
    {
        if (!IsFocallengthNearRange(focal, hfovdata[0].focallength, hfovdata[1].focallength, 0.15f))
        {
            // difference to nearest point too big, ignoring
            return false;
        }
        fov = InterpolateValue(focal, hfovdata[0].focallength, hfovdata[0].HFOV, hfovdata[1].focallength, hfovdata[1].HFOV);
        if (fov < 0.1)
        {
            fov = 0;
        };
    };
    return (fov > 0);
};

bool LensDB::GetDistortion(const std::string& lens, const double focal, std::vector<double>& distortion) const
{
    distortion.clear();
    if (m_db == NULL)
    {
        return false;
    };
    std::vector<Database::Distortiondata> distdata;
    if (!m_db->GetDistortionData(lens, focal, distdata))
    {
        return false;
    };
    if (distdata.size() == 1)
    {
        // only one entry found
        // check focal length
        if (fabs(distdata[0].focallength - focal) <= 0.075f * focal)
        {
            distortion.push_back(distdata[0].a);
            distortion.push_back(distdata[0].b);
            distortion.push_back(distdata[0].c);
            return true;
        }
        else
        {
            std::cout << "Invalid focallength" << std::endl;
            return false;
        };
    }
    else
    {
        if (!IsFocallengthNearRange(focal, distdata[0].focallength, distdata[1].focallength, 0.15f))
        {
            // difference to nearest point too big, ignoring
            return false;
        }
        distortion.push_back(InterpolateValue(focal, distdata[0].focallength, distdata[0].a, distdata[1].focallength, distdata[1].a));
        distortion.push_back(InterpolateValue(focal, distdata[0].focallength, distdata[0].b, distdata[1].focallength, distdata[1].b));
        distortion.push_back(InterpolateValue(focal, distdata[0].focallength, distdata[0].c, distdata[1].focallength, distdata[1].c));
        return true;
    };
};

bool LensDB::GetVignetting(const std::string& lens, const double focal, const double aperture, const double distance, std::vector<double>& vignetting) const
{
    vignetting.clear();
    if (m_db == NULL)
    {
        return false;
    };
    std::vector<Database::Vignettingdata> vigdata;
    if (!m_db->GetVignettingData(lens, focal, aperture, vigdata))
    {
        return false;
    };
    const bool unknownAperture = (fabs(aperture) < 0.001f);
    if (vigdata.size() == 1)
    {
        if ((fabs(vigdata[0].focallength - focal) <= 0.075f * focal) && (unknownAperture || fabs(vigdata[0].aperture - aperture) < 0.3f))
        {
            vignetting.push_back(1.0);
            vignetting.push_back(vigdata[0].Vb);
            vignetting.push_back(vigdata[0].Vc);
            vignetting.push_back(vigdata[0].Vd);
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        if (vigdata.size() == 2)
        {
            if (fabs(vigdata[0].focallength - vigdata[1].focallength) < 0.001f)
            {
                // variant a: 2 datasets with same focal length
                if (unknownAperture)
                {
                    vignetting.push_back(1.0);
                    vignetting.push_back(vigdata[0].Vb);
                    vignetting.push_back(vigdata[0].Vc);
                    vignetting.push_back(vigdata[0].Vd);
                    return true;
                }
                else
                {
                    if (vigdata[0].aperture - 0.3 <= aperture && aperture <= vigdata[1].aperture + 0.3)
                    {
                        vignetting.push_back(1.0);
                        vignetting.push_back(InterpolateValue(aperture, vigdata[0].aperture, vigdata[0].Vb, vigdata[1].aperture, vigdata[1].Vb));
                        vignetting.push_back(InterpolateValue(aperture, vigdata[0].aperture, vigdata[0].Vc, vigdata[1].aperture, vigdata[1].Vc));
                        vignetting.push_back(InterpolateValue(aperture, vigdata[0].aperture, vigdata[0].Vd, vigdata[1].aperture, vigdata[1].Vd));
                        return true;
                    }
                    else
                    {
                        return false;
                    };
                };
            }
            else
            {
                // variant b: 2 datasets from different focal length
                if (!IsFocallengthNearRange(focal, vigdata[0].focallength, vigdata[1].focallength, 0.15f))
                {
                    return false;
                };
                const double interpolatedAperture = InterpolateValue(focal, vigdata[0].focallength, vigdata[0].aperture, vigdata[1].focallength, vigdata[1].aperture);
                if (fabs(interpolatedAperture - aperture) < 0.3f || unknownAperture)
                {
                    // return value only, if aperture matches the 2 found values
                    vignetting.push_back(1.0);
                    vignetting.push_back(InterpolateValue(focal, vigdata[0].focallength, vigdata[0].Vb, vigdata[1].focallength, vigdata[1].Vb));
                    vignetting.push_back(InterpolateValue(focal, vigdata[0].focallength, vigdata[0].Vc, vigdata[1].focallength, vigdata[1].Vc));
                    vignetting.push_back(InterpolateValue(focal, vigdata[0].focallength, vigdata[0].Vd, vigdata[1].focallength, vigdata[1].Vd));
                    return true;
                }
                else
                {
                    return false;
                };
            };
        }
        else
        {
            if (vigdata.size() == 3)
            {
                if (!IsFocallengthNearRange(focal, vigdata[0].focallength, vigdata[2].focallength, 0.15f))
                {
                    return false;
                };
                vignetting.push_back(1.0);
                vignetting.push_back(InterpolateValueTriangle(focal, aperture,
                    vigdata[0].focallength, vigdata[0].aperture, vigdata[0].Vb,
                    vigdata[1].focallength, vigdata[1].aperture, vigdata[1].Vb,
                    vigdata[2].focallength, vigdata[2].aperture, vigdata[2].Vb
                    ));
                vignetting.push_back(InterpolateValueTriangle(focal, aperture,
                    vigdata[0].focallength, vigdata[0].aperture, vigdata[0].Vc,
                    vigdata[1].focallength, vigdata[1].aperture, vigdata[1].Vc,
                    vigdata[2].focallength, vigdata[2].aperture, vigdata[2].Vc
                    ));
                vignetting.push_back(InterpolateValueTriangle(focal, aperture,
                    vigdata[0].focallength, vigdata[0].aperture, vigdata[0].Vd,
                    vigdata[1].focallength, vigdata[1].aperture, vigdata[1].Vd,
                    vigdata[2].focallength, vigdata[2].aperture, vigdata[2].Vd
                    ));
                return true;
            }
            else
            {
                // we have now 4 points for interpolation
                if (!IsFocallengthNearRange(focal, vigdata[0].focallength, vigdata[2].focallength, 0.15f))
                {
                    return false;
                };
                if (unknownAperture)
                {
                    // unknown aperture, take smallest aperture
                    vignetting.push_back(1.0);
                    vignetting.push_back(InterpolateValue(focal, vigdata[0].focallength, vigdata[0].Vb, vigdata[2].focallength, vigdata[2].Vb));
                    vignetting.push_back(InterpolateValue(focal, vigdata[0].focallength, vigdata[0].Vc, vigdata[2].focallength, vigdata[2].Vc));
                    vignetting.push_back(InterpolateValue(focal, vigdata[0].focallength, vigdata[0].Vd, vigdata[2].focallength, vigdata[2].Vd));
                    return true;
                }
                else
                {
                    // interpolate for each focal length to desired aperture
                    double Vb1, Vc1, Vd1, Vb2, Vc2, Vd2;
                    if (vigdata[0].aperture - 0.3 <= aperture && aperture <= vigdata[1].aperture + 0.3)
                    {
                        Vb1 = InterpolateValue(aperture, vigdata[0].aperture, vigdata[0].Vb, vigdata[1].aperture, vigdata[1].Vb);
                        Vc1 = InterpolateValue(aperture, vigdata[0].aperture, vigdata[0].Vc, vigdata[1].aperture, vigdata[1].Vc);
                        Vd1 = InterpolateValue(aperture, vigdata[0].aperture, vigdata[0].Vd, vigdata[1].aperture, vigdata[1].Vd);
                    }
                    else
                    {
                        return false;
                    };
                    if (vigdata[2].aperture - 0.3 <= aperture && aperture <= vigdata[3].aperture + 0.3)
                    {
                        Vb2 = InterpolateValue(aperture, vigdata[2].aperture, vigdata[2].Vb, vigdata[3].aperture, vigdata[3].Vb);
                        Vc2 = InterpolateValue(aperture, vigdata[2].aperture, vigdata[2].Vc, vigdata[3].aperture, vigdata[3].Vc);
                        Vd2 = InterpolateValue(aperture, vigdata[2].aperture, vigdata[2].Vd, vigdata[3].aperture, vigdata[3].Vd);
                    }
                    else
                    {
                        return false;
                    };
                    // now we have 2 values for the same aperture, but different focal length
                    // interpolate focal length
                    vignetting.push_back(1.0);
                    vignetting.push_back(InterpolateValue(focal, vigdata[0].focallength, Vb1, vigdata[2].focallength, Vb2));
                    vignetting.push_back(InterpolateValue(focal, vigdata[0].focallength, Vc1, vigdata[2].focallength, Vc2));
                    vignetting.push_back(InterpolateValue(focal, vigdata[0].focallength, Vd1, vigdata[2].focallength, Vd2));
                    return true;
                };
            };
        };
    };
};

bool LensDB::GetTCA(const std::string& lens, const double focal, std::vector<double>& tca_red, std::vector<double>& tca_blue) const
{
    tca_red.clear();
    tca_blue.clear();
    if (m_db == NULL)
    {
        return false;
    };
    std::vector<Database::TCAdata> tcadata;
    if (!m_db->GetTCAData(lens, focal, tcadata))
    {
        return false;
    };
    if (tcadata.size() == 1)
    {
        // only one entry found
        // check focal length
        if (fabs(tcadata[0].focallength - focal) <= 0.075f * focal)
        {
            tca_red.push_back(tcadata[0].ra);
            tca_red.push_back(tcadata[0].rb);
            tca_red.push_back(tcadata[0].rc);
            tca_red.push_back(tcadata[0].rd);
            tca_blue.push_back(tcadata[0].ba);
            tca_blue.push_back(tcadata[0].bb);
            tca_blue.push_back(tcadata[0].bc);
            tca_blue.push_back(tcadata[0].bd);
            return true;
        }
        else
        {
            return false;
        };
    }
    else
    {
        if (!IsFocallengthNearRange(focal, tcadata[0].focallength, tcadata[1].focallength, 0.15f))
        {
            // difference to nearest point too big, ignoring
            return false;
        };
        tca_red.push_back(InterpolateValue(focal, tcadata[0].focallength, tcadata[0].ra, tcadata[1].focallength, tcadata[1].ra));
        tca_red.push_back(InterpolateValue(focal, tcadata[0].focallength, tcadata[0].rb, tcadata[1].focallength, tcadata[1].rb));
        tca_red.push_back(InterpolateValue(focal, tcadata[0].focallength, tcadata[0].rc, tcadata[1].focallength, tcadata[1].rc));
        tca_red.push_back(InterpolateValue(focal, tcadata[0].focallength, tcadata[0].rd, tcadata[1].focallength, tcadata[1].rd));
        tca_blue.push_back(InterpolateValue(focal, tcadata[0].focallength, tcadata[0].ba, tcadata[1].focallength, tcadata[1].ba));
        tca_blue.push_back(InterpolateValue(focal, tcadata[0].focallength, tcadata[0].bb, tcadata[1].focallength, tcadata[1].bb));
        tca_blue.push_back(InterpolateValue(focal, tcadata[0].focallength, tcadata[0].bc, tcadata[1].focallength, tcadata[1].bc));
        tca_blue.push_back(InterpolateValue(focal, tcadata[0].focallength, tcadata[0].bd, tcadata[1].focallength, tcadata[1].bd));
        return true;
    };
};

bool LensDB::GetLensNames(const bool distortion, const bool vignetting, const bool tca, LensList& lensList) const
{
    lensList.clear();
    if (m_db == NULL)
    {
        return false;
    };
    return m_db->GetLensNames(distortion, vignetting, tca, lensList);
};

bool LensDB::SaveCameraCrop(const std::string& maker, const std::string& model, const double cropfactor)
{
    if (m_db == NULL)
    {
        return false;
    };
    return m_db->SaveCropFactor(maker, model, cropfactor);
};

bool LensDB::SaveEMoR(const std::string& maker, const std::string& model, const int iso, const std::vector<float>& emor, const int weight)
{
    if (m_db == NULL || emor.size() != 5)
    {
        return false;
    };
    return m_db->SaveEMoR(maker, model, iso, emor[0], emor[1], emor[2], emor[3], emor[4], weight);
};

bool LensDB::SaveLensProjection(const std::string& lens, const BaseSrcPanoImage::Projection projection)
{
    if (m_db == NULL)
    {
        return false;
    };
    return m_db->SaveLensProjection(lens, projection);
};

bool LensDB::SaveLensCrop(const std::string& lens, const double focal, const vigra::Size2D& imageSize, const vigra::Rect2D& cropRect)
{
    if (m_db == NULL)
    {
        return false;
    };
    if (cropRect.isEmpty())
    {
        return m_db->RemoveLensCrop(lens, focal, imageSize.width(), imageSize.height());
    }
    else
    {
        return m_db->SaveLensCrop(lens, focal, imageSize.width(), imageSize.height(), cropRect.left(), cropRect.right(), cropRect.top(), cropRect.bottom());
    };
};

bool LensDB::SaveLensFov(const std::string& lens, const double focal, const double fov, const int weight)
{
    if (m_db == NULL)
    {
        return false;
    };
    return m_db->SaveHFOV(lens, focal, fov, weight);
};

bool LensDB::SaveDistortion(const std::string& lens, const double focal, const std::vector<double>& distortion, const int weight)
{
    if (m_db == NULL || distortion.size()!=4)
    {
        return false;
    };
    return m_db->SaveDistortion(lens, focal, distortion[0], distortion[1], distortion[2], weight);
};

bool LensDB::SaveVignetting(const std::string& lens, const double focal, const double aperture, const double distance, const std::vector<double>& vignetting, const int weight)
{
    if (m_db == NULL || vignetting.size()!=4)
    {
        return false;
    };
    return m_db->SaveVignetting(lens, focal, aperture, distance, vignetting[1], vignetting[2], vignetting[3], weight);
};

bool LensDB::SaveTCA(const std::string& lens, const double focal, const std::vector<double>& tca_red, const std::vector<double>& tca_blue, const int weight)
{
    if (m_db == NULL || tca_red.size()!=4 || tca_blue.size()!=4)
    {
        return false;
    };
    return m_db->SaveTCAData(lens, focal, tca_red[0], tca_red[1], tca_red[2], tca_red[3], tca_blue[0], tca_blue[1], tca_blue[2], tca_blue[3], weight);
};

bool LensDB::CleanUpDatabase()
{
    if (m_db == NULL)
    {
        return false;
    };
    return m_db->CleanUp();
};

bool LensDB::RemoveLens(const std::string& lensname)
{
    if (m_db == NULL)
    {
        return false;
    };
    return m_db->RemoveLens(lensname);
};

bool LensDB::RemoveCamera(const std::string& maker, const std::string& model)
{
    if (m_db == NULL)
    {
        return false;
    };
    return m_db->RemoveCamera(maker, model);
};

bool LensDB::ExportToFile(const std::string& filename)
{
    if (m_db == NULL)
    {
        return false;
    };
    return m_db->ExportToFile(filename);
};

bool LensDB::ImportFromFile(const std::string& filename)
{
    if (m_db == NULL)
    {
        return false;
    };
    return m_db->ImportFromFile(filename);
};

bool SaveLensDataFromPano(const HuginBase::Panorama& pano)
{
    if (pano.getNrOfImages() < 2)
    {
        // ignore project with only one image
        return false;
    };
    HuginBase::ConstStandardImageVariableGroups lenses(pano);
    if (lenses.getLenses().getNumberOfParts() == 1)
    {
        const SrcPanoImage& img0 = pano.getImage(0);
        LensDB& lensDB = LensDB::GetSingleton();
        const std::string camMaker = img0.getExifMake();
        const std::string camModel = img0.getExifModel();
        if (!camMaker.empty() && !camModel.empty())
        {
            if (img0.getExifCropFactor() < 0.1f)
            {
                lensDB.SaveCameraCrop(camMaker, camModel, img0.getCropFactor());
            };
            // now check EMoR parameters
            const std::vector<float> emor=img0.getEMoRParams();
            if (emor.size() == 5)
            {
                float sum = 0;
                for (size_t i = 0; i < 5; ++i)
                { 
                    sum += fabs(emor[i]);
                };
                if (sum>0.001)
                {
                    lensDB.SaveEMoR(camMaker, camModel, img0.getExifISO(), emor);
                };
            };
        };
        const std::string lensname = img0.getDBLensName();
        const double focal = img0.getExifFocalLength();
        if (!lensname.empty() && focal > 0)
        {
            bool success = lensDB.SaveLensProjection(lensname, img0.getProjection());
            if (img0.getCropMode() == BaseSrcPanoImage::NO_CROP)
            {
                // pass empty Rect2D to remove crop information
                success = success | lensDB.SaveLensCrop(lensname, focal, img0.getSize(), vigra::Rect2D(0, 0, 0, 0));
            }
            else
            {
                const BaseSrcPanoImage::CropMode cropMode = img0.getCropMode();
                const vigra::Rect2D cropRect = img0.getCropRect();
                bool sameCrop = true;
                for (size_t i = 1; i < pano.getNrOfImages() && sameCrop; ++i)
                {
                    const SrcPanoImage& img = pano.getImage(i);
                    sameCrop = (img.getCropMode() == cropMode) && (img.getCropRect() == cropRect);
                };
                if (sameCrop)
                {
                    success = success | lensDB.SaveLensCrop(lensname, focal, img0.getSize(), cropRect);
                };
            };
            double min;
            double max;
            double mean;
            double var;
            // update cp errors
            CalculateCPStatisticsError::calcCtrlPntsErrorStats(pano, min, max, mean, var);
            if (mean < 15)
            {
                // save hfov and distortion only if error is small enough
                //@TODO add more robust check which takes also distribution of cp into account
                // recalculate to default aspect ratio of 3:2
                const double newFocallength = SrcPanoImage::calcFocalLength(img0.getProjection(), img0.getHFOV(), img0.getCropFactor(), img0.getSize());
                const double newHFOV = SrcPanoImage::calcHFOV(img0.getProjection(), newFocallength, img0.getCropFactor(), vigra::Size2D(3000, 2000));
                success = success | lensDB.SaveLensFov(lensname, focal, newHFOV);
                const std::vector<double> dist = img0.getRadialDistortion();
                if (dist.size() == 4)
                {
                    // check if values are plausible
                    if (fabs(dist[0]) + fabs(dist[1]) + fabs(dist[2])>0.001 && 
                        fabs(dist[0] + dist[1] + dist[2]) < 0.1)
                    {
                        success = success | lensDB.SaveDistortion(lensname, focal, dist);
                    };
                };
            };
            // check if aperture matches for all images
            bool sameAperture = true;
            for (size_t i = 1; i < pano.getNrOfImages() && sameAperture; ++i)
            {
                sameAperture = fabs(pano.getImage(i).getExifAperture() - img0.getExifAperture()) < 0.05;
            }
            // save vignetting data only if all images were shoot with same aperture
            if (sameAperture)
            {
                const std::vector<double> vigParam = img0.getRadialVigCorrCoeff();
                if (vigParam.size() == 4)
                {
                    // now check, if the vignetting parameter are plausible
                    const double sum = vigParam[0] + vigParam[1] + vigParam[2] + vigParam[3];
                    if (sum>0.5 && sum <= 1.01)
                    {
                        success = success | lensDB.SaveVignetting(lensname, focal, img0.getExifAperture(), img0.getExifDistance(), vigParam);
                    };
                };
            };
            return success;
        }
        else
        {
            return false;
        };
    };
    return false;
};

} //namespace LensDB
} //namespace HuginBase
