// -*- c-basic-offset: 4 -*-
/** @file AutoCtrlPointCreator.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _AUTOCTRLPOINTCREATOR_H
#define _AUTOCTRLPOINTCREATOR_H

#include <string>
#include <map>

#include <hugin_shared.h>
#include "CPDetectorConfig.h"
#include "panodata/Panorama.h"
#include "panodata/ControlPoint.h"
#include "panoinc_WX.h"

/** Base class for control point creators.
 *
 */
class ICPIMPEX AutoCtrlPointCreator
{
public:

    /** ctor.
     */
    AutoCtrlPointCreator() {};

    /** dtor.
     */
    virtual ~AutoCtrlPointCreator() {};

    /** Do cp matching, calles the right routines, based
     *  on the matcher selected
     */
    virtual HuginBase::CPVector automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                           int nFeatures, int & ret_value, wxWindow *parent=NULL);
    virtual HuginBase::CPVector automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                           int nFeatures, wxWindow *parent=NULL);
    virtual void Cleanup(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                           std::vector<wxString> &keyFiles, wxWindow *parent=NULL);

protected:
    HuginBase::CPVector readUpdatedControlPoints(const std::string & file,
                                  HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs, bool reordered);
};

/** A matcher that uses Sebastians Nowozin's excellent sift matcher */
class ICPIMPEX AutoPanoSift : public AutoCtrlPointCreator
{
public:

    /** ctor.
     */
    AutoPanoSift() {};

    /** dtor.
     */
    virtual ~AutoPanoSift() {} ;

    virtual HuginBase::CPVector automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                           int nFeatures, int & ret_value, wxWindow *parent=NULL);
    virtual HuginBase::CPVector automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                           int nFeatures, std::vector<wxString> &keyFiles, int & ret_value, wxWindow *parent=NULL);
};

/** A matcher that uses Alexandres sift matcher */
class ICPIMPEX AutoPanoKolor : public AutoCtrlPointCreator
{
public:

    /** ctor.
     */
    AutoPanoKolor() {};

    /** dtor.
     */
    virtual ~AutoPanoKolor() {} ;

    virtual HuginBase::CPVector automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                           int nFeatures, int & ret_value, wxWindow *parent=NULL);
};

/** A matcher that uses Sebastians Nowozin's excellent sift matcher and considers stacks */
class ICPIMPEX AutoPanoSiftStack : public AutoCtrlPointCreator
{
public:

    /** ctor.
     */
    AutoPanoSiftStack() {};

    /** dtor.
     */
    virtual ~AutoPanoSiftStack() {} ;

    virtual HuginBase::CPVector automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                           int nFeatures, int & ret_value, wxWindow *parent=NULL);
};

/** A matcher for multi-row panoramas based on an idea by Bruno Postle */
class ICPIMPEX AutoPanoSiftMultiRow : public AutoCtrlPointCreator
{
public:

    /** ctor.
     */
    AutoPanoSiftMultiRow() {};

    /** dtor.
     */
    virtual ~AutoPanoSiftMultiRow() {} ;

    virtual HuginBase::CPVector automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                           int nFeatures, int & ret_value, wxWindow *parent=NULL);
};

/** A matcher for multi-rows, which considers stacks */
class ICPIMPEX AutoPanoSiftMultiRowStack : public AutoCtrlPointCreator
{
public:

    /** ctor.
     */
    AutoPanoSiftMultiRowStack() {};

    /** dtor.
     */
    virtual ~AutoPanoSiftMultiRowStack() {} ;

    virtual HuginBase::CPVector automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                           int nFeatures, int & ret_value, wxWindow *parent=NULL);
};

/** A matcher that uses the align information in the panorama to generate cp between overlapping images */
class ICPIMPEX AutoPanoSiftPreAlign : public AutoCtrlPointCreator
{
public:

    /** ctor.
     */
    AutoPanoSiftPreAlign() {};

    /** dtor.
     */
    virtual ~AutoPanoSiftPreAlign() {} ;

    virtual HuginBase::CPVector automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                           int nFeatures, int & ret_value, wxWindow *parent=NULL);
    virtual HuginBase::CPVector automatch(CPDetectorSetting &setting, HuginBase::Panorama & pano, const HuginBase::UIntSet & imgs,
                           int nFeatures, std::vector<wxString> &keyFiles, int & ret_value, wxWindow *parent=NULL);
};

#endif // _AUTOCTRLPOINTCREATOR_H
