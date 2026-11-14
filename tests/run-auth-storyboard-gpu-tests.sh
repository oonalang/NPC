#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
output_dir=${1:-"${TMPDIR:-/tmp}/npc-auth-storyboard"}
header_dir=${EGL_INCLUDE_DIR:-/usr/include}
if [ ! -f "$header_dir/EGL/egl.h" ] && [ -d /workspace/glass-validation/include ]; then
    header_dir=/workspace/glass-validation/include
fi
test_binary=$(mktemp "${TMPDIR:-/tmp}/npc-auth-storyboard-tests.XXXXXX")
trap 'rm -f "$test_binary"' EXIT HUP INT TERM
mkdir -p "$output_dir"
sha256sum "$project_dir/tests/auth_storyboard_gpu_test.cpp" \
    "$project_dir/src/main/jni/System/UI/AuthenticateScene.h" \
    "$project_dir/src/main/jni/System/UI/AuthenticateFlow.h" \
    "$project_dir/src/main/jni/System/UI/AuthenticateButton.h" \
    "$project_dir/src/main/jni/System/UI/StartupState.h" \
    "$project_dir/src/main/jni/System/UI/StartupUI.h" \
    "$project_dir/src/main/jni/System/UI/StartupMedia.h" \
    "$project_dir/src/main/jni/ImGui/ClayGlass.h" > "$output_dir/tested-sources.sha256"
"${CXX:-c++}" -std=c++17 -O0 -g -DWL_EGL_PLATFORM -DIMGUI_IMPL_OPENGL_ES3 \
    -ffunction-sections -fdata-sections -I"$header_dir" -I"$project_dir/src/main/jni" \
    "$project_dir/tests/auth_storyboard_gpu_test.cpp" \
    "$project_dir/src/main/jni/ImGui/imgui.cpp" \
    "$project_dir/src/main/jni/ImGui/imgui_draw.cpp" \
    "$project_dir/src/main/jni/ImGui/imgui_widgets.cpp" \
    "$project_dir/src/main/jni/ImGui/imgui_tables.cpp" \
    "$project_dir/src/main/jni/ImGui/imgui_impl_opengl3.cpp" \
    "$project_dir/src/main/jni/ImGui/stb_image.cpp" \
    -Wl,--gc-sections,-l:libEGL.so.1,-l:libGL.so.1 -o "$test_binary"
MESA_SHADER_CACHE_DISABLE=true "$test_binary" "$output_dir" "${AUTH_GPU_MODE:-full}"
sha256sum -c "$output_dir/tested-sources.sha256"
if [ "${AUTH_GPU_MODE:-full}" = --probes-only ]; then
    exit 0
fi
"${PYTHON:-python3}" - "$output_dir" "${AUTH_REFERENCE_GIF:-}" <<'PY'
from pathlib import Path
from PIL import Image, ImageDraw, ImageOps
import csv
import json
import sys

output = Path(sys.argv[1])
for source in output.glob('*.ppm'):
    Image.open(source).save(source.with_suffix('.png'))
    source.unlink()
with (output / 'frames.csv').open(newline='') as stream:
    rows = list(csv.DictReader(stream))
for row in rows:
    row['file'] = str(Path(row['file']).with_suffix('.png'))
with (output / 'frames.csv').open('w', newline='') as stream:
    writer = csv.DictWriter(stream, ['requested_ms', 'actual_ms', 'stage', 'file'])
    writer.writeheader()
    writer.writerows(rows)
