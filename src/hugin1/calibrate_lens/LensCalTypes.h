// -*- c-basic-offset: 4 -*-

/** @file LensCalTypes.h
 *
 *  @brief declaration of helper class for LensCal
 *
 *  @author T. Modes
 *
 */

/* 
 *  This program is free software; you can redistribute it and/or
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

#ifndef LENSCALTYPES_H
#define LENSCALTYPES_H

#include <vector>
#include "panoinc_WX.h"
#include "panodata/Panorama.h"
#include "lines/FindLines.h"

class ImageLineList
{
public:
    /** constructor */
    explicit ImageLineList(wxString newFilename);
    /** destructor, cleans up */
    ~ImageLineList();
    /** returns the number of valid lines for given image */
    const unsigned int GetNrOfValidLines();
    /** sets the edge detected image (old image will be deleted)*/
    void SetEdgeImage(vigra::BImage* newEdgeImage);
    /** return pointer to edge image */
    vigra::BImage* GetEdgeImage();
    /** sets the filename, will also regenerated the m_panoImage */ 
    void SetFilename(wxString newFilename);
    /** returns the filename */
    const wxString GetFilename();
    /** return the SrcPanoImage from the given filename */
    HuginBase::SrcPanoImage* GetPanoImage();
    /** store given lines in member variable */
    void SetLines(HuginLines::Lines lines);
    /** returns the list of detected lines */
    const HuginLines::Lines GetLines();
    /** scale all lines by given scaleFactor */
    void ScaleLines(double scaleFactor);
private:
    /** pointer to edge image */
    vigra::BImage* m_edge;
    /** list of detected lines */
    HuginLines::Lines m_lines;
    /** the filename */
    wxString m_filename;
    /** the HuginBase::SrcPanoImage, for generating the panorama class and necessary transformations */
    HuginBase::SrcPanoImage* m_panoImage;
};

#endif // LENSCALTYPES_H
