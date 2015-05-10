// -*- c-basic-offset: 4 -*-

/** @file PanoToolsTransformGPU.cpp
 *
 *  @brief GPU shader program conversion for PTools::Transform
 *
 *  @author Andrew Mihal
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

#include <hugin_config.h>

#include <stdlib.h>

#include "PanoToolsInterface.h"

#include <iostream>
#include <iomanip>

using std::ostringstream;
using std::endl;

#define 	distanceparam	(*((double*)params))
#define 	shift		(*((double*)params))
#define		var0		((double*)params)[0]
#define		var1		((double*)params)[1]
#define		var2		((double*)params)[2]
#define		var3		((double*)params)[3]
#define		var4		((double*)params)[4]
#define		var5		((double*)params)[5]
#define		mp		((struct MakeParams*)params)

// Instead of discard, need to set coords to something far outside the src image and return.
// e.g. (-1000, -1000).
#define DISCARD "{ discardA = 0.0; discardB = 1.0; }"

static void rotate_erect_glsl(ostringstream& oss, const void* params) {
    //oss << "    // rotate_erect(" << var0 << ", " << var1 << ")" << endl
    //    << "    " << ((var1 == 0.0) ? "//" : "") << "src.s += " << var1 << ";" << endl
    //    << "    while (src.s < " << -var0 << ") src.s += " << (2.0 * var0) << ";" << endl
    //    << "    while (src.s > " <<  var0 << ") src.s -= " << (2.0 * var0) << ";" << endl
    //    << endl;

    // Version without loops
    oss << "    // rotate_erect(" << var0 << ", " << var1 << ")" << endl
        << "    {" << endl
        << "        " << ((var1 == 0.0) ? "//" : "") << "src.s += " << var1 << ";" << endl
        << "        float w = (abs(src.s) > " << var0 << ") ? 1.0 : 0.0;" << endl
        << "        float n = (src.s < 0.0) ? 0.5 : -0.5;" << endl
        << "        src.s += w * " << (-2.0 * var0) << " * ceil(src.s / " << (2.0 * var0) << " + n);" << endl
        << "    }" << endl
        << endl;
}

static void resize_glsl(ostringstream& oss, const void* params) {
    oss << "    // resize(" << var0 << ", " << var1 << ")" << endl
        << "    src *= vec2(" << var0 << ", " << var1 << ");" << endl
        << endl;
}

static void vert_glsl(ostringstream& oss, const void* params) {
    oss << "    // vert(" << shift << ")" << endl
        << "    src.t += " << shift << ";" << endl
        << endl;
}

static void horiz_glsl(ostringstream& oss, const void* params) {
    oss << "    // horiz(" << shift << ")" << endl
        << "    src.s += " << shift << ";" << endl
        << endl;
}

static void shear_glsl(ostringstream& oss, const void* params) {
    oss << "    // shear(" << var0 << ", " << var1 << ")" << endl
        << "    src += (src.ts * vec2(" << var0 << ", " << var1 << "));" << endl
        << endl;
}

static void erect_pano_glsl(ostringstream& oss, const void* params) {
    oss << "    // erect_pano(" << distanceparam << ")" << endl
        << "    src.t = " << distanceparam << " * atan_safe(src.t / " << distanceparam << ");" << endl
        << endl;
}

static void erect_rect_glsl(ostringstream& oss, const void* params) {
    oss << "    // erect_rect(" << distanceparam << ")" << endl
        << "    src.t = " << distanceparam << " * atan2_xge0(src.t, length(vec2(" << distanceparam << ", src.s)));" << endl
        << "    src.s = " << distanceparam << " * atan2_safe(src.s, " << distanceparam << ");" << endl
        << endl;
}

static void erect_sphere_tp_glsl(ostringstream& oss, const void* params) {
    oss << "    // erect_sphere_tp(" << distanceparam << ")" << endl
        << "    {" << endl
        << "        float r = length(src);" << endl
        << "        float theta = r / " << distanceparam << ";" << endl
        << "        float s = " << (1.0 / distanceparam) << ";" << endl
        << "        if (theta != 0.0) { s = sin(theta) / r; }" << endl
        << "        float v1 = s * src.s;" << endl
        << "        float v0 = cos(theta);" << endl
        << "        src.s = " << distanceparam << " * atan2_safe(v1, v0);" << endl
        << "        src.t = " << distanceparam << " * atan_safe(s * src.t / length(vec2(v0, v1)));" << endl
        << "    }" << endl
        << endl;
}

static void sphere_tp_erect_glsl(ostringstream& oss, const void* params) {
    oss << "    // sphere_tp_erect(" << distanceparam << ")" << endl
        << "    {" << endl
        << "        float phi = src.s / " << distanceparam << ";" << endl
        << "        float theta = -src.t / " << distanceparam << " + " << (M_PI/2) << ";" << endl
        << "        if (theta < 0.0) {" << endl
        << "            theta = -theta;" << endl
        << "            phi += " << M_PI << ";" << endl
        << "        }" << endl
        << "        if (theta > " << M_PI << ") {" << endl
        << "            theta = " << M_PI << " - (theta - " << M_PI << ");" << endl
        << "            phi += " << M_PI << ";" << endl
        << "        }" << endl
        << "        float s = sin(theta);" << endl
        << "        vec2 v = vec2(s * sin(phi), cos(theta));" << endl
        << "        float r = length(v);" << endl
        << "        theta = " << distanceparam << " * atan2_safe(r, s * cos(phi));" << endl
        << "        src = v * (theta / r);" << endl
        << "    }" << endl
        << endl;
}

static void vertical_glsl(ostringstream& oss, const void* params) {
    oss << "    // vertical(" << var0 << ", " << var1 << ", " << var2 << ", " << var3 << ", " << var4 << ")" << endl
        << "    {" << endl
        << "        float r = abs(src.t / " << var4 << ");" << endl
        << "        float scale = ((" << var3 << " * r + " << var2 << ") * r + " << var1 << ") * r + " << var0 << ";" << endl
        << "        src.t *= scale;" << endl
        << "    }" << endl
        << endl;
}

static void deregister_glsl(ostringstream& oss, const void* params) {
    oss << "    // deregister(" << var1 << ", " << var2 << ", " << var3 << ", " << var4 << ")" << endl
        << "    {" << endl
        << "        float r = abs(src.t / " << var4 << ");" << endl
        << "        float scale = (" << var3 << " * r + " << var2 << ") * r + " << var1 << ";" << endl
        << "        src.s += abs(src.t) * scale;" << endl
        << "    }" << endl
        << endl;
}

static void radial_glsl(ostringstream& oss, const void* params) {
    oss << "    // radial(" << var0 << ", " << var1 << ", " << var2 << ", " << var3 << ", " << var4 << ", " << var5 << ")" << endl
        << "    {" << endl
        << "        float r = length(src) / " << var4 << ";" << endl
        << "        float scale = 1000.0; " << endl
        << "        if (r < " << var5 << ") {" << endl
        << "            scale = ((" << var3 << " * r + " << var2 << ") * r + " << var1 << ") * r + " << var0 << ";" << endl
        << "        }" << endl
        << "        src *= scale;" << endl
        << "    }" << endl
        << endl;
}

static void pano_sphere_tp_glsl(ostringstream& oss, const void* params) {
    oss << "    // pano_sphere_tp(" << distanceparam << ")" << endl
        << "    {" << endl
        << "        float r = length(src);" << endl
        << "        float theta = r / " << distanceparam << ";" << endl
        << "        float s = " << (1.0 / distanceparam) << ";" << endl
        << "        if (theta != 0.0) s = sin(theta) / r;" << endl
        << "        vec2 v = vec2(cos(theta), s * src.s);" << endl
        << "        src.s = " << distanceparam << " * atan2_safe(v.t, v.s);" << endl
        << "        src.t = " << distanceparam << " * s * src.t / length(v);" << endl
        << "    }" << endl
        << endl;
}

static void rect_sphere_tp_glsl(ostringstream& oss, const void* params) {
    oss << "    // rect_sphere_tp(" << distanceparam << ")" << endl
        << "    {" << endl
        << "        float r = length(src);" << endl
        << "        float theta = r / " << distanceparam << ";" << endl
        << "        float rho = 0.0;" << endl
        << "        if (theta >= " << (M_PI / 2.0) << ") rho = 1.6e16;" << endl
        << "        else if (theta == 0.0) rho = 1.0;" << endl
        << "        else rho = tan(theta) / theta;" << endl
        << "        src *= rho;" << endl
        << "    }" << endl
        << endl;
}

static void persp_sphere_glsl(ostringstream& oss, const void* params) {
    double d = *((double*) ((void**)params)[1]);
    double (*m)[3] = (double(*)[3]) ((void**)params)[0];
    oss << "    // persp_sphere(" << d << ")" << endl
        << "    {" << endl
        << "        mat3 m = mat3(" << m[0][0] << ", " << m[1][0] << ", " << m[2][0] << "," << endl
        << "                      " << m[0][1] << ", " << m[1][1] << ", " << m[2][1] << "," << endl
        << "                      " << m[0][2] << ", " << m[1][2] << ", " << m[2][2] << ");" << endl
        << "        float r = length(src);" << endl
        << "        float theta = r / " << d << ";" << endl
        << "        float s = 0.0;" << endl
        << "        if (r != 0.0) s = sin(theta) / r;" << endl
        << "        vec3 v = vec3(s * src.s, s * src.t, cos(theta));" << endl
        << "        vec3 u = v * m;" << endl
        << "        r = length(u.st);" << endl
        << "        theta = 0.0;" << endl
        << "        if (r != 0.0) theta = " << d << " * atan2_safe(r, u.p) / r;" << endl
        << "        src = theta * u.st;" << endl
        << "    }" << endl
        << endl;
}

static void erect_mercator_glsl(ostringstream& oss, const void* params) {
    oss << "    // erect_mercator(" << distanceparam << ")" << endl
        << "    src.t = " << distanceparam << " * atan_safe(sinh(src.t/" << distanceparam << "));" << endl
        << endl;
}

static void erect_millercylindrical_glsl(ostringstream& oss, const void* params) {
    oss << "    // erect_millercylindrical(" << distanceparam << ")" << endl
        << "    src.t = " << (1.25 * distanceparam) << " * atan_safe(sinh(src.t * " << (4 / (5.0 * distanceparam)) << "));" << endl
        << endl;
}

static void erect_lambert_glsl(ostringstream& oss, const void* params) {
    oss << "    // erect_lambert(" << distanceparam << ")" << endl
        << "    src.t = " << distanceparam << " * asin(src.t / " << distanceparam << ");" << endl
        << endl;
}

static void erect_transmercator_glsl(ostringstream& oss, const void* params) {
    oss << "    // erect_transmercator(" << distanceparam << ")" << endl
        << "    {" << endl
        << "        src /= " << distanceparam << ";" << endl
        << "        if (abs(src.t) > " << M_PI << ") " << DISCARD << endl
        << "        float x = src.s;" << endl
        << "        src.s = " << distanceparam << " * atan2_safe(sinh(src.s), cos(src.t));" << endl
        << "        src.t = " << distanceparam << " * asin(sin(src.t) / cosh(x));" << endl
        << "    }" << endl
        << endl;
}

static void erect_sinusoidal_glsl(ostringstream& oss, const void* params) {
    oss << "    // erect_sinusoidal(" << distanceparam << ")" << endl
        << "    src.s /= cos(src.t / " << distanceparam << ");" << endl
        << "    if (abs(src.s) > " << (M_PI * distanceparam) << ") " << DISCARD << endl
        << endl;
}

static void erect_lambertazimuthal_glsl(ostringstream& oss, const void* params) {
    oss << "    // erect_lambertazimuthal(" << distanceparam << ")" << endl
        << "    {" << endl
        << "        src /= " << distanceparam << ";" << endl
        << "        if (any(greaterThan(abs(src), vec2(" << M_PI << ", " << M_PI << ")))) " << DISCARD << endl
        << "        float ro = length(src);" << endl
        << "        if (abs(ro) <= 1.0e-10) src = vec2(0.0, 0.0);" << endl
        << "        else {" << endl
        << "            float c = 2.0 * asin(ro / 2.0);" << endl
        << "            src.t = " << distanceparam << " * asin((src.t * sin(c)) / ro);" << endl
        << "            if (abs(ro * cos(c)) <= 1.0e-10) src.s = 0.0;" << endl
        << "            else src.s = " << distanceparam << " * atan2_safe(src.s * sin(c), (ro * cos(c)));" << endl
        << "        }" << endl
        << "    }" << endl
        << endl;
}

static void erect_hammer_glsl(ostringstream& oss, const void* params) {
    oss << "    // erect_hammer(" << distanceparam << ")" << endl
        << "    {" << endl
        << "        src /= " << distanceparam << ";" << endl
        << "        float z2 = 1.0 - src.s * src.s / 16.0 - src.t * src.t / 4.0;" << endl
        << "        if (z2 < 0.0 ) " << DISCARD << endl
        << "        float z = sqrt(z2);" << endl
        << "        src.s = 2.0 * atan2_safe( z * src.s, 2.0*(2.0*z2-1.0));" << endl
        << "        src.t = asin (src.t * z);" << endl
        << "        if(any(greaterThan(abs(src), vec2(" << M_PI << "," << HALF_PI << "))))" << DISCARD << endl
        << "        src *= " << distanceparam << ";" << endl
        << "    }" << endl
        << endl;
}

static void erect_arch_glsl(ostringstream& oss, const void* params) {
    oss << "    // erect_arch(" << distanceparam << ")" << endl
        << "    {" << endl
        << "        if(src.t < 0.0) {" << endl
        << "            src.t = " << (1.25 * distanceparam) << " * atan_safe(sinh(src.t * " << (4 / (5.0 * distanceparam)) << "));" << endl
        << "        } else {" << endl
        << "            src.t = " << distanceparam << " * asin(src.t / " << distanceparam << ");" << endl
        << "        }" << endl
        << "    }" << endl
        << endl;
}

static void erect_stereographic_glsl(ostringstream& oss, const void* params) {
    oss << "    // erect_stereographic(" << distanceparam << ")" << endl
        << "    {" << endl
        << "        src /= " << distanceparam << ";" << endl
        << "        float rh = length(src);" << endl
        << "        float c = 2.0 * atan_safe(rh / 2.0);" << endl
        << "        float sin_c = sin(c);" << endl
        << "        float cos_c = cos(c);" << endl
        << "        if (abs(rh) <= 1.0e-10) " << DISCARD << endl
        << "        src.t = asin((src.t * sin_c) / rh) * " << distanceparam << ";" << endl
        << "        if (abs(cos_c) < 1.0e-10 && abs(src.s) < 1.0e-10) " << DISCARD << endl
        << "        float y = src.s * sin_c;" << endl
        << "        float x = cos_c * rh;" << endl
        << "        src.s = atan2_safe(y, x) * " << distanceparam << ";" << endl
        << "    }" << endl
        << endl;
}

static void stereographic_erect_glsl(ostringstream& oss, const void* params) {
    oss << "    // stereographic_erect(" << distanceparam << ")" << endl
        << "    {" << endl
        << "        src /= " << distanceparam << ";" << endl
        << "        vec2 cos_lon_lat=cos(src);" << endl
        << "        float g=cos_lon_lat.s * cos_lon_lat.t;" << endl
        << "        src = " << distanceparam << " * 2.0 / (1.0 + g) * vec2(cos_lon_lat.t * sin(src.s), sin(src.t));" << endl
        << "    }" << endl
        << endl;
}

static void erect_albersequalareaconic_glsl(ostringstream& oss, const void* params) {
    oss << "    // erect_albersequalareaconic(...)" << endl
        << "    {" << endl;

    // Get the albersEqualAreaConic_ParamCheck to run.
    double junk0, junk1;
    int result = erect_albersequalareaconic(0.0, 0.0, &junk0, &junk1, const_cast<void*>(params));
    if (result == 0) {
        oss << "        // albersEqualAreaConic_ParamCheck failed" << endl;
    }

    const double n = mp->pn->precomputedValue[3];
    const double C = mp->pn->precomputedValue[4];
    const double rho0 = mp->pn->precomputedValue[5];
    const double yoffset = mp->pn->precomputedValue[6];
    const double n2 = mp->pn->precomputedValue[7];
    const double twiceN = mp->pn->precomputedValue[9];

    oss << "        src /= " << mp->distance << ";" << endl
        << "        src.t += " << yoffset << ";" << endl
        << "        float rho2 = (src.s * src.s + (" << rho0 << " - src.t) * (" << rho0 << " - src.t));" << endl
        << "        float theta = atan2_safe(" << ((n < 0) ? "-" : "") << "src.s, " << ((n < 0) ? "-1.0 * " : "") << "(" << rho0 << " - src.t));" << endl
        << "        float phi = asin((" << C << " - rho2 * " << n2 << ") / " << twiceN << ");" << endl
        << "        float lambda = theta / " << n << ";" << endl
        << "        if (abs(lambda) > " << M_PI << ") " << DISCARD << endl
        << "        src.s = " << mp->distance << " * lambda;" << endl
        << "        src.t = " << mp->distance << " * phi;" << endl
        << "    }" << endl
        << endl;

}

static void lambertazimuthal_erect_glsl(ostringstream& oss, const void* params) {
    oss << "    // lambertazimuthal_erect(" << distanceparam << ")" << endl
        << "    {" << endl
        << "        src /= " << distanceparam << ";" << endl
        << "        float a=cos(src.t) * cos(src.s) + 1.0;" << endl
        << "        if (abs(a) <= 1e-10) " << DISCARD << endl
        << "        src = " << distanceparam << " * sqrt (2.0/a) * vec2 ( cos(src.t) * sin(src.s), sin(src.t));" << endl
        << "    }" << endl
        << endl;
}

static void sphere_tp_equisolid_glsl(ostringstream& oss, const void* params) {
    oss << "    // sphere_tp_equisolid(" << distanceparam << ")" << endl
        << "    {" << endl
        << "        float phi = atan2_safe(src.t, src.s);" << endl
        << "        src = " << distanceparam << " * 2.0 * asin( length(src) / (2.0 * " << distanceparam << ")) * vec2 (cos(phi), sin(phi));" << endl
        << "    }" << endl
        << endl;
}

static void sphere_tp_orthographic_glsl(ostringstream& oss, const void* params) {
    oss << "    // sphere_tp_orthographic(" << distanceparam << ")" << endl
        << "    {" << endl
        << "        float rho=length(src);" << endl
        << "        if (rho >" << distanceparam << ") " << DISCARD << endl
        << "        float phi = atan2_safe(src.t, src.s);" << endl
        << "        src = " << distanceparam << " * asin( rho / " << distanceparam << ") * vec2 (cos(phi), sin(phi));" << endl
        << "    }" << endl
        << endl;
}

static void orthographic_sphere_tp_glsl(ostringstream& oss, const void* params) {
    oss << "    // orthographic_sphere_tp(" << distanceparam << ")" << endl
        << "    {" << endl
        << "        float theta = length(src) / " << distanceparam << ";" << endl
        << "        float phi = atan2_safe(src.t, src.s);" << endl
        << "        if ( abs(theta) > " << HALF_PI << ") " << DISCARD << endl
        << "        " << endl
        << "        src = " << distanceparam << " * sin( theta ) * vec2 (cos(phi), sin(phi));" << endl
        << "    }" << endl
        << endl;
}

static void sphere_tp_thoby_glsl(ostringstream& oss, const void* params) {
    oss << "    // sphere_tp_thoby(" << distanceparam << ")" << endl
        << "    {" << endl
        << "        float rho = length(src) / " << distanceparam << ";" << endl
        << "        if (abs(rho) > " << THOBY_K1_PARM << ") " << DISCARD << endl
        << "        float phi = atan2_safe(src.t, src.s);" << endl
        << "        src = " << distanceparam << " * asin(rho/" << THOBY_K1_PARM << ") / " << THOBY_K2_PARM << " * vec2 (cos(phi), sin(phi));" << endl
        << "    }" << endl
        << endl;
}

static void thoby_sphere_tp_glsl(ostringstream& oss, const void* params) {
    oss << "    // thoby_sphere_tp(" << distanceparam << ")" << endl
        << "    {" << endl
        << "        float theta = length(src) / " << distanceparam << ";" << endl
        << "        float phi = atan2_safe(src.t, src.s);" << endl
        << "        src = " << distanceparam << " * " << THOBY_K1_PARM << " * sin(theta * " << THOBY_K2_PARM << ") * vec2 (cos(phi), sin(phi));" << endl
        << "    }" << endl
        << endl;
}

static void plane_transfer_to_camera_glsl(ostringstream& oss, const void* params) {
    oss << "    // plane_transfer_to_camera" << endl
        << "    //     distance   : " << mp->distance << endl
        << "    //     x          : " << mp->trans[0] << endl
        << "    //     y          : " << mp->trans[1] << endl
        << "    //     z          : " << mp->trans[2] << endl
        << "    //     plane yaw  : " << mp->trans[3] << endl
        << "    //     plane pitch: " << mp->trans[4] << endl
        << "    {" << endl
        << "        float phi = src.s / " << mp->distance << ";" << endl
        << "        float theta = " << HALF_PI << " - src.t / " << mp->distance << ";" << endl
        << "        vec3 p = vec3(sin(theta)*sin(phi), cos(theta), sin(theta)*-cos(phi));" << endl
        << "        vec3 plane_coeff=vec3(" << sin(HALF_PI+mp->trans[4])*sin(mp->trans[3]) << ", " << cos(HALF_PI+mp->trans[4]) << ", " << sin(HALF_PI+mp->trans[4])*-cos(mp->trans[3]) << ");" << endl
        << "        float den = -dot(plane_coeff, p);" << endl
        << "        if ( abs(den) < 1E-15 ) " << DISCARD << endl
        << "        float u = length(plane_coeff);" << endl
        << "        u = -u * u / den;" << endl
        << "        if ( u < 0.0 ) " << DISCARD << endl
        << "        p *= u;" << endl
        << "        p -= vec3(" << mp->trans[0] << "," << mp->trans[1] << "," << mp->trans[2] << ");" << endl
        << "        src = " << mp->distance << " * vec2( atan2_safe(p.s, -p.p), asin(p.t/length(p)));" << endl
        << "    }" << endl
        << endl;
}

namespace HuginBase { namespace PTools {

bool Transform::emitGLSL(ostringstream& oss) const {

    oss << "    vec2 src = gl_TexCoord[0].st;" << endl
        << "    src -= vec2(" << m_srcTX << ", " << m_srcTY << ");" << endl
        << endl;

    bool foundUnsupportedFunction = false;
    int i = 0;
    const fDesc* stack = m_stack;

    while ( (stack->func) != NULL ) {
        if      (stack->func == rotate_erect)               rotate_erect_glsl(oss, stack->param);
        else if (stack->func == resize)                     resize_glsl(oss, stack->param);
        else if (stack->func == vert)                       vert_glsl(oss, stack->param);
        else if (stack->func == horiz)                      horiz_glsl(oss, stack->param);
        else if (stack->func == shear)                      shear_glsl(oss, stack->param);
        else if (stack->func == erect_pano)                 erect_pano_glsl(oss, stack->param);
        else if (stack->func == erect_rect)                 erect_rect_glsl(oss, stack->param);
        else if (stack->func == erect_sphere_tp)            erect_sphere_tp_glsl(oss, stack->param);
        else if (stack->func == sphere_tp_erect)            sphere_tp_erect_glsl(oss, stack->param);
        else if (stack->func == vertical)                   vertical_glsl(oss, stack->param);
        else if (stack->func == deregister)                 deregister_glsl(oss, stack->param);
        else if (stack->func == radial)                     radial_glsl(oss, stack->param);
        else if (stack->func == pano_sphere_tp)             pano_sphere_tp_glsl(oss, stack->param);
        else if (stack->func == rect_sphere_tp)             rect_sphere_tp_glsl(oss, stack->param);
        else if (stack->func == persp_sphere)               persp_sphere_glsl(oss, stack->param);
        else if (stack->func == erect_mercator)             erect_mercator_glsl(oss, stack->param);
        else if (stack->func == erect_millercylindrical)    erect_millercylindrical_glsl(oss, stack->param);
        else if (stack->func == erect_lambert)              erect_lambert_glsl(oss, stack->param);
        else if (stack->func == erect_transmercator)        erect_transmercator_glsl(oss, stack->param);
        else if (stack->func == erect_sinusoidal)           erect_sinusoidal_glsl(oss, stack->param);
        else if (stack->func == erect_lambertazimuthal)     erect_lambertazimuthal_glsl(oss, stack->param);
        else if (stack->func == erect_stereographic)        erect_stereographic_glsl(oss, stack->param);
        else if (stack->func == erect_albersequalareaconic) erect_albersequalareaconic_glsl(oss, stack->param);
        else if (stack->func == erect_hammer)               erect_hammer_glsl(oss, stack->param);
        else if (stack->func == erect_arch)                 erect_arch_glsl(oss, stack->param);
        else if (stack->func == stereographic_erect)        stereographic_erect_glsl(oss, stack->param);
        else if (stack->func == lambertazimuthal_erect)     lambertazimuthal_erect_glsl(oss, stack->param);
        else if (stack->func == sphere_tp_equisolid)        sphere_tp_equisolid_glsl(oss, stack->param);
        else if (stack->func == sphere_tp_orthographic)     sphere_tp_orthographic_glsl(oss, stack->param);
        else if (stack->func == orthographic_sphere_tp)     orthographic_sphere_tp_glsl(oss, stack->param);
        else if (stack->func == sphere_tp_thoby)            sphere_tp_thoby_glsl(oss, stack->param);
        else if (stack->func == thoby_sphere_tp)            thoby_sphere_tp_glsl(oss, stack->param);
        else if (stack->func == plane_transfer_to_camera)   plane_transfer_to_camera_glsl(oss, stack->param);
        else {
            oss << "    // Unknown function " << (const void*)stack->func << endl << endl;
            foundUnsupportedFunction = true;
        }
        ++stack;
        ++i;
    }

    oss << "    src += vec2(" << (m_destTX-0.5) << ", " << (m_destTY-0.5) << ");" << endl
        << endl;

    return !foundUnsupportedFunction;
}

bool Transform::transformImgCoordPartial(double & x_dest, double & y_dest, double x_src, double y_src) const
{
    x_src -= m_srcTX - 0.5;
    y_src -= m_srcTY - 0.5;

    double xd = x_src;
    double yd = y_src;

    const fDesc* stack = m_stack;

    for (int i = 0; i < 2; ++i) {
        if ((stack->func) == NULL) break;
        if ( (stack->func)(xd, yd, &x_dest, &y_dest, stack->param) ) {
            xd = x_dest;
            yd = y_dest;
            stack++;
        } else {
            return 0;
        }
    }
 
    x_dest += m_destTX - 0.5;
    y_dest += m_destTY - 0.5;

    return 1;
}

    
}} // namespace
