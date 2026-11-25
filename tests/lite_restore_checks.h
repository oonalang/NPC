#pragma once

void ValidateRestore() {
    using lite_restore::Swipe;
    for(const ImVec2 size : {ImVec2(320,568),ImVec2(360,780),ImVec2(640,360),ImVec2(960,540),ImVec2(1280,720),ImVec2(1920,1080)}) {
        const auto geometry=lite_restore::Fit(size,lite_ui::Fit(size).scale);
        const auto point=geometry.line.GetCenter();
        assert(std::fabs(point.x-size.x*.5f)<.001f && geometry.hit.Contains(point));
        assert(geometry.hit.Min.x>=0 && geometry.hit.Max.x<=size.x && geometry.hit.Max.y<size.y);
        assert(geometry.travel>=32 && geometry.travel<=64);
        Swipe tap;
        assert(!tap.Update(true,geometry,point,true,true,false) && tap.Captures());
        assert(!tap.Update(true,geometry,point+ImVec2(3,-4),false,false,true) && !tap.Captures());
        Swipe swipe;
        assert(!swipe.Update(true,geometry,point,true,true,false));
        for(int step=1;step<=4;++step)
            assert(!swipe.Update(true,geometry,point-ImVec2(0,geometry.travel*step/5.f),false,true,false));
        const auto end=point-ImVec2(0,geometry.travel+1);
        assert(swipe.Update(true,geometry,end,false,true,false));
        assert(swipe.Captures());
        assert(!swipe.Update(false,geometry,end,false,true,false) && swipe.Captures());
        assert(!swipe.Update(false,geometry,end,false,false,true) && !swipe.Captures());
        assert(!swipe.Update(true,geometry,end,false,false,false));
        for(const ImVec2 movement : {ImVec2(90,0),ImVec2(-90,0),ImVec2(0,60),ImVec2(90,-60)}) {
            Swipe wrong;
            wrong.Update(true,geometry,point,true,true,false);
            assert(!wrong.Update(true,geometry,point+movement,false,true,false));
            assert(!wrong.Update(true,geometry,end,false,false,true) && !wrong.Captures());
        }
        Swipe outside;
        assert(!outside.Update(true,geometry,size*.5f,true,true,false));
        assert(!outside.Update(true,geometry,end,false,true,false) && !outside.Captures());
        Swipe canceled;
        canceled.Update(true,geometry,point,true,true,false);
        assert(!canceled.Update(true,geometry,{-FLT_MAX,-FLT_MAX},false,false,true));
        Swipe resized;
        resized.Update(true,geometry,point,true,true,false);
        const auto changed=lite_restore::Fit({size.x+1,size.y},1.f);
        assert(!resized.Update(true,changed,end,false,true,false));
        assert(!resized.Update(true,changed,end,false,false,true));
        Swipe release;
        release.Update(true,geometry,point,true,true,false);
        assert(release.Update(true,geometry,end,false,false,true) && !release.Captures());
    }
    auto& io=ImGui::GetIO();io.DisplaySize={960,540};
    const bool trickle=io.ConfigInputTrickleEventQueue;lite_touch::Input::Configure(io);
    io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    view.ready=view.authorized=view.interactive=true;view.startup={};
    state={};state.minimized=true;state.tab=lite_ui::Tab::Esp;Config={};Frame();Frame();
    const auto geometry=lite_restore::Fit(io.DisplaySize,lite_ui::Fit(io.DisplaySize).scale);
    auto* bar=Find("##lite_restore");assert(bar && !Find("ETHNIR LITE##menu"));
    assert(std::fabs((bar->Pos.x+bar->Size.x*.5f)-io.DisplaySize.x*.5f)<=1.1f);
    Tap(geometry.line.GetCenter());assert(state.minimized);
    const auto point=geometry.line.GetCenter();
    io.AddMousePosEvent(point.x,point.y);io.AddMouseButtonEvent(0,true);Frame();
    assert(state.minimized && state.restore.Captures());
    io.AddMousePosEvent(point.x,point.y-geometry.travel-1);Frame();
    assert(!state.minimized && state.restore.Captures() && Find("ETHNIR LITE##menu"));
    auto* players=Find("/players_");assert(players);
    const auto row=players->DC.CursorStartPos+ImVec2(50,50);
    io.AddMousePosEvent(row.x,row.y);Frame();assert(!Config.ESPMenu.isPlayerLine);
    io.AddMouseButtonEvent(0,false);Frame();Frame();
    assert(!state.restore.Captures() && !Config.ESPMenu.isPlayerLine);
    Tap(row);assert(Config.ESPMenu.isPlayerLine);
    state.minimized=true;view.ready=view.authorized=false;Frame();
    assert(Find("ETHNIR LITE##login") && !Find("##lite_restore"));
    state={};view.ready=view.authorized=true;view.interactive=true;view.startup={};
    lite_ui::EnableHandcam(state);
    assert(state.handcam && state.hideRecord && state.minimized);
    Frame();Frame();
    auto* blank=Find("##lite_restore");assert(blank && !Find("ETHNIR LITE##menu"));
    assert(blank->DrawList->VtxBuffer.Size==0);
    const auto handcam=lite_restore::Fit(io.DisplaySize,lite_ui::Fit(io.DisplaySize).scale);
    const auto handcamPoint=handcam.line.GetCenter();
    assert(handcam.hit.Contains(handcamPoint));
    io.AddMousePosEvent(handcamPoint.x,handcamPoint.y);io.AddMouseButtonEvent(0,true);Frame();
    assert(state.minimized && state.restore.Captures());
    io.AddMousePosEvent(handcamPoint.x,handcamPoint.y-handcam.travel-1);Frame();
    assert(!state.minimized && Find("ETHNIR LITE##menu") && state.handcam && state.hideRecord);
    io.AddMouseButtonEvent(0,false);Frame();Frame();
    state.handcam=false;state.minimized=true;Frame();
    auto* shown=Find("##lite_restore");assert(shown && shown->DrawList->VtxBuffer.Size>0);
    state={};view.ready=view.authorized=true;
    io.ConfigInputTrickleEventQueue=trickle;io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
    io.AddMousePosEvent(-FLT_MAX,-FLT_MAX);Frame();
    std::puts("Restore gesture: bottom-center swipe across six viewports; tap/wrong-direction/outside/cancel/resize rejection; one reveal and capture until release; no accidental toggle or hidden login; Handcam Mode closes the menu, enables Hide Record, hides the bar while its swipe still opens the menu, and restores the bar when off");
}
