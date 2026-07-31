FBO / RTT / MRT formal regression tests
======================================

Suite file: `testing/test_fbo.cpp`  
Registered in: `testing/run_tests.cpp`  
Harness: 640×640 default back buffer PNG vs `expected_output/`.

Related implementation notes: `scratch/ai_notes/render_to_texture.md` (Phases A–D).

### RGB565 formal goldens

`fbo_*_RGB565.png` goldens exist for the RGB565 window `pix_t` build. Treat them as **regression / change detectors**, not as proof that offscreen color is 565. FBO color attachments still use RGBA8/`Color` or float storage; see design doc §17.

---

## Constraints (same as the rest of the suite)

- Each test is `void name(int num, char** argv, void* data)` where `num` is `test_suite[].num`.
- Only the **default framebuffer** (`bbufpix`) is captured. Offscreen work must be sampled/presented into it.
- Prefer large solid regions so RGB565 / edge fill-rule noise does not flake.

---

## Test list

| Suite name | Function | `num` | Depth build? |
|------------|----------|-------|----------------|
| `fbo_default_untouched` | `test_fbo_default_safe` | 0 | always |
| `fbo_color_sample` | `test_fbo_color` | 0 | always |
| `fbo_y_origin` | `test_fbo_y_origin` | 0 | always |
| `fbo_y_origin_texbb` | `test_fbo_y_origin` | 1 | always |
| `fbo_depth` | `test_fbo_depth` | 0 | `#ifndef PGL_NO_DEPTH_NO_STENCIL` |
| `fbo_mrt_split` | `test_fbo_mrt` | 0 | always |
| `fbo_mrt_single_buffer` | `test_fbo_mrt` | 1 | always |

---

## 1. `fbo_default_untouched` — `test_fbo_default_safe(0)`

### What it covers

Save/restore of the window color buffer when binding a user FBO: FBO clear/draw must not clobber the default FB until you draw to it again.

### Setup

1. Clear default FB **solid blue**.
2. Create a small color texture + FBO; clear FBO **red**, draw **green** full-screen into FBO.
3. `glBindFramebuffer(0)`.
4. **Do not draw** to the default FB.

### Expected image

- **Solid blue** entire 640×640.

### Failure modes (visual)

| Bug | Image |
|-----|--------|
| Redirect never restored / shared pointer | Red, green, or garbage instead of blue |
| Accidental present of FBO | FBO content visible |

---

## 2. `fbo_color_sample` — `test_fbo_color(0)`

### What it covers

Phase B happy path: gen/bind FBO → color0 texture → clear + draw → bind 0 → sample texture onto the default FB.

**Not** a full-screen present: the composite is an **inset** textured quad (~60% of NDC) so the gray clear is visible around it.

### Setup

1. FBO color0 texture at full 640×640.
2. Bind FBO: clear **dark red** (`0.5, 0, 0`), draw a **green** triangle (center).
3. Bind 0: clear **medium gray** (`0.25`), draw **inset** textured quad (`PGL`-style UV with v=0 at bottom of RT).

### Expected image

| Region | Color |
|--------|--------|
| Border / outside inset quad | Medium gray |
| FBO background on the quad | Dark red |
| Triangle | Green |

### Failure modes (visual)

| Bug | Image |
|-----|--------|
| No composite | Solid gray only |
| FBO draw failed | Red inset, no green triangle |
| Sample/bind failed | Black/garbage inset |
| Full-screen by mistake | No gray frame (still useful but wrong test) |

---

## 3. `fbo_y_origin` — `test_fbo_y_origin(0)`  ★ Phase A

### What it covers

**RT `invert_y`**: lastrow writes and texture sample agree. This is the multipass dual-ripple class of bug, made geometric.

### Setup

1. FBO color texture full viewport.
2. FS colors by **`gl_FragCoord.y`** (y=0 = bottom of FB):
   - **Bottom half** → **red**
   - **Top half** → **blue**
3. Bind 0, clear dark gray, **full-screen** sample of the RT.

### Expected image

```text
+------------------+
|      BLUE        |  ← top of window
|      BLUE        |
+------------------+
|      RED         |
|      RED         |  ← bottom of window
+------------------+
```

(Equal horizontal halves.)

### Failure modes (visual)

| Bug | Image |
|-----|--------|
| Linear sample on lastrow RT | **Red on top, blue on bottom** (vertical flip) |
| Correct RT origin | **Blue top, red bottom** |
| No draw | Dark gray only |

---

## 4. `fbo_y_origin_texbb` — `test_fbo_y_origin(1)`

### What it covers

