#pragma once
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "../ImGui/imgui_internal.h"
#include "Colors.h"
#include "SakugaArt.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace sakuga {
inline bool enabled = true, reduced = false, lean = false;
inline float scale = 1.f, opacity = 1.f;
inline constexpr int effectSlots = 64, motionSlots = 18, sliderSlots = 8;

inline float Clamp(float value, float low = 0.f, float high = 1.f) { return value < low ? low : value > high ? high : value; }
inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float Out(float t) { const float k = 1.f - Clamp(t); return 1.f - k * k * k; }
inline float In(float t) { t = Clamp(t); return t * t * t; }
inline float Back(float t) { t = Clamp(t) - 1.f; return 1.f + 2.70158f * t * t * t + 1.70158f * t * t; }
inline float Px(float value) { return value * scale; }
inline uint32_t Hash(uint32_t value) {
    value ^= value >> 16; value *= 0x7feb352du; value ^= value >> 15; value *= 0x846ca68bu; return value ^ (value >> 16);
}
inline float Random(uint32_t seed, int index, int salt) {
    const uint32_t mixed = uint32_t(index) * 374761393u + uint32_t(salt) * 668265263u;
    return float(Hash(seed + mixed) & 0xffffu) / 65535.f;
}
inline uint32_t nextSeed = 0x9e3779b9u;
inline uint32_t Seed() { nextSeed ^= nextSeed << 13; nextSeed ^= nextSeed >> 17; nextSeed ^= nextSeed << 5; return nextSeed; }
inline float Dt() { return ImMin(ImGui::GetIO().DeltaTime, .034f); }

inline ImU32 Alpha(ImU32 color, float alpha = 1.f) {
    const unsigned value = static_cast<unsigned>(((color >> 24) & 255u) *
        Clamp(alpha * opacity * ImGui::GetStyle().Alpha));
    return (color & 0x00ffffffu) | (value << 24);
}

struct Spring {
    float x = 0.f, velocity = 0.f, target = 0.f, stiffness = 300.f, damping = 20.f;
    Spring(float value = 0.f, float k = 300.f, float d = 20.f) : x(value), target(value), stiffness(k), damping(d) {}
    float Step(float dt) {
        if (!(dt > 0.f)) return x;
        const int count = static_cast<int>(std::ceil(dt / .008f));
        const float h = dt / static_cast<float>(ImMax(count, 1));
        for (int step = 0; step < count; ++step) {
            velocity += (-stiffness * (x - target) - damping * velocity) * h;
            x += velocity * h;
        }
        return x;
    }
};

struct Motion {
    ImGuiID id = 0;
    Spring squash{1.f, 520.f, 15.f}, pop{1.f, 300.f, 12.f}, tilt{0.f, 260.f, 10.f};
    bool checkbox = false, held = false;
    double invertUntil = 0.;
};
inline Motion motions[motionSlots];

inline Motion* MotionSlot(ImGuiID id, bool create, bool checkbox = false) {
    if (!id) return nullptr;
    for (auto& motion : motions) if (motion.id == id) return &motion;
    if (!create) return nullptr;
    Motion* slot = nullptr;
    for (auto& motion : motions) if (!motion.id) { slot = &motion; break; }
    if (!slot) slot = &motions[0];
    *slot = Motion{};
    slot->id = id;
    slot->checkbox = checkbox;
    return slot;
}
struct Pose { float x = 1.f, y = 1.f, rotation = 0.f; bool active = false; };
inline Pose ControlPose(ImGuiID id, bool held, bool create, float low = .5f, float wide = .24f, bool checkbox = false) {
    auto* motion = MotionSlot(id, create || held, checkbox);
    if (!motion) return {};
    motion->held = held;
    motion->squash.target = checkbox ? 1.f : held ? low : 1.f;
    const float dt = Dt(), y = motion->squash.Step(dt), p = motion->pop.Step(dt), r = motion->tilt.Step(dt);
    const bool still = std::fabs(y - 1.f) < .001f && std::fabs(motion->squash.velocity) < .01f &&
        std::fabs(p - 1.f) < .001f && std::fabs(r) < .02f && !held;
    if (still) {
        const bool inverting = ImGui::GetTime() < motion->invertUntil;
        if (!inverting) { motion->id = 0; return {}; }
    }
    Pose pose;
    pose.active = true;
    pose.x = (1.f + (1.f - y) * wide) * p;
    pose.y = y * p;
    pose.rotation = r;
    return pose;
}
inline bool Inverting(ImGuiID id) {
    auto* motion = MotionSlot(id, false);
    return motion && ImGui::GetTime() < motion->invertUntil;
}

enum class Kind : uint8_t { Burst, Puff, Shards, Slash, Lines, Ticks, Donut, Wob, Kira, Word };
struct Effect {
    Kind kind{};
    ImGuiID owner = 0;
    bool behind = false;
    double born = 0.;
    float delay = 0.f, life = 1.f;
    float x = 0.f, y = 0.f, a = 0.f, b = 0.f, c = 0.f, d = 0.f, e = 0.f;
    float vx = 0.f, vy = 0.f;
    ImU32 color = 0;
    int count = 0, word = 0;
    uint32_t seed = 0;
    float springX = .2f, springVelocity = 0.f, lastT = 0.f;
};
inline Effect effects[effectSlots];

