// -*- c-basic-offset: 4 -*-
/** @file ImageTransformsGPU.cpp
 *
 *  Support functions for GPU remapping.
 *
 *  @author Andrew Mihal
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

#include <iostream>
#include <iomanip>

#if !defined Hugin_shared || !defined _WINDOWS
#define GLEW_STATIC
#endif
#include <GL/glew.h>
#ifdef __APPLE__
  #include <GLUT/glut.h>
#else
  #include <GL/glut.h>
#endif

#include <string.h>
#ifdef _WIN32
// weird errors in accessor.hxx if DIFFERENCE is defined
  #undef DIFFERENCE
#endif
#include <vigra/diff2d.hxx>
#include <vigra/utilities.hxx>
#include <vigra/error.hxx>
#include <vigra_ext/ImageTransformsGPU.h>

#ifdef _WIN32
#include <windows.h>
long getms()
{
    return GetTickCount();
};
#else
#include <sys/time.h>
long getms()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (long)(tv.tv_sec*1000+(tv.tv_usec/1000));
};
#endif
#include <time.h>

#include <vector>

using std::cout;
using std::cerr;
using std::endl;
using std::vector;

using vigra::Rect2D;

#define ___NCFILE___ ((char *) __FILE__)
#define CHECK_GL() checkGLErrors(__LINE__, ___NCFILE___)


static GLenum XGLMap[] = {
    // gltypes
    GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT, GL_INT, GL_UNSIGNED_INT, GL_FLOAT,
    // Internalformats
    GL_RGBA8, GL_RGBA16, GL_RGBA32F_ARB, GL_LUMINANCE8_ALPHA8, GL_LUMINANCE16_ALPHA16, GL_LUMINANCE_ALPHA32F_ARB,
    GL_RGB8,  GL_RGB16,  GL_RGB32F_ARB,  GL_LUMINANCE8,        GL_LUMINANCE16,         GL_LUMINANCE32F_ARB,
    // formats
    GL_LUMINANCE, GL_RGB, GL_LUMINANCE_ALPHA, GL_RGBA
};

static const char* XGLStringMap[] = {
    // gltypes
    "GL_BYTE", "GL_UNSIGNED_BYTE", "GL_SHORT", "GL_UNSIGNED_SHORT", "GL_INT", "GL_UNSIGNED_INT", "GL_FLOAT",
    // Internalformats
    "GL_RGBA8", "GL_RGBA16", "GL_RGBA32F_ARB", "GL_LUMINANCE8_ALPHA8", "GL_LUMINANCE16_ALPHA16", "GL_LUMINANCE_ALPHA32F_ARB",
    "GL_RGB8",  "GL_RGB16",  "GL_RGB32F_ARB",  "GL_LUMINANCE8",        "GL_LUMINANCE16",         "GL_LUMINANCE32F_ARB",
    // formats
    "GL_LUMINANCE", "GL_RGB", "GL_LUMINANCE_ALPHA", "GL_RGBA"
};

static int BytesPerPixel[] = {
    1, 1, 2, 2, 4, 4, 4,
    4, 8, 16, 2, 4, 8,
    3, 6, 12, 1, 2, 4,
    0, 0, 0, 0
};

static const char* AlphaCompositeKernelSource = {
"#version 110\n"
"#extension GL_ARB_texture_rectangle : enable\n"
"uniform sampler2DRect SrcAlphaTexture;\n"
"void main(void)\n"
"{\n"
"   float alpha = texture2DRect(SrcAlphaTexture, gl_TexCoord[0].st).a;\n"
"   if (alpha != 0.0) discard;\n"
"   gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n"
"}\n"
};

static void checkGLErrors(int line, char* file) {
    GLenum errCode;
    if ((errCode = glGetError()) != GL_NO_ERROR) {
        cerr << "nona: GL error in " << file << ":" << line << ": " << gluErrorString(errCode) << endl;
        exit(1);
    }
}

static void printInfoLog(GLhandleARB obj) {
  GLint infologLength = 0;
  GLint charsWritten = 0;
  char *infoLog;
    glGetObjectParameterivARB(obj, GL_OBJECT_INFO_LOG_LENGTH_ARB, &infologLength);
    if (infologLength > 1) {
        infoLog = new char[infologLength];
        glGetInfoLogARB(obj, infologLength, &charsWritten, infoLog);
        cout << "nona: GL info log:" << endl << infoLog << endl << endl;
        delete[] infoLog;
    }
}

static bool printDebug=false;

static bool checkFramebufferStatus(int line, char* file) {
    GLenum status;
    status = (GLenum) glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE_EXT:
            return true;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
            cerr << "nona: GL error: Framebuffer incomplete, incomplete attachment in: " << file << ":" << line << endl;
            return false;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
            cerr << "nona: Unsupported framebuffer format in: " << file << ":" << line << endl;
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
            cerr << "nona: Framebuffer incomplete, missing attachment in: " << file << ":" << line << endl;
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            cerr << "nona: Framebuffer incomplete, attached images must have same dimensions in: " << file << ":" << line << endl;
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
            cerr << "nona: Framebuffer incomplete, attached images must have same format in: " << file << ":" << line << endl;
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
            cerr << "nona: Framebuffer incomplete, missing draw buffer in: " << file << ":" << line << endl;
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
            cerr << "nona: Framebuffer incomplete, missing read buffer in: " << file << ":" << line << endl;
            return false;
    }

    return false;
}

static void compileGLSL(const char* programName,
                        GLhandleARB& programObject,
                        GLhandleARB& shaderObject,
                        const char** source)
{
    GLint success;

    programObject = glCreateProgramObjectARB();
    shaderObject = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

    glShaderSourceARB(shaderObject, 1, source, NULL);
    glCompileShaderARB(shaderObject);

    glGetObjectParameterivARB(shaderObject, GL_OBJECT_COMPILE_STATUS_ARB, &success);
    if (!success) {
        cerr << "nona: " << programName << " shader program could not be compiled." << endl;
        printInfoLog(shaderObject);
        exit(1);
    }

    if(printDebug)
        printInfoLog(shaderObject);

    glAttachObjectARB(programObject, shaderObject);
    glLinkProgramARB(programObject);

    glGetObjectParameterivARB(programObject, GL_OBJECT_LINK_STATUS_ARB, &success);
    if (!success) {
        cerr << "nona: " << programName << " shader program could not be linked." << endl;
        printInfoLog(programObject);
        exit(1);
    }

    if(printDebug)
        printInfoLog(programObject);
}

static void makeChunks(const int width,
                       const int height,
                       const int maxTextureSize,
                       const long long int maxPixels,
                       vector<Rect2D>& result)
{
    int numXChunks = 1;
    int numYChunks = 1;

    // Make chunks small enough to fit into maxTextureSize
    while (ceil(static_cast<double>(width) / numXChunks) > maxTextureSize) numXChunks++;
    while (ceil(static_cast<double>(height) / numYChunks) > maxTextureSize) numYChunks++;

    // Make chunks small enough to fit into maxPixels limit
    while ((ceil(static_cast<double>(width) / numXChunks) * ceil(static_cast<double>(height) / numYChunks))
           > maxPixels) {

        if (ceil(static_cast<double>(width) / numXChunks) > ceil(static_cast<double>(height) / numYChunks)) {
            ++numXChunks;
        } else {
            ++numYChunks;
        }
    }

    // Make chunks small enough to fit in GL_PROXY_TEXTURE_2D of the biggest internalformat type.
    while (1) {
        glTexImage2D(GL_PROXY_TEXTURE_2D,
                     0,
                     GL_RGBA32F_ARB,
                     static_cast<int>(ceil(static_cast<double>(width) / numXChunks)),
                     static_cast<int>(ceil(static_cast<double>(height) / numYChunks)),
                     0,
                     GL_RGBA,
                     GL_FLOAT,
                     NULL);
        GLint returnedWidth;
        glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &returnedWidth);

        if (returnedWidth != 0) break;

        if (ceil(static_cast<double>(width) / numXChunks) > ceil(static_cast<double>(height) / numYChunks)) {
            ++numXChunks;
        } else {
            ++numYChunks;
        }
    }

    for (int yChunk = 0, y = 0; yChunk < numYChunks; yChunk++) {
        int yEnd = std::min<int>(height, static_cast<int>(ceil(static_cast<double>(height) / numYChunks)) + y);
        for (int xChunk = 0, x = 0; xChunk < numXChunks; xChunk++) {
            int xEnd = std::min<int>(width, static_cast<int>(ceil(static_cast<double>(width) / numXChunks)) + x);
            result.push_back(Rect2D(x, y, xEnd, yEnd));
            x = xEnd;
        }
        y = yEnd;
    }
}


namespace vigra_ext
{

void SetGPUDebugMessages(const bool doPrint)
{
    printDebug=doPrint;
};

bool transformImageGPUIntern(const std::string& coordXformGLSL,
                             const std::string& interpolatorGLSL,
                             const int interpolatorSize,
                             const std::string& photometricGLSL,
                             const vector<double>& invLut,
                             const vector<double>& destLut,
                             const vigra::Diff2D srcSize,
                             const void* const srcBuffer,
                             const int srcGLInternalFormat, const int srcGLTransferFormat, const int srcGLFormat, const int srcGLType,
                             const void* const srcAlphaBuffer,
                             const int srcAlphaGLType,
                             const vigra::Diff2D destUL,
                             const vigra::Diff2D destSize,
                             void* const destBuffer,
                             const int destGLInternalFormat, const int destGLTransferFormat, const int destGLFormat, const int destGLType,
                             void* const destAlphaBuffer,
                             const int destAlphaGLType,
                             const bool warparound)
{
    long t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18, t19, t20, t21;

    const int xstart = destUL.x;
    const int xend   = destUL.x + destSize.x;
    const int ystart = destUL.y;
    const int yend   = destUL.y + destSize.y;

    if(printDebug)
    {
        t1=getms();
        cout << "destStart=[" << xstart << ", " << ystart << "]" << endl
             << "destEnd=[" << xend << ", " << yend << "]" << endl
             << "destSize=[" << destSize << "]" << endl
             << "srcSize=[" << srcSize << "]" << endl
             << "srcBuffer=" << srcBuffer << endl
             << "srcAlphaBuffer=" << srcAlphaBuffer << endl
             << "destBuffer=" << destBuffer << endl
             << "destAlphaBuffer=" << destAlphaBuffer << endl
             << "destGLInternalFormat=" << XGLStringMap[destGLInternalFormat] << endl
             << "destGLFormat=" << XGLStringMap[destGLFormat] << endl
             << "destGLType=" << XGLStringMap[destGLType] << endl
             << "srcGLInternalFormat=" << XGLStringMap[srcGLInternalFormat] << endl
             << "srcGLFormat=" << XGLStringMap[srcGLFormat] << endl
             << "srcGLType=" << XGLStringMap[srcGLType] << endl
             << "srcAlphaGLType=" << XGLStringMap[srcAlphaGLType] << endl
             << "destAlphaGLType=" << XGLStringMap[destAlphaGLType] << endl
             << endl
             << "warparound=" << warparound << endl;
    };

    vigra_precondition((srcSize.x % 8) == 0, "src image width not a multiple of 8");
    vigra_precondition((destSize.x % 8) == 0, "dest image width not a multiple of 8");

    vigra_precondition((reinterpret_cast<const uintptr_t>(srcBuffer) & 0x7) == 0, "src image buffer not 8-byte aligned");
    vigra_precondition((reinterpret_cast<const uintptr_t>(srcAlphaBuffer) & 0x7) == 0, "src alpha image buffer not 8-byte aligned");
    vigra_precondition((reinterpret_cast<const uintptr_t>(destBuffer) & 0x7) == 0, "dest image buffer not 8-byte aligned");
    vigra_precondition((reinterpret_cast<const uintptr_t>(destAlphaBuffer) & 0x7) == 0, "dest alpha image buffer not 8-byte aligned");

    const char* const gpuVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const bool needsAtanWorkaround = (strncmp(gpuVendor, "ATI", 3) == 0);

    if(printDebug)
        cout << "needsAtanWorkaround=" << needsAtanWorkaround << endl;

    GLint maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    if(printDebug)
        cout << "maxTextureSize=" << maxTextureSize << endl;

    // Artificial limit: binding big textures to fbos seems to be very slow.
    //maxTextureSize = 2048;

    const long long int GpuMemoryInBytes = 512 << 20;
    const double SourceAllocationRatio = 0.7;

    const int bytesPerSourcePixel = BytesPerPixel[srcGLInternalFormat]
                                    + ((srcGLInternalFormat != srcGLTransferFormat) ? BytesPerPixel[srcGLTransferFormat] : 0)
                                    + ((srcAlphaBuffer != NULL) ? 1 : 0);
    const long long int maxSourcePixels = static_cast<long long int>(GpuMemoryInBytes * SourceAllocationRatio) / bytesPerSourcePixel;

    vector<Rect2D> sourceChunks;
    makeChunks(srcSize.x, srcSize.y, maxTextureSize, maxSourcePixels, sourceChunks);

    const long long int actualSourcePixels = sourceChunks[0].area();
    const long long int gpuMemoryRemaining = GpuMemoryInBytes - (actualSourcePixels * bytesPerSourcePixel);

    // 16 bytes/pixel * 2 dest images for ping/pong multipass rendering
    //  8 bytes/pixel for coordinate texture
    //  destXfer + GL_ALPHA
    const int bytesPerDestPixel = 16 + 16 + 8
                                  + ((XGLMap[destGLTransferFormat] != GL_RGBA32F_ARB) ? BytesPerPixel[destGLTransferFormat] : 0)
                                  + ((destAlphaBuffer != NULL) ? 1 : 0);
    const long long int maxDestPixels = gpuMemoryRemaining / bytesPerDestPixel;

    vector<Rect2D> destChunks;
    makeChunks(destSize.x, destSize.y, 2048, maxDestPixels, destChunks);

    const long long int totalGpuMemoryUsed = (sourceChunks[0].area() * bytesPerSourcePixel) + (destChunks[0].area() * bytesPerDestPixel);
    vigra_assert(totalGpuMemoryUsed <= GpuMemoryInBytes,
                 "failed to subdivide source and dest images into pieces small enough to fit in gpu memory.");

    if(printDebug)
    {
        cout << "Source chunks:" << endl;
        for (vector<Rect2D>::iterator rI = sourceChunks.begin(); rI != sourceChunks.end(); ++rI) {
            cout << "    " << *rI << endl;
        }
        cout << "Dest chunks:" << endl;
        for (vector<Rect2D>::iterator rI = destChunks.begin(); rI != destChunks.end(); ++rI) {
            cout << "    " << *rI << endl;
        }
        cout << "Total GPU memory used: " << totalGpuMemoryUsed << endl;
    };


    const int TextureFetchesPerInterpolatorPass = 16;
    vector<Rect2D> interpolatorChunks;
    makeChunks(interpolatorSize, interpolatorSize, TextureFetchesPerInterpolatorPass, TextureFetchesPerInterpolatorPass, interpolatorChunks);
    if(printDebug)
    {
        cout << "Interpolator chunks:" << endl;
        for (vector<Rect2D>::iterator rI = interpolatorChunks.begin(); rI != interpolatorChunks.end(); ++rI) {
            cout << "    " << *rI << endl;
        }
    };
    bool allInterpolatorChunksAreEqual = true;
    const Rect2D& firstInterpolatorChunk = interpolatorChunks.front();
    for (vector<Rect2D>::iterator rI = ++(interpolatorChunks.begin()); rI != interpolatorChunks.end(); ++rI) {
        if (rI->width() != firstInterpolatorChunk.width()
            || rI->height() != firstInterpolatorChunk.height()) {
            allInterpolatorChunksAreEqual = false;
            break;
        }
    }


    // Prepare coord transform GLSL program
    std::ostringstream oss;
    oss << std::setprecision(20) << std::showpoint;
    oss << "#version 110" << endl
        << "#extension GL_ARB_texture_rectangle : enable" << endl
        << "uniform sampler2DRect SrcTexture;" << endl
        << "float sinh(in float x) { return (exp(x) - exp(-x)) / 2.0; }" << endl
        << "float cosh(in float x) { return (exp(x) + exp(-x)) / 2.0; }" << endl;
    // TODO: OpenGL Shader vers. 1.5 has built-in sinh and cosh function,
    // need to check is this functions are available and compile only when
    // this functions are not available

    if (needsAtanWorkaround) {
        oss << "float atan2_xge0(const in float y, const in float x) {" << endl
            << "    if (abs(y) > x) {" << endl
            << "        return sign(y) * (" << (M_PI/2.0) << " - atan(x, abs(y)));" << endl
            << "    } else {" << endl
            << "        return atan(y, x);" << endl
            << "    }" << endl
            << "}" << endl
            << "float atan2_safe(const in float y, const in float x) {" << endl
            << "    if (x >= 0.0) return atan2_xge0(y, x);" << endl
            << "    else return (sign(y) * " << M_PI << ") - atan2_xge0(y, -x);" << endl
            << "}" << endl
            << "float atan_safe(const in float yx) {" << endl
            << "    if (abs(yx) > 1.0) {" << endl
            << "        return sign(yx) * (" << (M_PI/2.0) << " - atan(1.0/abs(yx)));" << endl
            << "    } else {" << endl
            << "        return atan(yx);" << endl
            << "    }" << endl
            << "}" << endl;
    } else {
        oss << "float atan2_xge0(const in float y, const in float x) {" << endl
            << "    return atan(y, x);" << endl
            << "}" << endl
            << "float atan2_safe(const in float y, const in float x) {" << endl
            << "    return atan(y, x);" << endl
            << "}" << endl
            << "float atan_safe(const in float yx) {" << endl
            << "    return atan(yx);" << endl
            << "}" << endl;
    }

    oss << "void main(void)" << endl
        << "{" << endl
        << "    float discardA = 1.0;" << endl
        << "    float discardB = 0.0;" << endl
        << coordXformGLSL
        << "    src = src * discardA + vec2(-1000.0, -1000.0) * discardB;" << endl
        << "    gl_FragColor = vec4(src.s, 0.0, 0.0, src.t);" << endl
        << "}" << endl;

    std::string coordXformKernelSourceString = oss.str();
    const char* coordXformKernelSource = coordXformKernelSourceString.c_str();
    if(printDebug)
        cout << coordXformKernelSource;

    GLhandleARB coordXformProgramObject;
    GLhandleARB coordXformShaderObject;
    compileGLSL("coordinate transform",
                coordXformProgramObject,
                coordXformShaderObject,
                &coordXformKernelSource);



    // Prepare alpha composite shader program.
    // This is the same for all images so we just do it once.
    static bool createdAlphaShader = false;
    static GLhandleARB alphaCompositeProgramObject;
    static GLhandleARB alphaCompositeShaderObject;
    static GLint srcAlphaTextureParam;
    if (!createdAlphaShader
        && (srcGLInternalFormat == srcGLTransferFormat)
        && (srcAlphaBuffer != NULL)) {

        compileGLSL("alpha composite",
                    alphaCompositeProgramObject,
                    alphaCompositeShaderObject,
                    &AlphaCompositeKernelSource);
        srcAlphaTextureParam = glGetUniformLocationARB(alphaCompositeProgramObject, "SrcAlphaTexture");
        createdAlphaShader = true;
    }


    // Prepare interpolation shader program.
    oss.str("");
    oss << std::setprecision(20) << std::showpoint;
    oss << "#version 110" << endl
        << "#extension GL_ARB_texture_rectangle : enable" << endl
        << "uniform sampler2DRect CoordTexture;" << endl
        << "uniform sampler2DRect SrcTexture;" << endl
        << "uniform sampler2DRect AccumTexture;" << endl
        << "uniform vec2 SrcUL;" << endl
        << "uniform vec2 SrcLR;" << endl
        << "uniform vec2 KernelUL;" << endl
        << "uniform vec2 KernelWH;" << endl
        << "float w(const in float i, const in float f) {" << endl
        << interpolatorGLSL
        << "}" << endl
        << "void main(void)" << endl
        << "{" << endl
        << "    vec2 src = texture2DRect(CoordTexture, gl_TexCoord[0].st).sq;" << endl
        << "    vec4 accum = texture2DRect(AccumTexture, gl_TexCoord[0].st);" << endl
        << endl;

    // Unexpectedly, this checking slows down the interpolator render passes by 3x-5x.
    //// Add nothing to pixels where the source image has no overlap with the KernelUL/KernelLR range.
    //oss << "    if (any(lessThan(src, SrcUL - KernelLR + " << (interpolatorSize / 2.0) << "))) {" << endl
    //    << "        gl_FragColor = accum;" << endl
    //    << "        return;" << endl
    //    << "    }" << endl
    //    << "    if (any(greaterThanEqual(src, SrcLR - KernelUL + " << ((interpolatorSize / 2.0) - 1.0) << "))) {" << endl
    //    << "        gl_FragColor = accum;" << endl
    //    << "        return;" << endl
    //    << "    }" << endl
    //    << endl;

    oss << "    src -= SrcUL;" << endl
        << "    vec2 t = floor(src) + " << (1.5 - (interpolatorSize / 2)) << ";" << endl
        << "    vec2 f = fract(src);" << endl
        << "    vec2 k = vec2(0.0, 0.0);" << endl
        << endl;

    // Interpolator loop
    if (allInterpolatorChunksAreEqual) {
        oss << "    for (float ky = 0.0; ky < " << static_cast<double>(firstInterpolatorChunk.height()) << "; ky += 1.0) {" << endl;
    } else {
        oss << "    for (float ky = 0.0; ky < KernelWH.t; ky += 1.0) {" << endl;
    }

    oss << "        k.t = ky + KernelUL.t;" << endl
        << "        float wy = w(k.t, f.t);" << endl;

    if (allInterpolatorChunksAreEqual) {
        oss << "        for (float kx = 0.0; kx < " << static_cast<double>(firstInterpolatorChunk.width()) << "; kx += 1.0) {" << endl;
    } else {
        oss << "        for (float kx = 0.0; kx < KernelWH.s; kx += 1.0) {" << endl;
    }

    // FIXME support warparound
    oss << "            k.s = kx + KernelUL.s;" << endl
        << "            float wx = w(k.s, f.s);" << endl
        << "            vec2 ix = t + k;" << endl
        << "            vec4 sp = texture2DRect(SrcTexture, ix);" << endl
        << "            float weight = wx * wy * sp.a;" << endl
        << "            accum += sp * weight;" << endl
        << "        }" << endl
        << "    }" << endl;

    //// Interpolator loop
    //for (int ky = 0; ky < interpolatorSize; ++ky) {
    //    double bounded_ky_offset = 1.5 + ky - (interpolatorSize / 2);
    //    for (int kx = 0; kx < interpolatorSize; ++kx) {
    //        double bounded_kx_offset = 1.5 + kx - (interpolatorSize / 2);
    //        oss << "    {" << endl
    //            << "        // (" << kx << ", " << ky << ")" << endl
    //            << "        vec2 ix = t + vec2(" << bounded_kx_offset << ", " << bounded_ky_offset << ");" << endl
    //            << "        vec4 sp = texture2DRect(SrcTexture, ix);" << endl
    //            //<< "        float weight = w[" << kx << "].s * w[" << ky << "].t * sp.a;" << endl
    //            << "        float weight = w(" << (double)kx << ", f.s) * w(" << (double)ky << ", f.t) * sp.a;" << endl
    //            << "        accum += sp * weight;" << endl
    //            << "    }" << endl;
    //    }
    //}

    oss << endl
        << "    gl_FragColor = accum;" << endl
        << "}" << endl
        << endl;

    std::string interpolatorKernelSourceString = oss.str();
    const char* interpolatorKernelSource = interpolatorKernelSourceString.c_str();
    if(printDebug)
        cout << interpolatorKernelSource;

    GLhandleARB interpolatorProgramObject;
    GLhandleARB interpolatorShaderObject;
    compileGLSL("interpolator",
                interpolatorProgramObject,
                interpolatorShaderObject,
                &interpolatorKernelSource);

    GLint coordTextureParam = glGetUniformLocationARB(interpolatorProgramObject, "CoordTexture");
    GLint accumTextureParam = glGetUniformLocationARB(interpolatorProgramObject, "AccumTexture");
    GLint srcTextureParam = glGetUniformLocationARB(interpolatorProgramObject, "SrcTexture");
    GLint srcULParam = glGetUniformLocationARB(interpolatorProgramObject, "SrcUL");
    GLint srcLRParam = glGetUniformLocationARB(interpolatorProgramObject, "SrcLR");
    GLint kernelULParam = glGetUniformLocationARB(interpolatorProgramObject, "KernelUL");
    GLint kernelWHParam = glGetUniformLocationARB(interpolatorProgramObject, "KernelWH");


    // Prepare normalization/photometric shader program
    oss.str("");
    oss << std::setprecision(20) << std::showpoint;
    oss << "#version 120" << endl
        << "#extension GL_ARB_texture_rectangle : enable" << endl
        << "uniform sampler2DRect NormTexture;" << endl
        << "uniform sampler2DRect CoordTexture;" << endl;

    if (!invLut.empty()) {
        oss << "uniform sampler2DRect InvLutTexture;" << endl;
    }

    if (!destLut.empty()) {
        oss << "uniform sampler2DRect DestLutTexture;" << endl;
    }

    oss << "void main(void)" << endl
        << "{" << endl
        << "    // Normalization" << endl
        << "    vec4 n = texture2DRect(NormTexture, gl_TexCoord[0].st);" << endl
        << "    vec4 p = vec4(0.0, 0.0, 0.0, 0.0);" << endl
        << "    if (n.a >= 0.2) p = n / n.a;" << endl
        << endl
        << "    // Photometric" << endl
        << photometricGLSL
        << endl
        << "    gl_FragColor = p;" << endl
        << "}" << endl
        << endl;

    std::string normalizationPhotometricKernelSourceString = oss.str();
    const char* normalizationPhotometricKernelSource = normalizationPhotometricKernelSourceString.c_str();
    if(printDebug)
        cout << normalizationPhotometricKernelSource;

    GLhandleARB normalizationPhotometricProgramObject;
    GLhandleARB normalizationPhotometricShaderObject;
    compileGLSL("normalization/photometric",
                normalizationPhotometricProgramObject,
                normalizationPhotometricShaderObject,
                &normalizationPhotometricKernelSource);

    GLint normTextureParam = glGetUniformLocationARB(normalizationPhotometricProgramObject, "NormTexture");
    GLint normCoordTextureParam = glGetUniformLocationARB(normalizationPhotometricProgramObject, "CoordTexture");

    float *invLutTextureData = NULL;
    GLuint invLutTexture;
    GLint invLutTextureParam;
    if (!invLut.empty()) {
        invLutTextureData = new float[invLut.size() * 2];
        for (int i = 0; i < invLut.size(); ++i) {
            invLutTextureData[2*i] = invLut[i];
            invLutTextureData[2*i+1] = ((i + 1) < invLut.size()) ? invLut[i+1] : invLut[i];
        }
        invLutTextureParam = glGetUniformLocationARB(normalizationPhotometricProgramObject, "InvLutTexture");
        glGenTextures(1, &invLutTexture);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, invLutTexture);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_LUMINANCE_ALPHA32F_ARB, invLut.size(), 1, 0, GL_LUMINANCE_ALPHA, GL_FLOAT, invLutTextureData);
        CHECK_GL();
    }

    float *destLutTextureData = NULL;
    GLuint destLutTexture;
    GLint destLutTextureParam;
    if (!destLut.empty()) {
        destLutTextureData = new float[destLut.size() * 2];
        for (int i = 0; i < destLut.size(); ++i) {
            destLutTextureData[2*i] = destLut[i];
            destLutTextureData[2*i+1] = ((i + 1) < destLut.size()) ? destLut[i+1] : destLut[i];
        }
        destLutTextureParam = glGetUniformLocationARB(normalizationPhotometricProgramObject, "DestLutTexture");
        glGenTextures(1, &destLutTexture);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, destLutTexture);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_LUMINANCE_ALPHA32F_ARB, destLut.size(), 1, 0, GL_LUMINANCE_ALPHA, GL_FLOAT, destLutTextureData);
        CHECK_GL();
    }

    glFinish();
    if(printDebug)
    {
        t21=getms();
        cout << "gpu shader program compile time = " << ((t21 - t1)/1000.0) << endl;
    };

    // General GL setup
    glPixelStorei(GL_PACK_ALIGNMENT, 8);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 8);

    const float borderColor[] = {0.0, 0.0, 0.0, 0.0};

    glClearColor(0.0, 0.0, 0.0, 0.0);

    GLuint framebuffers[5];
    glGenFramebuffersEXT(5, framebuffers);
    GLuint srcFB       = framebuffers[0];
    GLuint coordFB     = framebuffers[1];
    GLuint accumFB     = framebuffers[2];
    GLuint destFB      = framebuffers[3];
    GLuint destAlphaFB = framebuffers[4];

    const int viewportWidth = std::max<int>(destChunks[0].width(), sourceChunks[0].width());
    const int viewportHeight = std::max<int>(destChunks[0].height(), sourceChunks[0].height());
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, viewportWidth, 0.0, viewportHeight);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glViewport(0, 0, viewportWidth, viewportHeight);

    const int destOdd = (destChunks[0].height() & 1);
    // Setup coordinate texture
    GLuint coordTexture;
    glGenTextures(1, &coordTexture);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, coordTexture);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_LUMINANCE_ALPHA32F_ARB, destChunks[0].width(), destChunks[0].height() + destOdd, 0, GL_LUMINANCE_ALPHA, GL_FLOAT, NULL);
    CHECK_GL();

    // Setup coordinate framebuffer
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, coordFB);
    CHECK_GL();
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, coordTexture, 0);
    CHECK_GL();
    if (!checkFramebufferStatus(__LINE__, ___NCFILE___)) {
        exit(1);
    }

    // Setup accumulator ping-pong textures
    GLuint accumTextures[2];
    glGenTextures(2, accumTextures);

    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, accumTextures[0]);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, destChunks[0].width(), destChunks[0].height() + destOdd, 0, GL_RGBA, GL_FLOAT, NULL);
    CHECK_GL();

    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, accumTextures[1]);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA32F_ARB, destChunks[0].width(), destChunks[0].height() + destOdd, 0, GL_RGBA, GL_FLOAT, NULL);
    CHECK_GL();

    // Attach accumulator ping-pong textures to framebuffer
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, accumFB);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, accumTextures[0], 0);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_RECTANGLE_ARB, accumTextures[1], 0);
    if (!checkFramebufferStatus(__LINE__, ___NCFILE___)) {
        exit(1);
    }

    // Setup src texture
    const int sourceOdd = (sourceChunks[0].height() & 1);
    GLuint srcTexture;
    glGenTextures(1, &srcTexture);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, srcTexture);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, XGLMap[srcGLInternalFormat], sourceChunks[0].width(), sourceChunks[0].height() + sourceOdd, 0, XGLMap[srcGLFormat], XGLMap[srcGLType], NULL);
    CHECK_GL();

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, srcFB);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, srcTexture, 0);
    if (!checkFramebufferStatus(__LINE__, ___NCFILE___)) {
        exit(1);
    }

    // Setup alpha composite framebuffer
    GLuint srcAlphaTexture;
    if (srcAlphaBuffer != NULL) {
        // If there is a separate alpha buffer given, prepare to composite the alpha data into srcTexture.
        glGenTextures(1, &srcAlphaTexture);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, srcAlphaTexture);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_ALPHA, sourceChunks[0].width(), sourceChunks[0].height() + sourceOdd, 0, GL_ALPHA, XGLMap[srcAlphaGLType], NULL);
        CHECK_GL();
    }

    GLuint srcTransferTexture;
    if (srcGLInternalFormat != srcGLTransferFormat) {
        // If the rgb data layout does not match the rgb internal texture format, prepare a separate texture
        // for the rgb data for fast GPU I/O. Then composite into srcTexture.
        glGenTextures(1, &srcTransferTexture);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, srcTransferTexture);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, XGLMap[srcGLTransferFormat], sourceChunks[0].width(), sourceChunks[0].height() + sourceOdd, 0, XGLMap[srcGLFormat], XGLMap[srcGLType], NULL);
        CHECK_GL();
    }

    // Setup dest and destalpha textures and framebuffers
    GLuint destTexture;
    if (XGLMap[destGLTransferFormat] != GL_RGBA32F_ARB) {
        // If the dest image rgb data layout does not match that of the accumulator textures,
        // create a separate texture of the appropriate layout and prepare to copy the output accumulator
        // texture to it. This is for fast GPU I/O.
        glGenTextures(1, &destTexture);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, destTexture);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, XGLMap[destGLTransferFormat], destChunks[0].width(), destChunks[0].height() + destOdd, 0, XGLMap[destGLFormat], XGLMap[destGLType], NULL);

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, destFB);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, destTexture, 0);
        if (!checkFramebufferStatus(__LINE__, ___NCFILE___)) {
            exit(1);
        }
    }

    GLuint destAlphaTexture;
    if (destAlphaBuffer != NULL) {
        // If a separate dest alpha buffer is provided, create a texture for transferring this data out.
        // This is for fast GPU I/O.
        glGenTextures(1, &destAlphaTexture);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, destAlphaTexture);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_ALPHA, destChunks[0].width(), destChunks[0].height() + destOdd, 0, GL_ALPHA, XGLMap[destAlphaGLType], NULL);
        CHECK_GL();

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, destAlphaFB);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, destAlphaTexture, 0);
        if (!checkFramebufferStatus(__LINE__, ___NCFILE___)) {
            exit(1);
        }
    }


    glFinish();
    if(printDebug)
    {
        t2=getms();
        cout << "gpu shader texture/framebuffer setup time = " << ((t2-t21)/1000.0) << endl;
    };

    // Render each dest chunk
    int destChunkNumber = 0;
    for (vector<Rect2D>::iterator dI = destChunks.begin(); dI != destChunks.end(); ++dI, ++destChunkNumber) {

        glFinish();
        if(printDebug)
            t3=getms();

        // Render coord image
        glUseProgramObjectARB(coordXformProgramObject);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, coordFB);
        glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

        glPolygonMode(GL_FRONT, GL_FILL);
        glBegin(GL_QUADS);
            glTexCoord2f(xstart + dI->left(),  ystart + dI->top());     glVertex2f(0.0,         0.0);
            glTexCoord2f(xstart + dI->right(), ystart + dI->top());     glVertex2f(dI->width(), 0.0);
            glTexCoord2f(xstart + dI->right(), ystart + dI->bottom());  glVertex2f(dI->width(), dI->height());
            glTexCoord2f(xstart + dI->left(),  ystart + dI->bottom());  glVertex2f(0.0,         dI->height());
        glEnd();
        CHECK_GL();

        glFinish();
        if(printDebug)
        {
            t4=getms();
            cout << "gpu dest chunk=" << *dI << " coord image render time = " << ((t4-t3)/1000.0) << endl;
        };

        // Multipass rendering of dest image
        int pass = 0;
        for (vector<Rect2D>::iterator sI = sourceChunks.begin(); sI != sourceChunks.end(); ++sI) {

            if (destChunkNumber == 0 || sourceChunks.size() > 1) {
                glFinish();
                if(printDebug)
                    t5=getms();

                glPixelStorei(GL_UNPACK_ROW_LENGTH, srcSize.x);
                glPixelStorei(GL_UNPACK_SKIP_PIXELS, sI->left());
                glPixelStorei(GL_UNPACK_SKIP_ROWS, sI->top());
                CHECK_GL();

                glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, srcFB);
                glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
                glClear(GL_COLOR_BUFFER_BIT);

                if (srcGLInternalFormat == srcGLTransferFormat) {
                    // Upload directly to source texture.
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, srcTexture);
                    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, sI->width(), sI->height(), XGLMap[srcGLFormat], XGLMap[srcGLType], srcBuffer);
                    CHECK_GL();

                    glFinish();
                    if(printDebug)
                    {
                        t6=getms();
                        cout << "gpu dest chunk=" << *dI << " source chunk=" << *sI << " src upload = " << ((t6-t5)/1000.0) << endl;
                    };

                    if (srcAlphaBuffer != NULL) {
                        // Upload to srcAlphaTexture and composite to srcTexture.
                        glUseProgramObjectARB(alphaCompositeProgramObject);

                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, srcAlphaTexture);
                        glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, sI->width(), sI->height(), GL_ALPHA, XGLMap[srcAlphaGLType], srcAlphaBuffer);
                        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
                        glUniform1iARB(srcAlphaTextureParam, 0);
                        CHECK_GL();

                        glFinish();
                        if(printDebug)
                        {
                            t7=getms();
                            cout << "gpu dest chunk=" << *dI << " source chunk=" << *sI << " src alpha upload = " << ((t7-t6)/1000.0) << endl;
                        };

                        glPolygonMode(GL_FRONT, GL_FILL);
                        glBegin(GL_QUADS);
                            glTexCoord2f(0.0,         0.0);           glVertex2f(0.0,         0.0);
                            glTexCoord2f(sI->width(), 0.0);           glVertex2f(sI->width(), 0.0);
                            glTexCoord2f(sI->width(), sI->height());  glVertex2f(sI->width(), sI->height());
                            glTexCoord2f(0.0,         sI->height());  glVertex2f(0.0,         sI->height());
                        glEnd();
                        CHECK_GL();

                        glFinish();
                        if(printDebug)
                        {
                            t8=getms();
                            cout << "gpu dest chunk=" << *dI << " source chunk=" << *sI << " src+alpha render = " << ((t8-t7)/1000.0) << endl;
                        };
                    }
                }
                else {
                    // Composite to srcTexture
                    glUseProgramObjectARB(0);

                    // Upload to srcTransferTexture
                    glActiveTexture(GL_TEXTURE0);
                    glEnable(GL_TEXTURE_RECTANGLE_ARB);
                    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, srcTransferTexture);
                    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, sI->width(), sI->height(), XGLMap[srcGLFormat], XGLMap[srcGLType], srcBuffer);
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
                    CHECK_GL();

                    glFinish();
                    if(printDebug)
                    {
                        t6=getms();
                        cout << "gpu dest chunk=" << *dI << " source chunk=" << *sI << " src upload = " << ((t6-t5)/1000.0) << endl;
                    };

                    if (srcAlphaBuffer != NULL) {
                        // Upload to srcAlphaTexture
                        glActiveTexture(GL_TEXTURE1);
                        glEnable(GL_TEXTURE_RECTANGLE_ARB);
                        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, srcAlphaTexture);
                        glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, sI->width(), sI->height(), GL_ALPHA, XGLMap[srcAlphaGLType], srcAlphaBuffer);
                        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
                        CHECK_GL();

                        glFinish();
                        if(printDebug)
                        {
                            t7=getms();
                            cout << "gpu dest chunk=" << *dI << " source chunk=" << *sI << " src alpha upload = " << ((t7- t6)/1000.0) << endl;
                        };

                        glPolygonMode(GL_FRONT, GL_FILL);
                        glBegin(GL_QUADS);
                            glMultiTexCoord2f(GL_TEXTURE0, 0.0,         0.0);           glMultiTexCoord2f(GL_TEXTURE1, 0.0,         0.0);           glVertex2f(0.0,         0.0);
                            glMultiTexCoord2f(GL_TEXTURE0, sI->width(), 0.0);           glMultiTexCoord2f(GL_TEXTURE1, sI->width(), 0.0);           glVertex2f(sI->width(), 0.0);
                            glMultiTexCoord2f(GL_TEXTURE0, sI->width(), sI->height());  glMultiTexCoord2f(GL_TEXTURE1, sI->width(), sI->height());  glVertex2f(sI->width(), sI->height());
                            glMultiTexCoord2f(GL_TEXTURE0, 0.0,         sI->height());  glMultiTexCoord2f(GL_TEXTURE1, 0.0,         sI->height());  glVertex2f(0.0,         sI->height());
                        glEnd();
                        CHECK_GL();

                        glActiveTexture(GL_TEXTURE0);
                        glDisable(GL_TEXTURE_RECTANGLE_ARB);
                        glActiveTexture(GL_TEXTURE1);
                        glDisable(GL_TEXTURE_RECTANGLE_ARB);
                        CHECK_GL();

                        glFinish();
                        if(printDebug)
                        {
                            t8=getms();
                            cout << "gpu dest chunk=" << *dI << " source chunk=" << *sI << " src+alpha render = " << ((t8-t7)/1000.0) << endl;
                        };
                    }
                    else {
                        glPolygonMode(GL_FRONT, GL_FILL);
                        glBegin(GL_QUADS);
                            // According to the spec, GL_REPLACE uses the incoming fragment color for dest alpha when
                            // texturing a RGB texture into an RGBA texture. Thus the color is explicitly set
                            // with alpha=1.0, rather than relying on the default color.
                            glColor4f(0.0, 0.0, 0.0, 1.0);
                            glTexCoord2f(0.0,         0.0);           glVertex2f(0.0,         0.0);
                            glTexCoord2f(sI->width(), 0.0);           glVertex2f(sI->width(), 0.0);
                            glTexCoord2f(sI->width(), sI->height());  glVertex2f(sI->width(), sI->height());
                            glTexCoord2f(0.0,         sI->height());  glVertex2f(0.0,         sI->height());
                        glEnd();
                        CHECK_GL();

                        glActiveTexture(GL_TEXTURE0);
                        glDisable(GL_TEXTURE_RECTANGLE_ARB);
                        CHECK_GL();

                        glFinish();
                        if(printDebug)
                        {
                            t7=getms();
                            cout << "gpu dest chunk=" << *dI << " source chunk=" << *sI << " src render = " << ((t7-t6)/1000.0) << endl;
                        };
                    }
                }
            }

            glFinish();
            if(printDebug)
                t9=getms();

            // Render dest image
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, accumFB);
            glUseProgramObjectARB(interpolatorProgramObject);

            if (pass == 0) {
                // Clear ping accum texture on first pass.
                glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
                glClear(GL_COLOR_BUFFER_BIT);
            }

            glUniform2fARB(srcULParam, sI->left(), sI->top());
            CHECK_GL();
            glUniform2fARB(srcLRParam, sI->right(), sI->bottom());
            CHECK_GL();

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB, coordTexture);
            glUniform1iARB(coordTextureParam, 0);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            CHECK_GL();

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB, srcTexture);
            glUniform1iARB(srcTextureParam, 1);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            CHECK_GL();

            glActiveTexture(GL_TEXTURE2);
            glUniform1iARB(accumTextureParam, 2);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            CHECK_GL();

            // Multipass interpolation.
            for (vector<Rect2D>::iterator iI = interpolatorChunks.begin(); iI != interpolatorChunks.end(); ++iI, ++pass) {
                glBindTexture(GL_TEXTURE_RECTANGLE_ARB, (pass & 1) ? accumTextures[0] : accumTextures[1]);
                CHECK_GL();

                glUniform2fARB(kernelULParam, iI->left(), iI->top());
                CHECK_GL();
                glUniform2fARB(kernelWHParam, iI->width(), iI->height());
                CHECK_GL();

                glDrawBuffer((pass & 1) ? GL_COLOR_ATTACHMENT1_EXT : GL_COLOR_ATTACHMENT0_EXT);

                glFinish();
                if(printDebug)
                {
                    t10=getms();
                    cout << "gpu dest chunk=" << *dI << " source chunk=" << *sI << " interpolation chunk=" << *iI << " setup = " << ((t10-t9)/1000.0) << endl;
                };

                glPolygonMode(GL_FRONT, GL_FILL);
                glBegin(GL_QUADS);
                    glTexCoord2f(0.0,         0.0);           glVertex2f(0.0,         0.0);
                    glTexCoord2f(dI->width(), 0.0);           glVertex2f(dI->width(), 0.0);
                    glTexCoord2f(dI->width(), dI->height());  glVertex2f(dI->width(), dI->height());
                    glTexCoord2f(0.0,         dI->height());  glVertex2f(0.0,         dI->height());
                glEnd();
                CHECK_GL();

                glFinish();
                if(printDebug)
                {
                    t11=getms();
                    t9=getms();
                    cout << "gpu dest chunk=" << *dI << " source chunk=" << *sI << " interpolation chunk=" << *iI << " render = " << ((t11-t10)/1000.0) << endl;
                };

            } // next interpolation chunk

        } // next source chunk

        // normalization/photometric rendering pass
        glUseProgramObjectARB(normalizationPhotometricProgramObject);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, (pass & 1) ? accumTextures[0] : accumTextures[1]);
        glUniform1iARB(normTextureParam, 0);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        CHECK_GL();

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, coordTexture);
        glUniform1iARB(normCoordTextureParam, 1);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        CHECK_GL();

        if (!invLut.empty()) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB, invLutTexture);
            glUniform1iARB(invLutTextureParam, 2);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            CHECK_GL();
        }

        if (!destLut.empty()) {
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB, destLutTexture);
            glUniform1iARB(destLutTextureParam, 3);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            CHECK_GL();
        }

        glDrawBuffer((pass & 1) ? GL_COLOR_ATTACHMENT1_EXT : GL_COLOR_ATTACHMENT0_EXT);
        
        glFinish();
        if(printDebug)
        {
            t12=getms();
            cout << "gpu dest chunk=" << *dI << " normalization setup = " << ((t12-t11)/1000.0) << endl;
        };

        glPolygonMode(GL_FRONT, GL_FILL);
        glBegin(GL_QUADS);
            glTexCoord2f(0.0,         0.0);           glVertex2f(0.0,         0.0);
            glTexCoord2f(dI->width(), 0.0);           glVertex2f(dI->width(), 0.0);
            glTexCoord2f(dI->width(), dI->height());  glVertex2f(dI->width(), dI->height());
            glTexCoord2f(0.0,         dI->height());  glVertex2f(0.0,         dI->height());
        glEnd();
        CHECK_GL();

        glFinish();
        if(printDebug)
        {
            t13=getms();
            cout << "gpu dest chunk=" << *dI << " normalization render = " << ((t13-t12)/1000.0) << endl;
        };

        pass++;


        // Readback dest chunk
        glPixelStorei(GL_PACK_ROW_LENGTH, destSize.x);
        glPixelStorei(GL_PACK_SKIP_PIXELS, dI->left());
        glPixelStorei(GL_PACK_SKIP_ROWS, dI->top());

        if (XGLMap[destGLTransferFormat] == GL_RGBA32F_ARB) {
            // Transfer to destBuffer directly from last-written accum texture.
            glReadBuffer((pass & 1) ? GL_COLOR_ATTACHMENT0_EXT : GL_COLOR_ATTACHMENT1_EXT);

            glReadPixels(0, 0, dI->width(), dI->height(), XGLMap[destGLFormat], XGLMap[destGLType], destBuffer);
            CHECK_GL();

            glFinish();
            if(printDebug)
            {
                t14=getms();
                cout << "gpu dest chunk=" << *dI << " rgb readback = " << ((t14-t13)/1000.0) << endl;
            };
        }
        else {
            // Move output accumTexture to dest texture then readback.
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, destFB);
            glUseProgramObjectARB(0);

            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_RECTANGLE_ARB);
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB, (pass & 1) ? accumTextures[0] : accumTextures[1]);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            CHECK_GL();

            glFinish();
            if(printDebug)
            {
                t14=getms();
                cout << "gpu dest chunk=" << *dI << " dest rgb disassembly setup = " << ((t14-t13)/1000.0) << endl;
            };

            glPolygonMode(GL_FRONT, GL_FILL);
            glBegin(GL_QUADS);
                glTexCoord2f(0.0,         0.0);           glVertex2f(0.0,         0.0);
                glTexCoord2f(dI->width(), 0.0);           glVertex2f(dI->width(), 0.0);
                glTexCoord2f(dI->width(), dI->height());  glVertex2f(dI->width(), dI->height());
                glTexCoord2f(0.0,         dI->height());  glVertex2f(0.0,         dI->height());
            glEnd();
            CHECK_GL();

            glActiveTexture(GL_TEXTURE0);
            glDisable(GL_TEXTURE_RECTANGLE_ARB);
            CHECK_GL();

            glFinish();
            if(printDebug)
            {
                t15=getms();
                cout << "gpu dest chunk=" << *dI << " dest rgb disassembly render = " << ((t15-t14)/1000.0) << endl;
            };

            glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
            CHECK_GL();

            glReadPixels(0, 0, dI->width(), dI->height(), XGLMap[destGLFormat], XGLMap[destGLType], destBuffer);
            CHECK_GL();

            glFinish();
            if(printDebug)
            {
                t16=getms();
                cout << "gpu dest chunk=" << *dI << " rgb readback = " << ((t16-t15)/1000.0) << endl;
            };
        }

        if (destAlphaBuffer != NULL) {
            // Move output accumTexture to dest alpha texture
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, destAlphaFB);
            glUseProgramObjectARB(0);

            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_RECTANGLE_ARB);
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB, (pass & 1) ? accumTextures[0] : accumTextures[1]);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            CHECK_GL();

            glFinish();
            if(printDebug)
            {
                t17=getms();
                cout << "gpu dest chunk=" << *dI << " dest alpha disassembly setup = " << ((t17-t16)/1000.0) << endl;
            };

            glPolygonMode(GL_FRONT, GL_FILL);
            glBegin(GL_QUADS);
                glTexCoord2f(0.0,         0.0);           glVertex2f(0.0,         0.0);
                glTexCoord2f(dI->width(), 0.0);           glVertex2f(dI->width(), 0.0);
                glTexCoord2f(dI->width(), dI->height());  glVertex2f(dI->width(), dI->height());
                glTexCoord2f(0.0,         dI->height());  glVertex2f(0.0,         dI->height());
            glEnd();
            CHECK_GL();

            glActiveTexture(GL_TEXTURE0);
            glDisable(GL_TEXTURE_RECTANGLE_ARB);
            CHECK_GL();

            glFinish();
            if(printDebug)
            {
                t18=getms();
                cout << "gpu dest chunk=" << *dI << " dest alpha disassembly render = " << ((t18-t17)/1000.0) << endl;
            };

            // Readback dest alpha chunk
            glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
            CHECK_GL();

            glReadPixels(0, 0, dI->width(), dI->height(), GL_ALPHA, XGLMap[destAlphaGLType], destAlphaBuffer);
            CHECK_GL();

            glFinish();
            if(printDebug)
            {
                t19=getms();
                cout << "gpu dest chunk=" << *dI << " alpha readback = " << ((t19-t18)/1000.0) << endl;
            };
        }

    } // next dest chunk

    glFinish();
    if(printDebug)
        t19=getms();

    glDeleteTextures(2, accumTextures);
    glDeleteTextures(1, &coordTexture);
    glDeleteTextures(1, &srcTexture);
    glDeleteTextures(1, &srcAlphaTexture);
    glDeleteTextures(1, &srcTransferTexture);
    glDeleteTextures(1, &destTexture);
    glDeleteTextures(1, &destAlphaTexture);

    if (!invLut.empty()) {
        glDeleteTextures(1, &invLutTexture);
        delete [] invLutTextureData;
    }

    if (!destLut.empty()) {
        glDeleteTextures(1, &destLutTexture);
        delete [] destLutTextureData;
    }

    glDeleteFramebuffersEXT(5, framebuffers);

    glUseProgramObjectARB(0);
    glDeleteObjectARB(coordXformShaderObject);
    glDeleteObjectARB(coordXformProgramObject);
    glDeleteObjectARB(interpolatorShaderObject);
    glDeleteObjectARB(interpolatorProgramObject);
    glDeleteObjectARB(normalizationPhotometricShaderObject);
    glDeleteObjectARB(normalizationPhotometricProgramObject);

    glFinish();
    if(printDebug)
    {
        t20=getms();
        cout << "gpu destruct time = " << ((t20-t19)/1000.0) << endl;
        cout << "gpu total time = " << ((t20-t1)/1000.0) << endl;
    };

    return true;
}

}; // namespace
