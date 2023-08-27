#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include "tgaimage.h"
#include "model.h"

const int width  = 800;
const int height = 800;
const int depth = 255;
TGAColor white {255, 255, 255, 255};

Model *model = NULL;
Vec3f light_Dir{0, 0, -1};
Vec3f camera_Pos{0, 0, 3};

void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color){
    bool steep = false;
    if(std::abs(x0 - x1) < std::abs(y0 - y1))
    {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if(x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    int dx = x1 - x0;
    int dy = y1 - y0;
    int derror2 = std::abs(dy) * 2;
    int error2 = 0;
    int y = y0;
    for (int x = x0; x <= x1; x++) {
        if(steep) 
            image.set(y, x, color);
        else
            image.set(x, y, color);
        error2 += derror2;
        if(error2 > dx)
        {
            y += (y1 > y0 ? 1 : -1);
            error2 -= dx * 2;
        }
    }
}

void triangle(Vec2f t0, Vec2f t1, Vec2f t2, TGAImage &image, TGAColor color)
{
    if(t0.y == t1.y && t0.y == t2.y) return;
    if(t0.y > t1.y) std::swap(t0, t1);
    if(t0.y > t2.y) std::swap(t0, t2);
    if(t1.y > t2.y) std::swap(t1, t2);
    int h = t2.y - t0.y;
    for(int i = 0; i < h; i++) {
        bool second_part = i > t1.y - t0.y || t1.y == t0.y;
        int segment_height = second_part ? t2.y - t1.y : t1.y - t0.y;
        float k1 = (float) i / h;
        float k2 = (float) (i - (second_part ? t1.y - t0.y : 0)) / segment_height;
        Vec2f A = t0 + (t2 - t0) * k1;
        Vec2f B = second_part ? t1 + (t2 - t1) * k2 : t0 + (t1 - t0) * k2;
        if(A.x > B.x) std::swap(A, B);
        for(int j = A.x; j <= B.x; j++) {
            image.set(j, t0.y + i, color);
        }
    }
}

Vec3f barycentric(Vec3f* pts, Vec2f p)
{
    Vec3f u = Vec3f(pts[2].x - pts[0].x, pts[1].x - pts[0].x, pts[0].x - p.x) ^ Vec3f(pts[2].y - pts[0].y, pts[1].y - pts[0].y, pts[0].y - p.y);
    if(std::abs(u.z) < 0.1f) return Vec3f(-1, 1, 1);
    return Vec3f(1.f-(u.x+u.y)/u.z, u.y/u.z, u.x/u.z);
}

void triangle(Vec3f *pts, Vec2i* _uv, float* zbuffer, TGAImage &image, float intensity) { 
    Vec2f bboxmin( std::numeric_limits<float>::max(),  std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.width()-1, image.height()-1); 

    for (int i=0; i<3; i++) {
        bboxmin.x = std::max(0.f, std::min(bboxmin.x, pts[i].x));
        bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, pts[i].x));

        bboxmin.y = std::max(0.f, std::min(bboxmin.y, pts[i].y));
        bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, pts[i].y));
    }

    //PROBLEM: bboxmin[0] / bboxmin[1] 同时指向y?
    // for(int i = 0; i < 3; i++)
    // {
    //     for(int j = 0; j < 2; j++)
    //     {
    //         bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts[i][j]));
    //         bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
    //     }
    // }

    Vec2i x_range((int)std::floor(bboxmin.x), (int)std::ceil(bboxmax.x));
    Vec2i y_range((int)std::floor(bboxmin.y), (int)std::ceil(bboxmax.y));

    Vec2i P; 
    for (P.x=x_range.x; P.x<=x_range.y; P.x++) { 
        for (P.y=y_range.x; P.y<=bboxmax.y; P.y++) { 
            Vec3f bc_screen  = barycentric(pts, Vec2f(P.x, P.y)); 
            Vec2i uv;
            if (bc_screen.x< 0 || bc_screen.y< 0 || bc_screen.z< 0) continue; 
            float z_interpolation = 0;
            for(int i = 0; i < 3; i++){
                z_interpolation += pts[i].z * bc_screen[i];
            } 
            uv = _uv[0] * bc_screen.x + _uv[1] * bc_screen.y + _uv[2] * bc_screen.z;
            if(zbuffer[int(P.x + P.y * width)] < z_interpolation)
            {
                zbuffer[int(P.x + P.y * width)] = z_interpolation;
                TGAColor color = model->diffuse(uv);
                image.set(P.x, P.y, TGAColor{color[0] * intensity, color[1] * intensity, color[2] * intensity, color[3] * intensity}); 
            }
        } 
    } 
} 