inline Effect& Add(Kind kind, float x, float y, float life, float delay = 0.f, ImGuiID owner = 0, bool behind = false) {
    static Effect rejected;
    int active = 0;
    for (const auto& effect : effects) if (effect.life > 0.f) ++active;
    if (active >= (lean ? 16 : 26)) { rejected = Effect{}; rejected.life = 0.f; return rejected; }
    Effect* slot = nullptr;
    for (auto& effect : effects) if (effect.life <= 0.f) { slot = &effect; break; }
    if (!slot) {
        slot = &effects[0];
        for (auto& effect : effects) if (effect.born + effect.delay < slot->born + slot->delay) slot = &effect;
    }
    const double now = ImGui::GetTime();
    *slot = Effect{};
    slot->kind = kind; slot->born = now; slot->delay = delay; slot->life = life;
    slot->x = x; slot->y = y; slot->owner = owner; slot->behind = behind; slot->seed = Seed();
    return *slot;
}

inline void Burst(float x, float y, float radius, int spikes, float life, ImGuiID owner = 0, bool behind = false) {
    if (reduced) return;
    auto& effect = Add(Kind::Burst, x, y, life, 0.f, owner, behind);
    effect.a = Px(radius); effect.count = ImMax(1, spikes);
}
inline void Shards(float x, float y, int count, float speed, float size, float life, float dir, float arc, float gravity) {
    if (reduced) return;
    auto& effect = Add(Kind::Shards, x, y, life);
    effect.count = ImMin(16, ImMax(1, count)); effect.a = Px(speed); effect.b = Px(size);
    effect.c = dir; effect.d = arc; effect.e = Px(gravity);
}
inline void Puff(float x, float y, int count, float size, float vx, float vy, float life) {
    if (reduced) return;
    auto& effect = Add(Kind::Puff, x, y, life);
    effect.count = ImClamp(count, 1, 5); effect.a = Px(size); effect.vx = Px(vx); effect.vy = Px(vy);
}
inline void Slash(float x, float y, float length, float angle, float thickness, float life, ImU32 color = lite_colors::Accent) {
    if (reduced) return;
    auto& effect = Add(Kind::Slash, x, y, life);
    effect.a = Px(length); effect.b = angle; effect.c = Px(thickness); effect.color = color;
}
inline void Lines(float x, float y, int count, float dir, float length, float spread, float life) {
    if (reduced) return;
    auto& effect = Add(Kind::Lines, x, y, life);
    effect.count = ImMin(10, ImMax(1, count)); effect.a = dir; effect.b = Px(length); effect.c = Px(spread);
}
inline void Ticks(float x, float y, int count, float radius, float length, float dir, float life, float fan = .42f) {
    if (reduced) return;
    auto& effect = Add(Kind::Ticks, x, y, life);
    effect.count = ImClamp(count, 1, 6); effect.a = Px(radius); effect.b = Px(length); effect.c = dir; effect.d = fan;
}
inline void Donut(float x, float y, float radius, float vx, float vy, float life) {
    if (reduced || lean) return;
    auto& effect = Add(Kind::Donut, x, y, life);
    effect.a = Px(radius); effect.vx = Px(vx); effect.vy = Px(vy);
}
inline void Wob(float x, float y, float dir, float radius, float life) {
    if (reduced) return;
    auto& effect = Add(Kind::Wob, x, y, life);
    effect.a = dir; effect.b = Px(radius);
}
inline void Kira(float x, float y, float radius, float life) {
    if (reduced) return;
    auto& effect = Add(Kind::Kira, x, y, life);
    effect.a = Px(radius);
}
inline void Word(int word, float x, float y, float size, float rotation, float life) {
    if (reduced) return;
    auto& effect = Add(Kind::Word, x, y, life);
    effect.word = ImClamp(word, 0, 2); effect.a = Px(size); effect.b = rotation;
}

inline void ButtonDown(ImVec2 center, float halfWidth, float halfHeight) {
    Ticks(center.x, center.y - halfHeight - Px(6), 3, 4, 14, -IM_PI * .5f, .5f, .5f);
    Wob(center.x - halfWidth - Px(6), center.y, -1.f, 10, .3f);
    Wob(center.x + halfWidth + Px(6), center.y, 1.f, 10, .3f);
    Lines(center.x, center.y + halfHeight + Px(10), 3, 0.f, 16, Px(.1f), .25f);
}

