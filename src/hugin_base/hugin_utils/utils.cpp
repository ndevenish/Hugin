// -*- c-basic-offset: 4 -*-

/** @file hugin_utils/utils.cpp
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
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

#include "utils.h"
#include "hugin_version.h"

#ifdef _WIN32
    #define NOMINMAX
    #include <sys/utime.h>
    #include <ShlObj.h>
#else
    #include <sys/time.h>
    #include <cstdlib>
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
#endif
#include <time.h>
#include <fstream>
#include <stdio.h>
#include <cstdio>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <hugin_config.h>
#endif
#include <algorithm>
#define BOOST_FILESYSTEM_VERSION 3
#include <boost/filesystem.hpp>
#include <lcms2.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>  /* _NSGetExecutablePath */
#include <limits.h>       /* PATH_MAX */
#include <libgen.h>       /* dirname */
#endif

#if !defined Hugin_shared || !defined _WIN32
#define GLEW_STATIC
#endif
#include <GL/glew.h>
#ifdef _WIN32
#include <GL/wglew.h>
#elif defined __APPLE__
  #include <GLUT/glut.h>
#endif

namespace hugin_utils {
    
#ifdef UNIX_LIKE
std::string GetCurrentTimeString()
{
  char tmp[100];
  struct tm t;
  struct timeval tv;
  gettimeofday(&tv,NULL);
  localtime_r((time_t*)&tv.tv_sec, &t); // is the casting safe?
  strftime(tmp,99,"%H:%M:%S",&t);
  sprintf(tmp+8,".%06ld", (long)tv.tv_usec);
  return tmp;
}
#else
std::string GetCurrentTimeString()
{
    // FIXME implement for Win
    return "";
}
#endif


std::string getExtension(const std::string & basename2)
{
	std::string::size_type idx = basename2.rfind('.');
    // check if the dot is not followed by a \ or a /
    // to avoid cutting pathes.
    if (idx == std::string::npos) {
        // no dot found
		return std::string("");
    }
#ifdef UNIX_LIKE
    // check for slashes after dot
    std::string::size_type slashidx = basename2.find('/', idx);
    if ( slashidx == std::string::npos)
    {
        return basename2.substr(idx+1);
    } else {
        return std::string("");
    }
#else
    // check for slashes after dot
    std::string::size_type slashidx = basename2.find('/', idx);
    std::string::size_type backslashidx = basename2.find('\\', idx);
    if ( slashidx == std::string::npos &&  backslashidx == std::string::npos)
    {
        return basename2.substr(idx+1);
    } else {
		return std::string("");
    }
#endif
}

std::string stripExtension(const std::string & basename2)
{
    std::string::size_type idx = basename2.rfind('.');
    // check if the dot is not followed by a \ or a /
    // to avoid cutting pathes.
    if (idx == std::string::npos) {
        // no dot found
        return basename2;
    }
#ifdef UNIX_LIKE
    std::string::size_type slashidx = basename2.find('/', idx);
    if ( slashidx == std::string::npos)
    {
        return basename2.substr(0, idx);
    } else {
        return basename2;
    }
#else
    // check for slashes after dot
    std::string::size_type slashidx = basename2.find('/', idx);
    std::string::size_type backslashidx = basename2.find('\\', idx);
    if ( slashidx == std::string::npos &&  backslashidx == std::string::npos)
    {
        return basename2.substr(0, idx);
    } else {
        return basename2;
    }
#endif
}

std::string stripPath(const std::string & filename)
{
#ifdef UNIX_LIKE
    std::string::size_type idx = filename.rfind('/');
#else
    std::string::size_type idx1 = filename.rfind('\\');
    std::string::size_type idx2 = filename.rfind('/');
    std::string::size_type idx;
    if (idx1 == std::string::npos) {
        idx = idx2;
    } else if (idx2 == std::string::npos) {
        idx = idx1;
    } else {
        idx = std::max(idx1, idx2);
    }
#endif
    if (idx != std::string::npos) {
//        DEBUG_DEBUG("returning substring: " << filename.substr(idx + 1));
        return filename.substr(idx + 1);
    } else {
        return filename;
    }
}

std::string getPathPrefix(const std::string & filename)
{
#ifdef UNIX_LIKE
    std::string::size_type idx = filename.rfind('/');
#else
    std::string::size_type idx1 = filename.rfind('\\');
    std::string::size_type idx2 = filename.rfind('/');
    std::string::size_type idx;
    if (idx1 == std::string::npos) {
        idx = idx2;
    } else if (idx2 == std::string::npos) {
        idx = idx1;
    } else {
        idx = std::max(idx1, idx2);
    }
#endif
    if (idx != std::string::npos) {
//        DEBUG_DEBUG("returning substring: " << filename.substr(idx + 1));
        return filename.substr(0, idx+1);
    } else {
        return "";
    }
}

std::string StrTrim(const std::string& str)
{
    std::string s(str);
    std::string::size_type pos = s.find_last_not_of(" \t");
    if (pos != std::string::npos)
    {
        s.erase(pos + 1);
        pos = s.find_first_not_of(" \t");
        if (pos != std::string::npos)
        {
            s.erase(0, pos);
        };
    }
    else
    {
        s.erase(s.begin(), s.end());
    };
    return s;
}

std::string doubleToString(double d, int digits)
{
    char fmt[10];
    if (digits < 0) {
        strcpy(fmt,"%f");
    } else {
        std::sprintf(fmt,"%%.%df",digits);
    }
    char c[1024];
    c[1023] = 0;
#ifdef _MSC_VER
    _snprintf (c, 1023, fmt, d);
#else
    snprintf (c, 1023, fmt, d);
#endif

    std::string number (c);

    int l = (int)number.length()-1;

    while ( l != 0 && number[l] == '0' ) {
      number.erase (l);
      l--;
    }
    if ( number[l] == ',' ) {
      number.erase (l);
      l--;
    }
    if ( number[l] == '.' ) {
      number.erase (l);
      l--;
    }
    return number;
}

bool stringToInt(const std::string& s, int& val)
{
    if (StrTrim(s) == "0")
    {
        val = 0;
        return true;
    };
    int x = atoi(s.c_str());
    if (x != 0)
    {
        val = x;
        return true;
    };
    return false;
};

bool stringToUInt(const std::string&s, unsigned int& val)
{
    int x;
    if (stringToInt(s, x))
    {
        if (x >= 0)
        {
            val = static_cast<unsigned int>(x);
            return true;
        };
    };
    return false;
};

std::vector<std::string> SplitString(const std::string& s, const std::string& sep)
{
    std::vector<std::string> result;
    std::size_t pos = s.find_first_of(sep, 0);
    std::size_t pos2 = 0;
    while (pos != std::string::npos)
    {
        if (pos - pos2 > 0)
        {
            std::string t(s.substr(pos2, pos - pos2));
            t=StrTrim(t);
            if (!t.empty())
            {
                result.push_back(t);
            };
        };
        pos2 = pos + 1;
        pos = s.find_first_of(sep, pos2);
    }
    if (pos2 < s.length())
    {
        std::string t(s.substr(pos2));
        t = StrTrim(t);
        if (!t.empty())
        {
            result.push_back(t);
        };
    };
    return result;
};

void ReplaceAll(std::string& s, const std::string& oldChar, char newChar)
{
    std::size_t found = s.find_first_of(oldChar);
    while (found != std::string::npos)
    {
        s[found] = newChar;
        found = s.find_first_of(oldChar, found + 1);
    };
};

