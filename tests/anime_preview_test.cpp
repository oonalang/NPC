#define IMGUI_DEFINE_MATH_OPERATORS
#define CLAY_CPU_PREVIEW
#include "../src/main/jni/ImGui/AnimePreview.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>

ImFont* F50=nullptr;
ImFont* F107=nullptr;
float menu[4]={.69f,.43f,.96f,1.f};
namespace font { ImFont* inter_semibold=nullptr; ImFont* inter_title=nullptr; }

namespace {
constexpr int kMeshVertices=(anime_preview::columns+1)*(anime_preview::rows+1);

bool Finite(const ImVec2& value) { return std::isfinite(value.x) && std::isfinite(value.y); }

void CheckIndices(const ImDrawList& draw) {
    for(const auto& command : draw.CmdBuffer) {
        assert(command.IdxOffset+command.ElemCount<=static_cast<unsigned>(draw.IdxBuffer.Size));
        for(unsigned i=command.IdxOffset;i<command.IdxOffset+command.ElemCount;++i)
            assert(draw.IdxBuffer[i]+command.VtxOffset<static_cast<unsigned>(draw.VtxBuffer.Size));
    }
    for(const auto& vertex : draw.VtxBuffer) assert(Finite(vertex.pos) && Finite(vertex.uv));
}

struct Drawn {
    anime_preview::Bounds bounds;
    int addedVertices=0;
};

Drawn RenderFrame(double time,float delta,bool textured,const ImVec2& viewMin,const ImVec2& viewMax) {
    ImGui::NewFrame();
    ImGui::SetNextWindowPos({0,0},ImGuiCond_Always);
    ImGui::SetNextWindowSize({420,560},ImGuiCond_Always);
    ImGui::Begin("##anime_preview_host",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoSavedSettings|
        ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoMove);
    auto* draw=ImGui::GetWindowDrawList();
    anime_preview::SetArtwork(textured?reinterpret_cast<ImTextureID>(static_cast<std::intptr_t>(7)):nullptr,1024,1536);
    const int before=draw->VtxBuffer.Size;
    Drawn result;
    result.bounds=anime_preview::Draw(draw,viewMin,viewMax,time,delta);
    result.addedVertices=draw->VtxBuffer.Size-before;
    CheckIndices(*draw);
    ImGui::End();
    ImGui::Render();
    return result;
}
}