inline void ButtonImpact(ImGuiID id, ImVec2 low, ImVec2 high) {
    if (reduced) return;
    const ImVec2 center((low.x + high.x) * .5f, (low.y + high.y) * .5f);
    const float width = high.x - low.x, height = high.y - low.y;
    auto* motion = MotionSlot(id, true);
    if (motion) {
        motion->squash.x = ImMin(motion->squash.x, .6f);
        motion->pop.x = 1.24f;
        motion->tilt.x = -5.f;
        motion->invertUntil = ImGui::GetTime() + .19;
    }
    Burst(center.x, center.y, 98, lean ? 12 : 17, .48f, id, true);
    Shards(center.x, center.y, lean ? 6 : 11, 260, 9, .95f, 0.f, 6.28f, 300);
    const float dx[4]{-1.f, 1.f, -1.f, 1.f}, dy[4]{-.5f, -1.f, .9f, 1.f};
    for (int index = 0; index < 4; ++index)
        Puff(center.x + dx[index] * (width * .5f + Px(6)), center.y + dy[index] * Px(26),
            lean ? 3 : 5, 12.f + Random(motion ? motion->id : 1u, index, 1) * 4.f,
            dx[index] * 46, dy[index] * 30 - 6, .75f + Random(motion ? motion->id : 1u, index, 2) * .3f);
    Word(0, center.x + Px(10), center.y - Px(84), 176, -.12f, 1.05f);
    Ticks(center.x - Px(96), center.y - Px(70), 2, 4, 22, -2.4f, .45f);
    Ticks(center.x + Px(110), center.y - Px(60), 2, 4, 22, -.7f, .45f);
    Wob(center.x - width * .5f - Px(8), center.y, -1.f, 16, .45f);
    Wob(center.x + width * .5f + Px(8), center.y, 1.f, 16, .45f);
    {
        auto& donut = Add(Kind::Donut, center.x + Px(6), center.y - Px(44), 1.25f, .2f);
        donut.a = Px(30); donut.vx = Px(22); donut.vy = Px(-52);
    }
    {
        auto& donut = Add(Kind::Donut, center.x + Px(54), center.y - Px(50), 1.1f, .29f);
        donut.a = Px(18); donut.vx = Px(34); donut.vy = Px(-40);
    }
    {
        auto& donut = Add(Kind::Donut, center.x + Px(86), center.y - Px(46), .95f, .37f);
        donut.a = Px(10); donut.vx = Px(40); donut.vy = Px(-30);
    }
    Shards(center.x - Px(30), center.y + Px(30), 2, 20, 6, .75f, IM_PI * .5f, 1.f, 380);
    Shards(center.x + Px(34), center.y + Px(30), 2, 20, 6, .75f, IM_PI * .5f, 1.f, 380);
}

inline void CheckboxImpact(ImGuiID id, ImVec2 center, bool checked) {
    if (reduced) return;
    auto* motion = MotionSlot(id, true, true);
    if (motion) motion->squash.x = .68f;
    if (!checked) { Puff(center.x, center.y, 3, 7, 0, -18, .4f); return; }
    Slash(center.x + Px(8), center.y + Px(2), 150, -.42f, 13, .5f);
    Slash(center.x - Px(6), center.y + Px(12), 110, 2.72f, 7, .55f);
    Lines(center.x - Px(34), center.y + Px(26), 6, 2.75f, 60, Px(14), .3f);
    Shards(center.x, center.y, 6, 190, 6, .75f, 0.f, 6.28f, 260);
    Kira(center.x + Px(24), center.y - Px(22), 9, .6f);
    Word(2, center.x + Px(30), center.y - Px(62), 112, -.12f, .95f);
}