    void ControlPointErrorColour(const double cperr, 
        double &r,double &g, double &b)
    {
        //Colour change points
#define XP1 5.0f
#define XP2 10.0f

        if ( cperr<= XP1) 
        {
            //low error
            r = cperr / XP1;
            g = 0.75;
        }
        else
        {
            r = 1.0;
            g = 0.75 * ((1.0 - std::min<double>(cperr - XP1, XP2 - XP1) / (XP2 - XP1)));
        } 
        b = 0.0;
    }

bool FileExists(const std::string& filename)
{
    std::ifstream ifile(filename.c_str());
    return !ifile.fail();
}

std::string GetAbsoluteFilename(const std::string& filename)
{
#ifdef _WIN32
    char fullpath[_MAX_PATH];
    _fullpath(fullpath,filename.c_str(),_MAX_PATH);
    return std::string(fullpath);
#else
    //realpath works only with existing files
    //so as work around we create the file first, call then realpath 
    //and delete the temp file
    /** @TODO replace realpath with function with works without this hack */
    bool tempFileCreated=false;
    if(!FileExists(filename))
    {
        tempFileCreated=true;
        std::ofstream os(filename.c_str());
        os.close();
    };
    char *real_path = realpath(filename.c_str(), NULL);
    std::string absPath;
    if(real_path!=NULL)
    {
        absPath=std::string(real_path);
        free(real_path);
    }
    else
    {
        absPath=filename;
    };
    if(tempFileCreated)
    {
        remove(filename.c_str());
    };
    return absPath;
#endif
};

std::string GetDataDir()
{
#if _WIN32
    char buffer[MAX_PATH];//always use MAX_PATH for filepaths
    GetModuleFileName(NULL,buffer,sizeof(buffer));
    std::string working_path(buffer);
    std::string data_path("");
    //remove filename
    std::string::size_type pos=working_path.rfind("\\");
    if(pos!=std::string::npos)
    {
        working_path.erase(pos);
        //remove last dir: should be bin
        pos=working_path.rfind("\\");
        if(pos!=std::string::npos)
        {
            working_path.erase(pos);
            //append path delimiter and path
            working_path.append("\\share\\hugin\\data\\");
            data_path=working_path;
        }
    }
#elif defined MAC_SELF_CONTAINED_BUNDLE
    char path[PATH_MAX + 1];
    uint32_t size = sizeof(path);
    std::string data_path("");
    if (_NSGetExecutablePath(path, &size) == 0)
    {
        data_path=dirname(path);
        data_path.append("/../Resources/xrc/");
    }
#else
    std::string data_path = (INSTALL_DATA_DIR);
#endif
    return data_path;
};

std::string GetUserAppDataDir()
{
    boost::filesystem::path path;
#ifdef _WIN32
    char fullpath[_MAX_PATH];
    if(SHGetFolderPath(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, fullpath)!=S_OK)
    {
        return std::string();
    };
    path = boost::filesystem::path(fullpath);
    path /= "hugin";
#else
    char *homedir = getenv("HOME");
    struct passwd *pw;
    if (homedir == NULL)
    {
        pw = getpwuid(getuid());
        if(pw != NULL)
        {
            homedir = pw->pw_dir;
        };
    };
    if(homedir == NULL)
    {
        return std::string();
    };
    path = boost::filesystem::path(homedir);
    // we have already a file with name ".hugin" for our wxWidgets settings
    // therefore we use directory ".hugindata" in homedir
    path /= ".hugindata";
#endif
    if (!boost::filesystem::exists(path))
    {
        if (!boost::filesystem::create_directories(path))
        {
            std::cerr << "ERROR: Could not create destination directory: " << path.string() << std::endl
                << "Maybe you have not sufficent rights to create this directory." << std::endl;
            return std::string();
        };
    };
    return path.string();
};

// initialization and wrapup of GPU for GPU remapping
#ifdef _WIN32
struct ContextSettings
{
    HWND window;
    HDC dc;
    HGLRC renderingContext;
    
