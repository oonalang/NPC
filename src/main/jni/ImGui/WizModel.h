#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "WizMotion.h"

namespace wiz_model {
struct Vertex {
    float position[3],normal[3],uv[2];
    std::uint8_t joints[4];
    std::uint16_t weights[4];
    float blink[3],smile[3],speak[3];
};
static_assert(sizeof(Vertex)==80,"Wiz vertex layout");
struct Material {
    std::uint32_t first=0,count=0;
    int width=0,height=0;
    std::vector<unsigned char> png,rgba;
};
struct Model {
    std::uint32_t version=1;
    std::uint32_t bones=0,frames=0,fps=0;
    float duration=0;
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<Material> materials;
    std::vector<float> animation;
    Rig rig;
    std::size_t Stride() const { return version>=2?bones*10+5:bones*16+56; }
};
bool Decode(const unsigned char* bytes,std::size_t size,Model& model);
bool DecodeTextures(Model& model);
bool Sample(const Model& model,double seconds,std::vector<float>& frame);
struct Frame {
    std::uintptr_t texture=0;
    std::array<std::array<float,2>,15> points{};
    std::array<float,2> min{},max{},floor{};
    float lift=0;
    bool excited=false;
};
void Prepare();
bool Ready();
bool Failed();
Frame Render(double seconds,float aspect);
unsigned RenderCount();
unsigned UploadCount();
void Release();
void ResetMotion();
void SetDynamics(Dynamics mode);
MotionStats PhysicsStats();
void SetView(float yaw,float pitch=0);
std::array<float,2> View();
struct ResourceUsage {std::size_t cpuBytes=0,gpuBytes=0,assetBytes=0;unsigned bones=0,springs=0;};
ResourceUsage Resources();
}