struct Slider {
    ImGuiID id = 0;
    Spring spring{0.f, 420.f, 30.f};
    float accumulated = 0.f, peak = 0.f, said = -100.f, time = 0.f;
    bool active = false;
};
inline Slider sliders[sliderSlots];
inline Slider* SliderSlot(ImGuiID id) {
    for (auto& slider : sliders) if (slider.id == id) return &slider;
    for (auto& slider : sliders) if (!slider.id) { slider = Slider{}; slider.id = id; return &slider; }
    sliders[0] = Slider{}; sliders[0].id = id; return &sliders[0];
}
struct SliderPose { float position = 0.f, stretch = 1.f; };
inline SliderPose SliderMotion(ImGuiID id, float amount, bool held, bool released, ImVec2 center) {
    auto* slider = SliderSlot(id);
    if (!slider) return {amount, 1.f};
    const float dt = Dt();
    slider->time += dt;
    slider->spring.target = amount;
    const float x = slider->spring.Step(dt), velocity = slider->spring.velocity;
    slider->active = held;
    SliderPose pose;
    pose.position = x;
    pose.stretch = 1.f + ImMin(1.5f, std::fabs(velocity) * .3f);
    if (std::fabs(velocity) > std::fabs(slider->peak)) slider->peak = velocity; else slider->peak *= .96f;
    if (!reduced && std::fabs(velocity) > .35f) {
        slider->accumulated += dt * std::fabs(velocity) * 20.f;
        const float dir = velocity > 0.f ? 1.f : -1.f, big = ImMin(1.f, std::fabs(velocity) / 2.4f);
        int spawned = 0;
        while (slider->accumulated > 1.f && spawned < 2) {
            slider->accumulated -= 1.f; ++spawned;
            Puff(center.x - dir * Px(14.f + Random(slider->id, spawned, 1) * 16.f),
                center.y - Px(2.f + Random(slider->id, spawned, 2) * 10.f + big * 8.f),
                lean ? 2 : 4, 6.f + Random(slider->id, spawned, 3) * 3.f + big * 9.f,
                -dir * (26.f + Random(slider->id, spawned, 4) * 44.f),
                -(22.f + Random(slider->id, spawned, 5) * 36.f) * (.5f + big),
                .7f + Random(slider->id, spawned, 6) * .45f);
            Lines(center.x - dir * Px(16), center.y + Px(-4.f + Random(slider->id, spawned, 7) * 16.f),
                1, dir > 0.f ? IM_PI : 0.f, 50.f + big * 50.f, Px(2), .24f);
        }
        if (std::fabs(velocity) > 1.5f && slider->time - slider->said > 1.1f) {
            slider->said = slider->time;
            Word(1, center.x - dir * Px(6), center.y - Px(78), 104, -.16f, .75f);
        }
    }
    if (released && !reduced) {
        const float dir = slider->peak > 0.f ? 1.f : -1.f;
        if (std::fabs(slider->peak) > 1.3f) {
            Slash(center.x + dir * Px(34), center.y, 150, dir > 0.f ? 0.f : IM_PI, 12, .5f, lite_colors::Soft);
            Burst(center.x + dir * Px(4), center.y, 24, 11, .28f);
            Shards(center.x + dir * Px(10), center.y, 4, 190, 4, .75f, dir > 0.f ? 0.f : IM_PI, 2.4f, 420);
            Ticks(center.x + dir * Px(52), center.y - Px(30), 3, 4, 10, dir > 0.f ? -.6f : -2.5f, .4f);
            Puff(center.x - dir * Px(60), center.y - Px(34), 4, 11, 0, -10, .8f);
            Puff(center.x - dir * Px(22), center.y - Px(22), 3, 6, 0, -8, .6f);
            Wob(center.x - dir * Px(82), center.y - Px(34), -1.f, 9, .45f);
        }
        if (amount >= .999f || amount <= .001f)
            Shards(center.x, center.y, 5, 190, 5, .75f, dir > 0.f ? IM_PI : 0.f, 2.f, 420);
        slider->peak = 0.f;
    }
    return pose;
}

struct Tabs {
    Spring x{0.f, 320.f, 23.f}, width{10.f, 320.f, 26.f}, up{0.f, 420.f, 13.f};
    float targetX = 0.f, targetWidth = 0.f;
    bool pending = false, initialized = false;
    int direction = 1, selected = -1;
};
inline Tabs tabs;
struct TabPose { float x = 0.f, width = 0.f, lift = 0.f; };
inline TabPose TabMotion(float x, float width, int current, ImVec2 center, float height) {
    const bool changed = std::fabs(tabs.targetX - x) > .25f || std::fabs(tabs.targetWidth - width) > .25f;
    if (!tabs.initialized) {
        tabs.initialized = true;
        tabs.x.x = tabs.x.target = x;
        tabs.width.x = tabs.width.target = width;
    } else if (current != tabs.selected) {
        tabs.direction = x >= tabs.x.target ? 1 : -1;
        tabs.x.target = x;
        tabs.width.target = width;
        tabs.pending = true;
        tabs.up.x = -5.f;
    } else if (changed) {
        tabs.x.target = x;
        tabs.width.target = width;
        if (!tabs.pending) { tabs.x.x = x; tabs.width.x = width; }
    }
    tabs.targetX = x;
    tabs.targetWidth = width;
    tabs.selected = current;
    const float dt = Dt(), sx = tabs.x.Step(dt), sw = tabs.width.Step(dt), lift = tabs.up.Step(dt);
    if (tabs.pending && std::fabs(sx - tabs.x.target) < 3.f) {
        tabs.pending = false;
        tabs.up.x = -7.f;
        Ticks(center.x, center.y - height * .5f - Px(12), 3, 4, 15, -IM_PI * .5f, .5f, .5f);
        Puff(center.x - tabs.direction * (Px(60)), center.y - Px(6), 4, 9, -tabs.direction * 20, -12, .7f);
        Lines(center.x - tabs.direction * Px(60), center.y, 4,
            tabs.direction > 0 ? IM_PI : 0.f, 34, Px(10), .3f);
    }
    return {sx, sw + ImMin(46.f, std::fabs(tabs.x.velocity) * .06f), lift};
}

inline void CircleLobe(ImDrawList* draw, ImVec2 center, float radius, float light) {
    if (radius <= Px(.3f)) return;
    draw->AddCircleFilled(center, radius, Alpha(lite_colors::Soft), 12);
    const float shifted = radius - Px(1.5f);
    if (shifted > 0.f) draw->AddCircleFilled(center + ImVec2(-radius * .18f, -radius * .24f),
        shifted, Alpha(lite_colors::Section), 12);
    draw->AddCircle(center, radius, Alpha(lite_colors::AccentDark), 0, Px(1.5f));
    if (light > Px(6.5f)) {
        const float rr = radius * .72f;
        draw->PathArcTo(center, rr, -2.7f, -.5f, 6);
        draw->PathStroke(Alpha(lite_colors::AccentDark), 0, Px(1.1f));
    }
}

