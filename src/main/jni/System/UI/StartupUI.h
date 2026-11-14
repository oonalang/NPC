#pragma once
#include "../../ImGui/imgui_internal.h"
#include "../../ImGui/ClayGlass.h"
#include "../../ImGui/imgui_settings.h"
#include "../../Fonts/Icon.h"
#include "StartupMedia.h"
#include "StartupState.h"
#include "AuthenticateButton.h"
#include "AuthenticateScene.h"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace font { extern ImFont* inter_title; }

namespace startup_ui {
inline ImFont* Body() { return font::inter_semibold ? font::inter_semibold : ImGui::GetFont(); }
inline ImFont* Title() { return font::inter_title ? font::inter_title : Body(); }
inline ImFont* Icons() { return F107 ? F107 : Body(); }
inline ImU32 Accent(float alpha = 1.f) { auto color=c::accent; color.w=alpha; return ImGui::GetColorU32(color); }
inline float Fit(ImVec2 logical) {
    const auto display=ImGui::GetIO().DisplaySize;
    return ImMax(.1f,ImMin(1.f,ImMin((display.x-28.f)/logical.x,(display.y-28.f)/logical.y)));
}
inline void Text(ImDrawList* draw, ImFont* font, float size, ImVec2 pos, ImU32 color, const char* text,float wrap=0.f) {
    const auto shadow=IM_COL32(4,3,9,((color>>IM_COL32_A_SHIFT)&255)*.85f);
    const float spread=ImClamp(size/24.f,.55f,1.f);
    for(const auto offset : {ImVec2(-spread,0),ImVec2(spread,0),ImVec2(0,-spread),ImVec2(0,spread)})
        draw->AddText(font,size,pos+offset,shadow,text,nullptr,wrap);
    draw->AddText(font,size,pos,color,text,nullptr,wrap);
}
inline void CenterText(ImDrawList* draw, ImFont* font, float size, float cx, float y, ImU32 color, const char* text) {
    const auto measured=font->CalcTextSizeA(size,FLT_MAX,0.f,text);
    Text(draw,font,size,{cx-measured.x*.5f,y},color,text);
}
inline void Icon(ImDrawList* draw, const char* icon, ImVec2 center, float size, ImU32 color) {
    const auto measured=Icons()->CalcTextSizeA(size,FLT_MAX,0.f,icon);
    Text(draw,Icons(),size,center-measured*.5f,color,icon);
}
inline void TintVertices(ImDrawList* draw,int first,ImVec2 lo,ImVec2 hi,ImVec4 top,ImVec4 bottom) {
    for(int i=first;i<draw->VtxBuffer.Size;++i) {
        auto& vertex=draw->VtxBuffer[i];
        const float t=ImClamp((vertex.pos.y-lo.y)/ImMax(1.f,hi.y-lo.y),0.f,1.f);
        auto color=ImLerp(top,bottom,t);
        color.w*=ImGui::ColorConvertU32ToFloat4(vertex.col).w;
        vertex.col=ImGui::ColorConvertFloat4ToU32(color);
    }
}
inline void Gradient(ImDrawList* draw,ImVec2 lo,ImVec2 hi,float radius,ImVec4 top,ImVec4 bottom) {
    const int first=draw->VtxBuffer.Size;
    draw->AddRectFilled(lo,hi,IM_COL32_WHITE,radius);
    TintVertices(draw,first,lo,hi,top,bottom);
}
inline void Frame(ImDrawList* draw,ImVec2 pos,ImVec2 size,float scale) {
    const auto hi=pos+size;
    const float radius=56.f*scale;
    for(int i=5;i>0;--i) {
        const float spread=i*2.f*scale;
        draw->AddRectFilled(pos+ImVec2(-spread,spread),hi+ImVec2(spread,spread*2.f),
            IM_COL32(8,7,17,10),radius+spread);
    }
    clay::Liquid(draw,pos,hi,radius,ImVec4(.055f,.05f,.095f,.27f),1.25f);
    Gradient(draw,pos,hi,radius,ImVec4(.81f,.81f,.94f,.035f),ImVec4(.40f,.36f,.58f,.015f));
    const int border=draw->VtxBuffer.Size;
    draw->AddRect(pos,hi,IM_COL32_WHITE,radius,0,1.2f*scale);
    TintVertices(draw,border,pos,hi,ImVec4(.91f,.91f,1.f,.28f),ImVec4(.74f,.70f,.92f,.13f));
    draw->AddRect(pos+ImVec2(2,2)*scale,hi-ImVec2(2,2)*scale,
        IM_COL32(233,228,255,18),radius-2.f*scale,0,1.f*scale);
    draw->AddCircleFilled(pos+ImVec2(40,33)*scale,4.f*scale,IM_COL32(187,188,206,175));
    draw->AddCircleFilled(pos+ImVec2(56,33)*scale,4.f*scale,IM_COL32(187,188,206,175));
    draw->AddRectFilled(pos+ImVec2(68,29)*scale,pos+ImVec2(84,37)*scale,
        IM_COL32(187,188,206,175),4.f*scale);
    const auto mark=pos+ImVec2(size.x/scale-70.f,26)*scale;
    draw->AddRectFilled(mark,mark+ImVec2(32,17)*scale,IM_COL32(180,177,202,14),8.5f*scale);
    draw->AddRect(mark,mark+ImVec2(32,17)*scale,IM_COL32(223,221,242,72),8.5f*scale,0,1.f*scale);
    draw->AddLine(mark+ImVec2(9,8.5f)*scale,mark+ImVec2(23,8.5f)*scale,
        IM_COL32(224,222,240,205),1.2f*scale);
}
inline void Brand(ImDrawList* draw, ImVec2 pos, float width, float scale) {
    const float cx=pos.x+width*.5f;
    const float avatarSize=56.f*scale;
    media::Avatar(draw,{cx-avatarSize*.5f,pos.y+26.f*scale},avatarSize);
    const float titleSize=30.f*scale;
    const auto titleSizePx=Title()->CalcTextSizeA(titleSize,FLT_MAX,0.f,"SHAI ETHNIR");
    const ImVec2 badgeSize(66.f*scale,23.f*scale);
    const float gap=12.f*scale;
    const float left=cx-(titleSizePx.x+gap+badgeSize.x)*.5f;
    const float y=pos.y+99.f*scale;
    Text(draw,Title(),titleSize,{left,y},IM_COL32(246,241,255,255),"SHAI ETHNIR");
    const auto badge=ImVec2(left+titleSizePx.x+gap,y+(titleSizePx.y-badgeSize.y)*.5f);
    draw->AddRectFilled(badge,badge+badgeSize,IM_COL32(183,145,229,26),badgeSize.y*.5f);
    draw->AddRect(badge,badge+badgeSize,IM_COL32(228,208,255,44),badgeSize.y*.5f,0,1.f*scale);
    const auto label=Body()->CalcTextSizeA(10.5f*scale,FLT_MAX,0.f,"EDITION");
    Text(draw,Body(),10.5f*scale,badge+(badgeSize-label)*.5f,IM_COL32(210,194,231,255),"EDITION");
}
inline void PillMaterial(ImDrawList* draw,ImVec2 lo,ImVec2 size,float scale,bool primary,bool held,bool hovered,float opacity=1.f) {
    if(opacity<=0.f) return;
    const int first=draw->VtxBuffer.Size;
#ifndef CLAY_CPU_PREVIEW
    const unsigned firstMaterial=clay::materialCount;
#endif
    auto tint=primary ? ImVec4(c::accent.x*.32f,c::accent.y*.32f,c::accent.z*.32f,1.f) : ImVec4(.10f,.075f,.15f,1.f);
    tint.w=primary ? (held?.80f:hovered?.74f:.70f) : .70f;
    clay::Liquid(draw,lo,lo+size,size.y*.5f,tint,primary?1.35f:1.f);
    draw->AddRect(lo,lo+size,ImGui::GetColorU32(ImVec4(1.f,.93f,1.f,primary?.32f:.19f)),size.y*.5f,0,1.f*scale);
    if(primary) draw->AddRect(lo+ImVec2(0,2.f*scale),lo+size,
        Accent(.10f),size.y*.5f,0,4.f*scale);
    if(opacity<1.f) {
        button_art::Fade(draw,first,opacity);
#ifndef CLAY_CPU_PREVIEW
        for(unsigned i=firstMaterial;i<clay::materialCount;++i) clay::materialDraws[i].alpha*=opacity;
#endif
    }
}
inline bool Pill(const char* id, const char* label, const char* icon, ImVec2 lo, ImVec2 size, float scale,
                 bool primary=false, bool disabled=false) {
    ImGui::SetCursorScreenPos(lo);
    ImGui::BeginDisabled(disabled);
    const bool pressed=ImGui::InvisibleButton(id,size);
    const bool held=ImGui::IsItemActive(), hovered=ImGui::IsItemHovered();
    auto* draw=ImGui::GetWindowDrawList();
    PillMaterial(draw,lo,size,scale,primary,held,hovered);
    const float textSize=17.f*scale, iconSize=19.f*scale;
    const auto labelSize=Body()->CalcTextSizeA(textSize,FLT_MAX,0,label);
    const float total=labelSize.x+iconSize+16.f*scale;
    const float x=lo.x+(size.x-total)*.5f;
    Icon(draw,icon,{x+iconSize*.5f,lo.y+size.y*.5f},iconSize,ImGui::GetColorU32(ImVec4(.96f,.92f,1.f,1.f)));
    Text(draw,Body(),textSize,{x+iconSize+16.f*scale,lo.y+(size.y-labelSize.y)*.5f},
        ImGui::GetColorU32(ImVec4(.96f,.92f,1.f,1.f)),label);
    ImGui::EndDisabled();
    return pressed;
}
inline LoginTransition loginTransition;
inline float Ease(float value) { return value*value*(3.f-2.f*value); }
inline void FadeLogin(ImDrawList* draw,int first,float opacity) {
    if(opacity>=1.f) return;
    for(int i=first;i<draw->VtxBuffer.Size;++i) {
        auto& color=draw->VtxBuffer[i].col;
        const auto alpha=static_cast<ImU32>(((color>>IM_COL32_A_SHIFT)&255)*opacity);
        color=(color&~IM_COL32_A_MASK)|(alpha<<IM_COL32_A_SHIFT);
    }
#ifndef CLAY_CPU_PREVIEW
    for(const auto& command : draw->CmdBuffer)
        if(command.UserCallback==clay::DrawMaterial)
            static_cast<clay::MaterialDraw*>(command.UserCallbackData)->alpha*=opacity;
#endif
}
inline void CollapseCard(ImDrawList* draw,int first,ImVec2 center,float amount) {
    if(amount<=0.f) return;
    const float ease=Ease(ImClamp(amount,0.f,1.f));
    const float shrink=1.f-.85f*ease;
    const float angle=2.6f*ease;
    const float sine=std::sin(angle),cosine=std::cos(angle);
    for(int i=first;i<draw->VtxBuffer.Size;++i) {
        auto& vertex=draw->VtxBuffer[i];
        const auto offset=(vertex.pos-center)*shrink;
        vertex.pos=center+ImVec2(offset.x*cosine-offset.y*sine,offset.x*sine+offset.y*cosine);
        const auto alpha=static_cast<ImU32>(((vertex.col>>IM_COL32_A_SHIFT)&255)*(1.f-ease));
        vertex.col=(vertex.col&~IM_COL32_A_MASK)|(alpha<<IM_COL32_A_SHIFT);
    }
#ifndef CLAY_CPU_PREVIEW
    for(const auto& command : draw->CmdBuffer)
        if(command.UserCallback==clay::DrawMaterial)
            static_cast<clay::MaterialDraw*>(command.UserCallbackData)->alpha*=1.f-ease;
#endif
}
inline void AuthOverlay(ImDrawList* draw,ImVec2 center,const LoginTransitionFrame& frame,float scale) {
    const float progress=ImClamp(frame.sceneProgress,0.f,1.f);
    if(frame.stage==AuthStage::Singularity) {
        const float amount=Ease(progress);
        draw->AddCircleFilled(center,ImLerp(3.f,23.f,amount)*scale,IM_COL32(4,3,9,235),28);
        const float ring=ImLerp(52.f,9.f,amount)*scale;
        draw->AddCircle(center,ring,IM_COL32(214,184,255,215),36,2.4f*scale);
        for(int i=1;i<=2;++i)
            draw->AddCircle(center,ring+(i*2.6f)*scale,IM_COL32(176,118,255,static_cast<ImU32>(70-i*18)),36,1.4f*scale);
        draw->AddCircleFilled(center,ImMax(1.f,ring*.34f),IM_COL32(255,252,255,240),18);
        return;
    }
    if(frame.stage==AuthStage::Flash) {
        const float amount=Ease(progress);
        for(int i=6;i>=1;--i)
            draw->AddCircleFilled(center,(5.f+i*6.f)*scale*(1.f+amount*.8f),
                ImGui::GetColorU32(ImVec4(1.f,.97f,1.f,.09f*(1.f-i/8.f))),32);
        draw->AddCircleFilled(center,ImLerp(5.f,22.f,amount)*scale,IM_COL32(255,255,255,255),28);
        const float arm=ImLerp(12.f,62.f,amount)*scale;
        draw->AddLine(center-ImVec2(arm,0),center+ImVec2(arm,0),IM_COL32(255,250,255,205),1.8f*scale);
        draw->AddLine(center-ImVec2(0,arm),center+ImVec2(0,arm),IM_COL32(255,250,255,205),1.8f*scale);
        return;
    }
}

inline bool AuthenticateButton(ImVec2 lo,ImVec2 size,float scale,bool pending,const LoginTransitionFrame& frame) {
    ImGui::SetCursorScreenPos(lo);
    ImGui::BeginDisabled(pending || frame.inputBlocked);
    const bool pressed=ImGui::InvisibleButton("##login_auth",size);
    const bool held=ImGui::IsItemActive(),hovered=ImGui::IsItemHovered();
    ImGui::EndDisabled();
    auto* draw=ImGui::GetWindowDrawList();
    const auto& button=frame.button;
    const auto surface=button_art::SurfaceAt(lo,size,button);
    PillMaterial(draw,surface.lo,surface.size,scale,true,held,hovered,surface.opacity);
    DrawAuthenticateContents(draw,lo,size,scale,button,Body());
    return pressed;
}
struct LoginActions { bool paste=false, load=false, submit=false; };
struct LoginPosition { ImVec2 resting,viewportPos,viewportSize; bool known=false, shifted=false; };
inline LoginPosition loginPosition;

template<class Input>
LoginActions RenderLogin(char* key, size_t capacity, bool pending, const char* error, bool cached,
                         bool& keyboard, Input input,const LoginTransitionFrame* overrideFrame=nullptr) {
    LoginActions result;
    const auto animation=overrideFrame?*overrideFrame:loginTransition.At(Now());
    if(!animation.loginVisible) {
        if(!auth_scene::Active() && (animation.stage==AuthStage::Singularity||animation.stage==AuthStage::Flash))
            AuthOverlay(ImGui::GetForegroundDrawList(),ImGui::GetMainViewport()->GetCenter(),animation,Fit({720,500}));
        return result;
    }
    const ImVec2 logical(720,500); const float scale=Fit(logical);
    const auto size=logical*scale;
    const auto windowSize=size+ImVec2(20,20)*scale;
    const auto* viewport=ImGui::GetMainViewport();
    auto* loginWindow=ImGui::FindWindowByName("Login Menu");
    const bool appearing=!loginWindow || !loginWindow->WasActive;
    if(appearing) {
        loginPosition={};
        keyboard=false;
    }
    const bool resized=!loginPosition.known || ImLengthSqr(viewport->Pos-loginPosition.viewportPos)>.25f ||
        ImLengthSqr(viewport->Size-loginPosition.viewportSize)>.25f;
    if(resized) {
        loginPosition.resting=viewport->GetCenter()-windowSize*.5f;
        loginPosition.viewportPos=viewport->Pos;
        loginPosition.viewportSize=viewport->Size;
        loginPosition.known=true;
    }
    if(keyboard) {
        ImGui::SetNextWindowPos({loginPosition.resting.x,viewport->Pos.y+viewport->Size.y*.44f-272.f*scale},ImGuiCond_Always);
        loginPosition.shifted=true;
    } else if(loginPosition.shifted) {
        ImGui::SetNextWindowPos(loginPosition.resting,ImGuiCond_Always);
        loginPosition.shifted=false;
    } else if(resized) ImGui::SetNextWindowPos(loginPosition.resting,ImGuiCond_Always);
    ImGui::SetNextWindowSize(windowSize,ImGuiCond_Always);
    const bool busy=pending || animation.inputBlocked || loginTransition.active;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha,1.f);
    if(ImGui::Begin("Login Menu",nullptr,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoSavedSettings|
        ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|
        (busy?ImGuiWindowFlags_NoMove:0))) {
        if(!keyboard) { loginPosition.resting=ImGui::GetWindowPos(); loginPosition.known=true; }
        const auto pos=ImGui::GetWindowPos()+ImVec2(10,10)*scale;
        auto* draw=ImGui::GetWindowDrawList();
        const int first=draw->VtxBuffer.Size;
        Frame(draw,pos,size,scale); Brand(draw,pos,size.x,scale);
        CenterText(draw,Body(),15.f*scale,pos.x+size.x*.5f,pos.y+153.f*scale,
            IM_COL32(193,183,207,255),"Use your authorized access key to continue.");
        Text(draw,Body(),11.5f*scale,pos+ImVec2(64,176)*scale,IM_COL32(220,208,239,255),"ACCESS KEY");
        const auto field=pos+ImVec2(60,196)*scale;
        const auto fieldSize=ImVec2(600,62)*scale;
        clay::Liquid(draw,field,field+fieldSize,31.f*scale,ImVec4(.10f,.07f,.15f,.70f),1.25f);
        draw->AddRect(field,field+fieldSize,IM_COL32(225,206,255,70),31.f*scale,0,1.f*scale);
        Icon(draw,ICON_FA_KEY,field+ImVec2(34,31)*scale,21.f*scale,Accent(.95f));
        draw->AddLine(field+ImVec2(64,17)*scale,field+ImVec2(64,45)*scale,IM_COL32(255,240,255,45),1.f*scale);
        ImGui::SetCursorScreenPos(field+ImVec2(70,5)*scale);
        ImGui::PushFont(Body());
        ImGui::BeginDisabled(busy);
        input("##key_login",key,capacity,ImVec2(515,52)*scale);
        ImGui::EndDisabled();
        ImGui::PopFont();
        const bool active=ImGui::IsItemActive(), hovered=ImGui::IsItemHovered();
        if(!busy && (ImGui::IsItemClicked() || active)) keyboard=true;
        if(active || hovered) draw->AddRect(field,field+fieldSize,Accent(active?.65f:.28f),31.f*scale,0,1.3f*scale);
        if(keyboard && !active && !hovered && ImGui::IsMouseClicked(0) &&
            ImGui::GetMousePos().y<ImGui::GetIO().DisplaySize.y*.44f) keyboard=false;
        const auto utility=pos+ImVec2(60,282)*scale;
        result.paste=Pill("##login_paste","Paste key",ICON_FA_PASTE,utility,ImVec2(288,52)*scale,scale,false,busy);
        result.load=Pill("##login_load","Load saved",ICON_FA_FOLDER_OPEN,utility+ImVec2(312,0)*scale,ImVec2(288,52)*scale,scale,false,busy);
        result.submit=AuthenticateButton(pos+ImVec2(60,358)*scale,ImVec2(600,66)*scale,scale,pending,animation);
        const bool failed=error && *error && std::strcmp(error,"OK")!=0;
        const char* status=failed?error:animation.granted?"Access granted":cached?"Saved key available":"Enter your access key to continue";
        const auto statusCard=pos+ImVec2(60,444)*scale;
        clay::Liquid(draw,statusCard,statusCard+ImVec2(600,42)*scale,21.f*scale,
            ImVec4(.075f,.06f,.125f,.68f),1.2f);
        draw->AddRect(statusCard,statusCard+ImVec2(600,42)*scale,IM_COL32(220,205,242,40),21.f*scale,0,1.f*scale);
        const ImVec2 statusMin=statusCard+ImVec2(14,10)*scale,statusMax=statusCard+ImVec2(586,40)*scale;
        draw->PushClipRect(statusMin,statusMax,true);
        Icon(draw,failed?ICON_FA_EXCLAMATION:ICON_FA_INFO_CIRCLE,statusMin+ImVec2(11,9)*scale,
            16.f*scale,failed?IM_COL32(255,145,164,255):Accent(.85f));
        Text(draw,Body(),13.f*scale,statusMin+ImVec2(32,0)*scale,
            failed?IM_COL32(255,170,183,255):IM_COL32(220,212,233,255),status,statusMax.x-statusMin.x-32.f*scale);
        draw->PopClipRect();
        if(animation.stage==AuthStage::Suck && !auth_scene::Active()) CollapseCard(draw,first,pos+size*.5f,animation.sceneProgress);
    }
    ImGui::End(); ImGui::PopStyleVar(2);
    return result;
}

