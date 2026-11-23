#pragma once

void ValidatePolledTouch() {
    auto& io=ImGui::GetIO();
    const bool trickle=io.ConfigInputTrickleEventQueue;
    lite_touch::Input::Configure(io);
    io.DisplaySize={640,640};
    lite_touch::Input input;
    using Phase=lite_touch::Phase;
    const ImVec2 resolution(1280,1280);
    const auto poll=[&](ImVec2 point,Phase phase,int finger=7,ImVec2 source=ImVec2(1280,1280)) {
        const lite_touch::Sample sample{finger,{point.x*source.x/io.DisplaySize.x,
            source.y-point.y*source.y/io.DisplaySize.y},phase};
        input.Update(io,&sample,source);
        ControlsFrame();
    };
    const auto empty=[&] { input.Update(io,nullptr,resolution);ControlsFrame(); };
    ControlsFrame(); ControlsFrame(); ResetScroll(); checked=false;
    for(int fps : {15,30,60,90,120}) {
        io.DeltaTime=1.f/fps;
        const auto point=checkRect.GetCenter();
        poll(point,Phase::Began);
        assert(io.MouseClicked[0] && io.MouseDown[0]);
        assert(ImLengthSqr(io.MousePos-point)<1.f && !checked);
        poll(point+ImVec2(3,4),Phase::Stationary);
        assert(!checked && scrollWindow->Scroll.y==0);
        poll(point+ImVec2(5,8),Phase::Ended);
        assert(checked && io.MouseReleased[0] && !io.MouseDown[0]);
        empty(); empty(); checked=false;
    }
    io.DeltaTime=1.f/60.f;
    const auto start=checkRect.GetCenter();
    poll(start,Phase::Began); empty();
    assert(checked && io.MouseReleased[0]);
    empty(); empty(); checked=false;
    poll(start,Phase::Began); poll(start,Phase::Canceled); empty(); empty();
    assert(!checked && !io.MouseDown[0]);
    poll(start,Phase::Began); poll(start,Phase::Moved,8); poll(start,Phase::Ended,8); empty(); empty();
    assert(!checked && !io.MouseDown[0]);
    poll(start,Phase::Began); poll(start,Phase::Moved,7,{640,640}); poll(start,Phase::Ended); empty(); empty();
    assert(!checked && !io.MouseDown[0]);
    poll(start,Phase::Began); poll({std::nanf(""),start.y},Phase::Moved); empty(); empty();
    assert(!checked && !io.MouseDown[0]);
    poll(start,Phase::Began); io.DisplaySize={640,620}; poll(start,Phase::Moved); empty(); empty();
    assert(!checked && !io.MouseDown[0]);
    io.DisplaySize={640,640}; ControlsFrame(); ControlsFrame();
    poll(start,Phase::Moved); poll(start,Phase::Ended); empty(); empty(); assert(checked);
    checked=false;
    const float initial=.35f;
    slider=initial;
    const auto knob=ImVec2(sliderRect.Min.x+sliderRect.GetWidth()*initial,sliderRect.GetCenter().y);
    poll(knob,Phase::Began); poll(knob+ImVec2(20,25),Phase::Moved);
    assert(custom::touch::gesture.intent==custom::touch::Intent::Horizontal && slider>initial && scrollWindow->Scroll.y==0);
    poll({sliderRect.Max.x+5,knob.y+27},Phase::Ended);
    assert(slider==1.f); empty(); empty();
    slider=initial;
    const auto vertical=sliderRect.GetCenter();
    poll(vertical,Phase::Began);
    for(int index=1;index<=24;++index) poll(vertical+ImVec2(index*.2f,-index*3.f),Phase::Moved);
    poll(vertical+ImVec2(5,-74),Phase::Ended); empty(); empty();
    assert(slider==initial && scrollWindow->Scroll.y>=73 && scrollWindow->Scroll.y<=75);
    ResetScroll();
    for(float offset : {3.f,12.f,25.f}) {
        slider=.5f;
        poll(sliderRect.Min+ImVec2(30,offset),Phase::Began);
        poll({sliderRect.Max.x+8,sliderRect.Min.y+offset+2},Phase::Moved);
        poll({sliderRect.Max.x+8,sliderRect.Min.y+offset+2},Phase::Ended);
        assert(slider==1.f); empty(); empty();
    }
    ResetScroll(); longChoices=true; choice=0; ControlsFrame();
    const auto open=comboRect.GetCenter(); poll(open,Phase::Began); poll(open+ImVec2(3,5),Phase::Ended); empty();
    assert(comboOpen);
    auto* popup=GImGui->OpenPopupStack.back().Window;
    assert(popup && popup->ScrollMax.y>0);
    const auto row=choiceRect.GetCenter();
    poll(row,Phase::Began); poll(row+ImVec2(35,0),Phase::Moved); poll(row+ImVec2(35,0),Phase::Ended); empty();
    assert(comboOpen && choice==0);
    const auto inside=popup->InnerRect.GetCenter();
    poll(inside,Phase::Began); poll(inside-ImVec2(0,600),Phase::Moved);
    poll(inside-ImVec2(0,600),Phase::Ended); empty();
    assert(comboOpen && choice==0 && popup->Scroll.y>500);
    const auto parentScroll=scrollWindow->Scroll.y;
    poll(inside,Phase::Began); poll(inside-ImVec2(0,900),Phase::Moved);
    poll(inside-ImVec2(0,900),Phase::Ended); empty();
    assert(comboOpen && popup->Scroll.y==popup->ScrollMax.y && scrollWindow->Scroll.y==parentScroll);
    assert(popup->InnerRect.Contains(lastChoiceRect.GetCenter()));
    const auto last=lastChoiceRect.GetCenter(); poll(last,Phase::Began); poll(last+ImVec2(4,-6),Phase::Ended); empty();
    assert(choice==39 && !comboOpen);
    longChoices=false;choice=0;empty();
    io.ConfigInputTrickleEventQueue=trickle;
    io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);io.AddMousePosEvent(-FLT_MAX,-FLT_MAX);Frame();
    std::puts("Polled touch: immediate scaled presses and jitter-tolerant taps at 15-120 FPS; cancel, missing end, finger/resolution changes, slider row/diagonal/final release, long dropdown scroll/selection passed");
}

