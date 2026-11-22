#pragma once
void ValidateKillCosmetics() {
    using namespace kill_cosmetics;
    auto& io=ImGui::GetIO();view.ready=view.authorized=view.interactive=true;view.startup={};
    available.store(true);
    for(unsigned kind=0;kind<3;++kind) for(unsigned row=0;row<120;++row)
        assert(catalog.Put(static_cast<Kind>(kind),{itemTypes[kind]*10000000u+21+row,2+static_cast<int>(row%4),400,401}));
    for(const ImVec2 size:{ImVec2(1280,720),ImVec2(960,540),ImVec2(640,360),ImVec2(360,780),ImVec2(320,568),ImVec2(1920,1080)}) {
        io.DisplaySize=size;
        for(int kind=0;kind<3;++kind) {
            state={};state.tab=lite_ui::Tab::Skins;state.skinCategory=7+kind;Frame();Frame();
            auto* root=Find("ETHNIR LITE##menu");auto* list=Find("##cosmetics");assert(root&&list);
            assert(root->Pos.x>=0 && root->Pos.y>=0 && root->Pos.x+root->Size.x<=size.x && root->Pos.y+root->Size.y<=size.y);
            assert(list->Flags&ImGuiWindowFlags_NoScrollbar);
            assert(lite_kill_cosmetics::caches[kind].rows.size()==120);
        }
    }
    io.DisplaySize={1280,720};state={};state.tab=lite_ui::Tab::Skins;state.skinCategory=7;Frame();Frame();
    auto* list=Find("##cosmetics");assert(list);
    const auto point=list->DC.CursorStartPos+ImVec2(12,14)*lite_widgets::scale;
    const bool trickle=io.ConfigInputTrickleEventQueue;lite_touch::Input::Configure(io);io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    Tap(point);assert(configured.ids[0]==600000021 && Requested(Kind::Icon)==600000021);
    Tap(point);assert(configured.ids[0]==0 && Requested(Kind::Icon)==0);
    auto& cache=lite_kill_cosmetics::caches[0];std::snprintf(cache.search,sizeof(cache.search),"%s","legendary");Frame();Frame();
    assert(cache.visible.size()==30);for(int index:cache.visible)assert(cache.rows[index].rarity==4);
    cache.search[0]=0;cache.rarity=5;Frame();Frame();assert(cache.visible.size()==30);
    for(int index:cache.visible)assert(cache.rows[index].rarity==5);
    const auto previous=allocations;const auto* rows=cache.rows.data();
    for(int frame=0;frame<300;++frame)Frame();assert(previous==allocations&&rows==cache.rows.data());
    cache.rarity=0;Frame();Frame();
    io.ConfigInputTrickleEventQueue=trickle;io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);io.AddMousePosEvent(-FLT_MAX,-FLT_MAX);
    configured={};Publish(configured);state={};Frame();Frame();
    std::puts("Kill cosmetic UI: three live-catalog categories at six viewports, hidden scrollbar, actual touch selection/clear, search/rarity, clipping and stable warm frame cache passed");
}
