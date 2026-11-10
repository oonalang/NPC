#pragma once

#include "ImGui/Call_ImGui.h"
#include "../../ImGui/imgui_settings.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

extern ImFont* F50;
extern ImFont* F107;

namespace font {
    extern ImFont* inter_semibold;
}

namespace ui_loading {

struct TechNode {
    ImVec2 pos;
    float angle;
    float speed;
    float pulse;
    int nodeType;
};

struct CircuitLine {
    ImVec2 start;
    ImVec2 end;
    float progress;
    float speed;
    bool active;
};

struct HexGrid {
    ImVec2 center;
    float size;
    float rotation;
    float glowIntensity;
};

static bool showLoadingAnimation = false;
static float loadingAnimationTimer = 0.0f;
static float loadingAnimationDuration = 30.0f;
static std::vector<TechNode> techNodes;
static std::vector<CircuitLine> circuitLines;
static std::vector<HexGrid> hexGrids;
static float animationTime = 0.0f;
static float rotationAngle = 0.0f;
static float pulseWave = 0.0f;
static bool seededRandom = false;

namespace ElectricRed {
    const ImVec4 Accent       = ImVec4(1.0000f, 0.1333f, 0.1333f, 1.00f);
    const ImVec4 AccentDark   = ImVec4(0.6000f, 0.0500f, 0.0500f, 1.00f);
    const ImVec4 AccentSoft   = ImVec4(1.0000f, 0.2500f, 0.2500f, 0.60f);
    const ImVec4 AccentGlow   = ImVec4(1.0000f, 0.3500f, 0.3500f, 0.40f);
    const ImVec4 AccentBright = ImVec4(1.0000f, 0.5500f, 0.5500f, 0.95f);
    const ImVec4 RedShadow    = ImVec4(0.4000f, 0.0800f, 0.0800f, 0.85f);
    const ImVec4 Background   = ImVec4(0.0314f, 0.0235f, 0.0275f, 1.00f);
    const ImVec4 PanelBg      = ImVec4(0.0510f, 0.0314f, 0.0392f, 0.98f);
    const ImVec4 TextPrimary  = ImVec4(1.0000f, 0.8500f, 0.8500f, 1.00f);
    const ImVec4 TextSecondary = ImVec4(0.7500f, 0.5000f, 0.5000f, 0.90f);
}

inline float EaseOutCubic(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    const float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

inline float EaseInOutSine(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return -(std::cos(3.14159265f * t) - 1.0f) * 0.5f;
}

inline void SeedRandomOnce() {
    if (!seededRandom) {
        std::srand((unsigned int)std::time(nullptr));
        seededRandom = true;
    }
}

inline void InitParticles(float orbitRadius) {
    SeedRandomOnce();
    techNodes.clear();
    circuitLines.clear();
    hexGrids.clear();
    techNodes.reserve(48);
    circuitLines.reserve(24);
    hexGrids.reserve(8);

    const int nodeCount = 48;
    for (int i = 0; i < nodeCount; ++i) {
        TechNode node{};
        const float angle = (i / (float)nodeCount) * 2.0f * 3.14159265f;
        const float radialJitter = ((std::rand() % 100) / 100.0f - 0.5f) * orbitRadius * 0.35f;
        const float radius = orbitRadius + radialJitter;
        node.pos = ImVec2(std::cos(angle) * radius, std::sin(angle) * radius);
        node.angle = angle;
        node.speed = 0.45f + ((std::rand() % 100) / 100.0f) * 1.25f;
        node.pulse = (std::rand() % 100) / 100.0f * 6.2831853f;
        node.nodeType = std::rand() % 3;
        techNodes.push_back(node);
    }

    const int lineCount = 24;
    for (int i = 0; i < lineCount; ++i) {
        const int idxA = std::rand() % nodeCount;
        int idxB = std::rand() % nodeCount;
        if (idxA == idxB) idxB = (idxB + 13) % nodeCount;
        CircuitLine line{};
        line.start = techNodes[idxA].pos;
        line.end = techNodes[idxB].pos;
        line.progress = (std::rand() % 100) / 100.0f;
        line.speed = 0.40f + ((std::rand() % 100) / 100.0f) * 1.30f;
        line.active = (std::rand() % 100) < 65;
        circuitLines.push_back(line);
    }

    const int hexCount = 8;
    for (int i = 0; i < hexCount; ++i) {
        HexGrid hex{};
        const float angle = (i / (float)hexCount) * 2.0f * 3.14159265f;
        const float radius = orbitRadius * (0.35f + (i % 5) * 0.15f);
        hex.center = ImVec2(std::cos(angle) * radius, std::sin(angle) * radius);
        hex.size = orbitRadius * (0.08f + (i % 4) * 0.04f);
        hex.rotation = ((std::rand() % 360) / 180.0f) * 3.14159265f;
        hex.glowIntensity = 0.50f + ((std::rand() % 100) / 100.0f) * 0.50f;
        hexGrids.push_back(hex);
    }
}

inline void Start(float durationSeconds = 30.0f) {
    showLoadingAnimation = true;
    loadingAnimationTimer = 0.0f;
    loadingAnimationDuration = std::max(5.0f, durationSeconds);
    animationTime = 0.0f;
    rotationAngle = 0.0f;
    pulseWave = 0.0f;
    InitParticles(220.0f);
}

inline bool IsActive() { return showLoadingAnimation; }

inline float GetProgress() {
    if (loadingAnimationDuration <= 0.0f) return 1.0f;
    return std::clamp(loadingAnimationTimer / loadingAnimationDuration, 0.0f, 1.0f);
}

// ── Step labels ──────────────────────────────────────────────────────────────
struct LoadStep {
    const char* icon;   // single UTF-8 char / icon glyph fallback
    const char* label;
};

static const LoadStep k_Steps[] = {
    { "I", "Initializing" },
    { "V", "Verifying"    },
    { "L", "Loading Data" },
    { "F", "Finalizing"   },
};
static const int k_StepCount = 4;

// Returns 0=pending, 1=active, 2=done
inline int StepState(int idx, float progress) {
    const float slice = 1.0f / (float)k_StepCount;
    const float stepDone = (idx + 1) * slice;
    const float stepStart = idx * slice;
    if (progress >= stepDone)  return 2;
    if (progress >= stepStart) return 1;
    return 0;
}

// ── Main draw ────────────────────────────────────────────────────────────────
inline bool RenderWindow(ImTextureID /*backgroundTexture*/ = nullptr) {
    if (!showLoadingAnimation) return false;

    ImGuiIO& io = ImGui::GetIO();
    // Clamp large frame gaps so the animation cannot jump wildly after a pause.
    const float dt = ImMin(io.DeltaTime, 1.0f / 30.0f);
    animationTime += dt;
    rotationAngle += dt * 1.4f;      // spinner speed
    pulseWave      = std::sin(animationTime * 2.8f) * 0.5f + 0.5f;

    const float progress = GetProgress();

    // ── Panel size & position ────────────────────────────────────────────────
    const ImVec2 panelSize(
        std::min(420.0f, io.DisplaySize.x - 48.0f),
        std::min(430.0f, io.DisplaySize.y - 72.0f)
    );
    const ImVec2 viewportCenter = ImGui::GetMainViewport()->GetCenter();

    ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    bool finished = false;

    if (!ImGui::Begin("##npc_loading", nullptr,
        ImGuiWindowFlags_NoBackground   |
        ImGuiWindowFlags_NoSavedSettings|
        ImGuiWindowFlags_NoScrollbar    |
        ImGuiWindowFlags_NoTitleBar     |
        ImGuiWindowFlags_NoResize       |
        ImGuiWindowFlags_NoCollapse     |
        ImGuiWindowFlags_NoMove))
    {
        ImGui::End();
        loadingAnimationTimer += dt;
        if (loadingAnimationTimer >= loadingAnimationDuration) {
            showLoadingAnimation  = false;
            loadingAnimationTimer = 0.0f;
            finished = true;
        }
        return finished;
    }

    ImDrawList* draw   = ImGui::GetWindowDrawList();
    const ImVec2 pos   = ImGui::GetWindowPos();
    const float  W     = panelSize.x;
    const float  H     = panelSize.y;

    // ── Fonts ────────────────────────────────────────────────────────────────
    ImFont* fontTitle = F50                 ? F50                 : ImGui::GetFont();
    ImFont* fontBody  = font::inter_semibold? font::inter_semibold: ImGui::GetFont();

    // ── Colors (NPC red HUD theme) ────────────────────────
    const ImVec4 acV  = ImVec4(0.93f, 0.035f, 0.12f, 1.0f);              // NPC red accent
    const ImU32  acC  = ImGui::GetColorU32(acV);
    const ImU32  acSoft = ImGui::GetColorU32(ImVec4(acV.x, acV.y, acV.z, 0.45f));
    const ImU32  acGlow = ImGui::GetColorU32(ImVec4(acV.x, acV.y, acV.z, 0.18f));
    const ImU32  acDim  = ImGui::GetColorU32(ImVec4(acV.x * 0.35f, acV.y * 0.35f, acV.z * 0.35f, 0.50f));
    const ImU32  white  = IM_COL32(220, 238, 255, 255);
    const ImU32  textMuted = IM_COL32(100, 160, 220, 200);

    // ── NPC red HUD frame ────────────────────────────────────────────────────
    const ImU32 hudRed = IM_COL32(238, 24, 52, 235);
    const ImU32 hudRedDim = IM_COL32(238, 24, 52, 90);
    const ImU32 hudBlack = IM_COL32(7, 5, 9, 245);

    draw->AddRectFilled(pos, ImVec2(pos.x + W, pos.y + H), hudBlack, 12.0f);
    draw->AddRect(pos, ImVec2(pos.x + W, pos.y + H), hudRedDim, 12.0f, 0, 1.0f);

    // Header rail
    draw->AddRectFilled(
        ImVec2(pos.x + 12.0f, pos.y + 10.0f),
        ImVec2(pos.x + W - 12.0f, pos.y + 43.0f),
        IM_COL32(13, 8, 12, 245),
        7.0f
    );
    draw->AddLine(
        ImVec2(pos.x + 18.0f, pos.y + 43.0f),
        ImVec2(pos.x + W - 18.0f, pos.y + 43.0f),
        hudRed,
        1.0f
    );
    draw->AddCircleFilled(
        ImVec2(pos.x + 27.0f, pos.y + 26.0f),
        3.0f,
        hudRed,
        12
    );

    // ── Panel background ─────────────────────────────────────────────────────
    // Outer shadow / glow
    for (int gl = 4; gl >= 0; --gl) {
        const float ex = (gl + 1) * 3.5f;
        const int   al = (int)(6.0f + pulseWave * 14.0f) - gl * 2;
        if (al <= 0) continue;
        draw->AddRectFilled(
            ImVec2(pos.x - ex, pos.y - ex),
            ImVec2(pos.x + W  + ex, pos.y + H + ex),
            ImGui::GetColorU32(ImVec4(acV.x * 0.4f, acV.y * 0.4f, acV.z * 0.4f, al / 255.0f)),
            18.0f + ex
        );
    }
    // Main dark panel
    draw->AddRectFilled(pos, ImVec2(pos.x + W, pos.y + H),
        IM_COL32(8, 5, 9, 250), 14.0f);
    // Subtle top gradient
    draw->AddRectFilledMultiColor(
        pos, ImVec2(pos.x + W, pos.y + H * 0.45f),
        ImGui::GetColorU32(ImVec4(acV.x * 0.18f, acV.y * 0.18f, acV.z * 0.18f, 0.14f)),
        ImGui::GetColorU32(ImVec4(acV.x * 0.18f, acV.y * 0.18f, acV.z * 0.18f, 0.14f)),
        IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0)
    );
    // Border
    const int borderA = (int)(130.0f + pulseWave * 70.0f);
    draw->AddRect(pos, ImVec2(pos.x + W, pos.y + H),
        ImGui::GetColorU32(ImVec4(acV.x, acV.y, acV.z, borderA / 255.0f)),
        14.0f, 0, 1.4f);

    // ── Title area ───────────────────────────────────────────────────────────
    {
        const char* titleStr = "NPC X LOADING";
        const float  tsz     = (fontTitle == F50) ? 26.0f : fontTitle->FontSize * 1.10f;
        const ImVec2 tSz     = fontTitle->CalcTextSizeA(tsz, FLT_MAX, 0.0f, titleStr);
        const float  tx      = pos.x + (W - tSz.x) * 0.5f;
        const float  ty      = pos.y + 28.0f;
        // shadow
        draw->AddText(fontTitle, tsz, ImVec2(tx + 1.2f, ty + 1.2f),
            IM_COL32(40, 2, 8, 180), titleStr);
        draw->AddText(fontTitle, tsz, ImVec2(tx, ty), white, titleStr);

        // Subtitle
        const char* sub    = "SYSTEM START  |  VERIFY  |  LOAD";
        const float  ssz   = fontBody ? fontBody->FontSize * 0.72f : 11.0f;
        const ImVec2 subSz = fontBody->CalcTextSizeA(ssz, FLT_MAX, 0.0f, sub);
        draw->AddText(fontBody, ssz,
            ImVec2(pos.x + (W - subSz.x) * 0.5f, ty + tSz.y + 6.0f),
            textMuted, sub);

        const float hudLineY = ty + tSz.y + 25.0f;
        draw->AddLine(
            ImVec2(pos.x + W * 0.18f, hudLineY),
            ImVec2(pos.x + W * 0.82f, hudLineY),
            IM_COL32(238, 24, 52, 150),
            1.0f
        );
        draw->AddCircleFilled(
            ImVec2(pos.x + W * 0.18f, hudLineY),
            2.5f,
            IM_COL32(255, 72, 92, 220),
            12
        );
    }

    // ── Multi-ring orbit animation ───────────────────────────────────────────
    {
        const float PI2    = 6.28318530f;
        const float cx     = pos.x + W * 0.5f;
        const float cy     = pos.y + H * 0.42f;
        const float baseR  = std::min(W, H) * 0.22f;

        // ── 1. Background dark circle ────────────────────────────────────────
        draw->AddCircleFilled(ImVec2(cx, cy), baseR * 1.15f,
            IM_COL32(10, 4, 7, 245), 80);

        // ── 2. Static dim rings (like the reference image concentric rings) ──
        const float ringRadii[] = { baseR * 0.28f, baseR * 0.52f, baseR * 0.75f, baseR * 1.00f };
        const int   ringSegs[]  = { 5, 8, 10, 14 };  // dots per ring
        const float ringSpeeds[]= { 1.8f, -1.2f, 0.9f, -0.6f }; // rotation speeds (neg = reverse)

        for (int r = 0; r < 4; ++r) {
            const float rr = ringRadii[r];

            // Dim static ring line
            draw->AddCircle(ImVec2(cx, cy), rr,
                ImGui::GetColorU32(ImVec4(acV.x * 0.5f, acV.y * 0.5f, acV.z * 0.5f,
                    0.12f + 0.06f * pulseWave)),
                40, 0.8f);

            // Spinning arc overlay on each ring
            {
                const float arcStart = animationTime * ringSpeeds[r];
                const float arcEnd   = arcStart + PI2 * 0.35f;
                const int   segs     = 12;
                for (int i = 0; i < segs; ++i) {
                    const float t0  = i / (float)segs;
                    const float t1  = (i + 1) / (float)segs;
                    const float a0  = arcStart + (arcEnd - arcStart) * t0;
                    const float a1  = arcStart + (arcEnd - arcStart) * t1;
                    const float fade = t0 * (0.5f + 0.3f * pulseWave);
                    draw->AddLine(
                        ImVec2(cx + std::cos(a0) * rr, cy + std::sin(a0) * rr),
                        ImVec2(cx + std::cos(a1) * rr, cy + std::sin(a1) * rr),
                        ImGui::GetColorU32(ImVec4(acV.x, acV.y, acV.z, fade)),
                        r == 3 ? 2.2f : 1.2f
                    );
                }
            }

            // Orbiting dots on each ring
            const int   nd    = ringSegs[r];
            const float speed = ringSpeeds[r];
            for (int d = 0; d < nd; ++d) {
                const float angle = animationTime * speed + PI2 * d / nd;
                const float dx    = cx + std::cos(angle) * rr;
                const float dy    = cy + std::sin(angle) * rr;

                // dot size varies by ring
                const float dotR  = (r == 3) ? 3.2f : (r == 2) ? 2.4f : (r == 1) ? 2.0f : 1.6f;

                // primary dot (first dot on each ring is brighter)
                const float bright = (d == 0) ? 1.0f : (d == 1 ? 0.7f : 0.4f);
                const ImU32 dotCol = ImGui::GetColorU32(
                    ImVec4(acV.x, acV.y, acV.z, bright * (0.7f + 0.3f * pulseWave)));

                draw->AddCircleFilled(ImVec2(dx, dy), dotR, dotCol, 12);

                // glow halo on bright dots
                if (d == 0) {
                    draw->AddCircle(ImVec2(dx, dy), dotR + 2.5f,
                        ImGui::GetColorU32(ImVec4(acV.x, acV.y, acV.z, 0.18f)), 12, 2.0f);
                }
            }
        }

        // ── 3. Hexagon / node decorations (outer area, like reference image) ─
        {
            const float hexR  = baseR * 0.22f;
            const float nodeAngles[] = { 0.3f, 1.1f, 2.0f, 2.9f, 4.0f, 5.1f };
            const float nodeDists[]  = { 1.35f, 1.42f, 1.38f, 1.45f, 1.40f, 1.36f };
            for (int n = 0; n < 6; ++n) {
                const float na = nodeAngles[n] + animationTime * 0.08f; // very slow drift
                const float nd = baseR * nodeDists[n];
                const float nx = cx + std::cos(na) * nd;
                const float ny = cy + std::sin(na) * nd;
                // draw small hexagon outline
                for (int h = 0; h < 6; ++h) {
                    const float a0 = PI2 * h / 6.0f;
                    const float a1 = PI2 * (h + 1) / 6.0f;
                    draw->AddLine(
                        ImVec2(nx + std::cos(a0) * hexR, ny + std::sin(a0) * hexR),
                        ImVec2(nx + std::cos(a1) * hexR, ny + std::sin(a1) * hexR),
                        ImGui::GetColorU32(ImVec4(acV.x * 0.6f, acV.y * 0.6f, acV.z * 0.6f, 0.22f)),
                        0.8f
                    );
                }
                // connector line from node to nearest ring
                draw->AddLine(
                    ImVec2(cx + std::cos(na) * baseR, cy + std::sin(na) * baseR),
                    ImVec2(nx, ny),
                    ImGui::GetColorU32(ImVec4(acV.x * 0.4f, acV.y * 0.4f, acV.z * 0.4f, 0.15f)),
                    0.7f
                );
                // small node dot
                draw->AddCircleFilled(ImVec2(nx, ny), 2.5f,
                    ImGui::GetColorU32(ImVec4(acV.x, acV.y, acV.z, 0.35f + 0.15f * pulseWave)), 8);
            }
        }

        // ── 4. Center glowing core ───────────────────────────────────────────
        {
            // Soft glow layers
            for (int g = 4; g >= 0; --g) {
                const float gr = baseR * 0.10f + g * 4.5f;
                const float ga = (0.08f - g * 0.014f) * (0.8f + pulseWave * 0.2f);
                draw->AddCircleFilled(ImVec2(cx, cy), gr,
                    ImGui::GetColorU32(ImVec4(acV.x, acV.y, acV.z, ga)), 32);
            }
            // Bright center dot
            draw->AddCircleFilled(ImVec2(cx, cy), baseR * 0.08f,
                ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.92f + 0.08f * pulseWave)), 24);
            // Inner ring
            draw->AddCircle(ImVec2(cx, cy), baseR * 0.14f,
                ImGui::GetColorU32(ImVec4(acV.x, acV.y, acV.z, 0.40f)), 32, 1.2f);
        }
    }

    // ── Step icons row ───────────────────────────────────────────────────────
    {
        const float  stepRowY  = pos.y + H * 0.66f;
        const float  iconR     = std::min(W * 0.07f, 24.0f);  // circle radius for each step
        const float  totalW    = (float)(k_StepCount - 1) * (W * 0.22f);
        const float  startX    = pos.x + (W - totalW) * 0.5f;
        const float  stepGapX  = W * 0.22f;
        const float  labelSz   = fontBody ? fontBody->FontSize * 0.62f : 9.5f;

        for (int i = 0; i < k_StepCount; ++i) {
            const int   state  = StepState(i, progress);
            const float cx     = startX + i * stepGapX;
            const float cy     = stepRowY;

            // Connector line to next step
            if (i < k_StepCount - 1) {
                const float lineY = cy;
                const float lx0   = cx + iconR + 4.0f;
                const float lx1   = cx + stepGapX - iconR - 4.0f;
                // background line
                draw->AddLine(ImVec2(lx0, lineY), ImVec2(lx1, lineY),
                    IM_COL32(65, 16, 25, 180), 1.5f);
                // filled portion
                if (state == 2) {
                    draw->AddLine(ImVec2(lx0, lineY), ImVec2(lx1, lineY), acC, 1.5f);
                } else if (state == 1) {
                    const float midX = lx0 + (lx1 - lx0) * EaseInOutSine(
                        (progress - i * (1.0f / k_StepCount)) * k_StepCount);
                    draw->AddLine(ImVec2(lx0, lineY), ImVec2(midX, lineY), acSoft, 1.5f);
                }
            }

            // Circle bg
            if (state == 2) {
                // Done: filled accent circle
                draw->AddCircleFilled(ImVec2(cx, cy), iconR, acC, 32);
                draw->AddCircle(ImVec2(cx, cy), iconR,
                    ImGui::GetColorU32(ImVec4(acV.x, acV.y, acV.z, 0.5f)), 32, 1.2f);
            } else if (state == 1) {
                // Active: pulsing ring
                const float pulse = 0.5f + pulseWave * 0.5f;
                draw->AddCircleFilled(ImVec2(cx, cy), iconR, IM_COL32(6, 12, 28, 240), 32);
                draw->AddCircle(ImVec2(cx, cy), iconR, acC, 32, 1.8f);
                // Outer glow
                draw->AddCircle(ImVec2(cx, cy), iconR + 4.0f,
                    ImGui::GetColorU32(ImVec4(acV.x, acV.y, acV.z, 0.20f * pulse)), 32, 3.0f);
                // Spinning mini-arc inside active circle
                {
                    const float ar  = iconR * 0.65f;
                    const float as  = animationTime * 2.8f;
                    const float ae  = as + 3.14159265f * 1.1f;
                    for (int s = 0; s < 20; ++s) {
                        const float t0 = s / 20.0f, t1 = (s+1) / 20.0f;
                        draw->AddLine(
                            ImVec2(cx + std::cos(as + (ae-as)*t0)*ar, cy + std::sin(as + (ae-as)*t0)*ar),
                            ImVec2(cx + std::cos(as + (ae-as)*t1)*ar, cy + std::sin(as + (ae-as)*t1)*ar),
                            ImGui::GetColorU32(ImVec4(acV.x, acV.y, acV.z, 0.55f + 0.45f*t0)), 1.5f
                        );
                    }
                }
            } else {
                // Pending: dim circle
                draw->AddCircleFilled(ImVec2(cx, cy), iconR, IM_COL32(18, 7, 11, 235), 32);
                draw->AddCircle(ImVec2(cx, cy), iconR, IM_COL32(105, 24, 39, 190), 32, 1.2f);
            }

            // Step label below circle
            const ImVec2 lblSz = fontBody->CalcTextSizeA(labelSz, FLT_MAX, 0.0f, k_Steps[i].label);
            draw->AddText(fontBody, labelSz,
                ImVec2(cx - lblSz.x * 0.5f, cy + iconR + 5.0f),
                state == 0 ? IM_COL32(40, 80, 160, 180) : (state == 2 ? acC : white),
                k_Steps[i].label
            );
        }
    }

    // ── Progress bar ─────────────────────────────────────────────────────────
    {
        const float barPad  = W * 0.08f;
        const float barY    = pos.y + H * 0.80f;
        const float barH    = 8.0f;
        const float barW    = W - barPad * 2.0f;
        const float filled  = barW * EaseOutCubic(progress);
        const float bx      = pos.x + barPad;

        // Track
        draw->AddRectFilled(
            ImVec2(bx, barY), ImVec2(bx + barW, barY + barH),
            IM_COL32(24, 8, 13, 235), barH * 0.5f);

        // Filled portion
        if (filled > 0.0f) {
            draw->AddRectFilled(
                ImVec2(bx, barY), ImVec2(bx + filled, barY + barH),
                ImGui::GetColorU32(ImVec4(acV.x, acV.y, acV.z, 0.92f)),
                barH * 0.5f);
            // Shimmer at tip
            draw->AddCircleFilled(ImVec2(bx + filled, barY + barH * 0.5f),
                barH * 0.6f, IM_COL32(255, 92, 108, 225), 12);
        }

        // Percentage text
        char pct[16] = {};
        std::snprintf(pct, sizeof(pct), "%d%%", (int)std::lround(progress * 100.0f));
        const float pSz  = fontBody ? fontBody->FontSize * 0.68f : 10.5f;
        const ImVec2 pSzV = fontBody->CalcTextSizeA(pSz, FLT_MAX, 0.0f, pct);
        draw->AddText(fontBody, pSz,
            ImVec2(bx + barW - pSzV.x, barY - pSzV.y - 4.0f),
            ImGui::GetColorU32(c::text::text_active), pct);
    }

    // ── "Please wait" info box ───────────────────────────────────────────────
    {
        const float  bxPad  = W * 0.06f;
        const float  boxY   = pos.y + H * 0.87f;
        const float  boxW   = W - bxPad * 2.0f;
        const float  boxH   = H * 0.10f;
        const ImVec2 bMin(pos.x + bxPad, boxY);
        const ImVec2 bMax(pos.x + bxPad + boxW, boxY + boxH);

        draw->AddRectFilled(bMin, bMax, IM_COL32(18, 7, 11, 220), 8.0f);
        draw->AddRect(bMin, bMax,
            ImGui::GetColorU32(ImVec4(acV.x * 0.5f, acV.y * 0.5f, acV.z * 0.5f, 0.35f)),
            8.0f, 0, 0.8f);

        // Info dot on the left
        const float dotX = bMin.x + 14.0f;
        const float dotY = bMin.y + boxH * 0.38f;
        draw->AddCircleFilled(ImVec2(dotX, dotY), 4.5f, acSoft, 12);

        // Text
        const float  tsz   = fontBody ? fontBody->FontSize * 0.72f : 11.0f;
        const float  tsz2  = fontBody ? fontBody->FontSize * 0.62f : 9.5f;
        const float  tx    = dotX + 12.0f;
        draw->AddText(fontBody, tsz,  ImVec2(tx, bMin.y + 6.0f),  white,     "Please wait");
        draw->AddText(fontBody, tsz2, ImVec2(tx, bMin.y + 6.0f + tsz + 2.0f),
            textMuted, "This may take a few seconds. Do not close the application.");
    }

    ImGui::End();

    loadingAnimationTimer += dt;
    if (loadingAnimationTimer >= loadingAnimationDuration) {
        showLoadingAnimation  = false;
        loadingAnimationTimer = 0.0f;
        finished = true;
    }

    return finished;
}

} // namespace ui_loading