Same banding as (3), but the RT is written via **`pglSetTexBackBuffer`** instead of an FBO object, then the window back buffer is restored with `pglSetBackBuffer` and the texture is sampled.

### Expected image

- Same as **`fbo_y_origin`**: blue top, red bottom.

### Failure modes (visual)

- Same flip/gray failures; if only this path is wrong, FBO variant still passes.

---

## 5. `fbo_depth` — `test_fbo_depth(0)`

### What it covers

Color + **depth attachment**, depth test while FBO is bound, then present color to the default FB.

Registered only when depth is compiled in (`#ifndef PGL_NO_DEPTH_NO_STENCIL`).

### Setup

1. FBO: color tex + depth tex (RGBA storage aliased as Z, Phase B rules).
2. `GL_DEPTH_TEST`, clear color black + depth.
3. Geometry similar to `zbuf_test`: far red tri, near green tri, far blue tri.
4. Bind 0, clear dark gray, full-screen sample of the color attachment.

### Expected image

- Like **`zbuf_depthon`**: green wins in the near triangle region; red/blue far geometry only where not occluded; black FBO clear where nothing drew (then sampled onto gray).

### Failure modes (visual)

| Bug | Image |
|-----|--------|
| Depth attach ignored | Order-only / wrong winner (e.g. red over green) |
| Matches depth-off | Painter’s order artifacts |
| Present broken | Solid gray/black |

---

## 6. `fbo_mrt_split` — `test_fbo_mrt(0)`

### What it covers

Phase C: two color attachments, `glDrawBuffers(2)`, `gl_FragData[0/1]`, multi-target clear/draw, composite to default FB.

### Setup

1. FBO: `COLOR_ATTACHMENT0` + `COLOR_ATTACHMENT1`.
2. `glDrawBuffers({COLOR0, COLOR1})`, clear both.
3. Covering triangle; MRT FS: `FragData[0]=red`, `FragData[1]=green`.
4. Bind 0: composite FS — **left half** of the window samples center of tex0, **right half** samples center of tex1.

### Expected image

```text
+----------+----------+
|   RED    |  GREEN   |
|   RED    |  GREEN   |
+----------+----------+
```

Vertical split at mid-screen.

### Failure modes (visual)

| Bug | Image |
|-----|--------|
| Only color0 written | Left red, right black |
| Only color1 written | Left black, right green |
| MRT used `gl_FragColor` only | Both black/clear |
| Clear only one target | Dirty color on one half |

---

## 7. `fbo_mrt_single_buffer` — `test_fbo_mrt(1)`

### What it covers

With two attachments present, `glDrawBuffers(1, {COLOR0})` uses the **single-target `gl_FragColor` path** (`mrt_active == false`).

### Setup

1. Same two attachments as MRT.
2. Draw buffers: only COLOR0.
3. FS writes **cyan** via `gl_FragColor` (fullscreen).
4. Present tex0 full-screen to default FB.

### Expected image

- **Solid cyan** entire 640×640.

### Failure modes (visual)

| Bug | Image |
|-----|--------|
| Still MRT path reading empty `gl_FragData[0]` | Black |
| Wrong attachment presented | Black or other color |

---

## Priority (reference)

| Priority | Test | Why |
|----------|------|-----|
| P0 | `fbo_y_origin` | Phase A; wrong Y is the multipass killer |
| P0 | `fbo_color_sample` | Basic FBO → present |
| P0 | `fbo_mrt_split` | Phase C end-to-end |
| P1 | `fbo_depth` | Depth attach + test |
| P1 | `fbo_default_untouched` | Save/restore |
| P2 | `fbo_y_origin_texbb` | `pglSetTexBackBuffer` parity |
| P2 | `fbo_mrt_single_buffer` | MRT ↔ single-buffer switch |

---

## Regenerating goldens

```bash
cd testing

# Default ABGR32
make run_tests
./run_tests fbo_default_untouched fbo_color_sample fbo_y_origin fbo_y_origin_texbb fbo_depth fbo_mrt_split fbo_mrt_single_buffer
cp test_output/fbo_default_untouched.png test_output/fbo_color_sample.png \
   test_output/fbo_y_origin.png test_output/fbo_y_origin_texbb.png \
   test_output/fbo_depth.png test_output/fbo_mrt_split.png \
   test_output/fbo_mrt_single_buffer.png expected_output/

# RGB565
make run_tests_rgb565
./run_tests_rgb565 fbo_default_untouched fbo_color_sample fbo_y_origin fbo_y_origin_texbb fbo_depth fbo_mrt_split fbo_mrt_single_buffer
cp test_output/fbo_*_RGB565.png expected_output/
```
