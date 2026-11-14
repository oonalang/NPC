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
inline void PillMaterial(ImDrawList* draw,ImVec2 lo,ImVec2 size,float scale,bool primary,bool held,bool hovered) {
    auto tint=primary ? ImVec4(c::accent.x*.32f,c::accent.y*.32f,c::accent.z*.32f,1.f) : ImVec4(.10f,.075f,.15f,1.f);
    tint.w=primary ? (held?.80f:hovered?.74f:.70f) : .70f;
    clay::Liquid(draw,lo,lo+size,size.y*.5f,tint,primary?1.35f:1.f);
    draw->AddRect(lo,lo+size,ImGui::GetColorU32(ImVec4(1.f,.93f,1.f,primary?.32f:.19f)),size.y*.5f,0,1.f*scale);
    if(primary) draw->AddRect(lo+ImVec2(0,2.f*scale),lo+size,
        Accent(.10f),size.y*.5f,0,4.f*scale);
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
inline void Spinner(ImDrawList* draw,const char* icon,ImVec2 center,float size,ImU32 color,float angle) {
    const int first=draw->VtxBuffer.Size;
    Icon(draw,icon,center,size,color);
    const float sine=std::sin(angle),cosine=std::cos(angle);
    for(int i=first;i<draw->VtxBuffer.Size;++i) {
        auto& point=draw->VtxBuffer[i].pos;
        const auto offset=point-center;
        point=center+ImVec2(offset.x*cosine-offset.y*sine,offset.x*sine+offset.y*cosine);
    }
}
inline void Droplet(ImDrawList* draw,ImVec2 center,float radius,ImU32 color) {
    draw->PathLineTo(ImVec2(center.x+std::cos(-.55f)*radius,center.y+std::sin(-.55f)*radius));
    draw->PathArcTo(center,radius,-.55f,3.14159265f+.55f,24);
    draw->PathLineTo(ImVec2(center.x,center.y-radius*1.85f));
    draw->PathFillConvex(color);
}
inline void LiquidFill(ImDrawList* draw,ImVec2 lo,ImVec2 size,float level,float time) {
    const ImVec2 hi=lo+size;
    const float radius=size.y*.5f,cy=lo.y+size.y-radius;
    const float top=ImLerp(hi.y,lo.y,ImClamp(level,0.f,1.f));
    const float amplitude=6.f*(1.f-level)*(size.y/66.f);
    draw->PathLineTo(ImVec2(lo.x,top));
    for(int i=1;i<=36;++i) {
        const float t=i/36.f;
        draw->PathLineTo(ImVec2(lo.x+size.x*t,top+std::sin(t*9.4f+time*8.f)*amplitude*std::sin(t*3.14159265f)));
    }
    draw->PathLineTo(ImVec2(hi.x,cy));
    draw->PathArcTo(ImVec2(hi.x-radius,cy),radius,0.f,1.5707963f,10);
    draw->PathLineTo(ImVec2(lo.x+radius,hi.y));
    draw->PathArcTo(ImVec2(lo.x+radius,cy),radius,1.5707963f,3.14159265f,10);
    draw->PathFillConvex(IM_COL32(255,255,255,255));
}
inline void Splash(ImDrawList* draw,ImVec2 lo,ImVec2 size,float amount,float scale) {
    const float bottom=lo.y+size.y-2.f*scale;
    const auto alpha=static_cast<ImU32>((1.f-amount)*(1.f-amount)*255.f);
    draw->AddLine(ImVec2(lo.x+22.f*scale,bottom),ImVec2(lo.x+size.x-22.f*scale,bottom),
        IM_COL32(255,255,255,alpha),1.6f*scale);
    for(int i=0;i<8;++i) {
        const float direction=(i%2?1.f:-1.f);
        const float spread=(16.f+i*11.f)*scale;
        const float rise=std::sin(3.14159265f*ImMin(1.f,amount*1.15f))*(10.f+(i%3)*8.f)*scale;
        const float fall=(1.f-amount)*8.f*scale;
        draw->AddCircleFilled(ImVec2(lo.x+size.x*.5f+direction*spread,bottom-rise+fall),
            (2.8f-(i%2)*1.1f)*scale*(1.f-amount*.45f),IM_COL32(255,255,255,alpha));
    }
}
inline void CollapseLogin(ImDrawList* draw,int first,ImVec2 center,float amount) {
    if(amount<=0.f) return;
    const float shrink=1.f-.84f*amount;
    const float angle=2.7f*amount;
    const float sine=std::sin(angle),cosine=std::cos(angle);
    const float fade=1.f-amount;
    for(int i=first;i<draw->VtxBuffer.Size;++i) {
        auto& vertex=draw->VtxBuffer[i];
        const auto offset=(vertex.pos-center)*shrink;
        vertex.pos=center+ImVec2(offset.x*cosine-offset.y*sine,offset.x*sine+offset.y*cosine);
        const auto alpha=static_cast<ImU32>(((vertex.col>>IM_COL32_A_SHIFT)&255)*fade);
        vertex.col=(vertex.col&~IM_COL32_A_MASK)|(alpha<<IM_COL32_A_SHIFT);
    }
#ifndef CLAY_CPU_PREVIEW
    for(const auto& command : draw->CmdBuffer)
        if(command.UserCallback==clay::DrawMaterial)
            static_cast<clay::MaterialDraw*>(command.UserCallbackData)->alpha*=fade;
#endif
}
inline void RevealDraw(ImDrawList* draw,int first,float reveal,ImVec2 lo,ImVec2 hi) {
    if(reveal>=1.f) return;
    for(int i=first;i<draw->VtxBuffer.Size;++i) {
        auto& vertex=draw->VtxBuffer[i];
        const float depth=ImClamp((vertex.pos.y-lo.y)/ImMax(1.f,hi.y-lo.y),0.f,1.f);
        const float amount=ImClamp(reveal*1.7f-depth*.7f,0.f,1.f);
        const auto alpha=static_cast<ImU32>(((vertex.col>>IM_COL32_A_SHIFT)&255)*amount);
        vertex.col=(vertex.col&~IM_COL32_A_MASK)|(alpha<<IM_COL32_A_SHIFT);
    }
#ifndef CLAY_CPU_PREVIEW
    for(const auto& command : draw->CmdBuffer)
        if(command.UserCallback==clay::DrawMaterial)
            static_cast<clay::MaterialDraw*>(command.UserCallbackData)->alpha*=reveal;
#endif
}
inline float Lumpy(float angle,float time) {
    return 1.f+.11f*std::sin(angle*5.f+time*2.1f)+.065f*std::sin(angle*9.f-time*1.4f)
        +.035f*std::sin(angle*17.f+time*3.3f);
}
inline void Shockwave(ImDrawList* draw,ImVec2 center,float radius,float alpha,float scale) {
    const float time=static_cast<float>(ImGui::GetTime());
    ImVec2 points[72],ring[72];
    for(int i=0;i<72;++i) {
        const float angle=i*6.283185307179586f/72.f;
        const float r=radius*Lumpy(angle,time);
        points[i]=center+ImVec2(std::cos(angle)*r,std::sin(angle)*r);
    }
    const auto a=static_cast<ImU32>(ImClamp(alpha,0.f,1.f)*255.f);
    draw->AddPolyline(points,72,IM_COL32(150,120,255,(a*26)/255),ImDrawFlags_Closed,radius*.16f);
    const auto scaled=[&](float factor) {
        for(int i=0;i<72;++i) ring[i]=center+(points[i]-center)*factor;
    };
    scaled(1.02f); draw->AddPolyline(ring,72,IM_COL32(255,72,96,(a*70)/255),ImDrawFlags_Closed,3.f*scale);
    scaled(.982f); draw->AddPolyline(ring,72,IM_COL32(84,255,206,(a*70)/255),ImDrawFlags_Closed,3.f*scale);
    scaled(1.f); draw->AddPolyline(ring,72,IM_COL32(118,132,255,(a*96)/255),ImDrawFlags_Closed,2.f*scale);
    scaled(.9f); draw->AddPolyline(ring,72,IM_COL32(255,150,66,(a*120)/255),ImDrawFlags_Closed,5.f*scale);
    scaled(1.f); draw->AddPolyline(ring,72,IM_COL32(226,232,255,a),ImDrawFlags_Closed,2.2f*scale);
}
inline void BlackHole(ImDrawList* draw,ImVec2 center,const LoginTransitionFrame& frame,float scale) {
    if(frame.hole>0.f && frame.flash<=0.f) {
        const float amount=Ease(frame.hole);
        draw->AddCircleFilled(center,ImLerp(3.f,23.f,amount)*scale,IM_COL32(4,3,9,235),32);
        const float ring=ImLerp(52.f,9.f,amount)*scale;
        draw->AddCircle(center,ring,IM_COL32(214,184,255,215),48,2.6f*scale);
        for(int i=1;i<=3;++i)
            draw->AddCircle(center,ring+(i*2.4f)*scale,IM_COL32(176,118,255,static_cast<ImU32>(72-i*15)),48,1.4f*scale);
        draw->AddCircleFilled(center,ImMax(1.f,ring*.34f),IM_COL32(255,252,255,240),24);
    }
    if(frame.flash>0.f && frame.shock<=0.f) {
        const float amount=Ease(frame.flash);
        for(int i=8;i>=1;--i)
            draw->AddCircleFilled(center,(5.f+i*6.5f)*scale*(1.f+amount*.75f),
                ImGui::GetColorU32(ImVec4(1.f,.97f,1.f,.10f*(1.f-i/10.f))),40);
        draw->AddCircleFilled(center,ImLerp(5.f,22.f,amount)*scale,IM_COL32(255,255,255,255),32);
        const float arm=ImLerp(12.f,62.f,amount)*scale;
        draw->AddLine(center-ImVec2(arm,0),center+ImVec2(arm,0),IM_COL32(255,250,255,205),1.8f*scale);
        draw->AddLine(center-ImVec2(0,arm),center+ImVec2(0,arm),IM_COL32(255,250,255,205),1.8f*scale);
    }
    if(frame.shock>0.f) {
        const float far=ImMin(ImGui::GetIO().DisplaySize.x,ImGui::GetIO().DisplaySize.y)*.72f;
        const float radius=ImLerp(26.f*scale,ImMax(far,40.f*scale),1.f-(1.f-frame.shock)*(1.f-frame.shock));
        const float alpha=frame.shock<.08f?frame.shock/.08f:ImMax(0.f,1.f-(frame.shock-.08f)/.92f);
        Shockwave(draw,center,radius,alpha,scale);
    }
}
inline bool AuthenticateButton(ImVec2 lo,ImVec2 size,float scale,bool pending,const LoginTransitionFrame& frame) {
    if(!loginTransition.active)
        return Pill("##login_auth","Authenticate",ICON_FA_SIGN_IN,lo,size,scale,true,pending);
    ImGui::SetCursorScreenPos(lo);
    ImGui::BeginDisabled();
    ImGui::InvisibleButton("##login_auth",size);
    ImGui::EndDisabled();
    auto* draw=ImGui::GetWindowDrawList();
    const float time=static_cast<float>(ImGui::GetTime());
    PillMaterial(draw,lo,size,scale,true,false,false);
    const auto center=lo+size*.5f;
    const auto paint=ImGui::GetColorU32(ImVec4(.96f,.92f,1.f,1.f));
    const float textSize=17.f*scale;
    const auto labelSize=Body()->CalcTextSizeA(textSize,FLT_MAX,0,"Authenticate");
    const float startX=lo.x+(size.x-labelSize.x-35.f*scale)*.5f+9.5f*scale;
    const auto iconCenter=ImLerp(ImVec2(startX,center.y),center,Ease(frame.morph));
    const float iconSize=ImLerp(19.f,32.f,Ease(frame.morph))*scale;
    if(frame.wave>0.f) {
        if(frame.wave>=.995f) draw->AddRectFilled(lo,lo+size,IM_COL32(255,255,255,255),size.y*.5f);
        else LiquidFill(draw,lo,size,Ease(frame.wave),time);
    }
    if(frame.splash>0.f) Splash(draw,lo,size,Ease(frame.splash),scale);
    if(frame.wave<=0.f && frame.splash<=0.f) {
        const auto dropCenter=ImVec2(iconCenter.x,ImLerp(iconCenter.y,lo.y+size.y-15.f*scale,Ease(frame.fall)));
        if(frame.droplet<1.f)
            Spinner(draw,ICON_FA_SIGN_IN,dropCenter,iconSize*ImLerp(1.f,.78f,Ease(frame.droplet)),paint,frame.rotation);
        if(frame.droplet>0.f)
            Droplet(draw,dropCenter,ImLerp(19.f,7.4f,Ease(frame.droplet))*scale,
                ImGui::GetColorU32(ImVec4(1,1,1,Ease(frame.droplet))));
    }
    if(frame.label>0.f) Text(draw,Body(),textSize,
        ImVec2(startX+25.5f*scale,lo.y+(size.y-labelSize.y)*.5f),
        ImGui::GetColorU32(ImVec4(.96f,.92f,1.f,frame.label)),"Authenticate");
    if(frame.check>0.f) {
        const float checkSize=26.f*scale*ImLerp(.75f,1.f,Ease(frame.check));
        ImGui::RenderCheckMark(draw,center-ImVec2(checkSize*.5f,checkSize*.55f),
            ImGui::GetColorU32(ImVec4(.025f,.025f,.035f,Ease(frame.check))),checkSize);
    }
    return false;
}
struct LoginActions { bool paste=false, load=false, submit=false; };
struct LoginPosition { ImVec2 resting,viewportPos,viewportSize; bool known=false, shifted=false; };
inline LoginPosition loginPosition;

template<class Input>
LoginActions RenderLogin(char* key, size_t capacity, bool pending, const char* error, bool cached,
                         bool& keyboard, Input input) {
    LoginActions result;
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
    const auto animation=loginTransition.At(Now());
    const bool busy=pending || loginTransition.active;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
    if(ImGui::Begin("Login Menu",nullptr,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoSavedSettings|
        ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse)) {
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
        const char* status=failed?error:loginTransition.succeeded?(animation.fall>0.f?"Access granted":"Checking access..."):busy?"Checking access...":cached?"Saved key available":"Enter your access key to continue";
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
        CollapseLogin(draw,first,pos+size*.5f,Ease(animation.suck));
        if(animation.hole>0.f) BlackHole(ImGui::GetForegroundDrawList(),pos+size*.5f,animation,scale);
    }
    ImGui::End(); ImGui::PopStyleVar();
    return result;
}

inline void LoadingCard(ImDrawList* draw, ImVec2 pos, ImVec2 size, float scale, float progress, float time) {
    Frame(draw,pos,size,scale); Brand(draw,pos,size.x,scale);
    const float cx=pos.x+size.x*.5f;
    const auto tip=TipAt(time);
    const float tipSize=13.f*scale,tipWidth=612.f*scale;
    const auto tipBounds=Body()->CalcTextSizeA(tipSize,FLT_MAX,tipWidth,loadingTips[tip.index]);
    draw->PushClipRect(pos+ImVec2(30,134)*scale,pos+ImVec2(650,160)*scale,true);
    const ImVec2 tipPos(cx-tipBounds.x*.5f,pos.y+146.f*scale-tipBounds.y*.5f);
    Text(draw,Body(),tipSize,tipPos,
        ImGui::GetColorU32(ImVec4(.86f,.84f,.92f,tip.opacity)),loadingTips[tip.index],tipWidth);
    draw->PopClipRect();
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
    clay::Liquid(draw,info,info+ImVec2(612,58)*scale,29.f*scale,ImVec4(.075f,.06f,.125f,.68f),1.2f);
    draw->AddRect(info,info+ImVec2(612,58)*scale,IM_COL32(229,222,248,68),29.f*scale,0,1.f*scale);
    Icon(draw,ICON_FA_INFO_CIRCLE,info+ImVec2(40,29)*scale,23.f*scale,IM_COL32(225,219,241,245));
    Text(draw,Body(),14.f*scale,info+ImVec2(66,9)*scale,IM_COL32(242,237,250,255),"Please wait");
    Text(draw,Body(),12.f*scale,info+ImVec2(66,32)*scale,IM_COL32(224,218,235,255),"Preparing your interface. Keep the application open.");
}
}
