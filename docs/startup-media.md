# Startup media

The logo and animated banner use separate assets from the anime's official site.
The brand icon stays static. The banner plays a different night scene; it is not
a crop of the logo or a camera pan over a still image.

| Role | Official source | Download |
| --- | --- | --- |
| Waguri brand portrait | [Character icons](https://kaoruhana-anime.com/special/icon/) | [Kaoruko icon](https://kaoruhana-anime.com/special/icon/dl/02_kaoruko.png) |
| Waguri banner | [Episode GIF gifts](https://kaoruhana-anime.com/special/gif/) | [Episode 1, GIF 5](https://kaoruhana-anime.com/assets/img/special/gif/1-5.gif) |

The source portrait is 400 by 400. The source GIF is 500 by 281, with 63 frames
at 40 ms each (2.52 seconds). The offline embedding tool resizes the portrait
to 160 by 160 and samples 24 banner frames, crops them for the wide slot, and
packs them into a JPEG atlas with filtering gutters. Playback preserves the
source loop duration using elapsed time. The unoptimized source GIF is not
embedded in the library.

These are official downloadable fan assets, not original artwork owned by this
project. Copyright remains with the source rights holders; an official download
does not establish a general redistribution or commercial-use license. Keep
these source credits and confirm the applicable rights for distribution.

The alternative researched was [this Gojo loop on Tenor](https://tenor.com/view/gojo-satoru-jujutsu-kaisen-purple-opening2-gif-19950478).
The selected implementation uses Waguri for both roles, with distinct imagery,
as requested.
