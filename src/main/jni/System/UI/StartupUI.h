#pragma once
#include "../../ImGui/imgui_internal.h"
#include "../../ImGui/ClayGlass.h"
#include "../../ImGui/imgui_settings.h"
#include "../../Fonts/Icon.h"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace startup_ui {
inline ImFont* Body() { return font::inter_semibold ? font::inter_semibold : ImGui::GetFont(); }
inline ImFont* Title() { return F50 ? F50 : Body(); }
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
inline void Brand(ImDrawList* draw, ImVec2 pos, float width, float scale, bool loading=false) {
    const float cx=pos.x+width*.5f;
    const ImVec2 center(cx,pos.y+49.f*scale);
    ImVec2 points[6];
    for(int i=0;i<6;++i) {
        const float a=-1.5707963f+i*1.0471976f;
        points[i]=center+ImVec2(std::cos(a),std::sin(a))*26.f*scale;
    }
    draw->AddConvexPolyFilled(points,6,Accent(.10f));
    draw->AddPolyline(points,6,Accent(.10f),ImDrawFlags_Closed,9.f*scale);
    draw->AddPolyline(points,6,Accent(.80f),ImDrawFlags_Closed,1.2f*scale);
    CenterText(draw,Title(),25.f*scale,cx,center.y-15.f*scale,IM_COL32(250,245,255,255),"E");
    const char* text="ETHNIR NOIR";
    const float size=(loading?29.f:32.f)*scale;
    const float tracking=3.f*scale;
    float total=0.f;
    for(const char* p=text;*p;++p) { char letter[2]{*p,0}; total+=Title()->CalcTextSizeA(size,FLT_MAX,0,letter).x+tracking; }
    float x=cx-(total-tracking)*.5f;
    const float y=pos.y+90.f*scale;
    for(const char* p=text;*p;++p) {
        char letter[2]{*p,0}; Text(draw,Title(),size,{x,y},IM_COL32(247,243,255,255),letter);
        x+=Title()->CalcTextSizeA(size,FLT_MAX,0,letter).x+tracking;
    }
    const float railY=y+size*.48f;
    const float railWidth=ImMax(10.f*scale,(width-total)*.5f-42.f*scale);
    draw->AddLine({pos.x+24.f*scale,railY},{pos.x+24.f*scale+railWidth,railY},Accent(.4f),1.f*scale);
    draw->AddLine({pos.x+width-24.f*scale-railWidth,railY},{pos.x+width-24.f*scale,railY},Accent(.4f),1.f*scale);
    for(int i=0;i<3;++i) draw->AddLine({cx-13.f*scale+i*9.f*scale,y+size+9.f*scale},
        {cx-7.f*scale+i*9.f*scale,y+size+3.f*scale},Accent(.85f),2.f*scale);
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
    Frame(draw,pos,size,scale); Brand(draw,pos,size.x,scale,true);
    const float cx=pos.x+size.x*.5f;
    CenterText(draw,Body(),12.5f*scale,cx,pos.y+139.f*scale,IM_COL32(181,163,207,255),"SYSTEM START   |   VERIFY   |   LOAD");
    const auto center=pos+ImVec2(310,282)*scale;
    const float radii[]{31,57,84,108};
    const int dots[]{5,7,9,11};
    for(int ring=0;ring<4;++ring) {
        const float radius=radii[ring]*scale, direction=ring%2?-1.f:1.f;
        draw->AddCircle(center,radius,Accent(.15f),64,1.f*scale);
        const float start=time*direction*(.28f+ring*.06f);
        draw->PathArcTo(center,radius,start,start+1.10f+ring*.13f,22);
        draw->PathStroke(Accent(ring==3?.8f:.46f),0,(ring==3?2.5f:1.4f)*scale);
        for(int i=0;i<dots[ring];++i) {
            const float a=start+i*6.2831853f/dots[ring];
            const auto p=center+ImVec2(std::cos(a),std::sin(a))*radius;
            draw->AddCircleFilled(p,(i==0?3.f:1.8f)*scale,Accent(i==0?.95f:.42f),10);
        }
    }
    draw->AddCircleFilled(center,16.f*scale,Accent(.12f),32);
    draw->AddCircle(center,20.f*scale,Accent(.32f),32,1.f*scale);
    draw->AddCircleFilled(center,8.f*scale,IM_COL32(248,237,255,255),24);
    const char* labels[]{"Initializing","Verifying","Loading Data","Finalizing"};
    for(int i=0;i<4;++i) {
        const float x=pos.x+(83.f+i*151.f)*scale,y=pos.y+424.f*scale;
        const float done=(i+1)*.25f;
        const bool complete=progress>=done, current=progress>=i*.25f && !complete;
        if(i<3) draw->AddLine({x+25.f*scale,y},{x+126.f*scale,y},Accent(complete?.70f:.16f),1.3f*scale);
        if(complete) { auto tint=c::accent; tint.w=.68f;
            clay::Liquid(draw,{x-24.f*scale,y-24.f*scale},{x+24.f*scale,y+24.f*scale},24.f*scale,tint,1.1f);
        } else draw->AddCircleFilled({x,y},24.f*scale,IM_COL32(35,24,49,190),36);
        draw->AddCircle({x,y},24.f*scale,Accent(current?.9f:.30f),36,1.3f*scale);
        if(complete) ImGui::RenderCheckMark(draw,{x-7.f*scale,y-5.f*scale},IM_COL32(255,248,255,255),13.f*scale);
        if(current) { draw->PathArcTo({x,y},20.f*scale,time*2.4f,time*2.4f+2.9f,20); draw->PathStroke(Accent(.95f),0,2.f*scale); }
        CenterText(draw,Body(),12.5f*scale,x,y+32.f*scale,complete?IM_COL32(226,211,246,255):IM_COL32(162,148,180,255),labels[i]);
    }
    const auto bar=pos+ImVec2(48,494)*scale; const auto barSize=ImVec2(468,13)*scale;
    draw->AddRectFilled(bar,bar+barSize,IM_COL32(55,41,72,210),7.f*scale);
    if(progress>0.f) {
        const auto end=bar+ImVec2(barSize.x*progress,barSize.y);
        auto progressTint=c::accent; progressTint.w=.66f;
        clay::Liquid(draw,bar,end,7.f*scale,progressTint,1.4f);
        if(barSize.x*progress>14.f*scale) draw->AddLine(bar+ImVec2(7,2)*scale,end-ImVec2(7,11)*scale,IM_COL32(247,219,255,110),1.f*scale);
    }
    char percentage[16]; std::snprintf(percentage,sizeof(percentage),"%d%%",static_cast<int>(progress*100.f+.5f));
    Text(draw,Body(),16.f*scale,pos+ImVec2(532,488)*scale,IM_COL32(229,219,243,255),percentage);
    const auto info=pos+ImVec2(45,528)*scale;
    draw->AddRectFilled(info,info+ImVec2(530,48)*scale,IM_COL32(78,55,99,58),24.f*scale);
    draw->AddRect(info,info+ImVec2(530,48)*scale,IM_COL32(221,191,255,36),24.f*scale,0,1.f*scale);
    Icon(draw,ICON_FA_INFO_CIRCLE,info+ImVec2(24,24)*scale,18.f*scale,Accent(.85f));
    Text(draw,Body(),13.f*scale,info+ImVec2(47,6)*scale,IM_COL32(232,221,245,255),"Please wait");
    Text(draw,Body(),10.5f*scale,info+ImVec2(47,26)*scale,IM_COL32(173,158,190,255),"Preparing your interface. Keep the application open.");
}
}
