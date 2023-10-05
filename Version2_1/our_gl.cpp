#include <SDL_render.h>
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

// vec3 barycentric(const vec2* pts, const vec2 P)
// {
//     mat<3, 3> ABC = {{embed<3>(pts[0]), embed<3>(pts[1]), embed<3>(pts[2])}};
//     if(ABC.det() < 1e-3) return {-1, 1, 1};
//     return ABC.invert_transpose() * embed<3>(P);
// }

vec3 barycentric(const vec2* v, const vec2 P)
{
    float c1 = (P.x*(v[1].y - v[2].y) + (v[2].x - v[1].x)*P.y + v[1].x*v[2].y - v[2].x*v[1].y) / (v[0].x*(v[1].y - v[2].y) + (v[2].x - v[1].x)*v[0].y + v[1].x*v[2].y - v[2].x*v[1].y);
	float c2 = (P.x*(v[2].y - v[0].y) + (v[0].x - v[2].x)*P.y + v[2].x*v[0].y - v[0].x*v[2].y) / (v[1].x*(v[2].y - v[0].y) + (v[0].x - v[2].x)*v[1].y + v[2].x*v[0].y - v[0].x*v[2].y);
	return vec3(c1, c2, 1 - c1 - c2);
}

void triangle(mat<4, 3> &clipc, IShader &shader, SDL_Renderer* renderer, float* zbuffer, int width, int height)
{
    mat<3, 4> pts = (_viewport * clipc).transpose();
    vec2 pts2[3];
    for(int i = 0; i < 3; i++) pts2[i] = proj<2>(pts[i] / pts[i][3]);

    vec2 bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    vec2 bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    vec2 clamp(width - 1, height - 1);
    for(int i = 0; i < 3; i++)
        for(int j = 0; j < 2; j++) {
            bboxmin[j] = std::max(0., std::min(bboxmin[j], pts2[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts2[i][j]));
        }
    bboxmin = vec2(std::round(bboxmin.x), std::round(bboxmin.y));
    bboxmax = vec2(std::round(bboxmax.x), std::round(bboxmax.y));

    vec2 P;
    TGAColor color;
    for(P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for(P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            vec3 bc_screen = barycentric(pts2, proj<2>(P));
            vec3 bc_clip = vec3(bc_screen.x/pts[0][3], bc_screen.y/pts[1][3], bc_screen.z/pts[2][3]);
            bc_clip = bc_clip/(bc_clip.x+bc_clip.y+bc_clip.z);
            float frag_depth = clipc[2]*bc_clip;
            if (bc_screen.x<0 || bc_screen.y<0 || bc_screen.z<0 || zbuffer[int(P.x+P.y*width)]>frag_depth) continue;
            bool discard = shader.fragment(bc_clip, color);
            if(!discard) {
                zbuffer[int(P.x+P.y*width)] = frag_depth;
                SDL_SetRenderDrawColor(renderer, color.bgra[2], color.bgra[1], color.bgra[0], color.bgra[3]);
                SDL_RenderDrawPoint(renderer, P.x, height - P.y);
            }
        }
    }
}