void ValidateNestedPan() {
    auto& io=ImGui::GetIO();io.DisplaySize={640,640};io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    ImGuiWindow *outer=nullptr,*inner=nullptr;
    const auto frame=[&] {
        ImGui::NewFrame();custom::UpdateTouchScroll();
        ImGui::SetNextWindowPos({20,20});ImGui::SetNextWindowSize({350,300});
        ImGui::Begin("Nested pan",nullptr,ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
        ImGui::BeginChild("Outer pan",{300,240},false,ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
        outer=ImGui::GetCurrentWindow();
        ImGui::BeginChild("Inner pan",{260,160},false,ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
        inner=ImGui::GetCurrentWindow();ImGui::Dummy({10,600});ImGui::EndChild();
        ImGui::Dummy({10,1000});ImGui::EndChild();ImGui::End();
        ImGui::Render();CheckFrame();
    };
    frame();frame();
    inner->Scroll.y=inner->ScrollMax.y;inner->ScrollTarget.y=FLT_MAX;frame();
    const auto point=inner->InnerRect.GetCenter();
    io.AddMousePosEvent(point.x,point.y);frame();io.AddMouseButtonEvent(0,true);frame();
    io.AddMousePosEvent(point.x,point.y-60);frame();
    assert(outer->Scroll.y>=59 && inner->Scroll.y==inner->ScrollMax.y);
    io.AddMouseButtonEvent(0,false);frame();frame();
    io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);io.AddMousePosEvent(-FLT_MAX,-FLT_MAX);Frame();
    std::puts("Nested touch scrolling passes remaining vertical motion from a full child to its parent");
}
