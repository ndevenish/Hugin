// -*- c-basic-offset: 4 -*-

/** @file CPDetectorConfig_default.h
 *
 *  @brief default settings for different control point detectors 
 *
 *  @author Thomas Modes
 *
 *  $Id$
 *
 */

/*  This is free software; you can redistribute it and/or
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _CPDETECTORCONFIG_DEFAULT_H
#define _CPDETECTORCONFIG_DEFAULT_H

#include "panoinc_WX.h"

/** struct to save the default setting of one CP detector */
struct cpdetector_default
{
    /** cp detector type */
    CPDetectorType type;
    /** name, which is shown in GUI */
    wxString desc;
    /** program name for detector or descriptor*/
    wxString prog;
    /** arguments of the detector or descriptor*/
    wxString args;
    /** arguments for the cleanup step */
    wxString args_cleanup;
    /** program name for matcher */
    wxString prog_matcher;
    /** arguments of matcher */
    wxString args_matcher;
    /** program name of cp detector for stacks */
    wxString prog_stack;
    /** arguments of cp detector for stacks */
    wxString args_stack;
    /** option, currently only used in multi row detectors */
    bool option;
};

/** this array saves all default settings */
const struct cpdetector_default default_cpdetectors[]=
{
/* use following order: {type, description, program for detector or descriptor, arguments for detector or descriptor,
   arguments for cleanup, program for matcher, arguments for matcher, program name stack, arguments stack, option} 
   attention: this array have to contain at least one item */
#if defined WIN32
    {CPDetector_AutoPanoSiftMultiRowStack, wxT("Hugins CPFind"),
        wxT("cpfind.exe"),wxT("--cache -o %o %s"),wxT("--clean %s"),
        wxT(""),wxT(""),wxT("cpfind.exe"),wxT("--cache -o %o %s"), true},
    {CPDetector_AutoPanoSift, wxT("Autopano-SIFT-C"),
        wxT("autopano-sift-c.exe"),wxT("--maxmatches %p --projection %f,%v %o %i"),wxT(""),
        wxT(""), wxT(""), wxT(""), wxT(""), true},
    {CPDetector_AutoPano, wxT("Autopano"),
        wxT("autopano.exe"),wxT("/allinone /path:%d /keys:%p /project:oto /name:%o /size:1024 /f %i"),wxT(""),
        wxT(""), wxT(""), wxT(""), wxT(""), true},
    {CPDetector_AutoPanoSift, wxT("Panomatic"),wxT("panomatic.exe"),wxT("-o %o %i"),wxT(""),wxT(""), wxT(""), wxT(""), wxT(""), true},
    {CPDetector_AutoPanoSift, wxT("Align image stack"),wxT("align_image_stack.exe"),wxT("-f %v -v -p %o %i"),wxT(""),wxT(""),wxT(""), wxT(""), wxT(""), true},
    {CPDetector_AutoPanoSiftMultiRowStack, wxT("Autopano-SIFT-C (multirow/stacked)"),
        wxT("generatekeys.exe"),wxT("%i %k 800"),wxT(""),wxT("autopano.exe"),wxT("--maxmatches %p %o %k"),
        wxT("align_image_stack.exe"),wxT("-f %v -v -p %o %i"), true},
    {CPDetector_AutoPanoSift, wxT("Match-n-shift"),wxT("match-n-shift.exe"),wxT("-b -a -f %f -v %v -c -p %p -o %o %i"),wxT(""),wxT(""),wxT(""), wxT(""), wxT(""), true}
#else 
  #if !defined MAC_SELF_CONTAINED_BUNDLE
    // Since medio 2008 the MacOSX bundle is built without patent/license restricted CP detectors.
    {CPDetector_AutoPanoSift,wxT("Autopano-SIFT-C"),wxT("autopano-noop.sh"),wxT("--maxmatches %p --projection %f,%v %o %i"),wxT(""),wxT(""),wxT(""), wxT(""), wxT(""), true},
    {CPDetector_AutoPanoSift,wxT("Panomatic"),wxT("panomatic"),wxT("-o %o %i"),wxT(""),wxT(""),wxT(""), wxT(""), wxT(""), true},
    {CPDetector_AutoPanoSiftMultiRowStack, wxT("Hugins CPFind"),wxT("cpfind"),wxT("--cache -o %o %s"),wxT("--clean %s"),wxT(""),wxT(""),wxT("cpfind"),wxT("--cache -o %o %s"), true},
    {CPDetector_AutoPanoSift,wxT("Match-n-shift"),wxT("match-n-shift"),wxT("-b -a -f %f -v %v -c -p %p -o %o %i"),wxT(""),wxT(""),wxT(""),wxT(""), wxT(""), true},
    {CPDetector_AutoPanoSiftMultiRowStack, wxT("Autopano-SIFT-C (multirow/stacked)"),
        wxT("generatekeys"),wxT("%i %k 800"), wxT("autopano"),wxT("--maxmatches %p %o %k"),wxT(""),
        wxT("align_image_stack"),wxT("-f %v -p %o %i"), true},
    {CPDetector_AutoPanoSift,wxT("Align image stack"),wxT("align_image_stack"),wxT("-f %v -p %o %i"),wxT(""),wxT(""),wxT(""), wxT(""), wxT(""), true},
    {CPDetector_AutoPanoSift,wxT("Align_image_stack FullFrameFisheye"),wxT("align_image_stack"),wxT("-f %v -e -p %o %i"),wxT(""),wxT(""),wxT(""), wxT(""), wxT(""), true}
  #else
    {CPDetector_AutoPanoSift,wxT("Pablo's patent free Panomatic"),wxT("patfree-panomatic"),wxT("--sieve1size 20 --sieve2size 2 -o %o %i"),wxT(""),wxT(""),wxT(""), wxT(""), wxT(""), true},
    {CPDetector_AutoPanoSift,wxT("Align_image_stack linear"),wxT("align_image_stack"),wxT("-v -f %v -p %o %i"),wxT(""),wxT(""),wxT(""), wxT(""), wxT(""), true},
    {CPDetector_AutoPanoSift,wxT("Align_image_stack FullFrameFisheye"),wxT("align_image_stack"),wxT("-v -f %v -e -p %o %i"),wxT(""),wxT(""),wxT(""), wxT(""), wxT(""), true},
    {CPDetector_AutoPanoSift,wxT("Autopano-SIFT-C"),wxT("== Use the Choose button to search for it =="),wxT("--maxmatches %p %o %i"),wxT(""),wxT(""),wxT(""), wxT(""), wxT(""), true},
    {CPDetector_AutoPanoSift,wxT("Panomatic"),wxT("== Use the Choose button to search for it =="),wxT("-o %o %i"),wxT(""),wxT(""),wxT(""), wxT(""), wxT(""), true}
  #endif
#endif
};

#endif
