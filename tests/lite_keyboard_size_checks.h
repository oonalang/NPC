#pragma once

bool keyboardOpen=true;
char keyboardText[64]{};
void KeyboardFrame() {
    ImGui::NewFrame();custom::UpdateTouchScroll();lite_ui::Theme();
    RenderVirtualKeyboard("##keyboard_fixture",keyboardText,sizeof(keyboardText),&keyboardOpen);
    ImGui::Render();CheckFrame();
}
void KeyboardTap(int index) {
    auto& io=ImGui::GetIO();const auto point=lite_keyboard::state.keys[index].bounds.GetCenter();
    io.AddMousePosEvent(point.x,point.y);KeyboardFrame();
    io.AddMouseButtonEvent(0,true);KeyboardFrame();
    io.AddMouseButtonEvent(0,false);KeyboardFrame();KeyboardFrame();
}
void ValidateKeyboard() {
    auto& io=ImGui::GetIO();const bool trickle=io.ConfigInputTrickleEventQueue;lite_touch::Input::Configure(io);
    io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    for(const ImVec2 size:{ImVec2(1280,720),ImVec2(960,540),ImVec2(640,360),ImVec2(360,780),ImVec2(320,568),ImVec2(1920,1080)}) {
        io.DisplaySize=size;keyboardOpen=true;keyboardText[0]=0;lite_keyboard::state={};
        for(int settle=0;settle<50;++settle) KeyboardFrame();
        auto* window=Find("##keyboard_fixture");assert(window);
        assert(window->Pos.x>=0 && window->Pos.y>=0 && window->Pos.x+window->Size.x<=size.x && window->Pos.y+window->Size.y<=size.y);
        for(int index:{lite_keyboard::closeKey,lite_keyboard::firstDigit,lite_keyboard::firstLetter,lite_keyboard::deleteKey,
            lite_keyboard::symbolsKey,lite_keyboard::clearKey,lite_keyboard::spaceKey,lite_keyboard::caseKey,lite_keyboard::doneKey}) {
            const auto rect=lite_keyboard::state.keys[index].bounds;
            assert(rect.Min.x>=window->Pos.x && rect.Min.y>=window->Pos.y && rect.Max.x<=window->Pos.x+window->Size.x && rect.Max.y<=window->Pos.y+window->Size.y);
        }
        KeyboardTap(lite_keyboard::firstDigit);assert(std::strcmp(keyboardText,"1")==0);
        KeyboardTap(lite_keyboard::caseKey);KeyboardTap(lite_keyboard::firstLetter);assert(std::strcmp(keyboardText,"1Q")==0);
        KeyboardTap(lite_keyboard::caseKey);KeyboardTap(lite_keyboard::firstLetter);assert(std::strcmp(keyboardText,"1Qq")==0);
        KeyboardTap(lite_keyboard::symbolsKey);KeyboardTap(lite_keyboard::firstLetter);assert(std::strcmp(keyboardText,"1Qq!")==0);
        KeyboardTap(lite_keyboard::spaceKey);assert(std::strcmp(keyboardText,"1Qq! ")==0);
        KeyboardTap(lite_keyboard::deleteKey);assert(std::strcmp(keyboardText,"1Qq!")==0);
        KeyboardTap(lite_keyboard::clearKey);assert(!keyboardText[0]);
        KeyboardTap(38);assert(std::strcmp(keyboardText,"\\")==0);
        std::snprintf(keyboardText,sizeof(keyboardText),"%s",u8"Aé😀");
        KeyboardTap(lite_keyboard::deleteKey);assert(std::strcmp(keyboardText,u8"Aé")==0);
        KeyboardTap(lite_keyboard::deleteKey);assert(std::strcmp(keyboardText,"A")==0);
        const auto before=std::string(keyboardText);
        const auto point=lite_keyboard::state.keys[lite_keyboard::firstLetter].bounds.GetCenter();
        io.AddMousePosEvent(point.x,point.y);io.AddMouseButtonEvent(0,true);KeyboardFrame();
        io.AddMousePosEvent(point.x,point.y-75);KeyboardFrame();
        io.AddMouseButtonEvent(0,false);KeyboardFrame();KeyboardFrame();assert(keyboardText==before);
        KeyboardTap(lite_keyboard::doneKey);assert(!keyboardOpen);KeyboardFrame();assert(!Find("##keyboard_fixture"));
    }
    struct Guarded {char before='L',text[3]{},after='R';} guarded;
    for(int index=0;index<10;++index) lite_keyboard::Append(guarded.text,sizeof(guarded.text),'x');
    assert(std::strcmp(guarded.text,"xx")==0 && guarded.before=='L' && guarded.after=='R');
    assert(!lite_keyboard::Append(nullptr,0,'x') && !lite_keyboard::Backspace(nullptr,0));
    io.DisplaySize={1280,720};keyboardOpen=true;lite_keyboard::state={};
    for(int settle=0;settle<60;++settle) KeyboardFrame();
    const auto previous=allocations;
    for(int frame=0;frame<300;++frame) KeyboardFrame();
    assert(allocations==previous);
    keyboardOpen=false;KeyboardFrame();lite_ui::keyboard=false;
    io.ConfigInputTrickleEventQueue=trickle;io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);io.AddMousePosEvent(-FLT_MAX,-FLT_MAX);
    state={};Frame();Frame();
    std::puts("Zuchizz keyboard: six viewport fits, case/symbols/backslash, UTF-8 deletion, bounded buffers, swipe rejection, Done and zero warm ImGui allocations passed");
}
void ValidateMenuSize() {
    auto& io=ImGui::GetIO();view.ready=view.authorized=view.interactive=true;view.startup={};
    for(const ImVec2 size:{ImVec2(1280,720),ImVec2(960,540),ImVec2(640,360),ImVec2(360,780),ImVec2(320,568),ImVec2(1920,1080)}) {
        io.DisplaySize=size;state={};state.tab=lite_ui::Tab::Settings;Frame();Frame();
        auto* root=Find("ETHNIR LITE##menu");auto* sidebar=Find("##sidebar");assert(root && sidebar);
        const auto full=root->Size,side=sidebar->Size;
        state.zoom=.85f;Frame();Frame();
        assert(root->Size.x<full.x && root->Size.y<full.y);
        assert(std::fabs(root->Size.x-full.x*.85f)<=2 && std::fabs(root->Size.y-full.y*.85f)<=2);
        assert(std::fabs(sidebar->Size.x-side.x*.85f)<=2 && std::fabs(sidebar->Size.y-side.y*.85f)<=2);
        state.zoom=1.35f;Frame();Frame();
        assert(root->Pos.x>=0 && root->Pos.y>=0 && root->Pos.x+root->Size.x<=size.x && root->Pos.y+root->Size.y<=size.y);
        if(size.x==1920) assert(root->Size.x>full.x*1.3f && root->Size.y>full.y*1.3f);
        assert(state.zoom<=lite_ui::Fit(size).maximumZoom);
    }
    io.DisplaySize={1920,1080};state={};state.tab=lite_ui::Tab::Settings;Frame();Frame();
    const bool trickle=io.ConfigInputTrickleEventQueue;lite_touch::Input::Configure(io);
    io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    auto* root=Find("ETHNIR LITE##menu");auto* panel=Find("/menu_");assert(root && panel);
    const auto original=root->Size;
    const auto line=panel->DC.CursorStartPos+ImVec2(0,20.f*lite_widgets::scale+2.f+8.f*lite_widgets::scale);
    const ImVec2 start(panel->WorkRect.Min.x+panel->WorkRect.GetWidth()*.5f,line.y);
    io.AddMousePosEvent(start.x,start.y);io.AddMouseButtonEvent(0,true);Frame();assert(state.sizing);
    const ImVec2 end(panel->WorkRect.Min.x+1.f,line.y);
    io.AddMousePosEvent(end.x,end.y);Frame();Frame();
    assert(ImLengthSqr(root->Size-original)<1 && state.zoom<1.f);
    io.AddMouseButtonEvent(0,false);Frame();assert(!state.sizing && ImLengthSqr(root->Size-original)<1);
    Frame();Frame();assert(root->Size.x<original.x && root->Size.y<original.y);
    io.ConfigInputTrickleEventQueue=trickle;io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);io.AddMousePosEvent(-FLT_MAX,-FLT_MAX);
    state={};io.DisplaySize={1280,720};Frame();Frame();
    std::puts("Menu size: outer window/sidebar/content resize together, viewport bounds and large-screen growth, stable slider drag/release passed");
}
