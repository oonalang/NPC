#pragma once
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "../../ImGui/imgui_internal.h"
#include "../../ImGui/imgui_settings.h"
#include "../../ImGui/ClayGlass.h"
#include "../../ImGui/custom_widgets.hpp"
#include "MusicPlayer.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace home_tab {
struct HomeStats {
    float fps=0.f;
    float frameMs=0.f;
    double uptime=0.;
    int width=0;
    int height=0;
    int activeFeatures=0;
    bool keySaved=false;
};
struct PerformanceHistory {
    static constexpr int capacity=240;
    static constexpr double interval=.1;
    std::array<float,capacity> fps{};
    std::array<float,capacity> frameMs{};
    ImGuiContext* context=nullptr;
    int next=0,count=0;
    double last=-1.;
    void Observe(const HomeStats& stats,double now) {
        if(!std::isfinite(now)) return;
        auto* current=ImGui::GetCurrentContext();
        if(context!=current || (last>=0. && (now<last || now-last>.5))) {
            next=count=0;
            last=-1.;
            context=current;
        }
        if(last>=0. && now-last<interval-.000001) return;
        fps[next]=std::isfinite(stats.fps)?std::max(0.f,stats.fps):0.f;
        frameMs[next]=std::isfinite(stats.frameMs)?std::max(0.f,stats.frameMs):0.f;
        next=(next+1)%capacity;
        count=std::min(count+1,capacity);
        last=now;
    }
    float Fps(int index) const { return fps[(next-count+index+capacity)%capacity]; }
    float Average() const {
        if(!count) return 0.f;
        float total=0.f;
        for(int i=0;i<count;++i) total+=Fps(i);
        return total/count;
    }
};
inline PerformanceHistory history;
inline void Observe(const HomeStats& stats,double now) { history.Observe(stats,now); }
inline ImFont* Body() { return font::inter_semibold?font::inter_semibold:ImGui::GetFont(); }
inline ImFont* Title() { return font::inter_title?font::inter_title:Body(); }
inline ImU32 Color(ImVec4 color,float opacity=1.f) { color.w*=opacity; return ImGui::GetColorU32(color); }
inline void Text(ImDrawList* draw,ImVec2 position,float size,ImU32 color,const char* text,ImFont* font=nullptr) {
    draw->AddText(font?font:Body(),size,position,color,text);
}
inline void Uptime(char* out,std::size_t size,double seconds) {
    const int total=std::isfinite(seconds)?static_cast<int>(std::clamp(seconds,0.,35999999.)):0;
    std::snprintf(out,size,"%02d:%02d:%02d",total/3600,(total/60)%60,total%60);
}
inline void Card(ImDrawList* draw,ImVec2 lo,ImVec2 size,float scale) {
    clay::Surface(draw,lo,lo+size,18.f*scale,false);
    draw->AddLine(lo+ImVec2(19,1)*scale,lo+ImVec2(size.x-19.f*scale,scale),
        IM_COL32(236,224,255,20),scale);
}
inline void Hero(const HomeStats& stats,float width,float scale) {
    auto* draw=ImGui::GetWindowDrawList();
    const auto lo=ImGui::GetCursorScreenPos();
    const float height=50.f*scale;
    clay::Liquid(draw,lo,lo+ImVec2(width,height),20.f*scale,ImVec4(.12f,.085f,.18f,.70f),1.f);
    draw->AddRect(lo,lo+ImVec2(width,height),IM_COL32(220,198,255,40),20.f*scale,0,scale);
    const auto title=lo+ImVec2(16,6)*scale;
    Text(draw,title,24.f*scale,Color(c::text::text_active),"Home",Title());
    Text(draw,lo+ImVec2(16,34)*scale,10.5f*scale,Color(c::text::text_hov),"Your session, performance and music.");
    const char* badge="LIVE SESSION";
    const float badgeFont=10.f*scale;
    const auto badgeText=Body()->CalcTextSizeA(badgeFont,FLT_MAX,0.f,badge);
    const float badgeWidth=badgeText.x+31.f*scale;
    const auto badgeLo=lo+ImVec2(width-badgeWidth-18.f*scale,6.f*scale);
    draw->AddRectFilled(badgeLo,badgeLo+ImVec2(badgeWidth,23.f*scale),IM_COL32(116,216,176,17),11.5f*scale);
    draw->AddCircleFilled(badgeLo+ImVec2(11,11.5f)*scale,2.5f*scale,IM_COL32(117,227,177,240),10);
    Text(draw,badgeLo+ImVec2(20,6)*scale,badgeFont,IM_COL32(173,226,201,255),badge);
    if(width>420.f*scale) {
        char surface[64];
        std::snprintf(surface,sizeof(surface),"%d \xC3\x97 %d  \xC2\xB7  %s",stats.width,stats.height,stats.keySaved?"Key saved":"Session active");
        const auto measured=Body()->CalcTextSizeA(10.f*scale,FLT_MAX,0.f,surface);
        Text(draw,lo+ImVec2(width-18.f*scale-measured.x,34.f*scale),10.f*scale,Color(c::text::text_hov,.85f),surface);
    }
    ImGui::Dummy(ImVec2(width,height));
}
inline void Metric(ImDrawList* draw,ImVec2 lo,ImVec2 size,float scale,const char* label,const char* value,int kind) {
    Card(draw,lo,size,scale);
    const auto accent=Color(c::accent);
    Text(draw,lo+ImVec2(15,12)*scale,10.5f*scale,Color(c::text::text_hov),label);
    const auto measured=Title()->CalcTextSizeA(23.f*scale,FLT_MAX,0.f,value);
    const float valueSize=23.f*scale*std::min(1.f,std::max(1.f,size.x-30.f*scale)/std::max(1.f,measured.x));
    Text(draw,lo+ImVec2(15,30)*scale,valueSize,Color(c::text::text_active),value,Title());
    const auto glyph=lo+ImVec2(size.x-27.f*scale,22.f*scale);
    const auto line=Color(c::accent,.60f);
    if(kind==0) {
        const ImVec2 points[]{glyph+ImVec2(-8,4)*scale,glyph+ImVec2(-3,4)*scale,glyph+ImVec2(0,-5)*scale,
            glyph+ImVec2(3,8)*scale,glyph+ImVec2(6,1)*scale,glyph+ImVec2(10,1)*scale};
        draw->AddPolyline(points,6,line,0,1.6f*scale);
    } else if(kind==1) {
        draw->AddCircle(glyph,7.f*scale,line,18,1.5f*scale);
        draw->AddLine(glyph,glyph+ImVec2(0,-4)*scale,accent,1.5f*scale);
        draw->AddLine(glyph,glyph+ImVec2(4,2)*scale,accent,1.5f*scale);
    } else if(kind==2) {
        draw->AddRect(glyph-ImVec2(8,6)*scale,glyph+ImVec2(8,6)*scale,line,3.f*scale,0,1.5f*scale);
        draw->AddLine(glyph+ImVec2(-3,0)*scale,glyph+ImVec2(4,0)*scale,accent,1.5f*scale);
    } else {
        for(int row=0;row<2;++row) for(int col=0;col<2;++col) {
            const auto p=glyph+ImVec2(-7.f+col*9.f,-7.f+row*9.f)*scale;
            draw->AddRectFilled(p,p+ImVec2(5,5)*scale,(row==0&&col==0)?accent:line,1.5f*scale);
        }
    }
}
inline void Metrics(const HomeStats& stats,float width,float scale) {
    char rate[32],frame[32],session[32],enabled[32];
    std::snprintf(rate,sizeof(rate),"%.0f FPS",std::isfinite(stats.fps)?std::max(0.f,stats.fps):0.f);
    std::snprintf(frame,sizeof(frame),"%.1f ms",std::isfinite(stats.frameMs)?std::max(0.f,stats.frameMs):0.f);
    Uptime(session,sizeof(session),stats.uptime);
    std::snprintf(enabled,sizeof(enabled),"%d",std::max(0,stats.activeFeatures));
    const char* labels[]{"FRAME RATE","FRAME TIME","SESSION","ENABLED OPTIONS"};
    const char* values[]{rate,frame,session,enabled};
    const int columns=width>=620.f*scale?4:2;
    const float gap=8.f*scale,height=56.f*scale;
    const float cardWidth=(width-gap*(columns-1))/columns;
    const auto origin=ImGui::GetCursorScreenPos();
    auto* draw=ImGui::GetWindowDrawList();
    for(int i=0;i<4;++i)
        Metric(draw,origin+ImVec2((i%columns)*(cardWidth+gap),(i/columns)*(height+gap)),
            ImVec2(cardWidth,height),scale,labels[i],values[i],i);
    ImGui::Dummy(ImVec2(width,(4/columns)*height+(4/columns-1)*gap));
}
inline void Performance(ImVec2 size,float scale) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(14,12)*scale);
    ImGui::BeginChild("##home_performance",size,false,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoMove|
        ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_AlwaysUseWindowPadding);
    auto* draw=ImGui::GetWindowDrawList();
    const auto lo=ImGui::GetWindowPos();
    Card(draw,lo,size,scale);
    ImGui::TextColored(c::text::text_active,"Performance");
    ImGui::PushStyleColor(ImGuiCol_Text,c::text::text_hov);
    ImGui::TextUnformatted(history.count==PerformanceHistory::capacity?"Last 24 seconds":"Live render history");
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0,5.f*scale));
    const auto graphLo=ImGui::GetCursorScreenPos();
    const auto available=ImGui::GetContentRegionAvail();
    const auto graphSize=ImVec2(std::max(1.f,available.x),std::max(50.f*scale,available.y-38.f*scale));
    const auto graphHi=graphLo+graphSize;
    draw->AddRectFilled(graphLo,graphHi,IM_COL32(11,11,19,105),10.f*scale);
    float peak=0.f,low=FLT_MAX;
    for(int i=0;i<history.count;++i) { peak=std::max(peak,history.Fps(i)); low=std::min(low,history.Fps(i)); }
    const float ceiling=std::max(90.f,std::ceil(peak/30.f)*30.f);
    const ImVec2 plotLo=graphLo+ImVec2(9,12)*scale,plotHi=graphHi-ImVec2(9,12)*scale;
    for(int i=0;i<3;++i) {
        const float y=plotLo.y+(plotHi.y-plotLo.y)*i*.5f;
        draw->AddLine({plotLo.x,y},{plotHi.x,y},IM_COL32(220,206,244,13),scale);
    }
    if(history.count>=2) {
        const auto point=[&](int i) {
            return ImVec2(plotLo.x+(plotHi.x-plotLo.x)*(PerformanceHistory::capacity-history.count+i)/(PerformanceHistory::capacity-1),
                plotHi.y-(plotHi.y-plotLo.y)*std::clamp(history.Fps(i)/ceiling,0.f,1.f));
        };
        for(int i=0;i+1<history.count;++i) {
            const auto a=point(i),b=point(i+1);
            draw->AddQuadFilled(a,b,{b.x,plotHi.y},{a.x,plotHi.y},Color(c::accent,.10f));
            draw->AddLine(a,b,Color(c::accent,.9f),1.8f*scale);
        }
        const auto last=point(history.count-1);
        draw->AddCircleFilled(last,3.f*scale,Color(c::accent),12);
    }
    if(history.count<10) {
        const char* message="Collecting live performance";
        const auto measured=Body()->CalcTextSizeA(11.f*scale,FLT_MAX,0.f,message);
        Text(draw,(graphLo+graphHi-measured)*.5f,11.f*scale,Color(c::text::text_hov),message);
    }
    char maximum[24];std::snprintf(maximum,sizeof(maximum),"%.0f FPS",ceiling);
    const auto measured=Body()->CalcTextSizeA(9.f*scale,FLT_MAX,0.f,maximum);
    Text(draw,{graphHi.x-measured.x-9.f*scale,graphLo.y+5.f*scale},9.f*scale,Color(c::text::text_hov,.7f),maximum);
    ImGui::Dummy(graphSize);
    ImGui::Dummy(ImVec2(0,7.f*scale));
    char summary[96];
    if(history.count>=2) std::snprintf(summary,sizeof(summary),"LOW %.0f   AVG %.0f   HIGH %.0f FPS",low,history.Average(),peak);
    else std::snprintf(summary,sizeof(summary),"Your recent frame rate appears here.");
    Text(draw,ImGui::GetCursorScreenPos(),10.f*scale,Color(c::text::text_hov),summary);
    ImGui::Dummy(ImVec2(0,14.f*scale));
    ImGui::EndChild();
    ImGui::PopStyleVar();
}
inline void Music(ImVec2 size,float scale) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(14,12)*scale);
    ImGui::BeginChild("##home_music",size,false,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoMove|
        ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_AlwaysUseWindowPadding);
    Card(ImGui::GetWindowDrawList(),ImGui::GetWindowPos(),size,scale);
    ImGui::TextColored(c::text::text_hov,"YOUR MUSIC");
    ImGui::Dummy(ImVec2(0,3.f*scale));
    music_player::Draw();
    ImGui::EndChild();
    ImGui::PopStyleVar();
}
inline void Draw(const HomeStats& stats,ImVec2 available) {
    if(available.x<1.f || available.y<1.f) return;
    Observe(stats,ImGui::GetTime());
    const float scale=std::clamp(c::scale,.8f,1.15f),gap=8.f*scale;
    ImGui::PushID("HomeDashboard");
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(1.f,1.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(0,gap));
    ImGui::BeginChild("##home_dashboard",available,false,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoMove|
        ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_AlwaysUseWindowPadding);
    const float width=std::max(1.f,ImGui::GetContentRegionAvail().x);
    Hero(stats,width,scale);
    const bool columns=width>=680.f*scale;
    if(columns) {
        Metrics(stats,width,scale);
        const float panelHeight=std::max(195.f,ImGui::GetContentRegionAvail().y);
        const float leftWidth=(width-gap)*.46f;
        Performance({leftWidth,panelHeight},scale);
        ImGui::SameLine(0,gap);
        Music({width-leftWidth-gap,panelHeight},scale);
    } else {
        const float musicHeight=std::clamp(ImGui::GetContentRegionAvail().y,200.f*scale,250.f*scale);
        Music({width,musicHeight},scale);
        Metrics(stats,width,scale);
        Performance({width,210.f*scale},scale);
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopID();
}
}