    ContextSettings()
    {
        window = NULL;
        dc = NULL;
        renderingContext = NULL;
    }
};
static ContextSettings context;

// create context, return false if failed
bool CreateContext(int *argcp, char **argv)
{
    WNDCLASS windowClass;
    /* register window class */
    ZeroMemory(&windowClass, sizeof(WNDCLASS));
    windowClass.hInstance = GetModuleHandle(NULL);
    windowClass.lpfnWndProc = DefWindowProc;
    windowClass.lpszClassName = "Hugin";
    if (RegisterClass(&windowClass) == 0)
    {
        return false;
    };
    /* create window */
    context.window = CreateWindow("Hugin", "Hugin", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (context.window == NULL)
    {
        return false;
    };
    /* get the device context */
    context.dc = GetDC(context.window);
    if (context.dc == NULL)
    {
        return false;
    };
    /* find pixel format */
    PIXELFORMATDESCRIPTOR pixelFormatDesc;
    ZeroMemory(&pixelFormatDesc, sizeof(PIXELFORMATDESCRIPTOR));
    pixelFormatDesc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pixelFormatDesc.nVersion = 1;
    pixelFormatDesc.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    int pixelFormat = ChoosePixelFormat(context.dc, &pixelFormatDesc);
    if (pixelFormat == 0)
    {
        return false;
    }
    /* set the pixel format for the dc */
    if (SetPixelFormat(context.dc, pixelFormat, &pixelFormatDesc) == FALSE)
    {
        return false;
    };
    /* create rendering context */
    context.renderingContext = wglCreateContext(context.dc);
    if (context.renderingContext == NULL)
    {
        return false;
    };
    if (wglMakeCurrent(context.dc, context.renderingContext) == FALSE)
    {
        return false;
    };
    return true;
}

void DestroyContext()
{
    if (context.renderingContext != NULL)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(context.renderingContext);
    }
    if (context.window != NULL && context.dc != NULL)
    {
        ReleaseDC(context.window, context.dc);
    };
    if (context.window != NULL)
    {
        DestroyWindow(context.window);
    };
    UnregisterClass("Hugin", GetModuleHandle(NULL));
}

#elif defined __APPLE__
static GLuint GlutWindowHandle;
bool CreateContext(int *argcp, char **argv)
{
    glutInit(argcp, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA | GLUT_ALPHA);
    GlutWindowHandle = glutCreateWindow("Hugin");
    return true;
}

void DestroyContext()
{
    glutDestroyWindow(GlutWindowHandle);
}

#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>

struct ContextSettings
{
    Display* display;
    XVisualInfo* visualInfo;
    GLXContext context;
    Window window;
    Colormap colormap;
    
