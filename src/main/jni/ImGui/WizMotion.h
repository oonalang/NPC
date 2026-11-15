#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace wiz_model {
struct Vec3 {
    float x=0,y=0,z=0;
    Vec3 operator+(Vec3 b) const {return {x+b.x,y+b.y,z+b.z};}
    Vec3 operator-(Vec3 b) const {return {x-b.x,y-b.y,z-b.z};}
    Vec3 operator*(float s) const {return {x*s,y*s,z*s};}
    Vec3 operator/(float s) const {return *this*(1.f/s);}
};
struct Quat {float x=0,y=0,z=0,w=1;};
using Matrix4=std::array<float,16>;
struct Transform {Vec3 position;Quat rotation;Vec3 scale{1,1,1};};
float Length(Vec3 v);
float Dot(Vec3 a,Vec3 b);
Vec3 Normalize(Vec3 v,Vec3 fallback={0,1,0});
Quat Normalize(Quat q);
Quat Multiply(Quat a,Quat b);
Quat Inverse(Quat q);
Quat AxisAngle(Vec3 axis,float angle);
Quat Slerp(Quat a,Quat b,float t);
Vec3 Rotate(Quat q,Vec3 v);
Matrix4 Matrix(const Transform& transform);
Matrix4 Multiply(const Matrix4& a,const Matrix4& b);
Vec3 Point(const Matrix4& m,Vec3 v);
Vec3 Direction(const Matrix4& m,Vec3 v);
struct Bone {
    std::int32_t parent=-1;
    std::string name;
    Transform rest;
    Matrix4 inverseBind{};
    Vec3 tail,low,high;
    bool bounded=false;
};
enum class SpringGroup:std::uint32_t {Bangs=0,SideLocks=1,BackHair=2,TopStrand=3,Skirt=4,Sleeves=5};
struct SpringBone {
    std::uint32_t bone=0;
    SpringGroup group=SpringGroup::BackHair;
    std::uint32_t collisionMask=0;
    float stiffness=0,damping=0,gravity=0,radius=0,angle=0;
};
struct Collider {std::uint32_t bone=0,mask=0;Vec3 a,b;float radius=0;};
struct Anchor {std::uint32_t a=0,b=0;Vec3 localA,localB;float mix=0;};
struct Rig {
    std::vector<Bone> bones;
    std::vector<SpringBone> springs;
    std::vector<Collider> colliders;
    std::array<Anchor,15> anchors;
    std::array<std::uint32_t,8> roles{};
    std::array<Vec3,2> footGround{};
};
struct Model;
bool SampleLocal(const Model& model,double seconds,std::vector<Transform>& local,std::array<float,5>& expression);
void Compose(const Rig& rig,const std::vector<Transform>& local,std::vector<Matrix4>& world);
void StabilizeFeet(const Rig& rig,std::vector<Transform>& local,std::vector<Matrix4>& world,float lift);
void PackFrame(const Model& model,const std::vector<Matrix4>& world,const std::array<float,5>& expression,std::vector<float>& output);
enum class Dynamics {Off,Hair,Full};
struct MotionStats {unsigned steps=0,resets=0,lastSteps=0;float maxAngle=0,maxPenetration=0;};
struct SpringState {Vec3 tip,velocity;};
struct WorldCollider {Vec3 a,b;float radius=0;std::uint32_t mask=0;};
Vec3 ConstrainTip(Vec3 origin,Vec3 target,Vec3 tip,float length,const SpringBone& spring,const std::vector<WorldCollider>& colliders);
float Separation(Vec3 point,const WorldCollider& collider);
class Simulation {
    const Model* model_=nullptr;
    std::int64_t tick_=-1;
    double lastTime_=-1;
    std::uint32_t seed_=0x57495a23;
    Dynamics dynamics_=Dynamics::Full;
    std::vector<Transform> local_;
    std::vector<Matrix4> animated_,world_;
    std::vector<SpringState> springs_;
    std::vector<WorldCollider> colliders_;
    std::array<int,128> springForBone_{};
    std::array<float,5> expression_{};
    MotionStats stats_;
    void Pose(double time);
    void Step(double time,bool initialize);
public:
    static constexpr double stepSeconds=1./60.;
    static constexpr unsigned maxCatchUp=6,warmSteps=10;
    void Reset();
    void SetSeed(std::uint32_t seed);
    void SetDynamics(Dynamics mode);
    void Advance(const Model& model,double seconds);
    void FrameData(std::vector<float>& frame) const;
    const std::vector<Matrix4>& World() const {return world_;}
    const std::vector<SpringState>& Springs() const {return springs_;}
    const MotionStats& Stats() const {return stats_;}
    const std::array<float,5>& Expressions() const {return expression_;}
    std::size_t StorageBytes() const;
    double Time() const {return tick_*stepSeconds;}
};
}
