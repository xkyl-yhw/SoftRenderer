#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <vector>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "our_gl.h"

const int width  = 800;
const int height = 800;
const float depth = 2000.f;
TGAColor white {255, 255, 255, 255};

Model* model = NULL;
float* shadowbuffer = NULL;
vec3 light_Dir{1, 1, 0};
vec3 eye{1, 1, 4};
vec3 center{0, 0, 0};
vec3 up{0, 1, 0};

// light_Dir = proj<3>((_projection*_view * _model *embed<4>(light_Dir, 0.f))).normalized();
struct PhongShader : public IShader {
    mat<2, 3> varying_uv;
    mat<4, 3> varying_tri;
    mat<3, 3> varying_nrm;
    mat<3, 3> ndc_tri;

    virtual vec4 vertex(int iface, int nthvert) {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        varying_nrm.set_col(nthvert, proj<3>((_projection * _view * _model).invert_transpose() * embed<4>(model->normal(iface, nthvert), 0.)));
        vec4 gl_Posistion = _projection * _view * _model * embed<4>(model->vert(iface, nthvert));
        varying_tri.set_col(nthvert, gl_Posistion);
        ndc_tri.set_col(nthvert, proj<3>(gl_Posistion / gl_Posistion[3]));
        return gl_Posistion;
    }

    virtual bool fragment(vec3 bar, TGAColor &color) {
        vec3 bn = (varying_nrm * bar).normalized();
        vec2 uv = varying_uv * bar;

        mat<3, 3> A;
        A[0] = ndc_tri.col(1) - ndc_tri.col(0);
        A[1] = ndc_tri.col(2) - ndc_tri.col(0);
        A[2] = bn;

        mat<3, 3> AI = A.invert();
        
        vec3 i = AI * vec3(varying_uv[0][1] - varying_uv[0][0], varying_uv[0][2] - varying_uv[0][0], 0);
        vec3 j = AI * vec3(varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0], 0);

        mat<3, 3> B;
        B.set_col(0, i.normalized());
        B.set_col(1, j.normalized());
        B.set_col(2, bn);

        vec3 n = (B * model->normal(uv)).normalized();

        float diff = std::max(0., n * light_Dir);
        color = model->diffuse(uv) * diff;
        
        return false;
    }
};

struct DepthShader : public IShader {
    mat<4, 3> varying_tri;
    mat<3, 3> varying_ndc_tri;

    virtual vec4 vertex(int iface, int nthvert) {
        vec4 gl_Position = embed<4>(model->vert(iface, nthvert));
        gl_Position = _projection * _view * _model * gl_Position;
        varying_tri.set_col(nthvert, gl_Position);
        gl_Position = _viewport * gl_Position;
        varying_ndc_tri.set_col(nthvert, proj<3>(gl_Position / gl_Position[3]));
        return gl_Position;
    }

    virtual bool fragment(vec3 bar, TGAColor &color) {
        vec3 p = varying_ndc_tri * bar;
        color = TGAColor(255, 255, 255) * (p.z / depth);
        return false;
    }
};

struct Shader : public IShader {
    mat<4, 4> uniform_M;
    mat<4, 4> uniform_MIT;
    mat<4, 4> uniform_Mshadow;
    mat<2, 3> varying_uv;
    mat<4, 3> varying_tri;
    mat<3, 3> varying_ndc_tri;

    Shader(mat<4, 4> M, mat<4, 4> MIT, mat<4, 4> MShadow) : uniform_M(M), uniform_MIT(MIT), uniform_Mshadow(MShadow) {}

    virtual vec4 vertex(int iface, int nthvert) {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        vec4 gl_Position = _projection * _view * _model * embed<4>(model->vert(iface, nthvert));
        varying_tri.set_col(nthvert, gl_Position);
        gl_Position = _viewport * gl_Position;
        varying_ndc_tri.set_col(nthvert, proj<3>(gl_Position / gl_Position[3]));
        return gl_Position;
    }

    virtual bool fragment(vec3 bar, TGAColor &color) {
        vec4 sb_p = uniform_Mshadow * embed<4>(varying_ndc_tri * bar);
        sb_p = sb_p / sb_p[3];
        int idx = int(sb_p[0]) + int(sb_p[1]) * width;
        float shadow = .3 + .7 * (shadowbuffer[idx] < sb_p[2]);
        vec2 uv = varying_uv * bar;
        vec3 n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalized();
        vec3 l = proj<3>(uniform_M * embed<4>(light_Dir)).normalized();
        vec3 r = (n * (n * l * 2) - l).normalized();
        float spec = std::pow(std::max(r.z, 0.0), model->specular(uv));
        float diff = std::max(0., n * l);
        color = model->diffuse(uv);
        for(int i = 0; i < 3; i++) color[i] = std::min(20. + color[i] * shadow * (1.2*diff + .6*spec), 255.);
        return false;
    }
};

int main(int argc, char** argv){
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("../../obj/african_head/african_head.obj");
    }

    float* zbuffer = new float[width * height];
    shadowbuffer = new float[width * height];
    for(int i = width * height; i--; zbuffer[i] = shadowbuffer[i] = -std::numeric_limits<float>::max());

    light_Dir.Normalized();

    {
        TGAImage depthImg(width, height, TGAImage::RGB);
        lookAt(light_Dir, center, up);
        viewportMat(width / 8, height / 8, width * 3 / 4, height * 3 / 4, depth);
        projectMat(0);

        DepthShader depthShader;
        for (int i = 0; i < model->nfaces(); i++) {
            for(int j = 0; j < 3; j++) {
                depthShader.vertex(i, j);
            }
            triangle(depthShader.varying_tri, depthShader, depthImg, shadowbuffer);
        }
        depthImg.write_tga_file("depth.tga");
    }

    mat<4, 4> m = _viewport * _projection * _view * _model;

    {
        lookAt(eye, center, up);
        viewportMat(width / 8, height / 8, width * 3 / 4, height * 3 / 4, depth);
        projectMat(-1.0f / (eye - center).norm());

        TGAImage image(width, height, TGAImage::RGB);

        Shader shader(_view * _model, (_projection * _view * _model).invert_transpose(), m * (_viewport * _projection * _view * _model).invert());
        for(int i = 0; i < model->nfaces(); i++)
        {
            for(int j = 0; j < 3; j++)
            {
                shader.vertex(i, j);
            }
            triangle(shader.varying_tri, shader, image, zbuffer);
        }
        //image.flip_vertically();
        std::cout << image.write_tga_file("output.tga") << std::endl;
    }

    delete model;
    delete [] zbuffer;
    delete [] shadowbuffer;
    return 0;
}
