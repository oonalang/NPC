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
    const float dt = ImMin(io.DeltaTime, 1.0f / 30.0f);
    animationTime += dt;
    rotationAngle += dt * 1.4f;
    pulseWave = std::sin(animationTime * 2.8f) * 0.5f + 0.5f;

    const float progress = GetProgress();
    const ImVec2 panelSize(
        std::min(460.0f, io.DisplaySize.x - 48.0f),
        std::min(486.0f, io.DisplaySize.y - 72.0f)
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

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos  = ImGui::GetWindowPos();
    const ImVec2 cardMax(pos.x + panelSize.x, pos.y + panelSize.y);
    const float W = panelSize.x;
    const float H = panelSize.y;

    ImFont* fontTitle = F50                  ? F50                  : ImGui::GetFont();
    ImFont* fontBody  = font::inter_semibold ? font::inter_semibold : ImGui::GetFont();

    const ImVec4 accentV(0.694f, 0.431f, 0.961f, 1.0f);
    const ImU32 accent     = ImGui::GetColorU32(accentV);
    const ImU32 accentSoft = ImGui::GetColorU32(ImVec4(accentV.x, accentV.y, accentV.z, 0.45f));
    const ImU32 white      = IM_COL32(247, 245, 252, 255);
    const ImU32 textMuted  = IM_COL32(150, 146, 168, 235);

    for (int layer = 4; layer >= 1; --layer) {
        const float expand = layer * 7.0f;
        draw->AddRectFilled(
            pos - ImVec2(expand, expand),
            cardMax + ImVec2(expand, expand),
            ImGui::GetColorU32(ImVec4(accentV.x, accentV.y, accentV.z, 0.026f)),
            32.0f + expand
        );
    }

    draw->AddRectFilled(pos, cardMax, IM_COL32(13, 11, 19, 248), 32.0f);
    draw->AddRectFilledMultiColor(
        pos,
        ImVec2(cardMax.x, pos.y + H * 0.5f),
        IM_COL32(255, 255, 255, 16),
        IM_COL32(206, 186, 255, 10),
        IM_COL32(255, 255, 255, 0),
        IM_COL32(255, 255, 255, 0)
    );
    draw->AddRect(pos, cardMax, IM_COL32(230, 222, 255, 42), 32.0f, 0, 1.4f);
    draw->AddRect(pos + ImVec2(2.0f, 2.0f), cardMax - ImVec2(2.0f, 2.0f), IM_COL32(255, 255, 255, 9), 30.0f, 0, 1.0f);
    draw->AddLine(
        ImVec2(pos.x + 46.0f, pos.y + 1.8f),
        ImVec2(cardMax.x - 46.0f, pos.y + 1.2f),
        IM_COL32(255, 255, 255, 96),
        1.6f
    );

    const float cx = pos.x + W * 0.5f;
    const ImVec2 logoCenter(cx, pos.y + 46.0f);
    for (int g = 3; g >= 1; --g) {
        draw->AddCircleFilled(
            logoCenter,
            21.0f + g * 4.0f,
            ImGui::GetColorU32(ImVec4(accentV.x, accentV.y, accentV.z, 0.040f + 0.010f * pulseWave)),
            32
        );
    }
    draw->AddCircleFilled(logoCenter, 21.0f, IM_COL32(26, 18, 36, 250), 32);
    {
        ImVec2 hexOuter[6];
        ImVec2 hexInner[6];
        for (int i = 0; i < 6; ++i) {
            const float a = -1.5707964f + (float)i * 1.0471976f;
            hexOuter[i] = ImVec2(logoCenter.x + std::cos(a) * 18.0f, logoCenter.y + std::sin(a) * 18.0f);
            hexInner[i] = ImVec2(logoCenter.x + std::cos(a) * 15.0f, logoCenter.y + std::sin(a) * 15.0f);
        }
        draw->AddPolyline(hexOuter, 6, accent, ImDrawFlags_Closed, 1.5f);
        draw->AddPolyline(hexInner, 6, IM_COL32(255, 255, 255, 26), ImDrawFlags_Closed, 1.0f);
    }
    {
        const char* mark = "E";
        const ImVec2 markSz = fontTitle->CalcTextSizeA(15.0f, FLT_MAX, 0.0f, mark);
        draw->AddText(
            fontTitle,
            15.0f,
            ImVec2(logoCenter.x - markSz.x * 0.5f, logoCenter.y - markSz.y * 0.5f),
            IM_COL32(246, 242, 255, 255),
            mark
        );
    }

    {
        const char* titleStr = "ETHNIR NOIR";
        const float titleSize = 21.0f;
        const ImVec2 titleSz = fontTitle->CalcTextSizeA(titleSize, FLT_MAX, 0.0f, titleStr);
        const ImVec2 titlePos(cx - titleSz.x * 0.5f, pos.y + 76.0f);
        draw->AddText(fontTitle, titleSize, titlePos + ImVec2(0.0f, 1.2f), IM_COL32(0, 0, 0, 170), titleStr);
        draw->AddText(fontTitle, titleSize, titlePos, white, titleStr);

        for (int i = 0; i < 3; ++i) {
            draw->AddLine(
                ImVec2(cx - 12.0f + i * 6.0f, titlePos.y + titleSz.y + 7.0f),
                ImVec2(cx - 6.0f + i * 6.0f, titlePos.y + titleSz.y + 2.0f),
                accent,
                1.8f
            );
        }

        const char* sub = "SYSTEM START   |   VERIFY   |   LOAD";
        const float subSize = fontBody ? fontBody->FontSize * 0.70f : 10.0f;
        const ImVec2 subSz = fontBody->CalcTextSizeA(subSize, FLT_MAX, 0.0f, sub);
        draw->AddText(fontBody, subSize, ImVec2(cx - subSz.x * 0.5f, pos.y + 112.0f), textMuted, sub);

        const float dividerY = pos.y + 134.0f;
        draw->AddLine(ImVec2(pos.x + 64.0f, dividerY), ImVec2(cx - 18.0f, dividerY), IM_COL32(255, 255, 255, 26), 1.0f);
        draw->AddLine(ImVec2(cx + 18.0f, dividerY), ImVec2(cardMax.x - 64.0f, dividerY), IM_COL32(255, 255, 255, 26), 1.0f);
        draw->AddCircleFilled(ImVec2(pos.x + 64.0f, dividerY), 2.0f, accent, 12);
        draw->AddCircleFilled(ImVec2(cardMax.x - 64.0f, dividerY), 2.0f, accent, 12);
    }

    {
        const float PI2 = 6.28318530f;
        const float cy = pos.y + 224.0f;
        const float baseR = 84.0f;

        draw->AddCircleFilled(ImVec2(cx, cy), baseR * 1.06f, IM_COL32(15, 12, 22, 220), 64);

        const float ringRadii[] = { baseR * 0.32f, baseR * 0.56f, baseR * 0.80f, baseR * 1.02f };
        const float ringSpeeds[] = { 1.6f, -1.15f, 0.85f, -0.55f };
        const int   ringDots[] = { 4, 7, 10, 13 };

        for (int r = 0; r < 4; ++r) {
            const float rr = ringRadii[r];
            draw->AddCircle(
                ImVec2(cx, cy),
                rr,
                ImGui::GetColorU32(ImVec4(accentV.x, accentV.y, accentV.z, 0.14f + 0.05f * pulseWave)),
                48,
                0.9f
            );

            const float arcStart = animationTime * ringSpeeds[r];
            const float arcLength = PI2 * (0.24f + 0.06f * r);
            const int segs = 20;
            for (int i = 0; i < segs; ++i) {
                const float t0 = (float)i / (float)segs;
                const float t1 = (float)(i + 1) / (float)segs;
                const float a0 = arcStart + arcLength * t0;
                const float a1 = arcStart + arcLength * t1;
                draw->AddLine(
                    ImVec2(cx + std::cos(a0) * rr, cy + std::sin(a0) * rr),
                    ImVec2(cx + std::cos(a1) * rr, cy + std::sin(a1) * rr),
                    ImGui::GetColorU32(ImVec4(accentV.x, accentV.y, accentV.z, 0.20f + 0.55f * t0)),
                    r == 3 ? 2.0f : 1.2f
                );
            }

            for (int d = 0; d < ringDots[r]; ++d) {
                const float angle = animationTime * ringSpeeds[r] * 0.75f + PI2 * d / ringDots[r];
                const float dx = cx + std::cos(angle) * rr;
                const float dy = cy + std::sin(angle) * rr;
                const float dotR = (d % 3 == 0) ? 2.6f : 1.8f;
                draw->AddCircleFilled(
                    ImVec2(dx, dy),
                    dotR,
                    ImGui::GetColorU32(ImVec4(accentV.x, accentV.y, accentV.z, (d % 3 == 0 ? 0.85f : 0.45f) * (0.75f + 0.25f * pulseWave))),
                    12
                );
            }
        }

        const float hexR = 13.0f;
        const float hexPositions[2][2] = {
            { cx - baseR * 1.55f, cy - baseR * 0.28f },
            { cx + baseR * 1.52f, cy + baseR * 0.34f }
        };
        for (int n = 0; n < 2; ++n) {
            const ImVec2 hc(hexPositions[n][0], hexPositions[n][1]);
            for (int h = 0; h < 6; ++h) {
                const float a0 = PI2 * h / 6.0f + animationTime * 0.08f;
                const float a1 = PI2 * (h + 1) / 6.0f + animationTime * 0.08f;
                draw->AddLine(
                    ImVec2(hc.x + std::cos(a0) * hexR, hc.y + std::sin(a0) * hexR),
                    ImVec2(hc.x + std::cos(a1) * hexR, hc.y + std::sin(a1) * hexR),
                    IM_COL32(255, 255, 255, 26),
                    1.0f
                );
            }
            draw->AddCircleFilled(hc, 2.2f, accentSoft, 12);
        }

        for (int g = 4; g >= 0; --g) {
            const float gr = baseR * 0.10f + g * 4.5f;
            const float ga = (0.07f - g * 0.012f) * (0.8f + pulseWave * 0.2f);
            draw->AddCircleFilled(ImVec2(cx, cy), gr, ImGui::GetColorU32(ImVec4(accentV.x, accentV.y, accentV.z, ga)), 32);
        }
        draw->AddCircleFilled(ImVec2(cx, cy), baseR * 0.075f, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.92f + 0.08f * pulseWave)), 24);
        draw->AddCircle(ImVec2(cx, cy), baseR * 0.15f, accentSoft, 32, 1.2f);
    }

    {
        const float stepY = pos.y + 340.0f;
        const float iconR = 18.0f;
        const float stepGapX = 92.0f;
        const float startX = cx - stepGapX * 1.5f;
        const float labelSize = fontBody ? fontBody->FontSize * 0.62f : 9.5f;

        for (int i = 0; i < k_StepCount; ++i) {
            const int state = StepState(i, progress);
            const float sx = startX + i * stepGapX;

            if (i < k_StepCount - 1) {
                const float lx0 = sx + iconR + 7.0f;
                const float lx1 = sx + stepGapX - iconR - 7.0f;
                draw->AddLine(ImVec2(lx0, stepY), ImVec2(lx1, stepY), IM_COL32(255, 255, 255, 20), 1.4f);
                if (state == 2) {
                    draw->AddLine(ImVec2(lx0, stepY), ImVec2(lx1, stepY), accent, 1.4f);
                } else if (state == 1) {
                    const float midX = lx0 + (lx1 - lx0) * EaseInOutSine((progress - i * (1.0f / k_StepCount)) * k_StepCount);
                    draw->AddLine(ImVec2(lx0, stepY), ImVec2(midX, stepY), accentSoft, 1.4f);
                }
            }

            if (state == 2) {
                draw->AddCircleFilled(ImVec2(sx, stepY), iconR, accent, 32);
                const ImVec2 checkA(sx - 6.0f, stepY);
                const ImVec2 checkB(sx - 2.0f, stepY + 4.5f);
                const ImVec2 checkC(sx + 6.5f, stepY - 4.5f);
                draw->AddLine(checkA, checkB, IM_COL32(255, 255, 255, 245), 2.2f);
                draw->AddLine(checkB, checkC, IM_COL32(255, 255, 255, 245), 2.2f);
            } else if (state == 1) {
                draw->AddCircleFilled(ImVec2(sx, stepY), iconR, IM_COL32(24, 17, 34, 245), 32);
                draw->AddCircleFilled(ImVec2(sx, stepY), iconR * 0.72f, ImGui::GetColorU32(ImVec4(accentV.x, accentV.y, accentV.z, 0.35f)), 32);
                draw->AddCircle(ImVec2(sx, stepY), iconR, accent, 32, 1.8f);
                draw->AddCircle(
                    ImVec2(sx, stepY),
                    iconR + 4.0f + pulseWave * 2.0f,
                    ImGui::GetColorU32(ImVec4(accentV.x, accentV.y, accentV.z, 0.16f * (0.5f + pulseWave * 0.5f))),
                    32,
                    3.0f
                );
            } else {
                draw->AddCircleFilled(ImVec2(sx, stepY), iconR, IM_COL32(19, 16, 26, 240), 32);
                draw->AddCircle(ImVec2(sx, stepY), iconR, IM_COL32(255, 255, 255, 22), 32, 1.2f);
            }

            const ImVec2 labelSz = fontBody->CalcTextSizeA(labelSize, FLT_MAX, 0.0f, k_Steps[i].label);
            draw->AddText(
                fontBody,
                labelSize,
                ImVec2(sx - labelSz.x * 0.5f, stepY + iconR + 7.0f),
                state == 0 ? IM_COL32(120, 116, 136, 210) : (state == 2 ? white : IM_COL32(214, 208, 232, 240)),
                k_Steps[i].label
            );
        }
    }

    {
        const float barY = pos.y + 396.0f;
        const float barH = 10.0f;
        const float barX = pos.x + 38.0f;
        const float pctW = 46.0f;
        const float barW = W - 76.0f - pctW;
        const float filled = barW * EaseOutCubic(progress);

        draw->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + barW, barY + barH), IM_COL32(255, 255, 255, 18), barH * 0.5f);
        if (filled > 0.5f) {
            draw->AddRectFilledMultiColor(
                ImVec2(barX, barY),
                ImVec2(barX + filled, barY + barH),
                ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.92f)),
                accent,
                accent,
                ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.92f))
            );
            draw->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + filled, barY + barH), ImGui::GetColorU32(ImVec4(accentV.x, accentV.y, accentV.z, 0.35f)), barH * 0.5f);
            draw->AddCircleFilled(ImVec2(barX + filled - barH * 0.5f, barY + barH * 0.5f), barH * 0.55f, IM_COL32(255, 255, 255, 230), 16);
        }

        char pct[16] = {};
        std::snprintf(pct, sizeof(pct), "%d%%", (int)std::lround(progress * 100.0f));
        const float pctSize = fontBody ? fontBody->FontSize * 0.78f : 12.0f;
        const ImVec2 pctSz = fontBody->CalcTextSizeA(pctSize, FLT_MAX, 0.0f, pct);
        draw->AddText(fontBody, pctSize, ImVec2(barX + barW + pctW - pctSz.x, barY + barH * 0.5f - pctSz.y * 0.5f), white, pct);
    }

    {
        const float boxX = pos.x + 38.0f;
        const float boxW = W - 76.0f;
        const float boxY = pos.y + 420.0f;
        const float boxH = 46.0f;
        const ImVec2 bMin(boxX, boxY);
        const ImVec2 bMax(boxX + boxW, boxY + boxH);

        draw->AddRectFilled(bMin, bMax, IM_COL32(255, 255, 255, 8), 16.0f);
        draw->AddRect(bMin, bMax, IM_COL32(255, 255, 255, 24), 16.0f, 0, 1.0f);

        const ImVec2 infoCenter(bMin.x + 26.0f, bMin.y + boxH * 0.5f);
        draw->AddCircleFilled(infoCenter, 11.0f, ImGui::GetColorU32(ImVec4(accentV.x, accentV.y, accentV.z, 0.30f)), 24);
        draw->AddCircle(infoCenter, 11.0f, accentSoft, 24, 1.0f);
        draw->AddCircleFilled(ImVec2(infoCenter.x, infoCenter.y - 3.0f), 1.5f, accent, 10);
        draw->AddRectFilled(ImVec2(infoCenter.x - 1.2f, infoCenter.y - 0.5f), ImVec2(infoCenter.x + 1.2f, infoCenter.y + 5.0f), accent, 1.0f);

        const float titleSize = fontBody ? fontBody->FontSize * 0.76f : 11.0f;
        const float subSize = fontBody ? fontBody->FontSize * 0.62f : 9.5f;
        draw->AddText(fontBody, titleSize, ImVec2(infoCenter.x + 22.0f, bMin.y + 8.0f), white, "Please wait");
        draw->AddText(fontBody, subSize, ImVec2(infoCenter.x + 22.0f, bMin.y + 8.0f + titleSize + 3.0f), textMuted,
            "This may take a few seconds. Do not close the application.");
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

