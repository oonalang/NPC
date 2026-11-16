#include "WizMotion.h"
#include "WizModel.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace wiz_model {
namespace {
constexpr float pi=3.14159265358979323846f;
float Clamp(float v,float a,float b) {return std::max(a,std::min(b,v));}
Vec3 Cross(Vec3 a,Vec3 b) {return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
float Smooth(float t) {t=Clamp(t,0,1);return t*t*(3-2*t);}
std::uint32_t Hash(std::uint32_t n) {n^=n>>16;n*=0x7feb352d;n^=n>>15;n*=0x846ca68b;n^=n>>16;return n;}
float Random(std::uint32_t n) {return (Hash(n)&0xffffff)/16777215.f;}
float Noise(double time,double period,std::uint32_t seed) {
    const double phase=std::max(0.,time)/period;const auto index=static_cast<std::uint32_t>(std::floor(phase));
    const float blend=Smooth(static_cast<float>(phase-index));
    return ((1-blend)*Random(seed+index)+blend*Random(seed+index+1))*2-1;
}
Quat FromTo(Vec3 a,Vec3 b) {
    a=Normalize(a);b=Normalize(b);const auto d=Clamp(Dot(a,b),-1,1);
    if(d<-.9999f) return AxisAngle(Normalize(Cross(a,{1,0,0}),Normalize(Cross(a,{0,0,1}))),pi);
    const auto axis=Cross(a,b);return Normalize(Quat{axis.x,axis.y,axis.z,1+d});
}
Quat Rotation(const Matrix4& m) {
    const float trace=m[0]+m[5]+m[10];Quat q;
    if(trace>0) {const float s=2*std::sqrt(1+trace);q={(m[6]-m[9])/s,(m[8]-m[2])/s,(m[1]-m[4])/s,.25f*s};}
    else if(m[0]>m[5]&&m[0]>m[10]) {const float s=2*std::sqrt(1+m[0]-m[5]-m[10]);q={.25f*s,(m[4]+m[1])/s,(m[8]+m[2])/s,(m[6]-m[9])/s};}
    else if(m[5]>m[10]) {const float s=2*std::sqrt(1+m[5]-m[0]-m[10]);q={(m[4]+m[1])/s,.25f*s,(m[9]+m[6])/s,(m[8]-m[2])/s};}
    else {const float s=2*std::sqrt(1+m[10]-m[0]-m[5]);q={(m[8]+m[2])/s,(m[9]+m[6])/s,.25f*s,(m[1]-m[4])/s};}
    return Normalize(q);
}
Vec3 Closest(Vec3 point,Vec3 a,Vec3 b) {const auto axis=b-a;return a+axis*Clamp(Dot(point-a,axis)/std::max(1e-8f,Dot(axis,axis)),0,1);}
Vec3 Cone(Vec3 direction,Vec3 rest,float limit) {
    direction=Normalize(direction,rest);rest=Normalize(rest);
    const float angle=std::acos(Clamp(Dot(direction,rest),-1,1));
    if(angle<=limit) return direction;
    const auto tangent=Normalize(direction-rest*Dot(direction,rest),Normalize(Cross(rest,{1,0,0}),{0,0,1}));
    return rest*std::cos(limit)+tangent*std::sin(limit);
}
}
float Dot(Vec3 a,Vec3 b) {return a.x*b.x+a.y*b.y+a.z*b.z;}
float Length(Vec3 v) {return std::sqrt(Dot(v,v));}
Vec3 Normalize(Vec3 v,Vec3 fallback) {const auto size=Length(v);return size>1e-8f?v/size:fallback;}
Quat Normalize(Quat q) {const float size=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);return size>1e-8f?Quat{q.x/size,q.y/size,q.z/size,q.w/size}:Quat{};}
Quat Inverse(Quat q) {q=Normalize(q);return {-q.x,-q.y,-q.z,q.w};}
Quat Multiply(Quat a,Quat b) {return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};}
Quat AxisAngle(Vec3 axis,float angle) {axis=Normalize(axis);const auto s=std::sin(angle*.5f);return {axis.x*s,axis.y*s,axis.z*s,std::cos(angle*.5f)};}
Quat Slerp(Quat a,Quat b,float t) {
    a=Normalize(a);b=Normalize(b);float d=a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w;
    if(d<0) {d=-d;b={-b.x,-b.y,-b.z,-b.w};}
    float x=1-t,y=t;
    if(d<.9995f) {const auto angle=std::acos(Clamp(d,-1,1)),inverse=1.f/std::sin(angle);x=std::sin((1-t)*angle)*inverse;y=std::sin(t*angle)*inverse;}
    return Normalize(Quat{a.x*x+b.x*y,a.y*x+b.y*y,a.z*x+b.z*y,a.w*x+b.w*y});
}
Vec3 Rotate(Quat q,Vec3 v) {const Vec3 u{q.x,q.y,q.z};return v+Cross(u,v)*(2*q.w)+Cross(u,Cross(u,v))*2;}
Matrix4 Matrix(const Transform& t) {
    const auto q=Normalize(t.rotation);const float x=q.x,y=q.y,z=q.z,w=q.w;
    return {(1-2*y*y-2*z*z)*t.scale.x,(2*x*y+2*z*w)*t.scale.x,(2*x*z-2*y*w)*t.scale.x,0,
        (2*x*y-2*z*w)*t.scale.y,(1-2*x*x-2*z*z)*t.scale.y,(2*y*z+2*x*w)*t.scale.y,0,
        (2*x*z+2*y*w)*t.scale.z,(2*y*z-2*x*w)*t.scale.z,(1-2*x*x-2*y*y)*t.scale.z,0,t.position.x,t.position.y,t.position.z,1};
}
Matrix4 Multiply(const Matrix4& a,const Matrix4& b) {Matrix4 m{};for(int c=0;c<4;++c) for(int r=0;r<4;++r) for(int k=0;k<4;++k) m[c*4+r]+=a[k*4+r]*b[c*4+k];return m;}
Vec3 Point(const Matrix4& m,Vec3 v) {return {m[0]*v.x+m[4]*v.y+m[8]*v.z+m[12],m[1]*v.x+m[5]*v.y+m[9]*v.z+m[13],m[2]*v.x+m[6]*v.y+m[10]*v.z+m[14]};}
Vec3 Direction(const Matrix4& m,Vec3 v) {return Point(m,v)-Point(m,{});}
bool SampleLocal(const Model& model,double seconds,std::vector<Transform>& local,std::array<float,5>& expression) {
    if(model.version<2||model.frames<2||model.duration<=0||model.rig.bones.size()!=model.bones||model.animation.size()!=model.Stride()*model.frames) return false;
    if(!std::isfinite(seconds)) seconds=0;
    const double phase=std::fmod(std::max(0.,seconds),model.duration)*model.fps;
    const auto first=std::min<std::size_t>(phase,model.frames-2),second=first+1;const auto mix=static_cast<float>(phase-first);
    const auto* a=model.animation.data()+model.Stride()*first;const auto* b=model.animation.data()+model.Stride()*second;
    local.resize(model.bones);
    for(unsigned i=0;i<model.bones;++i) {
        const auto* x=a+i*10;const auto* y=b+i*10;
        local[i]={{x[0]*(1-mix)+y[0]*mix,x[1]*(1-mix)+y[1]*mix,x[2]*(1-mix)+y[2]*mix},
            Slerp({x[3],x[4],x[5],x[6]},{y[3],y[4],y[5],y[6]},mix),
            {x[7]*(1-mix)+y[7]*mix,x[8]*(1-mix)+y[8]*mix,x[9]*(1-mix)+y[9]*mix}};
    }
    for(unsigned i=0;i<5;++i) expression[i]=a[model.bones*10+i]*(1-mix)+b[model.bones*10+i]*mix;
    return true;
}
void Compose(const Rig& rig,const std::vector<Transform>& local,std::vector<Matrix4>& world) {
    world.resize(local.size());for(unsigned i=0;i<local.size();++i) world[i]=rig.bones[i].parent<0?Matrix(local[i]):Multiply(world[rig.bones[i].parent],Matrix(local[i]));
}
void StabilizeFeet(const Rig& rig,std::vector<Transform>& local,std::vector<Matrix4>& world,float lift) {
    for(unsigned side=0;side<2;++side) {
        const unsigned foot=rig.anchors[12+side].a,shin=rig.bones[foot].parent,thigh=rig.bones[shin].parent;
        const auto hip=Point(world[thigh],{}),knee=Point(world[shin],{}),ankle=Point(world[foot],{}),target=rig.footGround[side]+Vec3{0,0,lift};
        const auto footRotation=Rotation(world[foot]);
        const float upper=Length(knee-hip),lower=Length(ankle-knee),distance=Clamp(Length(target-hip),.001f,upper+lower-.000001f);
        const auto forward=Normalize(target-hip),bend=Normalize(knee-hip-forward*Dot(knee-hip,forward),{0,-1,0});
        const float cosine=Clamp((upper*upper+distance*distance-lower*lower)/(2*upper*distance),-1,1);
        const auto solvedKnee=hip+forward*(upper*cosine)+bend*(upper*std::sqrt(std::max(0.f,1-cosine*cosine)));
        const auto swing=[&](unsigned joint,Vec3 from,Vec3 to) {
            const auto parent=rig.bones[joint].parent;const auto rotation=parent<0?Quat{}:Rotation(world[parent]);
            local[joint].rotation=Normalize(Multiply(Multiply(Inverse(rotation),Multiply(FromTo(from,to),rotation)),local[joint].rotation));
            Compose(rig,local,world);
        };
        swing(thigh,knee-hip,solvedKnee-hip);
        const auto currentKnee=Point(world[shin],{});swing(shin,Point(world[foot],{})-currentKnee,target-currentKnee);
        local[foot].rotation=Normalize(Multiply(Inverse(Rotation(world[shin])),footRotation));Compose(rig,local,world);
    }
}
void PackFrame(const Model& model,const std::vector<Matrix4>& world,const std::array<float,5>& expression,std::vector<float>& output,const Matrix4* projection,ScreenBounds* screen) {
    output.resize(model.bones*16+56);const auto offset=model.bones*16;
    if(screen) *screen=ScreenBounds{};
    Vec3 low{1e6f,1e6f,1e6f},high{-1e6f,-1e6f,-1e6f};
    for(unsigned i=0;i<model.bones;++i) {
        const auto matrix=Multiply(world[i],model.rig.bones[i].inverseBind);std::copy(matrix.begin(),matrix.end(),output.begin()+i*16);
    }
    for(const auto& vertex:model.vertices) {
        const Vec3 p{vertex.position[0]+vertex.blink[0]*expression[0]+vertex.smile[0]*expression[1]+vertex.speak[0]*expression[2],
            vertex.position[1]+vertex.blink[1]*expression[0]+vertex.smile[1]*expression[1]+vertex.speak[1]*expression[2],
            vertex.position[2]+vertex.blink[2]*expression[0]+vertex.smile[2]*expression[1]+vertex.speak[2]*expression[2]};
        Vec3 skinned;
        for(unsigned influence=0;influence<4;++influence) if(vertex.weights[influence]) {
            const auto* m=output.data()+vertex.joints[influence]*16;const float weight=vertex.weights[influence]/65535.f;
            skinned=skinned+Vec3{m[0]*p.x+m[4]*p.y+m[8]*p.z+m[12],m[1]*p.x+m[5]*p.y+m[9]*p.z+m[13],m[2]*p.x+m[6]*p.y+m[10]*p.z+m[14]}*weight;
        }
        low={std::min(low.x,skinned.x),std::min(low.y,skinned.y),std::min(low.z,skinned.z)};high={std::max(high.x,skinned.x),std::max(high.y,skinned.y),std::max(high.z,skinned.z)};
        if(projection&&screen) {
            const auto& m=*projection;const float w=m[3]*skinned.x+m[7]*skinned.y+m[11]*skinned.z+m[15];
            if(w>1e-6f) {
                const auto clip=Point(m,skinned);const float point[]{.5f+.5f*clip.x/w,.5f-.5f*clip.y/w};
                for(unsigned axis=0;axis<2;++axis) {screen->low[axis]=std::min(screen->low[axis],point[axis]);screen->high[axis]=std::max(screen->high[axis],point[axis]);}
            }
        }
    }
    std::copy_n(expression.begin(),3,output.begin()+offset);
    for(unsigned i=0;i<15;++i) {
        const auto& anchor=model.rig.anchors[i];const auto p=Point(world[anchor.a],anchor.localA)*(1-anchor.mix)+Point(world[anchor.b],anchor.localB)*anchor.mix;
        output[offset+3+i*3]=p.x;output[offset+4+i*3]=p.y;output[offset+5+i*3]=p.z;
    }
    output[offset+48]=low.x;output[offset+49]=low.y;output[offset+50]=low.z;output[offset+51]=high.x;output[offset+52]=high.y;output[offset+53]=high.z;
    output[offset+54]=expression[3];output[offset+55]=expression[4];
}
float Separation(Vec3 p,const WorldCollider& collider) {return Length(p-Closest(p,collider.a,collider.b))-collider.radius;}
Vec3 ConstrainTip(Vec3 origin,Vec3 target,Vec3 tip,float length,const SpringBone& spring,const std::vector<WorldCollider>& colliders) {
    const auto rest=Normalize(target-origin);
    const auto tangent=Normalize(Cross(rest,{0,0,1}),Normalize(Cross(rest,{1,0,0})));
    for(unsigned iteration=0;iteration<16;++iteration) {
        tip=origin+Cone(tip-origin,rest,spring.angle)*length;
        bool clear=true;
        for(const auto& collider:colliders) if(collider.mask&spring.collisionMask) {
            const auto closest=Closest(tip,collider.a,collider.b),away=tip-closest;const auto radius=collider.radius+spring.radius;
            if(Length(away)<radius) {tip=closest+Normalize(away,tangent)*radius;clear=false;}
        }
        if(clear) break;
    }
    tip=origin+Cone(tip-origin,rest,spring.angle)*length;
    const auto error=[&](Vec3 p) {float value=0;for(const auto& c:colliders) if(c.mask&spring.collisionMask) {const float penetration=std::max(0.f,spring.radius-Separation(p,c));value+=penetration*penetration;}return value;};
    float bestError=error(tip);
    if(bestError>1e-8f) {
        const auto bitangent=Cross(rest,tangent);
        for(unsigned ring=1;ring<=3;++ring) for(unsigned sample=0;sample<16;++sample) {
            const float angle=spring.angle*ring/3.f,phase=sample*2*pi/16;
            const auto candidate=origin+(rest*std::cos(angle)+(tangent*std::cos(phase)+bitangent*std::sin(phase))*std::sin(angle))*length;
            const auto candidateError=error(candidate);
            if(candidateError<bestError) {tip=candidate;bestError=candidateError;}
        }
    }
    return tip;
}
void Simulation::Reset() {model_=nullptr;tick_=-1;lastTime_=-1;}
void Simulation::SetSeed(std::uint32_t seed) {if(seed_!=seed) {seed_=seed;Reset();}}
void Simulation::SetDynamics(Dynamics mode) {if(dynamics_!=mode) {dynamics_=mode;Reset();}}
SpringSample IntegrateSpring(Vec3 position,Vec3 velocity,Vec3 previousTarget,Vec3 target,Vec3 gravity,float stiffness,float damping,float dt) {
    if(dt<=0) return {position,velocity};
    const auto targetVelocity=(target-previousTarget)/dt;
    const auto equilibrium=gravity/stiffness;
    const auto displacement=position-previousTarget-equilibrium,relativeVelocity=velocity-targetVelocity;
    const float half=damping*.5f,frequencySquared=stiffness-half*half;
    float positionFactor,velocityFactor,derivativeFactor;
    if(frequencySquared>1e-5f) {
        const float frequency=std::sqrt(frequencySquared),decay=std::exp(-half*dt);
        positionFactor=decay*std::cos(frequency*dt);velocityFactor=decay*std::sin(frequency*dt)/frequency;
        derivativeFactor=-frequencySquared*velocityFactor;
    } else if(frequencySquared< -1e-5f) {
        const float frequency=std::sqrt(-frequencySquared),a=std::exp((-half+frequency)*dt),b=std::exp((-half-frequency)*dt);
        positionFactor=(a+b)*.5f;velocityFactor=(a-b)/(2*frequency);derivativeFactor=-frequencySquared*velocityFactor;
    } else {positionFactor=std::exp(-half*dt);velocityFactor=positionFactor*dt;derivativeFactor=0;}
    const auto coefficient=relativeVelocity+displacement*half;
    const auto offset=displacement*positionFactor+coefficient*velocityFactor;
    return {target+equilibrium+offset,targetVelocity+displacement*derivativeFactor+coefficient*positionFactor-offset*half};
}
double GestureTime(double seconds,std::uint32_t seed) {
    if(seconds<=12) return seconds;
    const auto block=static_cast<std::uint32_t>((seconds-12)/20);
    const double phase=std::fmod(seconds-12,20.);
    const double wave=2.5+Random(seed+block*23)*3.5,hop=11.5+Random(seed+block*23+1)*4;
    if(phase>=wave&&phase<wave+2.97) return 2.98+phase-wave;
    if(phase>=hop&&phase<hop+1.26) return 7.4+phase-hop;
    return 1.;
}
void Simulation::Pose(double time) {
    SampleLocal(*model_,model_->version>=3?GestureTime(time,seed_):time,local_,expression_);
    const auto& roles=model_->rig.roles;
    const auto add=[&](unsigned bone,Vec3 angles) {
        const auto delta=Multiply(AxisAngle({0,0,1},angles.z),Multiply(AxisAngle({0,1,0},angles.y),AxisAngle({1,0,0},angles.x)));
        const auto basis=Rotation(model_->rig.bones[bone].inverseBind);
        local_[bone].rotation=Normalize(Multiply(local_[bone].rotation,Multiply(basis,Multiply(delta,Inverse(basis)))));
    };
    const float breath=std::sin(float(std::fmod(time,4.17))*2*pi/4.17f);
    const auto adjustmentCycle=static_cast<std::uint32_t>(time/9);
    const float adjustmentPhase=static_cast<float>(std::fmod(time,9.)),adjustmentStart=1.2f+1.7f*Random(seed_+adjustmentCycle*29);
    const float adjustment=Smooth((adjustmentPhase-adjustmentStart)/.9f)*(1-Smooth((adjustmentPhase-adjustmentStart-2.3f)/.9f))*(1-Smooth((expression_[1]-.24f)/.3f));
    const float sway=Noise(time,3.4,seed_+1),turn=Noise(time,2.7,seed_+4),nod=Noise(time,3.1,seed_+5);
    const unsigned pelvis=model_->rig.anchors[3].a;
    local_[pelvis].position=local_[pelvis].position+Vec3{.025f*sway,.005f*Noise(time,4.3,seed_+2),0};
    add(pelvis,{0,.025f*sway,.018f*Noise(time,5.1,seed_+3)});
    add(roles[4],{.014f*breath,-.036f*sway,.025f*Noise(time,4.6,seed_+12)});
    add(roles[5],{.006f*breath,-.012f*sway,0});
    const auto chestParent=model_->rig.bones[roles[4]].parent;
    local_[roles[4]].position=local_[roles[4]].position+Direction(model_->rig.bones[chestParent].inverseBind,{0,0,.002f*breath});
    add(roles[0],{.055f*nod+.025f*adjustment,.045f*Noise(time,3.9,seed_+6),.15f*turn});
    for(unsigned eye=2;eye<=3;++eye) {
        const auto& bind=model_->rig.bones[roles[eye]];
        const auto bindRotation=Rotation(bind.inverseBind);const auto gaze=Multiply(AxisAngle({0,0,1},.045f*Noise(time+.2,2.7,seed_+4)),AxisAngle({1,0,0},.024f*Noise(time+.16,3.1,seed_+5)));
        local_[roles[eye]].rotation=Multiply(local_[roles[eye]].rotation,Multiply(bindRotation,Multiply(gaze,Inverse(bindRotation))));
    }
    for(unsigned side=0;side<2;++side) {
        const auto hand=model_->rig.anchors[8+side].a;
        const auto forearm=model_->rig.bones[hand].parent,upper=model_->rig.bones[forearm].parent;
        const float sign=side? -1.f:1.f;
        add(upper,{.018f*Noise(time,3.2+side,seed_+31+side),sign*.025f*Noise(time-.18,3.6+side,seed_+34+side),.015f*breath});
        add(forearm,{.025f*Noise(time-.3,3.3+side,seed_+36+side),0,0});
        add(hand,{.03f*Noise(time-.5,2.9+side,seed_+38+side),.018f*Noise(time,3.8+side,seed_+41+side),0});
        if(side==1) {
            add(upper,{-.11f*adjustment,.06f*adjustment,-.05f*adjustment});
            add(forearm,{-.42f*adjustment,.08f*adjustment,0});
            add(hand,{.04f*adjustment,.1f*adjustment,0});
        }
        add(roles[6+side],{.035f*Noise(time,3.1+side*.23,seed_+43+side),0,0});
    }
    const auto block=static_cast<std::uint32_t>(std::floor(time/5));float blink=0;
    for(unsigned previous=0;previous<=1;++previous) {
        if(previous&&block==0) continue;
        const auto cycle=block-previous;
        const double start=cycle*5.+1.55+(cycle?(.7*Random(seed_+cycle*17)-.35):0);
        for(unsigned repeat=0;repeat<(Hash(seed_+cycle)%4==0?2u:1u);++repeat) {
            const float elapsed=static_cast<float>(time-start-repeat*.26);
            if(elapsed>=0&&elapsed<.18f) blink=std::max(blink,elapsed<.065f?Smooth(elapsed/.065f):1-Smooth((elapsed-.065f)/.115f));
        }
    }
    expression_[0]=blink;expression_[1]=Clamp(expression_[1]+.09f*Noise(time,3.6,seed_+23),0,1);
    Compose(model_->rig,local_,animated_);
    StabilizeFeet(model_->rig,local_,animated_,expression_[3]);
}
void Simulation::FitSprings(bool integrate,bool initialize) {
    const auto& rig=model_->rig;const auto dt=static_cast<float>(stepSeconds);
    for(unsigned i=0;i<model_->bones;++i) {
        const auto parent=rig.bones[i].parent;const auto parentMatrix=parent<0?Matrix(Transform{}):world_[parent];
        world_[i]=Multiply(parentMatrix,Matrix(local_[i]));const int index=springForBone_[i];if(index<0) continue;
        const auto& spring=rig.springs[index];auto& state=springs_[index];
        const auto origin=Point(world_[i],{}),target=Point(world_[i],rig.bones[i].tail);const float length=Length(target-origin);
        if(integrate) {
            if(initialize) {state.tip=ConstrainTip(origin,target,target+Vec3{0,0,-9.81f*spring.gravity/spring.stiffness},length,spring,colliders_);state.velocity={};state.target=target;}
            const auto next=IntegrateSpring(state.tip,state.velocity,state.target,target,{0,0,-9.81f*spring.gravity},spring.stiffness,spring.damping,dt);
            state.tip=next.position;state.velocity=next.velocity;state.target=target;predicted_[index]=next.position;
        }
        state.target=target;
        const bool active=dynamics_==Dynamics::Full||(dynamics_==Dynamics::Hair&&static_cast<unsigned>(spring.group)<4);
        if(!active) {state.tip=target;state.velocity={};continue;}
        state.tip=ConstrainTip(origin,target,state.tip,length,spring,colliders_);
        stats_.maxAngle=std::max(stats_.maxAngle,std::acos(Clamp(Dot(Normalize(target-origin),Normalize(state.tip-origin)),-1,1)));
        const auto swing=FromTo(target-origin,state.tip-origin),parentRotation=Rotation(parentMatrix);
        local_[i].rotation=Normalize(Multiply(Multiply(Inverse(parentRotation),Multiply(swing,parentRotation)),local_[i].rotation));
        world_[i]=Multiply(parentMatrix,Matrix(local_[i]));
    }
}
void Simulation::Step(double time,bool initialize) {
    Pose(time);const auto& rig=model_->rig;const float dt=static_cast<float>(stepSeconds);
    colliders_.resize(rig.colliders.size());
    for(unsigned i=0;i<rig.colliders.size();++i) {const auto& c=rig.colliders[i];colliders_[i]={Point(animated_[c.bone],c.a),Point(animated_[c.bone],c.b),c.radius,c.mask};}
    world_.resize(model_->bones);predicted_.resize(springs_.size());linkLambda_.assign(rig.links.size(),0);
    std::array<Quat,64> animatedRotations{};
    for(unsigned i=0;i<rig.springs.size();++i) animatedRotations[i]=local_[rig.springs[i].bone].rotation;
    const auto fit=[&](bool integrate) {for(unsigned i=0;i<rig.springs.size();++i) local_[rig.springs[i].bone].rotation=animatedRotations[i];FitSprings(integrate,initialize);};
    const unsigned iterations=dynamics_==Dynamics::Full&&!rig.links.empty()?clothIterations:1;
    for(unsigned iteration=0;iteration<iterations;++iteration) {
        fit(iteration==0);
        if(iteration+1==iterations) break;
        for(unsigned i=0;i<rig.links.size();++i) {
            const auto& link=rig.links[i];auto& a=springs_[link.a];auto& b=springs_[link.b];const auto delta=b.tip-a.tip;
            const float length=Length(delta),alpha=link.compliance/(dt*dt);
            if(length<1e-6f) continue;
            const float impulse=(-(length-link.length)-alpha*linkLambda_[i])/(2+alpha);linkLambda_[i]+=impulse;
            a.tip=a.tip-delta*(impulse/length);b.tip=b.tip+delta*(impulse/length);
        }
    }
    for(unsigned i=0;i<springs_.size();++i) {
        auto& state=springs_[i];const auto& spring=rig.springs[i];
        const bool active=dynamics_==Dynamics::Full||(dynamics_==Dynamics::Hair&&unsigned(spring.group)<4);
        if(active) {state.velocity=state.velocity+(state.tip-predicted_[i])/dt;const float speed=Length(state.velocity);if(speed>4) state.velocity=state.velocity*(4/speed);}
        if(active) for(const auto& collider:colliders_) if(collider.mask&spring.collisionMask) stats_.maxPenetration=std::max(stats_.maxPenetration,spring.radius-Separation(state.tip,collider));
    }
    if(dynamics_==Dynamics::Full) for(const auto& link:rig.links) stats_.maxLinkStretch=std::max(stats_.maxLinkStretch,std::fabs(Length(springs_[link.a].tip-springs_[link.b].tip)/link.length-1));
}
void Simulation::Advance(const Model& model,double seconds) {
    stats_.lastSteps=0;if(model.version<2) return;
    if(!std::isfinite(seconds)) seconds=0;seconds=std::max(0.,std::min(1e9,seconds));
    const auto target=static_cast<std::int64_t>(std::floor(seconds/stepSeconds+1e-7));
    const bool reset=model_!=&model||tick_<0||seconds<lastTime_-1e-8||target-tick_>maxCatchUp;
    if(reset) {
        model_=&model;springForBone_.fill(-1);springs_.resize(model.rig.springs.size());
        for(unsigned i=0;i<model.rig.springs.size();++i) springForBone_[model.rig.springs[i].bone]=i;
        tick_=target;Step(tick_*stepSeconds,true);
        for(unsigned i=0;i<warmSteps;++i) Step(tick_*stepSeconds,false);
        stats_.lastSteps=warmSteps+1;stats_.steps+=stats_.lastSteps;++stats_.resets;
    } else while(tick_<target) {++tick_;Step(tick_*stepSeconds,false);++stats_.steps;++stats_.lastSteps;}
    lastTime_=seconds;
}
void Simulation::FrameData(std::vector<float>& frame,const Matrix4* projection,ScreenBounds* screen) const {if(model_) PackFrame(*model_,world_,expression_,frame,projection,screen);}
std::size_t Simulation::StorageBytes() const {return local_.capacity()*sizeof(Transform)+(world_.capacity()+animated_.capacity())*sizeof(Matrix4)+springs_.capacity()*sizeof(SpringState)+colliders_.capacity()*sizeof(WorldCollider)+predicted_.capacity()*sizeof(Vec3)+linkLambda_.capacity()*sizeof(float);}
}
