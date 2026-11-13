#pragma once
namespace startup_ui { namespace media {
inline constexpr int artworkWidth=160,artworkHeight=160;
inline constexpr int bannerWidth=1296,bannerHeight=624;
inline constexpr int bannerFrameWidth=320,bannerFrameHeight=100,bannerGutter=2;
inline constexpr int bannerColumns=4,bannerFrameCount=24,bannerDurationMs=2520;
#ifndef CLAY_CPU_PREVIEW
inline constexpr unsigned char artwork[]{
#include "StartupMediaData/artwork-00.inc"
};
inline constexpr unsigned char bannerArtwork[]{
#include "StartupMediaData/bannerArtwork-00.inc"
#include "StartupMediaData/bannerArtwork-01.inc"
#include "StartupMediaData/bannerArtwork-02.inc"
#include "StartupMediaData/bannerArtwork-03.inc"
#include "StartupMediaData/bannerArtwork-04.inc"
};
#endif
}}