struct DrawSlice {
    int vertex=0;
    unsigned material=0;
};
inline DrawSlice BeginSlice(ImDrawList* draw) {
    DrawSlice slice;
    slice.vertex=draw->VtxBuffer.Size;
#ifndef CLAY_CPU_PREVIEW
    slice.material=clay::materialCount;
#endif
    return slice;
}
inline void RevealSlice(ImDrawList* draw,const DrawSlice& slice,ImVec2 center,float scale,float opacity) {
    for(int i=slice.vertex;i<draw->VtxBuffer.Size;++i) {
        auto& vertex=draw->VtxBuffer[i];
        vertex.pos=center+(vertex.pos-center)*scale;
        const auto alpha=static_cast<ImU32>(((vertex.col>>IM_COL32_A_SHIFT)&255)*opacity);
        vertex.col=(vertex.col&~IM_COL32_A_MASK)|(alpha<<IM_COL32_A_SHIFT);
    }
#ifndef CLAY_CPU_PREVIEW
    for(unsigned i=slice.material;i<clay::materialCount;++i) {
        auto& material=clay::materialDraws[i];
        material.lo=center+(material.lo-center)*scale;
        material.hi=center+(material.hi-center)*scale;
        material.radius*=scale;
        material.alpha*=opacity;
    }
#endif
}

