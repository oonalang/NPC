#pragma once
#include "../../ImGui/imgui_internal.h"
#include "../../ImGui/ClayGlass.h"
#include "../../ImGui/imgui_settings.h"
#include "../../Fonts/Icon.h"
#include "StartupMedia.h"
#include "StartupState.h"
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
inline void Text(ImDrawList* draw, ImFont* font, float size, ImVec2 pos, ImU32 color, const char* text) {
    draw->AddText(font,size,pos,color,text);
}
inline void CenterText(ImDrawList* draw, ImFont* font, float size, float cx, float y, ImU32 color, const char* text) {
    const auto measured=font->CalcTextSizeA(size,FLT_MAX,0.f,text);
    Text(draw,font,size,{cx-measured.x*.5f,y},color,text);
}
inline void Icon(ImDrawList* draw, const char* icon, ImVec2 center, float size, ImU32 color) {
    const auto measured=Icons()->CalcTextSizeA(size,FLT_MAX,0.f,icon);
    Text(draw,Icons(),size,center-measured*.5f,color,icon);
}
inline void Ribbon(ImDrawList* draw, ImVec2 a, ImVec2 b, ImVec2 c1, ImVec2 d, ImU32 color, float width) {
    const auto evaluate=[&](float t) {
        const float u=1.f-t;
        return a*(u*u*u)+b*(3.f*u*u*t)+c1*(3.f*u*t*t)+d*(t*t*t);
    };
    const auto base=ImGui::ColorConvertU32ToFloat4(color);
    for(int i=0;i<32;++i) {
        const float t0=i/32.f,t1=(i+1)/32.f;
        const auto p0=evaluate(t0),p1=evaluate(t1);
        const auto direction=p1-p0;
        const float length=std::sqrt(ImLengthSqr(direction));
        if(length<.001f) continue;
        const ImVec2 normal(-direction.y/length,direction.x/length);
        const float fade0=std::pow(ImMax(0.f,std::sin(t0*3.14159265f)),.55f);
        const float fade1=std::pow(ImMax(0.f,std::sin(t1*3.14159265f)),.55f);
        const float half0=width*.5f*fade0,half1=width*.5f*fade1;
        auto col0=base,col1=base; col0.w*=fade0; col1.w*=fade1;
        const auto color0=ImGui::ColorConvertFloat4ToU32(col0),color1=ImGui::ColorConvertFloat4ToU32(col1);
        const auto clear0=color0&~IM_COL32_A_MASK,clear1=color1&~IM_COL32_A_MASK;
        const auto uv=draw->_Data->TexUvWhitePixel;
        draw->PrimReserve(18,8);
        const ImDrawIdx index=static_cast<ImDrawIdx>(draw->_VtxCurrentIdx);
        for(int strip=0;strip<3;++strip) {
            draw->PrimWriteIdx(index+strip); draw->PrimWriteIdx(index+strip+4); draw->PrimWriteIdx(index+strip+5);
            draw->PrimWriteIdx(index+strip); draw->PrimWriteIdx(index+strip+5); draw->PrimWriteIdx(index+strip+1);
        }
        draw->PrimWriteVtx(p0-normal*(half0+1.f),uv,clear0);
        draw->PrimWriteVtx(p0-normal*half0,uv,color0); draw->PrimWriteVtx(p0+normal*half0,uv,color0);
        draw->PrimWriteVtx(p0+normal*(half0+1.f),uv,clear0);
        draw->PrimWriteVtx(p1-normal*(half1+1.f),uv,clear1);
        draw->PrimWriteVtx(p1-normal*half1,uv,color1); draw->PrimWriteVtx(p1+normal*half1,uv,color1);
        draw->PrimWriteVtx(p1+normal*(half1+1.f),uv,clear1);
    }
}
inline void Frame(ImDrawList* draw, ImVec2 pos, ImVec2 size, float scale) {
    const auto hi=pos+size;
    for(int i=3;i>0;--i) {
        const float offset=i*3.f*scale;
        draw->AddRectFilled(pos+ImVec2(-offset,4.f*scale),hi+ImVec2(offset,offset+4.f*scale),
            ImGui::GetColorU32(ImVec4(.02f,.01f,.04f,.065f)),(42.f+i*3.f)*scale);
    }
    clay::Liquid(draw,pos,hi,42.f*scale,ImVec4(.065f,.045f,.10f,.46f),1.2f);
    draw->AddRect(pos,hi,ImGui::GetColorU32(ImVec4(.90f,.82f,1.f,.20f)),42.f*scale,0,1.f*scale);
    const auto point=[&](float x,float y) { return pos+ImVec2(x*size.x,y*size.y); };
    Ribbon(draw,point(.002f,.25f),point(.008f,-.055f),point(.19f,.015f),point(.40f,.004f),
        ImGui::GetColorU32(ImVec4(.90f,.78f,1.f,.15f)),6.f*scale);
    Ribbon(draw,point(.004f,.23f),point(.008f,-.035f),point(.19f,.020f),point(.39f,.003f),
        ImGui::GetColorU32(ImVec4(1.f,.94f,1.f,.52f)),1.f*scale);
    Ribbon(draw,point(.62f,.004f),point(.80f,-.045f),point(.995f,.01f),point(.996f,.25f),
        ImGui::GetColorU32(ImVec4(.73f,.57f,1.f,.21f)),8.f*scale);
    Ribbon(draw,point(.68f,.006f),point(.84f,.075f),point(.98f,.04f),point(.996f,.24f),
        ImGui::GetColorU32(ImVec4(.95f,.86f,1.f,.35f)),1.2f*scale);
    Ribbon(draw,point(.998f,.69f),point(.98f,1.01f),point(.86f,.96f),point(.61f,.995f),
        Accent(.27f),5.f*scale);
    Ribbon(draw,point(.002f,.73f),point(.01f,.99f),point(.19f,.96f),point(.37f,.996f),
        ImGui::GetColorU32(ImVec4(.81f,.62f,1.f,.23f)),3.f*scale);
    const float y=hi.y-2.f*scale,cx=(pos.x+hi.x)*.5f;
    draw->AddLine({cx-60.f*scale,y},{cx+60.f*scale,y},Accent(.65f),1.4f*scale);
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
inline void LoadingFrame(ImDrawList* draw,ImVec2 pos,ImVec2 size,float scale) {
    const auto hi=pos+size;
    const float radius=56.f*scale;
    for(int i=5;i>0;--i) {
        const float spread=i*2.f*scale;
        draw->AddRectFilled(pos+ImVec2(-spread,spread),hi+ImVec2(spread,spread*2.f),
            IM_COL32(8,7,17,10),radius+spread);
    }
    clay::Liquid(draw,pos,hi,radius,ImVec4(.17f,.17f,.215f,.74f),.55f);
    Gradient(draw,pos,hi,radius,ImVec4(.81f,.81f,.94f,.14f),ImVec4(.40f,.36f,.58f,.03f));
    const int border=draw->VtxBuffer.Size;
    draw->AddRect(pos,hi,IM_COL32_WHITE,radius,0,1.2f*scale);
    TintVertices(draw,border,pos,hi,ImVec4(.91f,.91f,1.f,.65f),ImVec4(.74f,.70f,.92f,.27f));
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
inline void Brand(ImDrawList* draw, ImVec2 pos, float width, float scale, bool loading=false) {
    const float cx=pos.x+width*.5f;
    const float avatarSize=(loading?56.f:68.f)*scale;
    media::Avatar(draw,{cx-avatarSize*.5f,pos.y+(loading?26.f:16.f)*scale},avatarSize);
    const float titleSize=30.f*scale;
    const auto titleSizePx=Title()->CalcTextSizeA(titleSize,FLT_MAX,0.f,"SHAI ETHNIR");
    const ImVec2 badgeSize(66.f*scale,23.f*scale);
    const float gap=12.f*scale;
    const float left=cx-(titleSizePx.x+gap+badgeSize.x)*.5f;
    const float y=pos.y+(loading?99.f:103.f)*scale;
    Text(draw,Title(),titleSize,{left,y},IM_COL32(246,241,255,255),"SHAI ETHNIR");
    const auto badge=ImVec2(left+titleSizePx.x+gap,y+(titleSizePx.y-badgeSize.y)*.5f);
    draw->AddRectFilled(badge,badge+badgeSize,IM_COL32(183,145,229,26),badgeSize.y*.5f);
    draw->AddRect(badge,badge+badgeSize,IM_COL32(228,208,255,44),badgeSize.y*.5f,0,1.f*scale);
    const auto label=Body()->CalcTextSizeA(10.5f*scale,FLT_MAX,0.f,"EDITION");
    Text(draw,Body(),10.5f*scale,badge+(badgeSize-label)*.5f,IM_COL32(210,194,231,255),"EDITION");
}
inline bool Pill(const char* id, const char* label, const char* icon, ImVec2 lo, ImVec2 size, float scale,
                 bool primary=false, bool disabled=false) {
    ImGui::SetCursorScreenPos(lo);
    ImGui::BeginDisabled(disabled);
    const bool pressed=ImGui::InvisibleButton(id,size);
    const bool held=ImGui::IsItemActive(), hovered=ImGui::IsItemHovered();
    auto* draw=ImGui::GetWindowDrawList();
    auto tint=primary ? c::accent : ImVec4(.17f,.13f,.23f,1.f);
    tint.w=primary ? (held?.62f:hovered?.53f:.44f) : .36f;
    clay::Liquid(draw,lo,lo+size,size.y*.5f,tint,primary?1.35f:1.f);
    draw->AddRect(lo,lo+size,ImGui::GetColorU32(ImVec4(1.f,.93f,1.f,primary?.32f:.19f)),size.y*.5f,0,1.f*scale);
    if(primary) draw->AddRect(lo+ImVec2(0,2.f*scale),lo+size,
        Accent(.10f),size.y*.5f,0,4.f*scale);
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
struct LoginActions { bool paste=false, load=false, submit=false; };
struct LoginPosition { ImVec2 resting; bool known=false, shifted=false; };
inline LoginPosition loginPosition;

template<class Input>
LoginActions RenderLogin(char* key, size_t capacity, bool pending, const char* error, bool cached,
                         bool& keyboard, Input input) {
    LoginActions result;
    const ImVec2 logical(720,500); const float scale=Fit(logical);
    const auto size=logical*scale;
    const auto windowSize=size+ImVec2(20,20)*scale;
    if(!ImGui::FindWindowByName("Login Menu")) loginPosition={};
    if(keyboard) {
        if(!loginPosition.known) loginPosition.resting=ImGui::GetMainViewport()->GetCenter()-windowSize*.5f;
        ImGui::SetNextWindowPos({loginPosition.resting.x,ImGui::GetIO().DisplaySize.y*.44f-272.f*scale},ImGuiCond_Always);
        loginPosition.shifted=true;
    } else if(loginPosition.shifted) {
        ImGui::SetNextWindowPos(loginPosition.resting,ImGuiCond_Always);
        loginPosition.shifted=false;
    } else ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),ImGuiCond_Appearing,{.5f,.5f});
    ImGui::SetNextWindowSize(windowSize,ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
    if(ImGui::Begin("Login Menu",nullptr,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoSavedSettings|
        ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse)) {
        if(!keyboard) { loginPosition.resting=ImGui::GetWindowPos(); loginPosition.known=true; }
        const auto pos=ImGui::GetWindowPos()+ImVec2(10,10)*scale;
        auto* draw=ImGui::GetWindowDrawList();
        Frame(draw,pos,size,scale); Brand(draw,pos,size.x,scale);
        CenterText(draw,Body(),15.f*scale,pos.x+size.x*.5f,pos.y+153.f*scale,
            IM_COL32(193,183,207,255),"Use your authorized access key to continue.");
        const auto field=pos+ImVec2(60,196)*scale;
        const auto fieldSize=ImVec2(600,62)*scale;
        clay::Liquid(draw,field,field+fieldSize,31.f*scale,ImVec4(.16f,.115f,.205f,.36f),1.25f);
        draw->AddRect(field,field+fieldSize,IM_COL32(225,206,255,70),31.f*scale,0,1.f*scale);
        Icon(draw,ICON_FA_KEY,field+ImVec2(34,31)*scale,21.f*scale,Accent(.95f));
        draw->AddLine(field+ImVec2(64,17)*scale,field+ImVec2(64,45)*scale,IM_COL32(255,240,255,45),1.f*scale);
        ImGui::SetCursorScreenPos(field+ImVec2(70,5)*scale);
        ImGui::PushFont(Body());
        input("##key_login",key,capacity,ImVec2(515,52)*scale);
        ImGui::PopFont();
        const bool active=ImGui::IsItemActive(), hovered=ImGui::IsItemHovered();
        if(ImGui::IsItemClicked() || active) keyboard=true;
        if(active || hovered) draw->AddRect(field,field+fieldSize,Accent(active?.65f:.28f),31.f*scale,0,1.3f*scale);
        if(keyboard && !active && !hovered && ImGui::IsMouseClicked(0) &&
            ImGui::GetMousePos().y<ImGui::GetIO().DisplaySize.y*.44f) keyboard=false;
        const auto utility=pos+ImVec2(60,282)*scale;
        result.paste=Pill("##login_paste","PASTE",ICON_FA_PASTE,utility,ImVec2(288,52)*scale,scale);
        result.load=Pill("##login_load","LOAD",ICON_FA_FOLDER_OPEN,utility+ImVec2(312,0)*scale,ImVec2(288,52)*scale,scale);
        result.submit=Pill("##login_auth",pending?"CHECKING...":"AUTHENTICATE",ICON_FA_BULLSEYE,
            pos+ImVec2(60,358)*scale,ImVec2(600,66)*scale,scale,true,pending);
        const bool failed=error && *error && std::strcmp(error,"OK")!=0;
        const char* status=failed?error:pending?"Checking access...":cached?"Secure access  |  Local cache ready":"Secure access";
        const ImVec2 statusMin=pos+ImVec2(64,450)*scale,statusMax=pos+ImVec2(656,490)*scale;
        draw->PushClipRect(statusMin,statusMax,true);
        Icon(draw,failed?ICON_FA_EXCLAMATION:ICON_FA_SHIELD_ALT,statusMin+ImVec2(11,9)*scale,
            16.f*scale,failed?IM_COL32(255,145,164,255):Accent(.85f));
        draw->AddText(Body(),13.f*scale,statusMin+ImVec2(32,0)*scale,
            failed?IM_COL32(255,170,183,255):IM_COL32(174,160,193,255),status,nullptr,550.f*scale);
        draw->PopClipRect();
    }
    ImGui::End(); ImGui::PopStyleVar();
    return result;
}

inline void LoadingCard(ImDrawList* draw, ImVec2 pos, ImVec2 size, float scale, float progress, float time) {
    LoadingFrame(draw,pos,size,scale); Brand(draw,pos,size.x,scale,true);
    const float cx=pos.x+size.x*.5f;
    const auto tip=TipAt(time);
    const float tipSize=13.f*scale,tipWidth=612.f*scale;
    const auto tipBounds=Body()->CalcTextSizeA(tipSize,FLT_MAX,tipWidth,loadingTips[tip.index]);
    draw->PushClipRect(pos+ImVec2(30,134)*scale,pos+ImVec2(650,160)*scale,true);
    draw->AddText(Body(),tipSize,{cx-tipBounds.x*.5f,pos.y+146.f*scale-tipBounds.y*.5f},
        ImGui::GetColorU32(ImVec4(.80f,.78f,.88f,tip.opacity)),loadingTips[tip.index],nullptr,tipWidth);
    draw->PopClipRect();
    Gradient(draw,{cx-22.f*scale,pos.y+169.f*scale},{cx+22.f*scale,pos.y+177.f*scale},4.f*scale,
        ImVec4(.83f,.68f,1.f,1.f),ImVec4(.60f,.40f,.83f,1.f));
    media::Banner(draw,pos+ImVec2(24,188)*scale,ImVec2(632,194)*scale,scale,time);
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
    const auto info=pos+ImVec2(34,519)*scale;
    Gradient(draw,info,info+ImVec2(612,58)*scale,29.f*scale,ImVec4(.68f,.65f,.78f,.24f),ImVec4(.50f,.46f,.62f,.28f));
    draw->AddRect(info,info+ImVec2(612,58)*scale,IM_COL32(229,222,248,68),29.f*scale,0,1.f*scale);
    Icon(draw,ICON_FA_INFO_CIRCLE,info+ImVec2(40,29)*scale,23.f*scale,IM_COL32(225,219,241,245));
    Text(draw,Body(),14.f*scale,info+ImVec2(66,9)*scale,IM_COL32(242,237,250,255),"Please wait");
    Text(draw,Body(),12.f*scale,info+ImVec2(66,32)*scale,IM_COL32(193,185,207,255),"Preparing your interface. Keep the application open.");
}
}
