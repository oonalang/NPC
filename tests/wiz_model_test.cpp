#include "../src/main/jni/ImGui/WizModel.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstring>
#include <limits>
#include <zlib.h>
#include "../src/main/jni/ImGui/PreviewOrbit.h"
namespace {
using namespace wiz_model;
std::vector<unsigned char> Repack(const std::vector<unsigned char>& raw,const char* magic="WIZSKN02") {
    std::vector<unsigned char> bytes(magic,magic+8);const std::uint32_t size=raw.size();
    const auto* p=reinterpret_cast<const unsigned char*>(&size);bytes.insert(bytes.end(),p,p+4);
    uLongf length=compressBound(raw.size());const auto first=bytes.size();bytes.resize(first+length);
    assert(compress2(bytes.data()+first,&length,raw.data(),raw.size(),6)==Z_OK);bytes.resize(first+length);return bytes;
}
void ParserTests(const std::vector<unsigned char>& packed,const Model& model) {
    std::uint32_t size;std::memcpy(&size,packed.data()+8,4);std::vector<unsigned char> raw(size);uLongf count=size;
    assert(uncompress(raw.data(),&count,packed.data()+12,packed.size()-12)==Z_OK);
    const auto word=[&](std::size_t offset) {std::uint32_t v;std::memcpy(&v,raw.data()+offset,4);return v;};
    std::size_t rig=32+model.vertices.size()*sizeof(Vertex)+model.indices.size()*4;
    for(unsigned i=0;i<model.materials.size();++i) rig+=12+word(rig+8);
    const auto reject=[&](std::size_t offset,std::uint32_t value) {
        auto broken=raw;std::memcpy(broken.data()+offset,&value,4);const auto bytes=Repack(broken);Model output;assert(!Decode(bytes.data(),bytes.size(),output));
    };
    reject(0,3);reject(12,129);reject(rig+4,65);reject(rig+16,0);reject(rig+20,10000);
    std::size_t spring=rig+16;for(unsigned i=0;i<model.bones;++i) spring+=152+word(spring+4);
    reject(spring,model.bones);reject(spring+4,6);reject(spring+12,0x7fc00000);
    const auto animation=raw.size()-model.animation.size()*4;
    reject(animation+12,0x7fc00000);reject(animation+12,0x40000000);reject(animation+28,0);
    auto badTexture=model;badTexture.materials[0].png[20]^=1;assert(!DecodeTextures(badTexture));
    std::vector<unsigned char> old;
    const auto append=[&](const auto& value) {const auto* first=reinterpret_cast<const unsigned char*>(&value);old.insert(old.end(),first,first+sizeof(value));};
    for(std::uint32_t value:{1u,3u,3u,1u,2u,1u,1u}) append(value);append(1.f);
    for(unsigned i=0;i<3;++i) {Vertex v=model.vertices[i];for(unsigned j=0;j<4;++j) {v.joints[j]=0;v.weights[j]=j?0:65535;}append(v);}
    for(std::uint32_t value:{0u,1u,2u,0u,3u,std::uint32_t(model.materials[0].png.size())}) append(value);
    old.insert(old.end(),model.materials[0].png.begin(),model.materials[0].png.end());
    for(unsigned frame=0;frame<2;++frame) for(unsigned i=0;i<72;++i) append(i<16?(i%5==0?1.f:i==12?.2f*frame:0.f):0.f);
    const auto legacy=Repack(old,"WIZSKN01");Model v1;assert(Decode(legacy.data(),legacy.size(),v1)&&v1.version==1);
    std::vector<float> sampled;assert(Sample(v1,.5,sampled)&&std::fabs(sampled[12]-.1f)<1e-6f);assert(DecodeTextures(v1));
}
void MotionTests(const Model& model) {
    const auto midpoint=Slerp(AxisAngle({0,0,1},2.96705973f),AxisAngle({0,0,1},-2.96705973f),.5f);
    assert(Rotate(midpoint,{1,0,0}).x<-.999f);
    std::array<unsigned,6> groups{};for(const auto& s:model.rig.springs) ++groups[unsigned(s.group)];
    assert((groups==std::array<unsigned,6>{2,6,9,3,8,4}));
    std::vector<Matrix4> expected;float largestFootError=0,settledSpeed=0,peakSpeed=0;unsigned settledSamples=0;
    std::vector<float> frame;std::array<float,6> motion{};
    for(int rate:{30,60,120}) {
        Simulation sim;
        for(int i=0;i<=12*rate;++i) {
            const double time=double(i)/rate;sim.Advance(model,time);const auto before=sim.Stats().steps;sim.Advance(model,time);assert(before==sim.Stats().steps);
            assert(sim.Stats().lastSteps==0);assert(sim.Stats().resets==1);
            for(const auto& world:sim.World()) for(float value:world) assert(std::isfinite(value));
            for(unsigned side=0;side<2;++side) {
                const auto ankle=Point(sim.World()[model.rig.anchors[12+side].a],{});
                largestFootError=std::max(largestFootError,Length(ankle-model.rig.footGround[side]-Vec3{0,0,sim.Expressions()[3]}));
            }
            for(unsigned spring=0;spring<model.rig.springs.size();++spring) {
                const auto& def=model.rig.springs[spring];const auto& state=sim.Springs()[spring];const auto& world=sim.World()[def.bone];
                assert(Length(state.tip-Point(world,model.rig.bones[def.bone].tail))<.00005f);
                const auto parent=model.rig.bones[def.bone].parent;
                const float length=Length(Direction(world,model.rig.bones[def.bone].tail));
                assert(std::fabs(length-Length(model.rig.bones[def.bone].tail))<.00005f);
                motion[unsigned(def.group)]=std::max(motion[unsigned(def.group)],Length(state.velocity));
            }
            if(rate==60) {
                float speed=0;for(const auto& spring:sim.Springs()) speed+=Length(spring.velocity);
                if(time>8&&time<9) peakSpeed=std::max(peakSpeed,speed);
                if(time>10&&time<11) {settledSpeed+=speed;++settledSamples;}
            }
        }
        assert(sim.Stats().steps==Simulation::warmSteps+1+720&&sim.Stats().maxPenetration<.0001f&&sim.Stats().maxAngle<.3201f);
        if(expected.empty()) expected=sim.World();else for(unsigned i=0;i<expected.size();++i) for(unsigned j=0;j<16;++j) assert(std::fabs(expected[i][j]-sim.World()[i][j])<1e-6f);
        const auto bytes=sim.StorageBytes();sim.Advance(model,6000000);assert(sim.Stats().lastSteps<=Simulation::warmSteps+1&&sim.StorageBytes()==bytes);
        sim.Advance(model,1);assert(sim.Stats().lastSteps==Simulation::warmSteps+1);sim.Advance(model,std::numeric_limits<double>::quiet_NaN());assert(sim.Time()==0);
        sim.SetDynamics(Dynamics::Off);sim.Advance(model,3);for(const auto& spring:sim.Springs()) assert(Length(spring.velocity)==0);
        sim.SetDynamics(Dynamics::Hair);sim.Advance(model,3);for(unsigned i=0;i<model.rig.springs.size();++i) if(unsigned(model.rig.springs[i].group)>=4) assert(Length(sim.Springs()[i].velocity)==0);
    }
    assert(largestFootError<.00005f);for(float peak:motion) assert(peak>.001f);
    assert(settledSpeed/settledSamples<peakSpeed*.65f);
    Simulation a,b;a.SetSeed(2);b.SetSeed(3);a.Advance(model,1);b.Advance(model,1);const auto head=model.rig.roles[0];
    assert(Length(Point(a.World()[head],{0,.1f,0})-Point(b.World()[head],{0,.1f,0}))>.0001f);
    SpringBone spring;spring.angle=.4f;spring.radius=.01f;spring.collisionMask=1;
    const Vec3 origin{0,0,1},target{0,.4f,1};const std::vector<WorldCollider> obstacles{{target,target,.04f,1}};
    const auto tip=ConstrainTip(origin,target,target,.4f,spring,obstacles);
    assert(std::fabs(Length(tip-origin)-.4f)<1e-6f&&Separation(tip,obstacles[0])>=spring.radius-.0001f);
    const auto limited=ConstrainTip(origin,target,{4,2,5},.4f,spring,{});
    assert(std::acos(Dot(Normalize(limited-origin),Normalize(target-origin)))<spring.angle+.0001f);
    anime_preview::OrbitControl orbit;orbit.Update(true,true,true,100,100,200);assert(orbit.yaw==0);
    orbit.Toggle();orbit.Update(true,true,true,100,100,200);orbit.Update(false,true,true,101,140,200);assert(!orbit.dragging&&orbit.yaw==0);
    orbit.Update(false,false,false,100,100,200);orbit.Update(true,true,true,100,100,200);orbit.Update(false,true,true,180,103,200);assert(orbit.dragging&&orbit.yaw>.9f);
    orbit.Update(false,true,true,10000,100,200);assert(orbit.yaw<=1.570797f);orbit.Toggle();assert(!orbit.enabled&&orbit.yaw==0);
    std::printf("Physics: deterministic 30/60/120-Hz inputs, 60-Hz fixed steps, %zu bytes stable scratch, collision separation, joint limits, %.7fm planted-foot error, landing settle %.3f/%.3f, seeds, pause/seek reset and scroll-safe orbit passed\n",a.StorageBytes(),largestFootError,settledSpeed/settledSamples,peakSpeed);
}
}
int main(int argc,char** argv) {
    assert(argc>=2);std::ifstream input(argv[1],std::ios::binary);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)),{});
    wiz_model::Model model;assert(wiz_model::Decode(bytes.data(),bytes.size(),model));
    assert(model.version==2&&model.bones==112&&model.vertices.size()==17530&&model.frames==289);
    assert(model.rig.springs.size()==32&&model.rig.colliders.size()==10);
    assert(wiz_model::DecodeTextures(model));
    assert(model.materials.size()==3);for(const auto& m:model.materials) assert(m.rgba.size()==std::size_t(m.width*m.height*4));
    std::vector<float> idle,frame,loop;assert(wiz_model::Sample(model,0,idle));assert(wiz_model::Sample(model,12,loop));assert(idle==loop);
    const auto offset=model.bones*16;
    float blink=0,smile=0,lift=0,handTravel=0;
    for(int i=0;i<720;++i) {
        assert(wiz_model::Sample(model,i/60.,frame));for(float v:frame) assert(std::isfinite(v));
        blink=std::max(blink,frame[offset]);smile=std::max(smile,frame[offset+1]);lift=std::max(lift,frame[offset+54]);
        handTravel=std::max(handTravel,std::fabs(frame[offset+3+8*3+2]-idle[offset+3+8*3+2]));
        for(int side=0;side<2;++side) for(int axis=0;axis<3;++axis) {
            const auto index=offset+3+(12+side)*3+axis;
            const float change=frame[index]-idle[index]-(axis==2?frame[offset+54]:0);
            assert(std::fabs(change)<.00005f);
        }
    }
    assert(blink>.85f&&smile>.6f&&lift>.15f&&handTravel>.25f);
    ParserTests(bytes,model);MotionTests(model);
    wiz_model::Model invalid;assert(!wiz_model::Decode(nullptr,0,invalid));
    assert(!wiz_model::Decode(bytes.data(),bytes.size()/2,invalid));
    bytes[0]^=1;assert(!wiz_model::Decode(bytes.data(),bytes.size(),invalid));
    if(argc>2) {
        std::ifstream legacyInput(argv[2],std::ios::binary);std::vector<unsigned char> data((std::istreambuf_iterator<char>(legacyInput)),{});
        wiz_model::Model legacy;assert(wiz_model::Decode(data.data(),data.size(),legacy)&&legacy.version==1&&legacy.bones==98);
        std::vector<float> pose;assert(wiz_model::Sample(legacy,8.1,pose)&&wiz_model::DecodeTextures(legacy));std::puts("Original shipped WIZSKN01 asset remains decodable, sampleable and textured");
    }
    std::printf("Wiz: %zu vertices, %zu triangles, %u bones, hierarchy, 32 weighted spring bones, real facial morphs, articulated wave, %.3fm hop, planted feet, quaternion sampling and malformed-data rejection passed\n",model.vertices.size(),model.indices.size()/3,model.bones,lift);
}