inline void LoadingCard(ImDrawList* draw, ImVec2 pos, ImVec2 size, float scale, float progress, float time,const LoadingReveal* reveal=nullptr) {
    const LoadingReveal appearance=reveal?*reveal:LoadingReveal{};
    const auto center=pos+size*.5f;
    auto section=BeginSlice(draw);
    Frame(draw,pos,size,scale);
    RevealSlice(draw,section,center,appearance.Scale(),Smooth(appearance.progress));
    section=BeginSlice(draw);
    Brand(draw,pos,size.x,scale);
    const float cx=pos.x+size.x*.5f;
    const auto tip=TipAt(time);
    const float tipSize=13.f*scale,tipWidth=612.f*scale;
    const auto tipBounds=Body()->CalcTextSizeA(tipSize,FLT_MAX,tipWidth,loadingTips[tip.index]);
    draw->PushClipRect(pos+ImVec2(30,134)*scale,pos+ImVec2(650,160)*scale,true);
    const int tipFirstCommand=draw->CmdBuffer.Size-1;
    const ImVec2 tipPos(cx-tipBounds.x*.5f,pos.y+146.f*scale-tipBounds.y*.5f);
    Text(draw,Body(),tipSize,tipPos,
        ImGui::GetColorU32(ImVec4(.86f,.84f,.92f,tip.opacity)),loadingTips[tip.index],tipWidth);
    const int tipEndCommand=draw->CmdBuffer.Size;
    for(int i=tipFirstCommand;i<tipEndCommand;++i) {
        auto& clip=draw->CmdBuffer[i].ClipRect;
        const auto lo=center+(ImVec2(clip.x,clip.y)-center)*appearance.Scale();
        const auto hi=center+(ImVec2(clip.z,clip.w)-center)*appearance.Scale();
        clip={lo.x,lo.y,hi.x,hi.y};
    }
    draw->PopClipRect();
    RevealSlice(draw,section,center,appearance.Scale(),appearance.Group(0));
    section=BeginSlice(draw);
    media::Banner(draw,pos+ImVec2(24,188)*scale,ImVec2(632,194)*scale,scale,time);
    RevealSlice(draw,section,center,appearance.Scale(),appearance.Group(1));
    section=BeginSlice(draw);
    const char* labels[]{"Initializing","Verifying","Loading Data","Finalizing"};
    for(int i=0;i<4;++i) {
        const auto lo=pos+ImVec2(50.f+i*154.f,408.f)*scale;
        const auto hi=lo+ImVec2(38,38)*scale;
        const bool complete=progress>=(i+1)*.25f,current=progress>=i*.25f && !complete;
        if(current) draw->AddRect(lo-ImVec2(1,1)*scale,hi+ImVec2(1,1)*scale,
            Accent(.17f),12.f*scale,0,5.f*scale);
        Gradient(draw,lo,hi,11.f*scale,
            complete?ImVec4(.64f,.58f,.75f,.85f):current?ImVec4(.70f,.51f,.91f,.95f):ImVec4(.36f,.34f,.44f,.34f),
            complete?ImVec4(.42f,.36f,.56f,.9f):current?ImVec4(.42f,.26f,.65f,.96f):ImVec4(.24f,.22f,.31f,.34f));
        draw->AddRect(lo,hi,IM_COL32(225,211,255,current?155:complete?55:22),11.f*scale,0,1.f*scale);
        const auto center=(lo+hi)*.5f;
        if(complete) ImGui::RenderCheckMark(draw,center-ImVec2(6,4)*scale,IM_COL32(251,244,255,255),12.f*scale);
        if(current) { draw->PathArcTo(center,10.f*scale,time*2.4f,time*2.4f+3.8f,18); draw->PathStroke(Accent(.92f),0,2.f*scale); }
        const float labelSize=12.f*scale;
        const auto textSize=Body()->CalcTextSizeA(labelSize,FLT_MAX,0.f,labels[i]);
        Text(draw,Body(),labelSize,{hi.x+8.f*scale,center.y-textSize.y*.5f},
            complete?IM_COL32(237,233,246,255):IM_COL32(184,176,200,255),labels[i]);
        if(i<3) {
            const float start=hi.x+16.f*scale+textSize.x,end=lo.x+146.f*scale;
            if(end>start) draw->AddLine({start,center.y},{end,center.y},IM_COL32(195,184,213,complete?65:30),1.f*scale);
        }
    }
    RevealSlice(draw,section,center,appearance.Scale(),appearance.Group(2));
    section=BeginSlice(draw);
    const auto bar=pos+ImVec2(50,479)*scale; const auto barSize=ImVec2(514,18)*scale;
    Gradient(draw,bar,bar+barSize,9.f*scale,ImVec4(.18f,.17f,.24f,.75f),ImVec4(.35f,.32f,.43f,.5f));
    draw->AddRect(bar,bar+barSize,IM_COL32(233,225,255,38),9.f*scale,0,1.f*scale);
    if(progress>0.f) {
        const auto end=bar+ImVec2(barSize.x*progress,barSize.y);
        draw->AddRect(bar,end,Accent(.14f),9.f*scale,0,6.f*scale);
        Gradient(draw,bar,end,9.f*scale,ImVec4(.83f,.65f,1.f,1.f),ImVec4(.59f,.36f,.85f,1.f));
        if(barSize.x*progress>18.f*scale) draw->AddLine(bar+ImVec2(9,2)*scale,end-ImVec2(9,16)*scale,IM_COL32(250,234,255,145),1.f*scale);
    }
    char percentage[16]; std::snprintf(percentage,sizeof(percentage),"%d%%",static_cast<int>(progress*100.f+.5f));
    Text(draw,Body(),20.f*scale,pos+ImVec2(600,477)*scale,IM_COL32(238,233,247,255),percentage);
    RevealSlice(draw,section,center,appearance.Scale(),appearance.Group(3));
    section=BeginSlice(draw);
    const auto info=pos+ImVec2(34,519)*scale;
    clay::Liquid(draw,info,info+ImVec2(612,58)*scale,29.f*scale,ImVec4(.075f,.06f,.125f,.68f),1.2f);
    draw->AddRect(info,info+ImVec2(612,58)*scale,IM_COL32(229,222,248,68),29.f*scale,0,1.f*scale);
    Icon(draw,ICON_FA_INFO_CIRCLE,info+ImVec2(40,29)*scale,23.f*scale,IM_COL32(225,219,241,245));
    Text(draw,Body(),14.f*scale,info+ImVec2(66,9)*scale,IM_COL32(242,237,250,255),"Please wait");
    Text(draw,Body(),12.f*scale,info+ImVec2(66,32)*scale,IM_COL32(224,218,235,255),"Preparing your interface. Keep the application open.");
    RevealSlice(draw,section,center,appearance.Scale(),appearance.Group(4));
}
}