inline void Star(ImDrawList* draw, ImVec2 center, float radius, float turn, float hollow,
                 int spikes, uint32_t seed, ImU32 color, ImVec2 offset) {
    const int count = spikes * 2;
    ImVec2 points[36];
    for (int index = 0; index < count; ++index) {
        const float jitter = index % 2 ? .34f + Random(seed, index, 1) * .16f : .75f + Random(seed, index, 2) * .4f;
        const float angle = turn + float(index) / float(count) * IM_PI * 2.f;
        const float rr = radius * jitter * (index % 2 ? hollow : 1.f);
        points[index] = center + offset + ImVec2(std::cos(angle) * rr, std::sin(angle) * rr);
    }
    for (int index = 0; index < count; ++index) {
        const ImVec2 a = points[index], b = points[(index + 1) % count];
        draw->AddTriangleFilled(center + offset, a, b, color);
    }
}

inline void DrawBurst(ImDrawList* draw, const Effect& fx, float age) {
    if (age < 0.f || age > fx.life) return;
    const float k = Clamp(age / fx.life), radius = fx.a * Out(Clamp(k / .3f));
    const float hollow = 1.f + In(Clamp((k - .35f) / .65f)) * 1.3f;
    const float inner = radius * .6f * ImMin(1.9f, hollow);
    for (int index = 0; index < 8; ++index) {
        const float angle = float(index) / 8.f * IM_PI * 2.f + Random(fx.seed, index, 3) * .5f;
        const float length = radius * (1.25f + Random(fx.seed, index, 4) * .5f);
        const float half = .03f + Random(fx.seed, index, 5) * .025f;
        if (length <= inner) continue;
        const ImVec2 origin(fx.x, fx.y);
        draw->AddTriangleFilled(
            origin + ImVec2(std::cos(angle - half), std::sin(angle - half)) * inner,
            origin + ImVec2(std::cos(angle), std::sin(angle)) * length,
            origin + ImVec2(std::cos(angle + half), std::sin(angle + half)) * inner,
            Alpha(lite_colors::Accent));
    }
    Star(draw, {fx.x, fx.y}, radius * 1.16f, .19f, hollow, fx.count, fx.seed, Alpha(lite_colors::Soft), {});
    ImVec2 outline[36];
    const int count = fx.count * 2;
    for (int index = 0; index < count; ++index) {
        const float jitter = index % 2 ? (.34f + Random(fx.seed, index, 1) * .16f) * hollow : .75f + Random(fx.seed, index, 2) * .4f;
        const float angle = float(index) / float(count) * IM_PI * 2.f;
        const float rr = radius * .84f * jitter;
        outline[index] = {fx.x + std::cos(angle) * rr, fx.y + std::sin(angle) * rr};
    }
    Star(draw, {fx.x, fx.y}, radius * .84f, 0.f, hollow, fx.count, fx.seed, Alpha(lite_colors::AccentDark), {});
    Star(draw, {fx.x, fx.y}, radius * .84f * .88f, 0.f, hollow, fx.count, fx.seed, Alpha(lite_colors::Section), {Px(-2), Px(-3)});
    draw->AddPolyline(outline, count, Alpha(lite_colors::AccentDark), ImDrawFlags_Closed, Px(1.4f));
}

inline void DrawPuff(ImDrawList* draw, const Effect& fx, float age) {
    if (age < 0.f || age > fx.life) return;
    const float k = Clamp(age / fx.life), t = age;
    const float grow = Back(Clamp(t / .22f));
    const float die = 1.f - In(Clamp((k - .55f) / .45f));
    const ImVec2 move(fx.x + fx.vx * Out(k) * fx.life, fx.y + fx.vy * Out(k) * fx.life);
    CircleLobe(draw, move, fx.a * grow * die, fx.a);
    for (int index = 0; index < fx.count; ++index) {
        const float angle = float(index) / fx.count * IM_PI * 2.f + Random(fx.seed, index, 6) * .6f;
        const float distance = (.25f + Random(fx.seed, index, 7) * .65f) * fx.a;
        const float radius = (.55f + Random(fx.seed, index, 8) * .45f) * fx.a;
        const float phase = Random(fx.seed, index, 9) * IM_PI * 2.f;
        const float speed = .6f + Random(fx.seed, index, 10) * .7f;
        const ImVec2 p = move + ImVec2(std::cos(angle + t * .9f * speed), std::sin(angle + t * .9f * speed)) *
            distance * (.5f + Out(k) * .9f) - ImVec2(0, Out(k) * Px(5) * speed);
        CircleLobe(draw, p, radius * grow * die * (1.f + std::sin(t * 9.f + phase) * .05f), fx.a);
    }
}

