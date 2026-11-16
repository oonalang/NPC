#include "../src/main/jni/ImGui/WizModel.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstring>
#include <limits>
#include <map>
#include <tuple>
#include <zlib.h>
#include "../src/main/jni/ImGui/PreviewOrbit.h"
namespace {
using namespace wiz_model;
std::vector<unsigned char> Repack(const std::vector<unsigned char>& raw,const char* magic="WIZSKN03") {
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
    reject(0,4);reject(12,129);reject(rig+4,65);reject(rig+16,129);reject(rig+20,0);reject(rig+24,10000);
    std::size_t spring=rig+20;for(unsigned i=0;i<model.bones;++i) spring+=152+word(spring+4);
    reject(spring,model.bones);reject(spring+4,7);reject(spring+12,0x7fc00000);
    const auto animation=raw.size()-model.animation.size()*4;
    const auto links=animation-model.rig.links.size()*16;
    reject(links,model.rig.springs.size());reject(links+4,word(links));reject(links+8,0);reject(links+12,0x7fc00000);
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
void IntegratorTests() {
    for(float damping:{5.f,20.f,30.f}) {
        SpringSample state{{.1f,0,0},{}};
        float previousEnergy=100*.1f*.1f*.5f;
        for(unsigned i=0;i<600;++i) {
            state=IntegrateSpring(state.position,state.velocity,{},{},{},100,damping,1.f/60);
            const float energy=.5f*(Dot(state.velocity,state.velocity)+100*Dot(state.position,state.position));
            assert(energy<=previousEnergy+1e-7f);previousEnergy=energy;
        }
        assert(Length(state.position)<1e-6f&&Length(state.velocity)<1e-6f);
    }
    for(float dt:{1.f/30,1.f/60,1.f/120}) {
        const Vec3 target{.4f,-.1f,.8f},velocity{.8f,.2f,-.3f},gravity{0,0,-1.2f};
        const auto steady=IntegrateSpring(target+gravity/100,velocity,target,target+velocity*dt,gravity,100,9,dt);
        assert(Length(steady.position-target-velocity*dt-gravity/100)<1e-6f);
        assert(Length(steady.velocity-velocity)<1e-5f);
    }
    const auto response=IntegrateSpring({},{},{},{.02f,0,0},{},100,9,1.f/60);
    assert(response.position.x>0&&response.position.x<.02f&&response.velocity.x>0);
    for(unsigned seed:{1u,2u,0x57495a23u}) {
        double previous=GestureTime(12,seed);unsigned waveStarts=0,hopStarts=0;
        for(unsigned i=1;i<=60*60;++i) {
            const double sampled=GestureTime(12+i/60.,seed);
            assert(sampled>=0&&sampled<=12);
            if(sampled>=2.98&&sampled<5.95&&!(previous>=2.98&&previous<5.95)) ++waveStarts;
            if(sampled>=7.4&&sampled<8.66&&!(previous>=7.4&&previous<8.66)) ++hopStarts;
            previous=sampled;
        }
        assert(waveStarts==3&&hopStarts==3);
    }
    std::puts("Dynamics: passive damping for under/critical/overdamped springs, gravity equilibrium, moving-frame invariance, acceleration lag and independently scheduled gestures passed");
}
void MotionTests(const Model& model) {
    const auto midpoint=Slerp(AxisAngle({0,0,1},2.96705973f),AxisAngle({0,0,1},-2.96705973f),.5f);
    assert(Rotate(midpoint,{1,0,0}).x<-.999f);
    std::array<unsigned,7> groups{};for(const auto& s:model.rig.springs) ++groups[unsigned(s.group)];
    assert((groups==std::array<unsigned,7>{2,6,9,3,16,4,2}));
    std::vector<Matrix4> expected;float largestFootError=0,settledSpeed=0,peakSpeed=0;unsigned settledSamples=0;
    std::vector<float> frame;std::array<float,7> motion{};
    for(int rate:{30,60,120}) {
        Simulation sim;
        for(int i=0;i<=48*rate;++i) {
            const double time=double(i)/rate;sim.Advance(model,time);const auto before=sim.Stats().steps;sim.Advance(model,time);assert(before==sim.Stats().steps);
            assert(sim.Stats().lastSteps==0);assert(sim.Stats().resets==1);
            for(const auto& world:sim.World()) for(float value:world) assert(std::isfinite(value));
            for(unsigned side=0;side<2;++side) {
                const auto ankle=Point(sim.World()[model.rig.anchors[12+side].a],{});
                largestFootError=std::max(largestFootError,Length(ankle-model.rig.footGround[side]-Vec3{0,0,sim.Expressions()[3]}));
                const auto wrist=model.rig.anchors[8+side].a,elbow=static_cast<unsigned>(model.rig.bones[wrist].parent);
                const auto shoulder=static_cast<unsigned>(model.rig.bones[elbow].parent);
                for(const auto pair:{std::pair<unsigned,unsigned>{shoulder,elbow},{elbow,wrist}}) {
                    const auto measured=Length(Point(sim.World()[pair.second],{})-Point(sim.World()[pair.first],{}));
                    assert(std::fabs(measured-Length(model.rig.bones[pair.second].rest.position))<.00005f);
                }
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
        assert(sim.Stats().steps==Simulation::warmSteps+1+48*60&&sim.Stats().maxPenetration<.0001f&&sim.Stats().maxAngle<.4601f&&sim.Stats().maxLinkStretch<.03f);
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
void GarmentTests(const Model& model) {
    std::vector<bool> garmentBone(model.bones),garment(model.vertices.size());
    for(const auto& spring:model.rig.springs) {
        if(spring.group==SpringGroup::Skirt) garmentBone[spring.bone]=true;
        if(spring.group==SpringGroup::Chest) {
            const auto offset=spring.bone*10;
            for(unsigned frame=1;frame<model.frames;++frame) for(unsigned component=0;component<10;++component)
                assert(std::fabs(model.animation[frame*model.Stride()+offset+component]-model.animation[offset+component])<1e-5f);
        }
    }
    std::map<std::tuple<int,int,int>,unsigned> positions;std::vector<std::pair<unsigned,unsigned>> seams;
    for(unsigned i=0;i<model.vertices.size();++i) {
        const auto& v=model.vertices[i];float weight=0;
        for(unsigned j=0;j<4;++j) if(garmentBone[v.joints[j]]) weight+=v.weights[j]/65535.f;
        if(weight<=.05f) continue;garment[i]=true;
        const auto key=std::make_tuple(int(std::lround(v.position[0]*1e6)),int(std::lround(v.position[1]*1e6)),int(std::lround(v.position[2]*1e6)));
        const auto found=positions.emplace(key,i);if(!found.second) seams.emplace_back(found.first->second,i);
    }
    assert(seams.size()>50);
    Simulation dynamic,reference;reference.SetDynamics(Dynamics::Off);
    std::vector<Matrix4> palette(model.bones),base(model.bones);
    std::vector<Vec3> points(model.vertices.size()),rest(model.vertices.size());
    float minimum=1,maximum=1,largestSeam=0,chestPeak=0,chestSettle=0;unsigned settledSamples=0;
    const auto skin=[&](const Vertex& v,const std::vector<Matrix4>& matrices) {
        Vec3 p;for(unsigned j=0;j<4;++j) if(v.weights[j]) p=p+Point(matrices[v.joints[j]],{v.position[0],v.position[1],v.position[2]})*(v.weights[j]/65535.f);return p;
    };
    for(unsigned tick=0;tick<=48*60;++tick) {
        const double time=tick/60.;dynamic.Advance(model,time);reference.Advance(model,time);
        for(unsigned i=0;i<model.rig.springs.size();++i) if(model.rig.springs[i].group==SpringGroup::Chest) {
            const auto offset=Length(dynamic.Springs()[i].tip-reference.Springs()[i].tip);
            if(time>7.8&&time<9) chestPeak=std::max(chestPeak,offset);
            if(time>10&&time<11) {chestSettle+=offset;++settledSamples;}
        }
        if(tick%3) continue;
        for(unsigned i=0;i<model.bones;++i) {palette[i]=Multiply(dynamic.World()[i],model.rig.bones[i].inverseBind);base[i]=Multiply(reference.World()[i],model.rig.bones[i].inverseBind);}
        for(unsigned i=0;i<points.size();++i) if(garment[i]) {points[i]=skin(model.vertices[i],palette);rest[i]=skin(model.vertices[i],base);}
        for(auto seam:seams) largestSeam=std::max(largestSeam,Length(points[seam.first]-points[seam.second]));
        for(unsigned triangle=0;triangle<model.indices.size();triangle+=3) for(unsigned edge=0;edge<3;++edge) {
            const auto a=model.indices[triangle+edge],b=model.indices[triangle+(edge+1)%3];
            if(!garment[a]||!garment[b]) continue;
            const float baseline=Length(rest[a]-rest[b]);if(baseline<.004f) continue;
            const float ratio=Length(points[a]-points[b])/baseline;minimum=std::min(minimum,ratio);maximum=std::max(maximum,ratio);
        }
    }
    std::printf("Garment: 48s skinned-edge ratios %.4f .. %.4f, %zu duplicate seams with %.7fm separation; maximum cloth-link strain %.4f; chest acceleration lag %.4fm / settled %.4fm\n",minimum,maximum,seams.size(),largestSeam,dynamic.Stats().maxLinkStretch,chestPeak,chestSettle/settledSamples);
    assert(minimum>.75f&&maximum<1.25f&&largestSeam<.00005f&&dynamic.Stats().maxLinkStretch<.03f);
    assert(chestPeak>.01f&&chestSettle/settledSamples<chestPeak*.6f);
}
}
int main(int argc,char** argv) {
    assert(argc>=2);std::ifstream input(argv[1],std::ios::binary);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)),{});
    wiz_model::Model model;assert(wiz_model::Decode(bytes.data(),bytes.size(),model));
    assert(model.version==3&&model.bones==120&&model.vertices.size()==17530&&model.frames==289);
    assert(model.rig.springs.size()==42&&model.rig.colliders.size()==16&&model.rig.links.size()==48);
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
    ParserTests(bytes,model);IntegratorTests();MotionTests(model);GarmentTests(model);
    wiz_model::Model invalid;assert(!wiz_model::Decode(nullptr,0,invalid));
    assert(!wiz_model::Decode(bytes.data(),bytes.size()/2,invalid));
    bytes[0]^=1;assert(!wiz_model::Decode(bytes.data(),bytes.size(),invalid));
    if(argc>2) {
        std::ifstream legacyInput(argv[2],std::ios::binary);std::vector<unsigned char> data((std::istreambuf_iterator<char>(legacyInput)),{});
        wiz_model::Model legacy;assert(wiz_model::Decode(data.data(),data.size(),legacy)&&legacy.version==1&&legacy.bones==98);
        std::vector<float> pose;assert(wiz_model::Sample(legacy,8.1,pose)&&wiz_model::DecodeTextures(legacy));std::puts("Original shipped WIZSKN01 asset remains decodable, sampleable and textured");
    }
    if(argc>3) {
        std::ifstream v2Input(argv[3],std::ios::binary);std::vector<unsigned char> data((std::istreambuf_iterator<char>(v2Input)),{});
        Model v2;assert(Decode(data.data(),data.size(),v2)&&v2.version==2&&v2.bones==112&&v2.rig.links.empty());
        Simulation sim;for(unsigned i=0;i<720;++i) sim.Advance(v2,i/60.);
        assert(DecodeTextures(v2));std::puts("Previous WIZSKN02 asset remains decodable and runs its spring rig");
    }
    std::printf("Wiz: %zu vertices, %zu triangles, %u bones, hierarchy, 42 weighted spring bones, real facial morphs, articulated wave, %.3fm hop, planted feet, quaternion sampling and malformed-data rejection passed\n",model.vertices.size(),model.indices.size()/3,model.bones,lift);
}