void triangle(Vec3i t0, Vec3i t1, Vec3i t2, Vec2i uv0, Vec2i uv1, Vec2i uv2, TGAImage &image, float intensity, float *zbuffer) {
    if (t0.y==t1.y && t0.y==t2.y) return; // i dont care about degenerate triangles
    if (t0.y>t1.y) { std::swap(t0, t1); std::swap(uv0, uv1); }
    if (t0.y>t2.y) { std::swap(t0, t2); std::swap(uv0, uv2); }
    if (t1.y>t2.y) { std::swap(t1, t2); std::swap(uv1, uv2); }

    int total_height = t2.y-t0.y;
    for (int i=0; i<total_height; i++) {
        bool second_half = i>t1.y-t0.y || t1.y==t0.y;
        int segment_height = second_half ? t2.y-t1.y : t1.y-t0.y;
        float alpha = (float)i/total_height;
        float beta  = (float)(i-(second_half ? t1.y-t0.y : 0))/segment_height; // be careful: with above conditions no division by zero here
        Vec3i A   =               t0  + Vec3f(t2-t0  )*alpha;
        Vec3i B   = second_half ? t1  + Vec3f(t2-t1  )*beta : t0  + Vec3f(t1-t0  )*beta;
        Vec2i uvA =               uv0 +      (uv2-uv0)*alpha;
        Vec2i uvB = second_half ? uv1 +      (uv2-uv1)*beta : uv0 +      (uv1-uv0)*beta;
        if (A.x>B.x) { std::swap(A, B); std::swap(uvA, uvB); }
        for (int j=A.x; j<=B.x; j++) {
            float phi = B.x==A.x ? 1. : (float)(j-A.x)/(float)(B.x-A.x);
            Vec3i   P = Vec3f(A) + Vec3f(B-A)*phi;
            Vec2i uvP =     uvA +   (uvB-uvA)*phi;
            int idx = P.x+P.y*width;
            if (zbuffer[idx]<P.z) {
                zbuffer[idx] = P.z;
                TGAColor color = model->diffuse(uvP);
                image.set(P.x, P.y, TGAColor{color[0]*intensity, color[1]*intensity, color[2]*intensity});
            }
        }
    }
}


Vec3f world2screen(Vec3f v) {
    return Vec3f(int((v.x+1.)*width/2.+.5), int((v.y+1.)*height/2.+.5), v.z);
}

Vec3f m2v(Matrix m)
{
    return Vec3f(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
}

Matrix v2m(Vec3f v)
{
    Matrix m(4, 1);
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    m[3][0] = 1.f;
    return m;
}

Matrix projectMat()
{
    Matrix projection = Matrix::identity(4);
    projection[3][2] = -1.0f/camera_Pos.z;
    return projection;
}

Matrix viewportMat(int x, int y, int w, int h) {
    Matrix m = Matrix::identity(4);
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

    Matrix _model = Matrix::identity(4);
    Matrix _view = Matrix::identity(4);
    Matrix _projection = projectMat();
    Matrix _viewport = viewportMat(width / 8, height / 8, width * 3 / 4, height * 3 / 4);

    for(int i = 0; i < model->nfaces(); i++)
    {
        // 2D Vec2i((world_Coords[j].x+1.)*width/2., (world_Coords[j].y+1.)*height/2.)
        Vec3f Screen_Coords[3];
        Vec3f world_Coords[3];
        Vec2i _uv[3];
        for(int j = 0; j < 3; j++)
        {
            world_Coords[j] = model->vert(model->face(i)[j]);
            //Screen_Coords[j] = m2v(_viewport *  _projection * v2m(world_Coords[j]));
            Screen_Coords[j] = world2screen(world_Coords[j]);
            _uv[j] = model->uv(i, j);
        }

        Vec3f n = (world_Coords[2] - world_Coords[0]) ^ (world_Coords[1] - world_Coords[0]);
        n.normalize();
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
