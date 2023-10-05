#include "our_gl.h"
#include "model.h"

class RendererInitialLayer{
public:
    RendererInitialLayer(int width, int height);
    ~RendererInitialLayer();
    void initial();
    void drawScene(SDL_Renderer* renderer);
    struct PhongShader : public IShader {
    private:
        RendererInitialLayer& _renderer;
    public:
        mat<2, 3> varying_uv;
        mat<4, 3> varying_tri;
        mat<4, 4> uniform_MIT;
        mat<4, 4> uniform_M;
    public:
        PhongShader(RendererInitialLayer& _layer) : _renderer(_layer) {}
        virtual vec4 vertex(int iface, int nthvert) override;
        virtual bool fragment(vec3 bar, TGAColor &color) override;
    } _shader;
    
private:
    std::unique_ptr<Model> model = nullptr;
    float* zbuffer;
    int _width, _height;
};