    ContextSettings()
    {
        display=NULL;
        visualInfo=NULL;
        context=NULL;
        window=0;
        colormap=0;
    };
};

static ContextSettings context;

bool CreateContext(int *argcp, char **argv)
{
    /* open display */
    context.display = XOpenDisplay(NULL);
    if (context.display == NULL)
    {
        return false;
    };
    /* query for glx */
    int erb, evb;
    if (!glXQueryExtension(context.display, &erb, &evb))
    {
        return false;
    };
    /* choose visual */
    int attrib[] = { GLX_RGBA, None };
    context.visualInfo = glXChooseVisual(context.display, DefaultScreen(context.display), attrib);
    if (context.visualInfo == NULL)
    {
        return false;
    };
    /* create context */
    context.context = glXCreateContext(context.display, context.visualInfo, None, True);
    if (context.context == NULL)
    {
        return false;
    };
    /* create window */
    context.colormap = XCreateColormap(context.display, RootWindow(context.display, context.visualInfo->screen), context.visualInfo->visual, AllocNone);
    XSetWindowAttributes swa;
    swa.border_pixel = 0;
    swa.colormap = context.colormap;
    context.window = XCreateWindow(context.display, RootWindow(context.display, context.visualInfo->screen),
        0, 0, 1, 1, 0, context.visualInfo->depth, InputOutput, context.visualInfo->visual,
        CWBorderPixel | CWColormap, &swa);
    /* make context current */
    if (!glXMakeCurrent(context.display, context.window, context.context))
    {
        return false;
    };
    return true;
};

void DestroyContext()
{
    if (context.display != NULL && context.context != NULL)
    {
        glXDestroyContext(context.display, context.context);
    }
    if (context.display != NULL && context.window != 0)
    {
        XDestroyWindow(context.display, context.window);
    };
    if (context.display != NULL && context.colormap != 0)
    {
        XFreeColormap(context.display, context.colormap);
    };
    if (context.visualInfo != NULL)
    {
        XFree(context.visualInfo);
    };
    if (context.display != NULL)
    {
        XCloseDisplay(context.display);
    };
};
#endif

bool initGPU(int *argcp, char **argv)
{
    if (!CreateContext(argcp, argv))
    {
        return false;
    };
    int err = glewInit();
    if (err != GLEW_OK)
    {
        std::cerr << argv[0] << ": an error occured while setting up the GPU:" << std::endl;
        std::cerr << glewGetErrorString(err) << std::endl;
        std::cerr << argv[0] << ": Switching to CPU calculation." << std::endl;
        DestroyContext();
        return false;
    }

    std::cout << hugin_utils::stripPath(argv[0]) << ": using graphics card: " << glGetString(GL_VENDOR) << " " << glGetString(GL_RENDERER) << std::endl;

    const GLboolean has_arb_fragment_shader = glewGetExtension("GL_ARB_fragment_shader");
    const GLboolean has_arb_vertex_shader = glewGetExtension("GL_ARB_vertex_shader");
    const GLboolean has_arb_shader_objects = glewGetExtension("GL_ARB_shader_objects");
    const GLboolean has_arb_shading_language = glewGetExtension("GL_ARB_shading_language_100");
    const GLboolean has_ext_framebuffer = glewGetExtension("GL_EXT_framebuffer_object");
    const GLboolean has_arb_texture_rectangle = glewGetExtension("GL_ARB_texture_rectangle");
    const GLboolean has_arb_texture_border_clamp = glewGetExtension("GL_ARB_texture_border_clamp");
    const GLboolean has_arb_texture_float = glewGetExtension("GL_ARB_texture_float");

    if (!(has_arb_fragment_shader && has_arb_vertex_shader && has_arb_shader_objects && has_arb_shading_language && has_ext_framebuffer && has_arb_texture_rectangle && has_arb_texture_border_clamp && has_arb_texture_float)) {
        const char * msg[] = {"false", "true"};
        std::cerr << argv[0] << ": extension GL_ARB_fragment_shader = " << msg[has_arb_fragment_shader] << std::endl;
        std::cerr << argv[0] << ": extension GL_ARB_vertex_shader = " << msg[has_arb_vertex_shader] << std::endl;
        std::cerr << argv[0] << ": extension GL_ARB_shader_objects = " << msg[has_arb_shader_objects] << std::endl;
        std::cerr << argv[0] << ": extension GL_ARB_shading_language_100 = " << msg[has_arb_shading_language] << std::endl;
        std::cerr << argv[0] << ": extension GL_EXT_framebuffer_object = " << msg[has_ext_framebuffer] << std::endl;
        std::cerr << argv[0] << ": extension GL_ARB_texture_rectangle = " << msg[has_arb_texture_rectangle] << std::endl;
        std::cerr << argv[0] << ": extension GL_ARB_texture_border_clamp = " << msg[has_arb_texture_border_clamp] << std::endl;
        std::cerr << argv[0] << ": extension GL_ARB_texture_float = " << msg[has_arb_texture_float] << std::endl;
        std::cerr << argv[0] << ": This graphics system lacks the necessary extensions for -g." << std::endl;
        std::cerr << argv[0] << ": Switching to CPU calculation." << std::endl;
        DestroyContext();
        return false;
    }

    return true;
}

bool wrapupGPU()
{
    DestroyContext();
    return true;
}

std::string GetHuginVersion()
{
    return std::string(DISPLAY_VERSION);
};

std::string GetICCDesc(const vigra::ImageImportInfo::ICCProfile& iccProfile)
{
    if (iccProfile.empty())
    {
        // no profile
        return std::string();
    };
    cmsHPROFILE profile = cmsOpenProfileFromMem(iccProfile.data(), iccProfile.size());
    if (profile == NULL)
    {
        // invalid profile
        return std::string();
    };
    const std::string name=GetICCDesc(profile);
    cmsCloseProfile(profile);
    return name;
};

std::string GetICCDesc(const cmsHPROFILE& profile)
{
    const size_t size = cmsGetProfileInfoASCII(profile, cmsInfoDescription, cmsNoLanguage, cmsNoCountry, nullptr, 0);
    std::string information(size, '\000');
    cmsGetProfileInfoASCII(profile, cmsInfoDescription, cmsNoLanguage, cmsNoCountry, &information[0], size);
    StrTrim(information);
    return information;
}
} //namespace
