#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>
#include <iostream>
#include "tgaimage.h"
#include "model.h"

const int width  = 800;
const int height = 800;
const int depth = 255;
TGAColor white {255, 255, 255, 255};

Model *model = NULL;
vec3 light_Dir{0, 0, -1};
vec3 camera_Pos{0, 0, 3};

vec3 barycentric(vec3* pts, vec2 p)
{
    vec3 u = cross(vec3(pts[2].x - pts[0].x, pts[1].x - pts[0].x, pts[0].x - p.x), vec3(pts[2].y - pts[0].y, pts[1].y - pts[0].y, pts[0].y - p.y));
    if(std::abs(u.z) < 0.1f) return vec3(-1, 1, 1);
    return vec3(1.f-(u.x+u.y)/u.z, u.y/u.z, u.x/u.z);
}

void triangle(vec3 *pts, vec2* _uv, float* zbuffer, TGAImage &image, float intensity) { 
    vec2 bboxmin( std::numeric_limits<float>::max(),  std::numeric_limits<float>::max());
    vec2 bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    vec2 clamp(image.width()-1, image.height()-1); 

    for(int i = 0; i < 3; i++)
    {
        for(int j = 0; j < 2; j++)
        {
            bboxmin[j] = std::max(0., std::min(bboxmin[j], pts[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
        }
    }

    vec2 x_range((int)std::floor(bboxmin.x), (int)std::ceil(bboxmax.x));
    vec2 y_range((int)std::floor(bboxmin.y), (int)std::ceil(bboxmax.y));

    vec2 P; 
    for (P.x=x_range.x; P.x<=x_range.y; P.x++) { 
        for (P.y=y_range.x; P.y<=bboxmax.y; P.y++) { 
            vec3 bc_screen  = barycentric(pts, vec2(P.x, P.y)); 
            vec2 uv;
            if (bc_screen.x< 0 || bc_screen.y< 0 || bc_screen.z< 0) continue; 
            float z_interpolation = 0;
            for(int i = 0; i < 3; i++){
                z_interpolation += pts[i].z * bc_screen[i];
            } 
            uv = _uv[0] * bc_screen.x + _uv[1] * bc_screen.y + _uv[2] * bc_screen.z;
            if(zbuffer[int(P.x + P.y * width)] < z_interpolation)
            {
                zbuffer[int(P.x + P.y * width)] = z_interpolation;
                TGAColor color = model->diffuse().get((uv.x * model->diffuse().width()), (uv.y * model->diffuse().height()));
                image.set(P.x, P.y, TGAColor{static_cast<uint8_t>(color[0] * intensity), static_cast<uint8_t>(color[1] * intensity), static_cast<uint8_t>(color[2] * intensity), 255}); 
            }
        } 
    } 
} 

vec3 vec3f2Vec3i(vec3 f)
{
    return vec3(int(f.x + 0.5), int(f.y + 0.5), f.z);
}

//Before Use this function, First return vec3f2Vec3i. 在调用这个函数前先转Vec3i
void triangle(vec3 t0, vec3 t1, vec3 t2, vec2 uv0, vec2 uv1, vec2 uv2, TGAImage &image, float intensity, float *zbuffer) {
    if ((int)t0.y==(int)t1.y && (int)t0.y==(int)t2.y) return; // i dont care about degenerate triangles
    if (t0.y>t1.y) { std::swap(t0, t1); std::swap(uv0, uv1); }
    if (t0.y>t2.y) { std::swap(t0, t2); std::swap(uv0, uv2); }
    if (t1.y>t2.y) { std::swap(t1, t2); std::swap(uv1, uv2); }

    int total_height = t2.y-t0.y;
    for (int i=0; i<total_height; i++) {
        bool second_half = i>t1.y-t0.y || t1.y==t0.y;
        int segment_height = second_half ? t2.y-t1.y : t1.y-t0.y;
        float alpha = (float)i/total_height;
        float beta  = (float)(i-(second_half ? t1.y-t0.y : 0))/segment_height; // be careful: with above conditions no division by zero here
        vec3 A   =               t0  + (t2-t0  )*alpha;
        A = vec3f2Vec3i(A);
        vec3 B   = second_half ? t1  + (t2-t1  )*beta : t0  + (t1-t0  )*beta;
        B = vec3f2Vec3i(B);
        vec2 uvA =               uv0 +      (uv2-uv0)*alpha;
        vec2 uvB = second_half ? uv1 +      (uv2-uv1)*beta : uv0 +      (uv1-uv0)*beta;
        if (A.x>B.x) { std::swap(A, B); std::swap(uvA, uvB); }
        for (int j=A.x; j<=B.x; j++) {
            float phi = B.x==A.x ? 1. : (float)(j-A.x)/(float)(B.x-A.x);
            vec3   P = A + (B-A)*phi;
            P = vec3f2Vec3i(P);
            vec2 uvP =     uvA +   (uvB-uvA)*phi;
            int idx = P.x+P.y*width;
            if (P.x >= width||P.y >= height||P.x < 0||P.y < 0) continue;
            if (zbuffer[idx]<P.z) {
                zbuffer[idx] = P.z;
                TGAColor color = model->diffuse().get((uvP.x * model->diffuse().width()), (uvP.y * model->diffuse().height()));
                image.set(P.x, P.y, TGAColor{static_cast<uint8_t>(color[0]*intensity), static_cast<uint8_t>(color[1]*intensity), static_cast<uint8_t>(color[2]*intensity)});
            }
        }
    }
}


vec3 world2screen(vec3 v) {
    return vec3(int((v.x+1.)*width/2.+.5), int((v.y+1.)*height/2.+.5), v.z);
}

vec3 m2v(mat<4, 1> m)
{
    return vec3(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
}

mat<4, 1> v2m(vec3 v)
{
    mat<4, 1> m;
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    m[3][0] = 1.f;
    return m;
}

mat<4, 4> projectMat()
{
    mat<4, 4> projection = mat<4, 4>::identity();
    projection[3][2] = -1.0f/camera_Pos.z;
    return projection;
}

mat<4, 4> viewportMat(int x, int y, int w, int h) {
    mat<4, 4> m = mat<4, 4>::identity();
    m[0][3] = x + w/2.f;
    m[1][3] = y + h/2.f;
    m[2][3] = depth/2.f;

    m[0][0] = w/2.f;
    m[1][1] = h/2.f;
    m[2][2] = depth/2.f;
    return m;
}

int main(int argc, char** argv){
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("../../obj/african_head/african_head.obj");
    }

    TGAImage image(width, height, TGAImage::RGB);
    
    float* zbuffer = new float[width * height];
    for(int i = 0; i < width * height; i++)
        zbuffer[i] = std::numeric_limits<float>::min();

    mat<4, 4> _model = mat<4, 4>::identity();
    mat<4, 4> _view = mat<4, 4>::identity();
    mat<4, 4> _projection = projectMat();
    mat<4, 4> _viewport = viewportMat(width / 8, height / 8, width * 3 / 4, height * 3 / 4);

    for(int i = 0; i < model->nfaces(); i++)
    {
        // 2D vec2((world_Coords[j].x+1.)*width/2., (world_Coords[j].y+1.)*height/2.)
        vec3 Screen_Coords[3];
        vec3 world_Coords[3];
        vec2 _uv[3];
        for(int j = 0; j < 3; j++)
        {
            world_Coords[j] = model->vert(i, j);
            //Screen_Coords[j] = m2v(_viewport *  _projection * v2m(world_Coords[j]));
            //Screen_Coords[j] = vec3f2Vec3i(Screen_Coords[j]);
            Screen_Coords[j] = world2screen(world_Coords[j]);
            _uv[j] = model->uv(i, j);
        }

        vec3 n = cross((world_Coords[2] - world_Coords[0]) , (world_Coords[1] - world_Coords[0]));
        n.Normalized();
        float intensity = n * light_Dir;
        if(intensity > 0)
            triangle(Screen_Coords, _uv, zbuffer, image, intensity);
            //triangle(Screen_Coords[0], Screen_Coords[1], Screen_Coords[2], _uv[0], _uv[1], _uv[2], image, intensity, zbuffer);
    }

    //image.flip_vertically();
    std::cout << image.write_tga_file("output.tga") << std::endl;

    delete model;
    return 0;
}
