#pragma once

void ValidateRouting() {
    using lite_touch::Phase;
    lite_routing::Regions regions;
    regions.count=1;regions.rectangles[0]={{.3f,.2f},{.7f,.8f}};
    lite_routing::Router router;router.Publish(regions);
    assert(router.Filter(2,1,{.1f,.1f},Phase::Began)==2);
    assert(router.Filter(2,1,{.5f,.5f},Phase::Moved)==2);
    assert(router.Filter(2,1,{.5f,.5f},Phase::Ended)==2);
    assert(router.Filter(1,1,{.5f,.5f},Phase::Began)==0);
    assert(router.Filter(1,1,{.9f,.9f},Phase::Moved)==0);
    assert(router.Filter(1,1,{.9f,.9f},Phase::Ended)==0);
    assert(router.Filter(1,1,{.9f,.9f},Phase::Ended)==0);
    router.Publish({});
    assert(router.Filter(1,1,{.9f,.9f},Phase::Moved)==0);
    assert(router.Filter(0,1,{},Phase::Ended)==0);
    assert(router.Filter(1,1,{.5f,.5f},Phase::Began)==1);
    router.Filter(0,0,{},Phase::Ended);router.Publish(regions);
    assert(router.Filter(1,7,{std::nanf(""),.5f},Phase::Began)==1);
    router.Filter(0,0,{},Phase::Ended);
    assert(router.Filter(1,2,{.1f,.1f},Phase::Began)==1);
    std::thread publisher([&] { for(int index=0;index<5000;++index) router.Publish(regions); });
    for(int index=0;index<5000;++index) assert(router.Filter(1,2,{.5f,.5f},Phase::Moved)==1);
    publisher.join();router.Filter(0,0,{},Phase::Ended);
    auto& io=ImGui::GetIO();io.DisplaySize={1280,720};state={};view.pending=false;view.interactive=true;view.startup={};
    for(int page=0;page<3;++page) {
        view.ready=page==2;view.authorized=page>0;Frame();Frame();
        const auto visible=lite_routing::VisibleRegions();assert(visible.count>0 && visible.count<4);
        router.Publish(visible);router.Filter(0,0,{},Phase::Ended);
        assert(router.Filter(1,3,{.005f,.005f},Phase::Began)==1);
        router.Filter(0,0,{},Phase::Ended);
        assert(router.Filter(1,3,{.5f,.5f},Phase::Began)==0);
    }
    router.Filter(0,0,{},Phase::Ended);
    state.tab=lite_ui::Tab::Esp;Config={};Frame();Frame();router.Publish(lite_routing::VisibleRegions());
    auto* players=Find("/players_");assert(players);
    const auto row=players->DC.CursorStartPos+ImVec2(50,50);
    lite_touch::Input input;
    const bool trickle=io.ConfigInputTrickleEventQueue;lite_touch::Input::Configure(io);
    const auto poll=[&](ImVec2 point,Phase phase) {
        const auto allowed=router.Filter(1,3,{point.x/io.DisplaySize.x,point.y/io.DisplaySize.y},phase);
        const lite_touch::Sample sample{3,{point.x,io.DisplaySize.y-point.y},phase};
        if(allowed) input.Ignore(io);else input.Update(io,&sample,io.DisplaySize);
        Frame();return allowed;
    };
    assert(poll({5,5},Phase::Began)==1);
    assert(poll(row,Phase::Moved)==1);assert(poll(row,Phase::Ended)==1);
    assert(!Config.ESPMenu.isPlayerLine);
    router.Filter(0,0,{},Phase::Ended);input.Update(io,nullptr,{});Frame();
    assert(poll(row,Phase::Began)==0);assert(poll(row,Phase::Ended)==0);
    assert(Config.ESPMenu.isPlayerLine);
    router.Filter(0,0,{},Phase::Ended);input.Update(io,nullptr,{});Frame();Frame();
    io.ConfigInputTrickleEventQueue=trickle;state={};
    std::puts("Touch routing: outside passes during login/loading/menu, UI ownership survives leaving/moving panels, game gestures cannot activate UI, next touch recovers, fixed storage and concurrent publication passed");
}

void ValidateMenuDrag() {
    auto& io=ImGui::GetIO();io.DisplaySize={1280,720};
    const bool trickle=io.ConfigInputTrickleEventQueue;lite_touch::Input::Configure(io);
    io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);state={};view.ready=view.authorized=view.interactive=true;view.startup={};
    Frame();Frame();auto* root=Find("SHAINIME##menu");assert(root);
    const auto original=root->Pos;
    const auto start=original+ImVec2(35,35);
    io.AddMousePosEvent(start.x,start.y);io.AddMouseButtonEvent(0,true);Frame();
    io.AddMousePosEvent(start.x+80,start.y+20);Frame();
    assert(root->Pos.x>=original.x+78 && root->Pos.x<=original.x+81);
    assert(root->Pos.y>=0 && root->Pos.y+root->Size.y<=io.DisplaySize.y);
    io.AddMousePosEvent(-500,-500);Frame();assert(root->Pos.x==0 && root->Pos.y==0);
    io.AddMouseButtonEvent(0,false);Frame();Frame();assert(!state.placement.Tracking());
    const auto settled=root->Pos;
    Tap(settled+ImVec2(35,35));assert(ImLengthSqr(root->Pos-settled)<1);
    const auto body=settled+ImVec2(root->Size.x*.75f,root->Size.y*.5f);
    io.AddMousePosEvent(body.x,body.y);io.AddMouseButtonEvent(0,true);Frame();
    io.AddMousePosEvent(body.x-60,body.y-60);Frame();
    io.AddMouseButtonEvent(0,false);Frame();Frame();assert(ImLengthSqr(root->Pos-settled)<1);
    io.DisplaySize={640,360};Frame();Frame();
    assert(root->Pos.x>=0 && root->Pos.y>=0 && root->Pos.x+root->Size.x<=640 && root->Pos.y+root->Size.y<=360);
    io.ConfigInputTrickleEventQueue=trickle;io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
    io.AddMousePosEvent(-FLT_MAX,-FLT_MAX);state={};Frame();
    std::puts("Menu dragging: title-only movement, screen clamping, persistent placement, resize bounds and content-scroll separation passed");
}
