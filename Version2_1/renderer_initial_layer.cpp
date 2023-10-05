#include "renderer_initial_layer.h"
#include <__msvc_chrono.hpp>
#include <chrono>

const float depth = 2000.f;

vec3 light_Dir{1, 1, 1};
vec3 eye{1, 1, 3};
vec3 center{0, 0, 0};
vec3 up{0, 1, 0};

RendererInitialLayer::RendererInitialLayer(int width, int height) : _shader(*this), _width(width), _height(height) 
{
    zbuffer = new float[width * height];
}

RendererInitialLayer::~RendererInitialLayer()
{
    delete [] zbuffer;
}

void RendererInitialLayer::initial()
{
    model = std::make_unique<Model>("../../obj/african_head/african_head.obj");
    for(int i = _width * _height; i--; zbuffer[i] = -std::numeric_limits<float>::max());
    lookAt(eye, center, up);
    viewportMat(_width / 8, _height / 8, _width * 3 / 4, _height * 3 / 4, depth);
    projectMat(-1.0f / (eye - center).norm());
    light_Dir = proj<3>((_projection * _view * _model * embed<4>(light_Dir, 0.f))).normalized();

}

void RendererInitialLayer::drawScene(SDL_Renderer* renderer)
{
    _shader.uniform_M = (_projection * _view * _model);
    _shader.uniform_MIT = _shader.uniform_M.invert_transpose();
    auto begin = std::chrono::system_clock::now();
    for(int i = 0; i < model->nfaces(); i++)
    {
        for(int j = 0; j < 3; j++)
        {
            _shader.vertex(i, j);
        }

        triangle(_shader.varying_tri, _shader, renderer, zbuffer, _height, _width);

    }
    auto end = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
    std::cout << duration.count() << std::endl;
}

vec4 RendererInitialLayer::PhongShader::vertex(int iface, int nthvert)
{
    varying_uv.set_col(nthvert, _renderer.model->uv(iface, nthvert));
    vec4 gl_Posistion = uniform_M * embed<4>(_renderer.model->vert(iface, nthvert));
    varying_tri.set_col(nthvert, gl_Posistion);
    return gl_Posistion;
}

bool RendererInitialLayer::PhongShader::fragment(vec3 bar, TGAColor &color) {
    vec2 uv = varying_uv * bar;

    vec3 n = proj<3>(uniform_MIT * embed<4>(_renderer.model->normal(uv))).normalized();

    float diff = std::max(0., n * light_Dir);
    color = _renderer.model->diffuse(uv) * diff;
        
    return false;
}