inline void DrawShards(ImDrawList* draw, const Effect& fx, float age) {
    if (age < 0.f || age > fx.life) return;
    const float k = Clamp(age / fx.life);
    for (int index = 0; index < fx.count; ++index) {
        const float angle = (fx.c + (Random(fx.seed, index, 11) - .5f) * fx.d) * .5f;
        const float speed = fx.a * (.5f + Random(fx.seed, index, 12) * .65f);
        const float size = fx.b * (.6f + Random(fx.seed, index, 13) * .8f);
        const float spin = (Random(fx.seed, index, 14) - .5f) * 14.f;
        const ImVec2 p(fx.x + std::cos(angle) * speed * age, fx.y + std::sin(angle) * speed * age + fx.e * age * age);
        const float scale = size * (1.f - In(Clamp((k - .6f) / .4f)));
        if (scale <= .2f) continue;
        ImVec2 points[5];
        for (int vertex = 0; vertex < 5; ++vertex) {
            const float base[5][2]{{-1.4f,.1f},{-0.6f,-.7f},{1.2f,-.45f},{1.5f,.15f},{.4f,.75f}};
            const float variation = .8f + Random(fx.seed, index * 5 + vertex, 15) * .4f;
            const float turn = spin * age + angle, c = std::cos(turn), s = std::sin(turn);
            const float x = base[vertex][0] * scale * variation, y = base[vertex][1] * scale * variation;
            points[vertex] = p + ImVec2(x * c - y * s, x * s + y * c);
        }
        draw->AddConvexPolyFilled(points, 5, Alpha(lite_colors::Soft));
        ImVec2 lit[5];
        for (int vertex = 0; vertex < 5; ++vertex) lit[vertex] = points[vertex] + ImVec2(-scale * .3f, -scale * .45f);
        draw->AddConvexPolyFilled(lit, 5, Alpha(lite_colors::Section));
        draw->AddPolyline(points, 5, Alpha(lite_colors::AccentDark), ImDrawFlags_Closed, Px(2.2f));
    }
}

inline void DrawSlash(ImDrawList* draw, const Effect& fx, float age) {
    if (age < 0.f || age > fx.life) return;
    const float k = Clamp(age / fx.life), R = fx.a * .9f;
    const float angle = fx.b;
    const ImVec2 center(fx.x - std::cos(angle - IM_PI * .5f) * R * .86f,
        fx.y - std::sin(angle - IM_PI * .5f) * R * .86f);
    const float start = angle - IM_PI * .5f - .62f, sweep = 1.24f;
    const float head = Out(Clamp(k / .42f)), tail = In(Clamp((k - .22f) / .78f));
    if (head - tail < .01f) return;
    constexpr int segments = 24;
    ImVec2 outer[segments + 1], inner[segments + 1], thin[segments + 1];
    for (int index = 0; index <= segments; ++index) {
        const float u = Lerp(tail, head, float(index) / segments), a = start + sweep * u;
        const float q = float(index) / segments;
        const float profile = std::pow(ImMax(0.f, std::sin(q * IM_PI)), .7f);
        outer[index] = center + ImVec2(std::cos(a) * R, std::sin(a) * R);
        inner[index] = center + ImVec2(std::cos(a) * (R - fx.c * profile),
            std::sin(a) * (R - fx.c * profile));
        thin[index] = center + ImVec2(std::cos(a) * (R - fx.c * .42f * profile),
            std::sin(a) * (R - fx.c * .42f * profile));
    }
    for (int index = 0; index < segments; ++index) {
        draw->AddQuadFilled(outer[index], outer[index + 1], inner[index + 1], inner[index],
            Alpha(fx.color ? fx.color : lite_colors::Accent));
        draw->AddQuadFilled(outer[index], outer[index + 1], thin[index + 1], thin[index], Alpha(lite_colors::Section));
    }
    draw->AddPolyline(outer, segments + 1, Alpha(lite_colors::AccentDark), 0, Px(2.6f));
    draw->AddPolyline(inner, segments + 1, Alpha(lite_colors::AccentDark), 0, Px(1.3f));
}

inline void DrawLines(ImDrawList* draw, const Effect& fx, float age) {
    if (age < 0.f || age > fx.life) return;
    const float k = Clamp(age / fx.life), a = Out(k), b = In(k);
    for (int index = 0; index < fx.count; ++index) {
        const float offset = (Random(fx.seed, index, 16) - .5f) * 2.f * fx.c;
        const float length = fx.b * (.5f + Random(fx.seed, index, 17) * .8f);
        const float delay = Random(fx.seed, index, 18) * .25f;
        const float kk = Clamp((k - delay) / (1.f - delay));
        if (kk <= 0.f) continue;
        const ImVec2 p = ImVec2(fx.x, fx.y) + ImVec2(-std::sin(fx.a), std::cos(fx.a)) * offset;
        const ImVec2 axis(std::cos(fx.a), std::sin(fx.a));
        draw->AddLine(p + axis * length * In(kk) * 1.4f, p + axis * length * (.2f + Out(kk) * 1.2f),
            Alpha(lite_colors::AccentDark), Px(1.8f) * (1.f - b));
    }
    (void)a;
}

