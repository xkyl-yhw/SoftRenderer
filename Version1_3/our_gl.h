#ifndef __OUR_GL_H__
#define __OUR_GL_H__

#include "tgaimage.h"
#include "geometry.h"

extern mat<4, 4> _model;
extern mat<4, 4> _view;
extern mat<4, 4> _projection;
extern mat<4, 4> _viewport;

void viewportMat(int x, int y, int w, int h, int depth);
void projectMat(float coeff = 0.f);
void lookAt(vec3 eye, vec3 center, vec3 up);

struct IShader {
    virtual ~IShader();
    virtual vec4 vertex(int iface, int nthvert) = 0;
    virtual bool fragment(vec3 bar, TGAColor &color) = 0;
};

void triangle(mat<4, 3> &clipc, IShader &shader, TGAImage &image, float* zbuffer);

#endif