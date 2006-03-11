// -*- c-basic-offset: 4 -*-
/** @file config_defaults.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _CONFIG_DEFAULTS_H
#define _CONFIG_DEFAULTS_H

// contains various configuration defaults

// template matching
#define HUGIN_FT_TEMPLATE_SIZE                21l
#define HUGIN_FT_SEARCH_AREA_PERCENT          10l
#define HUGIN_FT_LOCAL_SEARCH_WIDTH           14l
#define HUGIN_FT_CORR_THRESHOLD               0.8
#define HUGIN_FT_CURV_THRESHOLD               0.0

#define HUGIN_FT_ROTATION_SEARCH              0l
#define HUGIN_FT_ROTATION_START_ANGLE         -30.0
#define HUGIN_FT_ROTATION_STOP_ANGLE           30.0
#define HUGIN_FT_ROTATION_STEPS               12l


// Image cache defaults
#define HUGIN_IMGCACHE_UPPERBOUND             78643200

#define HUGIN_CP_CURSOR                       1

#define HUGIN_CAPTURE_TIMESPAN                60l

#define HUGIN_PREVIEW_SHOW_DRUID              1l
#define HUGIN_USE_SELECTED_IMAGES             0l
#define HUGIN_CROP_SETS_CENTER                0l

// GUI defaults
#define HUGIN_LANGUAGE                        wxLANGUAGE_DEFAULT
// sort by filename (1), sort by date (2)
#define HUGIN_GUI_SORT_NEW_IMG_ON_ADD         1l

// Program defaults
#if defined WIN32

#define HUGIN_PT_STITCHER_EXE                 "PTStitcher.exe"
#define HUGIN_PT_OPTIMIZER_EXE                "PTOptimizer.exe"

#define HUGIN_ENBLEND_EXE                     "enblend.exe"

#define HUGIN_AP_TYPE                         0l
#define HUGIN_APKOLOR_EXE                     "autopano.exe"
#define HUGIN_APKOLOR_ARGS   "/allinone /path:%d /keys:%p /project:oto /name:%o /size:1024 /f %i"

#define HUGIN_APSIFT_EXE                      "autopano-win32.exe"


#elif defined __WXMAC__

#define HUGIN_PT_STITCHER_EXE                 "PTStitcher"
#define HUGIN_PT_OPTIMIZER_EXE                "PTOptimizer"

#define HUGIN_ENBLEND_EXE                     "enblend"

#define HUGIN_AP_TYPE                         1l
#define HUGIN_APKOLOR_EXE                     ""
#define HUGIN_APKOLOR_ARGS                    ""

#define HUGIN_APSIFT_EXE                      "autopano-complete.sh"
#define HUGIN_APSIFT_ARGS                     "-o %o -p %p %i"


#elif defined unix

#define HUGIN_PT_STITCHER_EXE                 "PTStitcher"
#define HUGIN_PT_OPTIMIZER_EXE                "PTOptimizer"

#define HUGIN_ENBLEND_EXE                     "enblend"

#define HUGIN_AP_TYPE                         1l
#define HUGIN_APKOLOR_EXE                     ""
#define HUGIN_APKOLOR_ARGS                    ""

#define HUGIN_APSIFT_EXE                      "autopanog.exe"


#endif

// enblend args
#define HUGIN_ENBLEND_ARGS                    "-v "
#define HUGIN_ENBLEND_DELETE_REMAPPED_FILES   0l
#define HUGIN_ENBLEND_USE_CROPPED_FILES       0l
// Autopano-SIFT args
#ifndef HUGIN_APSIFT_ARGS
#define HUGIN_APSIFT_ARGS                     "--output %o --imagelist %namefile"
#endif

#endif // _CONFIG_DEFAULTS_H