inline void DrawTicks(ImDrawList* draw, const Effect& fx, float age) {
    if (age < 0.f || age > fx.life) return;
    const float k = Clamp(age / fx.life), a = Out(Clamp(k / .5f)), b = In(Clamp((k - .4f) / .6f));
    for (int index = 0; index < fx.count; ++index) {
        const float angle = fx.c + (float(index) - float(fx.count - 1) * .5f) * fx.d;
        const float r0 = fx.a + fx.b * b * 1.25f, r1 = fx.a + fx.b * a * 1.25f;
        const float half = .07f + 2.2f / (r1 + Px(8));
        if (r1 - r0 < Px(.6f)) continue;
        draw->AddTriangleFilled(
            ImVec2(fx.x, fx.y) + ImVec2(std::cos(angle), std::sin(angle)) * r0,
            ImVec2(fx.x, fx.y) + ImVec2(std::cos(angle - half), std::sin(angle - half)) * r1,
            ImVec2(fx.x, fx.y) + ImVec2(std::cos(angle + half), std::sin(angle + half)) * r1,
            Alpha(lite_colors::Accent));
    }
}

inline void DrawDonut(ImDrawList* draw, const Effect& fx, float age) {
    if (age < 0.f || age > fx.life) return;
    const float k = Clamp(age / fx.life), scaleOut = Back(Clamp(age / .22f)) * (1.f - In(Clamp((k - .6f) / .4f)));
    if (scaleOut < .03f) return;
    const float r = fx.a * scaleOut;
    const ImVec2 center(fx.x + fx.vx * Out(k) * fx.life, fx.y + fx.vy * Out(k) * fx.life);
    const float hole = .42f + .22f * k, tilt = -.25f, ct = std::cos(tilt), st = std::sin(tilt);
    ImVec2 outer[24], inner[24];
    for (int index = 0; index < 24; ++index) {
        const float a = float(index) / 24.f * IM_PI * 2.f;
        const float bump = 1.f + .09f * std::sin(a * 9.f + Random(fx.seed, 0, 19) + age * 2.f);
        const float ex = std::cos(a) * r * bump, ey = std::sin(a) * r * bump * .52f;
        outer[index] = center + ImVec2(ex * ct - ey * st, ex * st + ey * ct);
        const float hx = std::cos(a) * r * hole * (1.f + .05f * std::sin(a * 9.f + Random(fx.seed, 0, 19) + age * 2.f));
        const float hy = std::sin(a) * r * hole * .52f;
        inner[index] = center + ImVec2(hx * ct - hy * st, hx * st + hy * ct);
    }
    for (int index = 0; index < 24; ++index) {
        const int next = (index + 1) % 24;
        draw->AddQuadFilled(outer[index], outer[next], inner[next], inner[index], Alpha(lite_colors::Soft));
        draw->AddQuadFilled(outer[index] + ImVec2(0, -Px(2)), outer[next] + ImVec2(0, -Px(2)),
            inner[next] + ImVec2(0, -Px(2)), inner[index] + ImVec2(0, -Px(2)), Alpha(lite_colors::Section));
    }
    draw->AddPolyline(outer, 24, Alpha(lite_colors::AccentDark), ImDrawFlags_Closed, Px(3));
    draw->AddPolyline(inner, 24, Alpha(lite_colors::AccentDark), ImDrawFlags_Closed, Px(2));
}

inline void DrawWob(ImDrawList* draw, const Effect& fx, float age) {
    if (age < 0.f || age > fx.life) return;
    const float k = Clamp(age / fx.life), width = Px(2.2f) * (1.f - k);
    if (width < Px(.2f)) return;
    for (int offset = 0; offset < 2; ++offset) {
        const float radius = fx.b + Px(7 * offset) + Out(k) * Px(8);
        const float angle = fx.a > 0.f ? 0.f : IM_PI;
        draw->PathArcTo(ImVec2(fx.x - fx.a * fx.b, fx.y), radius, angle - .45f, angle + .45f, 8);
    }
    draw->PathStroke(Alpha(lite_colors::AccentDark), 0, width);
}

inline void DrawKira(ImDrawList* draw, const Effect& fx, float age) {
    if (age < 0.f || age > fx.life) return;
    const float k = Clamp(age / fx.life), radius = fx.a * std::pow(ImMax(0.f, std::sin(k * IM_PI)), .6f), q = radius * .16f;
    if (radius < .5f) return;
    const ImVec2 c(fx.x, fx.y);
    draw->PathLineTo(c + ImVec2(0, -radius));
    draw->PathBezierQuadraticCurveTo(c + ImVec2(q, -q), c + ImVec2(radius * .7f, 0), 6);
    draw->PathBezierQuadraticCurveTo(c + ImVec2(q, q), c + ImVec2(0, radius), 6);
    draw->PathBezierQuadraticCurveTo(c + ImVec2(-q, q), c + ImVec2(-radius * .7f, 0), 6);
    draw->PathBezierQuadraticCurveTo(c + ImVec2(-q, -q), c + ImVec2(0, -radius), 6);
    draw->PathFillConvex(Alpha(lite_colors::Section));
    draw->PathStroke(Alpha(lite_colors::Accent), ImDrawFlags_Closed, Px(2));
}

