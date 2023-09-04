#include <algorithm>
#include <cmath>
#include <limits>
#include <cstdlib>
#include "our_gl.h"

mat<4, 4> _model = mat<4, 4>::identity();
mat<4, 4> _view = mat<4, 4>::identity();
mat<4, 4> _projection = mat<4, 4>::identity();
mat<4, 4> _viewport = mat<4, 4>::identity();

IShader::~IShader() {}

void viewportMat(int x, int y, int w, int h, int depth) 
{
    _viewport = mat<4, 4>::identity();
    _viewport[0][3] = x + w / 2.f;
    _viewport[1][3] = y + h / 2.f;
    _viewport[2][3] = depth / 2.f;
    _viewport[0][0] = w / 2.f;
    _viewport[1][1] = h / 2.f;
    _viewport[2][2] = depth / 2.f;
}

void projectMat(float coeff)
{
    _projection = mat<4, 4>::identity();
    _projection[3][2] = coeff;
}

void lookAt(vec3 eye, vec3 center, vec3 up)
{
    vec3 z = (eye - center).normalized();
    vec3 x = cross(up, z).normalized();
    vec3 y = cross(z, x).normalized();
    _view = mat<4, 4>::identity();
    for(int i = 0; i < 3; i++) {
        _view[0][i] = x[i];
        _view[1][i] = y[i];
        _view[2][i] = z[i];
        _view[i][3] = -center[i];
    }
}

vec3 barycentric(const vec2* pts, const vec2 P)
{
    mat<3, 3> ABC = {{embed<3>(pts[0]), embed<3>(pts[1]), embed<3>(pts[2])}};
    if(ABC.det() < 1e-3) return {-1, 1, 1};
    return ABC.invert_transpose() * embed<3>(P);
}

void triangle(vec4 *pts, IShader &shader, TGAImage &image, TGAImage &zbuffer)
{
    vec2 bboxmin(image.width() - 1, image.height() - 1);
    vec2 bboxmax(0, 0);
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 2; j++) {
            bboxmin[j] = std::min(bboxmin[j], std::round(pts[i][j] / pts[i][3]));
            bboxmax[j] = std::max(bboxmax[j], std::round(pts[i][j] / pts[i][3]));
        }

    vec2 P;
    TGAColor color;
    for(P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for(P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            vec2 bc_screen[3] = {proj<2>(pts[0] / pts[0][3]), proj<2>(pts[1] / pts[1][3]), proj<2>(pts[2] / pts[2][3])};
            vec3 c = barycentric(bc_screen, proj<2>(P));
            float z = pts[0][2] * c.x + pts[1][2] * c.y + pts[2][2] * c.z;
            float w = pts[0][3] * c.x + pts[1][3] * c.y + pts[2][3] * c.z;
            int frag_depth = std::max(0, std::min(255, int(z / w + .5)));
            if(c.x < 0 || c.y < 0 || c.z < 0 || zbuffer.get(P.x, P.y)[0] > frag_depth) continue;
            bool discard = shader.fragment(c, color);
            if(!discard) {
                zbuffer.set(P.x, P.y, TGAColor(frag_depth));
                image.set(P.x, P.y, color);
            }
        }
    }
}