thumb_width, thumb_height, caption_height, columns = 320, 180, 28, 3
sheet = Image.new('RGB', (columns * thumb_width, ((len(rows) + columns - 1) // columns) * (thumb_height + caption_height)), '#101018')
draw = ImageDraw.Draw(sheet)
for index, row in enumerate(rows):
    x = index % columns * thumb_width
    y = index // columns * (thumb_height + caption_height)
    draw.text((x + 6, y + 7), f"{row['actual_ms']} ms | {row['stage']}", fill='#eee5ff')
    image = Image.open(output / row['file']).convert('RGB').resize((thumb_width, thumb_height), Image.Resampling.LANCZOS)
    sheet.paste(image, (x, y + caption_height))
sheet.save(output / 'storyboard-captures.png')
config = json.loads((output / 'sequence.json').read_text())
movie_frames = []
frame_bytes = config['width'] * config['height'] * 3
with (output / 'sequence.rgb').open('rb') as stream:
    for index in range(config['frames']):
        data = stream.read(frame_bytes)
        if len(data) != frame_bytes:
            raise RuntimeError('Incomplete native frame stream')
        movie_frames.append(Image.frombytes('RGB', (config['width'], config['height']), data))
delays = [round((i + 1) * 100 / config['fps']) * 10 - round(i * 100 / config['fps']) * 10 for i in range(config['frames'])]
movie_frames[0].save(output / 'authenticate-flow.gif', save_all=True, append_images=movie_frames[1:], duration=delays, loop=0, disposal=2, optimize=False)
(output / 'sequence.rgb').unlink()
explosion = Image.open(output / 'explosion-220.png')
baseline = Image.open(output / 'E5-calm-baseline.png')
explosion.save(output / 'E1-irregular-front.png')
w, h = explosion.size
explosion.crop((w // 2 - 150, h // 2 - 150, w // 2 + 150, h // 2 + 150)).save(output / 'E3-transparent-interior.png')
def light(pixel):
    return pixel[0] * .2126 + pixel[1] * .7152 + pixel[2] * .0722
front_x = max(range(w // 2 - h // 2, w // 2 - 20), key=lambda x: light(explosion.getpixel((x, h // 2))) - light(baseline.getpixel((x, h // 2))))
explosion.crop((front_x - 80, h // 2 - 90, front_x + 80, h // 2 + 90)).save(output / 'E2-checker-bending.png')
explosion.crop((front_x - 60, h // 2 - 85, front_x + 60, h // 2 + 85)).save(output / 'E4-rgb-fringe.png')
reference_path = Path(sys.argv[2]) if sys.argv[2] else None
if reference_path and reference_path.is_file():
    reference = Image.open(reference_path)
    pairs = [(0, '000000'), (3, '000450'), (5, '001450'), (8, '002750'), (10, '002983'), (11, '003383'), (12, '004000'), (13, '004383'), (14, '004750'), (15, '005500'), (16, '005800'), (17, '006033'), (18, '006433'), (19, '006700'), (20, '007150'), (23, '008600'), (24, '008850')]
    reference_width, reference_height = reference.size
    native_width = round(reference_height * 960 / 540)
    comparison = Image.new('RGB', (reference_width + native_width, (reference_height + 30) * len(pairs)), '#101018')
    comparison_draw = ImageDraw.Draw(comparison)
    for index, (reference_index, native_prefix) in enumerate(pairs):
        candidates = sorted(output.glob(native_prefix + '-*.png'))
        if not candidates:
            raise RuntimeError('Missing comparison capture ' + native_prefix)
        reference.seek(reference_index)
        native = Image.open(candidates[0]).convert('RGB').resize((native_width, reference_height), Image.Resampling.LANCZOS)
        y = index * (reference_height + 30)
        comparison_draw.text((6, y + 8), f'GIF frame {reference_index} | {reference_index * 280} ms', fill='#eee5ff')
        comparison_draw.text((reference_width + 6, y + 8), f'Native {native_prefix} ms | semantic phase match', fill='#eee5ff')
        comparison.paste(reference.convert('RGB'), (0, y + 30))
        comparison.paste(native, (reference_width, y + 30))
    comparison.save(output / 'reference-side-by-side.png')
    reference_frames = []
    reference_starts = []
    source_time = 0
    for index in range(reference.n_frames):
        reference.seek(index)
        reference_frames.append(reference.convert('RGB').copy())
        reference_starts.append(source_time)
        source_time += reference.info.get('duration', 0)
    phase_map = [(0, 0), (150, 280), (450, 840), (2450, 1960), (2700, 2240), (2800, 2520), (3150, 3080), (3600, 3360), (4400, 3920), (4750, 3920), (5250, 4200), (5950, 4760), (6100, 4760), (6250, 5040), (8050, 6160), (8400, 6160), (8850, 6720), (9600, 6720)]
    aligned_frames = []
    with (output / 'comparison-map.csv').open('w', newline='') as stream:
        writer = csv.writer(stream)
        writer.writerow(['native_ms', 'gif_index', 'gif_source_ms', 'alignment'])
        for index, native_frame in enumerate(movie_frames):
            native_time = index * 1000 / config['fps']
            reference_time = phase_map[-1][1]
            for start, end in zip(phase_map, phase_map[1:]):
                if start[0] <= native_time <= end[0]:
                    phase = (native_time - start[0]) / (end[0] - start[0])
                    reference_time = start[1] + phase * (end[1] - start[1])
                    break
            source_index = max(i for i, time in enumerate(reference_starts) if time <= reference_time)
            aligned = Image.new('RGB', (reference_width + native_width, reference_height + 30), '#101018')
            aligned_draw = ImageDraw.Draw(aligned)
            aligned_draw.text((6, 8), f'GIF frame {source_index}: phase aligned', fill='#eee5ff')
            aligned_draw.text((reference_width + 6, 8), f'Native storyboard: {native_time:.0f} ms', fill='#eee5ff')
            aligned.paste(reference_frames[source_index], (0, 30))
            aligned.paste(native_frame.resize((native_width, reference_height), Image.Resampling.LANCZOS), (reference_width, 30))
            aligned_frames.append(aligned)
            writer.writerow([round(native_time, 3), source_index, reference_starts[source_index], 'semantic phase; updated storyboard timing'])
    aligned_frames[0].save(output / 'reference-side-by-side.gif', save_all=True, append_images=aligned_frames[1:], duration=delays, loop=0, disposal=2, optimize=False)
print('Native screenshots, timestamp table, E1-E5 crops and full sequence GIF saved to', output)
PY