inline void DrawWord(ImDrawList* draw, Effect& fx, float age) {
    if (age < 0.f || age > fx.life) return;
    if (sakuga_art::rectIds[fx.word] < 0 || sakuga_art::uv1[fx.word].x <= 0.f) return;
    const int widths[3]{sakuga_art::donWidth, sakuga_art::zuzaWidth, sakuga_art::shuWidth};
    const int heights[3]{sakuga_art::donHeight, sakuga_art::zuzaHeight, sakuga_art::shuHeight};
    Spring spring(fx.springX, 520.f, 17.f);
    spring.velocity = fx.springVelocity;
    spring.target = 1.f;
    const float dt = ImMax(0.f, age - fx.lastT);
    spring.Step(dt);
    fx.springX = spring.x;
    fx.springVelocity = spring.velocity;
    fx.lastT = age;
    const float k = Clamp(age / fx.life);
    const float scaleOut = spring.x * (1.f - In(Clamp((k - .86f) / .14f))) * fx.a / float(widths[fx.word]);
    if (scaleOut <= .001f) return;
    const float w = widths[fx.word] * scaleOut, h = heights[fx.word] * scaleOut;
    const ImVec2 center(fx.x + std::sin(age * 31.f) * Px(.8f), fx.y + std::cos(age * 27.f) * Px(.8f));
    const float rotation = fx.b + std::sin(age * 6.f) * .03f, c = std::cos(rotation), s = std::sin(rotation);
    const ImVec2 corners[4]{{-w * .5f, -h * .5f}, {w * .5f, -h * .5f}, {w * .5f, h * .5f}, {-w * .5f, h * .5f}};
    ImVec2 points[4];
    for (int index = 0; index < 4; ++index)
        points[index] = center + ImVec2(corners[index].x * c - corners[index].y * s,
            corners[index].x * s + corners[index].y * c);
    draw->AddImageQuad(ImGui::GetIO().Fonts->TexID, points[0], points[1], points[2], points[3],
        sakuga_art::uv0[fx.word], ImVec2(sakuga_art::uv1[fx.word].x, sakuga_art::uv0[fx.word].y),
        sakuga_art::uv1[fx.word], ImVec2(sakuga_art::uv0[fx.word].x, sakuga_art::uv1[fx.word].y),
        Alpha(lite_colors::Section));
}

inline void DrawEffect(ImDrawList* draw, Effect& fx, float age) {
    switch (fx.kind) {
        case Kind::Burst: DrawBurst(draw, fx, age); break;
        case Kind::Puff: DrawPuff(draw, fx, age); break;
        case Kind::Shards: DrawShards(draw, fx, age); break;
        case Kind::Slash: DrawSlash(draw, fx, age); break;
        case Kind::Lines: DrawLines(draw, fx, age); break;
        case Kind::Ticks: DrawTicks(draw, fx, age); break;
        case Kind::Donut: DrawDonut(draw, fx, age); break;
        case Kind::Wob: DrawWob(draw, fx, age); break;
        case Kind::Kira: DrawKira(draw, fx, age); break;
        case Kind::Word: DrawWord(draw, fx, age); break;
    }
}

inline void Behind(ImDrawList* draw, ImGuiID id) {
    if (!enabled || reduced || !id || !draw) return;
    const double now = ImGui::GetTime();
    for (auto& effect : effects) if (effect.life > 0.f && effect.behind && effect.owner == id)
        DrawEffect(draw, effect, static_cast<float>(now - effect.born - effect.delay));
}

inline void Draw(ImDrawList* draw) {
    if (!enabled || !draw) return;
    const double now = ImGui::GetTime();
    for (auto& effect : effects) {
        if (effect.life <= 0.f) continue;
        const float age = static_cast<float>(now - effect.born - effect.delay);
        if (age >= effect.life) { effect.life = 0.f; continue; }
        if (effect.behind || reduced) continue;
        DrawEffect(draw, effect, age);
    }
}

inline void ClearEffects() {
    for (auto& effect : effects) effect.life = 0.f;
    for (auto& motion : motions) motion.id = 0;
    for (auto& slider : sliders) slider.id = 0;
}

inline void Configure(bool reducedMotion, float framesPerSecond) {
    reduced = reducedMotion;
    static int slowFrames = 0;
    if (framesPerSecond > 0.f && framesPerSecond < 24.f) { if (slowFrames < 600) ++slowFrames; }
    else if (framesPerSecond >= 30.f) slowFrames = 0;
    lean = slowFrames > 90;
}
}
