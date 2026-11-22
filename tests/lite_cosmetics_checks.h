#pragma once
void ValidateKillCosmetics() {
    using namespace kill_cosmetics;
    auto& io=ImGui::GetIO();view.ready=view.authorized=view.interactive=true;view.startup={};
    available.store(true);configured={};Publish(configured);weapons.Record({2001,100,5001},5);
    for(unsigned kind=0;kind<3;++kind)for(unsigned index=0;index<120;++index){
        Row row;row.id=itemTypes[kind]*10000000u+21+index;row.asset=400;row.weapons[0]=2001;row.weaponCount=1;
        assert(catalog.Put(static_cast<Kind>(kind),row));
    }
    for(const ImVec2 size:{ImVec2(1280,720),ImVec2(960,540),ImVec2(640,360),ImVec2(360,780),ImVec2(320,568),ImVec2(1920,1080)}) {
        io.DisplaySize=size;
        for(int kind=0;kind<3;++kind){
            state={};state.tab=lite_ui::Tab::Skins;state.skinCategory=7+kind;Frame();Frame();
            auto* root=Find("ETHNIR LITE##menu");assert(root&&!Find("##cosmetics"));
            assert(root->Pos.x>=0&&root->Pos.y>=0&&root->Pos.x+root->Size.x<=size.x&&root->Pos.y+root->Size.y<=size.y);
            assert(catalog.Count(static_cast<Kind>(kind))==120);
        }
    }
    io.DisplaySize={1280,720};
    auto frame=[&]{
        ImGui::NewFrame();custom::UpdateTouchScroll();lite_ui::Theme();lite_widgets::scale=1;
        ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({500,600});ImGui::Begin("##automatic_fixture",nullptr,ImGuiWindowFlags_NoSavedSettings);
        lite_widgets::SetFontScale();lite_kill_cosmetics::Draw(Kind::Icon);ImGui::End();ImGui::Render();CheckFrame();
    };
    frame();frame();auto* fixture=Find("##automatic_fixture");assert(fixture);
    const ImVec2 point=fixture->DC.CursorStartPos+ImVec2(10,ImGui::GetFontSize()+2+14);
    const bool trickle=io.ConfigInputTrickleEventQueue;lite_touch::Input::Configure(io);io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    auto tap=[&]{io.AddMousePosEvent(point.x,point.y);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();frame();};
    tap();assert(!configured.automatic&&!requested.load());tap();assert(configured.automatic&&requested.load());
    const auto before=allocations;for(int i=0;i<300;++i)frame();assert(allocations==before);
    io.ConfigInputTrickleEventQueue=trickle;io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);io.AddMousePosEvent(-FLT_MAX,-FLT_MAX);
    state={};Frame();Frame();
    std::puts("Automatic mythic cosmetics UI: three categories at six viewports, no manual ID/tier lists, actual opt-out/re-enable touch and zero warm ImGui allocations passed");
}
