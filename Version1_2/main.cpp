#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "our_gl.h"

const int width  = 800;
const int height = 800;
const int depth = 255;
TGAColor white {255, 255, 255, 255};

Model *model = NULL;
vec3 light_Dir{1, 1, 1};
vec3 eye{1, 1, 3};
vec3 center{0, 0, 0};
vec3 up{0, 1, 0};

struct FlatShader : public IShader {
    vec3 pos[3];

    virtual vec4 vertex(int iface, int nthvert) {
        vec4 gl_Position = embed<4>(model->vert(iface, nthvert));
        gl_Position =  _projection * _view * _model * gl_Position;
        pos[nthvert] = proj<3>(gl_Position / gl_Position[3]);
        gl_Position = _viewport * gl_Position;
        return gl_Position;
    }

    virtual bool fragment(vec3 bar, TGAColor &color) {
        vec3 n = cross(pos[1] - pos[0], pos[2] - pos[0]).normalized();
        float intensity = std::max(0., n * light_Dir);
        color = TGAColor(255, 255, 255) * intensity;
        return false;
    }
};

struct GouraudShader : public IShader {
    vec3 varying_intensity;

    virtual vec4 vertex(int iface, int nthvert) {
        vec4 gl_Position = embed<4>(model->vert(iface, nthvert));
        gl_Position = _viewport * _projection * _view * _model * gl_Position;
        varying_intensity[nthvert] = std::max(0., model->normal(iface, nthvert) * light_Dir);
        return gl_Position;
    }

    virtual bool fragment(vec3 bar, TGAColor &color) {
        float intensity = varying_intensity * bar;
        color = TGAColor(255, 255, 255) * intensity;
        return false;
    }
};

struct ToonShader : public IShader {
    vec3 varying_intensity;

    virtual vec4 vertex(int iface, int nthvert) {
        vec4 gl_Position = embed<4>(model->vert(iface, nthvert));
        gl_Position = _projection * _view * _model * gl_Position;
        varying_intensity[nthvert] = std::max(0., model->normal(iface, nthvert) * light_Dir);
        gl_Position = _viewport * gl_Position;
        return gl_Position;
    }

    virtual bool fragment(vec3 bar, TGAColor &color) {
        float intensity = varying_intensity * bar;
        if(intensity > .85) intensity = 1;
        else if(intensity > .60) intensity = .80;
        else if(intensity > .45) intensity = .60;
        else if(intensity > .30) intensity = .45;
        else if(intensity > .15) intensity = .30;
        color = TGAColor(255, 155, 0) * intensity;
        return false;
    }
};

struct Shader : public IShader {
    mat<2, 3> varying_uv;
    mat<4, 4> uniform_M;
    mat<4, 4> uniform_MIT;
    mat<3, 3> varying_pos;

    virtual vec4 vertex(int iface, int nthvert) {
        vec4 gl_Position = embed<4>(model->vert(iface, nthvert));
        gl_Position =  _model * gl_Position;
        varying_pos.set_col(nthvert, proj<3>(gl_Position / gl_Position[3]));
        varying_uv.set_col(nthvert,  model->uv(iface, nthvert));
        gl_Position = _viewport * _projection * _view * gl_Position;
        return gl_Position;
    }

    virtual bool fragment(vec3 bar, TGAColor &color) {
        vec2 uv = varying_uv * bar;
        vec3 n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalized();
        vec3 l = proj<3>(uniform_M * embed<4>(light_Dir)).normalized();
        vec3 r = (n * (n * l * 2) - l).normalized();
        vec3 viewDir = (eye - varying_pos * bar).normalized();
        vec3 halfDir = (viewDir + l).normalized();
        // float spec = std::pow(std::max(r.z, 0.), model->specular(uv)[0]);
        float spec = std::pow(std::max(0., halfDir * n), model->specular(uv)[0]);
        float diff = std::max(0., n * l);
        color = model->diffuse(uv) * diff;
        for(int i = 0; i < 3; i++) color[i] = std::min(5. + color[i] * (diff + spec), 255.);
        return false; 
    }
};

int main(int argc, char** argv){
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("../../obj/african_head/african_head.obj");
    }

    lookAt(eye, center, up);
    viewportMat(width / 8, height / 8, width * 3 / 4, height * 3 / 4, depth);
    projectMat(-1.0f / (eye - center).norm());
    light_Dir.Normalized();

    TGAImage image(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

    Shader shader;
    shader.uniform_M = _projection * _view * _model;
    shader.uniform_MIT = (_projection * _view * _model).invert_transpose();
    for(int i = 0; i < model->nfaces(); i++)
    {
        vec4 screen_coords[3];
        for(int j = 0; j < 3; j++)
        {
            screen_coords[j] = shader.vertex(i, j);
        }
        triangle(screen_coords, shader, image, zbuffer);
    }

    //image.flip_vertically();
    std::cout << image.write_tga_file("output.tga") << std::endl;
    std::cout << zbuffer.write_tga_file("zbuffer.tga");

    delete model;
    return 0;
}
