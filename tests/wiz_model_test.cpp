#include "../src/main/jni/ImGui/WizModel.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
int main(int argc,char** argv) {
    assert(argc==2);std::ifstream input(argv[1],std::ios::binary);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)),{});
    wiz_model::Model model;assert(wiz_model::Decode(bytes.data(),bytes.size(),model));
    assert(model.bones==98&&model.vertices.size()==17530&&model.frames==289);
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
            assert(std::fabs(change)<.0002f);
        }
    }
    assert(blink>.85f&&smile>.6f&&lift>.15f&&handTravel>.25f);
    wiz_model::Model invalid;assert(!wiz_model::Decode(nullptr,0,invalid));
    assert(!wiz_model::Decode(bytes.data(),bytes.size()/2,invalid));
    bytes[0]^=1;assert(!wiz_model::Decode(bytes.data(),bytes.size(),invalid));
    std::printf("Wiz: %zu vertices, %zu triangles, 98 bones, real blink/smile morphs, articulated wave, %.3fm hop, planted feet, seamless sampling and malformed-data rejection passed\n",model.vertices.size(),model.indices.size()/3,lift);
}
