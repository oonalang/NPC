#pragma once
#include "Widgets.h"
#include <array>
#include <cctype>

namespace lite_keyboard {
inline constexpr ImU32 panel=IM_COL32(25,25,28,242),edge=IM_COL32(50,50,60,230);
inline constexpr ImU32 keyFill=IM_COL32(33,33,40,255),active=IM_COL32(39,39,49,255);
inline constexpr ImU32 accent=IM_COL32(192,195,255,255),ink=IM_COL32_WHITE,muted=IM_COL32(110,110,129,255);
inline constexpr int closeKey=0,firstDigit=1,firstLetter=11,deleteKey=39;
inline constexpr int symbolsKey=40,clearKey=41,spaceKey=42,caseKey=43,doneKey=44,keyCount=45;
struct Geometry {
    ImVec2 position{},size{};
    float x=1.f,y=1.f,text=1.f;
};
inline Geometry Fit(ImVec2 display,float reveal=1.f) {
    display.x=std::isfinite(display.x)?ImMax(1.f,display.x):1.f;
    display.y=std::isfinite(display.y)?ImMax(1.f,display.y):1.f;
    const float width=ImMax(1.f,display.x-24.f)/1120.f;
    const float height=ImMax(1.f,display.y-24.f)/338.f;
    const float donor=ImMax(.62f,ImMin(1.f,ImMin(width,display.y*.46f/338.f)));
    Geometry geometry;
    geometry.x=ImMin(donor,width);geometry.y=ImMin(donor,height);
    geometry.text=ImMin(geometry.y,ImMax(geometry.x,.62f));
    geometry.size={1120.f*geometry.x,338.f*geometry.y};
    const float bottom=ImMin(14.f*geometry.y,ImMax(0.f,display.y-geometry.size.y));
    geometry.position={(display.x-geometry.size.x)*.5f,
        display.y-geometry.size.y-bottom+(1.f-ImClamp(reveal,0.f,1.f))*34.f*geometry.y};
    return geometry;
}
inline std::size_t Length(const char* text,std::size_t capacity) {
    std::size_t length=0;
    if(text) while(length<capacity && text[length]) ++length;
    return length;
}
inline bool Append(char* buffer,std::size_t capacity,char value) {
    const auto length=Length(buffer,capacity);
    if(!buffer || !capacity || length>=capacity-1) return false;
    buffer[length]=value;buffer[length+1]=0;return true;
}
inline bool Backspace(char* buffer,std::size_t capacity) {
    auto length=Length(buffer,capacity);
    if(!buffer || !length) return false;
    --length;
    while(length && (static_cast<unsigned char>(buffer[length])&0xc0u)==0x80u) --length;
    buffer[length]=0;return true;
}
struct Key {
    ImRect bounds{};
    float hover=0,press=0;
    double pressedAt=-1;
};
struct State {
    ImGuiID owner=0;
    char* buffer=nullptr;
    std::array<Key,keyCount> keys{};
    double shownAt=0,lastTime=0;
    float progress=0;
    int frame=-2;
    bool visible=false,uppercase=false,symbols=false;
};
inline State state;
inline ImU32 Alpha(ImU32 color,float alpha) {
    const auto value=static_cast<ImU32>(((color>>24)&255)*ImClamp(alpha*ImGui::GetStyle().Alpha,0.f,1.f));
    return (color&0x00ffffffu)|(value<<24);
}
enum class Tone { Normal,Action,Accent };
inline bool DrawKey(int index,const char* label,ImVec2 position,ImVec2 size,const Geometry& geometry,
                    float alpha,float delta,double now,bool enabled,Tone tone=Tone::Normal) {
    auto& key=state.keys[index];key.bounds={position,position+size};
    ImGui::SetCursorScreenPos(position);ImGui::PushID(index);
    ImGui::BeginDisabled(!enabled);
    const bool clicked=lite_widgets::Tap(ImGui::InvisibleButton("##key",size));
    const bool hovered=ImGui::IsItemHovered();
    ImGui::EndDisabled();ImGui::PopID();
    key.hover+=((hovered?1.f:0.f)-key.hover)*ImMin(1.f,delta*14.f);
    if(clicked) key.pressedAt=now;
    key.press=key.pressedAt>=0?ImClamp(1.f-static_cast<float>(now-key.pressedAt)/.22f,0.f,1.f):0.f;
    ImU32 fill=keyFill,border=edge,text=ink;
    if(tone==Tone::Action) {fill=IM_COL32(30,30,36,255);text=accent;}
    if(tone==Tone::Accent) {fill=IM_COL32(35,36,54,255);border=accent;}
    if(key.hover>.18f || key.press>.05f) fill=active;
    const float rounding=6.f*geometry.y;
    const ImVec2 inset(2.4f*geometry.x*key.press,2.4f*geometry.y*key.press);
    const auto lo=position+inset,hi=position+size-inset;
    auto* draw=ImGui::GetWindowDrawList();
    draw->AddRectFilled(lo,hi,Alpha(fill,alpha),rounding);
    draw->AddRect(lo,hi,Alpha(border,alpha*(tone==Tone::Accent?.92f:.72f)),rounding,0,geometry.y);
    if(key.press>0) {
        const float release=1.f-key.press;
        const ImVec2 pulse(3.f*geometry.x*release,3.f*geometry.y*release);
        draw->AddRect(lo-pulse,hi+pulse,Alpha(accent,alpha*key.press*.58f),rounding+pulse.y,0,1.5f*geometry.y);
        draw->AddRectFilled(lo,hi,Alpha(accent,alpha*key.press*.18f),rounding);
    }
    auto* font=ImGui::GetFont();
    float fontSize=font->FontSize*(tone==Tone::Normal?1.f:.88f)*geometry.text;
    const auto measured=font->CalcTextSizeA(fontSize,FLT_MAX,0,label);
    if(measured.x>size.x-4.f*geometry.x) fontSize*=ImMax(1.f,size.x-4.f*geometry.x)/measured.x;
    const auto textSize=font->CalcTextSizeA(fontSize,FLT_MAX,0,label);
    draw->AddText(font,fontSize,lo+(hi-lo-textSize)*.5f+ImVec2(0,1.2f*geometry.y*key.press),Alpha(text,alpha),label);
    return clicked;
}
inline void Render(const char* id,char* buffer,std::size_t capacity,bool* show) {
    if(!id || !show || !*show || !buffer || capacity<2) return;
    const auto owner=ImHashStr(id);
    const auto frame=ImGui::GetFrameCount();
    const double now=ImGui::GetTime();
    if(!state.visible || state.owner!=owner || state.buffer!=buffer || frame-state.frame>1 ||
        now<state.lastTime || now-state.lastTime>.25) {
        const bool upper=state.uppercase,symbols=state.symbols;
        state={};state.uppercase=upper;state.symbols=symbols;
        state.visible=true;state.owner=owner;state.buffer=buffer;state.shownAt=state.lastTime=now;
    }
    const float delta=ImClamp(static_cast<float>(now-state.lastTime),1.f/240.f,.1f);
    state.lastTime=now;state.frame=frame;
    state.progress+=(1.f-state.progress)*ImMin(1.f,delta*9.f);
    if(state.progress>.995f) state.progress=1.f;
    const float eased=state.progress*state.progress*(3.f-2.f*state.progress);
    const auto geometry=Fit(ImGui::GetIO().DisplaySize,eased);
    const auto pos=geometry.position,size=geometry.size;
    const bool enabled=now-state.shownAt>.10;
    ImGui::SetNextWindowPos(pos,ImGuiCond_Always);ImGui::SetNextWindowSize(size,ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.f);
    ImGui::Begin(id,nullptr,ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|
        ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings|
        ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::SetWindowFontScale(1.f);
    auto* draw=ImGui::GetWindowDrawList();auto* font=ImGui::GetFont();
    draw->AddRectFilled(pos,pos+size,Alpha(panel,eased),9.f*geometry.y);
    draw->AddRect(pos,pos+size,Alpha(edge,eased),9.f*geometry.y,0,geometry.y);
    const ImVec2 innerMin=pos+ImVec2(18.f*geometry.x,12.f*geometry.y);
    const ImVec2 innerMax=pos+size-ImVec2(18.f*geometry.x,12.f*geometry.y);
    draw->AddRectFilled(innerMin+ImVec2(0,5.f*geometry.y),innerMin+ImVec2(44.f*geometry.x,8.f*geometry.y),Alpha(accent,eased),1.5f*geometry.y);
    std::size_t characters=0;
    for(std::size_t at=0,length=Length(buffer,capacity);at<length;++at)
        if((static_cast<unsigned char>(buffer[at])&0xc0u)!=0x80u) ++characters;
    char count[32];std::snprintf(count,sizeof(count),"%zu CHARS",characters);
    const float countSize=font->FontSize*.76f*geometry.text;
    const auto measured=font->CalcTextSizeA(countSize,FLT_MAX,0,count);
    const bool narrow=geometry.x<geometry.y;
    const float closeWidth=ImMin(innerMax.x-innerMin.x,ImMax(narrow?24.f:0.f,28.f*geometry.x));
    const float closeHeight=ImMin(45.f*geometry.y,ImMax(narrow?24.f:0.f,28.f*geometry.y));
    draw->AddText(font,countSize,{innerMax.x-closeWidth-14.f*geometry.x-measured.x,innerMin.y+2.f*geometry.y},Alpha(muted,eased),count);
    if(DrawKey(closeKey,"X",{innerMax.x-closeWidth,pos.y+(narrow?(45.f*geometry.y-closeHeight)*.5f:9.f*geometry.y)},
        {closeWidth,closeHeight},geometry,eased,delta,now,enabled,Tone::Action)) *show=false;
    draw->AddLine({innerMin.x,pos.y+45.f*geometry.y},{innerMax.x,pos.y+45.f*geometry.y},Alpha(edge,eased*.72f),geometry.y);
    const float width=innerMax.x-innerMin.x,gap=8.f*geometry.x;
    const float keyWidth=(width-gap*9.f)/10.f,keyHeight=44.f*geometry.y,rowGap=7.f*geometry.y;
    auto rowX=[&](int amount) {return innerMin.x+(width-amount*keyWidth-(amount-1)*gap)*.5f;};
    const char* rows[]{"1234567890",state.symbols?"!@#$%^&*()":"qwertyuiop",
        state.symbols?"_-+=:;/?.":"asdfghjkl",state.symbols?"[]{}<>~|\\":"-zxcvbnm"};
    int index=firstDigit;float y=pos.y+48.f*geometry.y;
    for(int row=0;row<4;++row) {
        const int amount=static_cast<int>(std::strlen(rows[row]));
        const float start=rowX(amount+(row==3?1:0));
        for(int column=0;column<amount;++column) {
            const char key[]{static_cast<char>(state.uppercase?std::toupper(static_cast<unsigned char>(rows[row][column])):rows[row][column]),0};
            if(DrawKey(index++,key,{start+column*(keyWidth+gap),y},{keyWidth,keyHeight},geometry,eased,delta,now,enabled))
                Append(buffer,capacity,key[0]);
        }
        if(row==3 && DrawKey(deleteKey,"DEL",{start+amount*(keyWidth+gap),y},{keyWidth,keyHeight},
            geometry,eased,delta,now,enabled,Tone::Action)) Backspace(buffer,capacity);
        y+=keyHeight+rowGap;
    }
    float x=innerMin.x;
    if(DrawKey(symbolsKey,state.symbols?"abc":"#+=",{x,y},{keyWidth,keyHeight},geometry,eased,delta,now,enabled,Tone::Action)) state.symbols=!state.symbols;
    x+=keyWidth+gap;
    const float clearWidth=2.f*keyWidth+gap,spaceWidth=4.f*keyWidth+3.f*gap,doneWidth=2.f*keyWidth+gap;
    if(DrawKey(clearKey,"CLEAR",{x,y},{clearWidth,keyHeight},geometry,eased,delta,now,enabled,Tone::Action)) buffer[0]=0;
    x+=clearWidth+gap;
    if(DrawKey(spaceKey,"SPACE",{x,y},{spaceWidth,keyHeight},geometry,eased,delta,now,enabled)) Append(buffer,capacity,' ');
    x+=spaceWidth+gap;
    if(DrawKey(caseKey,state.uppercase?"abc":"ABC",{x,y},{keyWidth,keyHeight},geometry,eased,delta,now,enabled,Tone::Action)) state.uppercase=!state.uppercase;
    x+=keyWidth+gap;
    if(DrawKey(doneKey,"DONE",{x,y},{doneWidth,keyHeight},geometry,eased,delta,now,enabled,Tone::Accent)) *show=false;
    if(!*show) state.visible=false;
    ImGui::End();ImGui::PopStyleVar(2);
}
}