int main() {
    ImGui::CreateContext();
    auto& io=ImGui::GetIO();
    io.IniFilename=nullptr;
    io.DisplaySize=ImVec2(420,560);
    io.Fonts->AddFontDefault();
    io.Fonts->Build();

    {
        anime_preview::Motion a,b;
        const auto first=anime_preview::Sample(2.f,1.f/60.f,a);
        const auto second=anime_preview::Sample(2.f,1.f/60.f,b);
        assert(first.bodyY==second.bodyY && first.sway==second.sway && first.squash==second.squash);
        assert(first.chestY==second.chestY && first.hairY==second.hairY && first.skirtX==second.skirtX);
        assert(first.blink==second.blink && first.excited==second.excited);
        assert(anime_preview::Sample(std::nanf(""),1.f/60.f,a).bodyY==
            anime_preview::Sample(0.f,1.f/60.f,b).bodyY);
    }

    {
        anime_preview::Motion motion;
        float lowest=0.f,highest=0.f,hairPeak=0.f,skirtPeak=0.f,chestPeak=0.f;
        bool airborne=false,landed=false;
        float previous=0.f;
        for(int i=0;i<=1440;++i) {
            const auto time=i/100.f;
            const auto pose=anime_preview::Sample(time,1.f/100.f,motion);
            assert(std::isfinite(pose.bodyY) && std::isfinite(pose.sway) && std::isfinite(pose.squash));
            assert(std::isfinite(pose.hairX) && std::isfinite(pose.hairY) && std::isfinite(pose.skirtX));
            assert(pose.blink>=0.f && pose.blink<=1.f);
            assert(std::fabs(pose.chestY)<=.019f && std::fabs(pose.hairX)<=.026f);
            assert(std::fabs(pose.hairY)<=.033f && std::fabs(pose.skirtX)<=.021f);
            assert(pose.bodyY>=-.3f && pose.bodyY<=.05f);
            assert(pose.squash>.9f && pose.squash<1.06f);
            assert(std::fabs(pose.bodyY-previous)<.03f);
            previous=pose.bodyY;
            lowest=std::fmin(lowest,pose.bodyY);
            highest=std::fmax(highest,pose.bodyY);
            hairPeak=std::fmax(hairPeak,std::fabs(pose.hairY));
            skirtPeak=std::fmax(skirtPeak,std::fabs(pose.skirtX));
            chestPeak=std::fmax(chestPeak,std::fabs(pose.chestY));
            if(pose.excited && pose.bodyY<-.02f) airborne=true;
            if(pose.squash<.975f) landed=true;
        }
        assert(airborne && landed);
        assert(lowest<-.02f && highest<.01f);
        assert(hairPeak>.001f && skirtPeak>.0001f && chestPeak>.0001f);
    }

    {
        anime_preview::SetArtwork(nullptr,1024,1536);
        anime_preview::BuildWeights();
        assert(anime_preview::weights.size()==static_cast<std::size_t>(kMeshVertices));
        for(const auto& weight : anime_preview::weights) {
            const auto direct=anime_preview::WeightAt(weight.uv);
            assert(direct.chest==weight.chest && direct.hair==weight.hair);
            assert(direct.skirt==weight.skirt && direct.arm==weight.arm && direct.side==weight.side);
            assert(direct.eye[0]==weight.eye[0] && direct.eye[1]==weight.eye[1]);
        }
    }

    const ImVec2 viewMin(24.f,60.f),viewMax(396.f,520.f);
    const float floor=viewMax.y-12.f;
    int stable=0;
    for(int frame=0;frame<420;++frame) {
        const double time=frame/60.;
        const bool textured=frame%2==0;
        const auto drawn=RenderFrame(time,1.f/60.f,textured,viewMin,viewMax);
        const auto& bounds=drawn.bounds;
        if(textured) assert(drawn.addedVertices>=kMeshVertices);
        else assert(drawn.addedVertices>0);
        assert(Finite(bounds.min) && Finite(bounds.max));
        assert(bounds.max.x>bounds.min.x && bounds.max.y>bounds.min.y);
        assert(bounds.height>0.f && std::isfinite(bounds.height));
        assert(bounds.min.x>=viewMin.x-40.f && bounds.max.x<=viewMax.x+40.f);
        assert(bounds.max.y<=viewMax.y+2.f && bounds.min.y>=viewMin.y-40.f);
        assert(std::fabs(bounds.feet.y-(floor-bounds.jump))<.01f);
        assert(bounds.head.y<bounds.chest.y && bounds.chest.y<bounds.hip.y && bounds.hip.y<bounds.feet.y);
        assert(std::fabs(bounds.jump)<.3f*bounds.height);
        for(const auto point : {bounds.head,bounds.neck,bounds.chest,bounds.hip,bounds.knee,bounds.feet,
                bounds.shoulder[0],bounds.shoulder[1],bounds.elbow[0],bounds.elbow[1],
                bounds.hand[0],bounds.hand[1],bounds.knees[0],bounds.knees[1],bounds.ankle[0],bounds.ankle[1]})
            assert(Finite(point));
        if(textured) {
            if(stable==0) stable=drawn.addedVertices;
            assert(std::abs(drawn.addedVertices-stable)<=128);
            assert(std::abs(kMeshVertices-stable)<=128);
        }
    }

    ImGui::DestroyContext();
    std::puts("ESP preview breathing, jumps, hair and skirt springs, blink, mesh indices, subtree weights and animated bounds stay finite, stable and inside the preview view");
}
