Mipmap / LOD formal regression tests
====================================

Suite file: `testing/test_mipmap.cpp`  
Registered in: `testing/run_tests.cpp`  
Harness: 640×640 default back buffer PNG vs `expected_output/`.

Standalone smoke programs (`test_mipmap_*.c`, etc.) remain useful for quick
edit/build loops and `PGL_CORE_PROFILE` / `PGL_UNSAFE` variants; the **suite**
is the convenient full-run path.

User-facing behavior: `src/header_docs.txt` (mip / Grad / `pgl_lod_*` / core).

---

## Suite inventory

| Suite name | Function | `num` | Golden |
|------------|----------|-------|--------|
| `mipmap_unit` | `test_mipmap_unit` | 0 | Solid green if logic OK, red if any unit check fails |
| `mipmap_auto_vis` | `test_mipmap_auto_vis` | 0 | Left large → L0 red; tiny center-right → L2 blue on gray |
| `mipmap_lod_bands` | `test_mipmap_lod_bands` | 0 | Vertical thirds Lod 0/1/2 → red \| green \| blue |
| `mipmap_grad_vis` | `test_mipmap_grad_vis` | 0 | Full-screen `texture2DGrad` high ρ → L2 blue |

```bash
cd testing
make -f run_tests.make   # or your premake config
./run_tests mipmap_unit mipmap_auto_vis mipmap_lod_bands mipmap_grad_vis
# or full suite
./run_tests
```

RGB565 goldens: `expected_output/mipmap_*_RGB565.png` (same four names with
`_RGB565` suffix). Refresh with:

```bash
./run_tests_rgb565 mipmap_unit mipmap_auto_vis mipmap_lod_bands mipmap_grad_vis
cp test_output/mipmap_*_RGB565.png expected_output/
```

**Release builds** of the test suite define `PGL_UNSAFE` (in addition to
`NDEBUG`) so PGL error logging and the name-0 unit checks are compiled out.
`NDEBUG` alone does **not** disable PGL_ERR.

---

## Constraints (same as the rest of the suite)

- Each test is `void name(int num, char** argv, void* data)` where `num` is
  `test_suite[].num`.
- Only the **default framebuffer** is captured.
- Prefer large solid regions so RGB565 / edge fill-rule noise does not flake.
- `mipmap_unit` encodes pass/fail as **full-frame clear color** so logic errors
  fail the PNG compare without a separate harness channel.

---

## Shared conventions under test

### λ (lod) vs mip level

- **λ / lod** = continuous scale; **level** = integer pyramid index.
- **λ ≤ 0** → base + **MAG_FILTER** (auto, Lod, Grad, cubemap).
- **λ > 0** → **MIN_FILTER** (mip pick + within-level NEAREST/LINEAR).

### Auto LOD (2B)

- `glDraw*` fill sets `c->mip_uv_per_px` from the **first two consecutive
  non-FLAT floats** in `vs_output`.
- Std tex-replace shader puts UV first (required for auto LOD).

### Default texture 0

- Typed samplers still accept 0.
- `textureSize` / `pgl_lod_*` reject 0 with `GL_INVALID_VALUE` (debug).

---

## 1. `mipmap_unit` — logic panel ★

### What it covers

Unit checks (same family as the old standalone Grad/Lod/helpers tests), then:

| Result | Image |
|--------|--------|
| All `MIP_EXPECT` pass | Solid **green** `(0, ~0.55, 0.1)` |
| Any fail | Solid **red** + messages on stderr |

Checks include:

- `pgl_lod_screen_wh` / `uv_scale` / `screen` / `grad` / `grad1D`
- `texture2DLod` / `texture2DGrad` / `texture1DGrad` level pick
- λ ≤ 0 mag vs λ > 0 minify (incl. trilinear lod 0 pure L0)
- Mag vs min on 2×2 multi-color (lod 0 pure NEAREST vs lod 0.4 LINEAR blend)
- Cubemap GenerateMipmap + `texture_cubemapLod` / `Grad`
- `textureSize` lod; name 0 → `INVALID_VALUE` (skipped under `PGL_UNSAFE` / Release suite builds)
- Contiguous chain size after `glGenerateMipmap` on 4×4

### Failure modes

| Bug | Image / log |
|-----|-------------|
| Any logic regression | **Solid red** + `mipmap FAIL: …` on stderr |
| Golden outdated after intentional green tweak | Diff on green shade only |

---

## 2. `mipmap_auto_vis` — per-triangle auto LOD

### What it covers

Per-triangle λ from UV/screen scale on the normal fill path.

### Setup

1. 8×8 chain: L0 red, L1 green, L2 blue, L3 white; `NEAREST_MIPMAP_NEAREST`.
2. Clear dark gray.
3. **Left half** large textured quad (UV 0–1 over half the FB) → λ < 0 → **L0 red**.
4. **Tiny** (~2 px) quad center-right → ρ ≈ 4 → λ ≈ 2 → **L2 blue**.

### Expected image

```text
+-------------+-------------+
|             |    gray     |
|    RED      |   [BLUE]    |  ← tiny ~2px blob near x=0.5 NDC
|   (large)   |    gray     |
+-------------+-------------+
```

### Failure modes

| Bug | Image |
|-----|--------|
| Auto LOD stuck at L0 | Tiny region red |
| Auto LOD too aggressive on large | Left not red |
| Tiny missing | Solid left red, right only gray |

---

## 3. `mipmap_lod_bands` — explicit `texture2DLod`

### What it covers

Explicit continuous lod → integer level with solid per-level colors.

### Setup

1. Same RGB mip chain.
2. Scissor three vertical strips; full-screen draw with FS `texture2DLod(..., lod)` for lod = 0, 1, 2.

### Expected image

```text
+----------+----------+----------+
|   RED    |  GREEN   |   BLUE   |
|  Lod 0   |  Lod 1   |  Lod 2   |
+----------+----------+----------+
```

### Failure modes

| Bug | Image |
|-----|--------|
| Lod ignored | All one color |
| Off-by-one level | Bands shifted (e.g. green\|blue\|white) |

---

## 4. `mipmap_grad_vis` — `texture2DGrad`

### What it covers

Isotropic λ from explicit derivatives in the FS (`dUdx = 0.5` → ρ = 4 → λ = 2 → L2).

### Expected image

- **Solid blue** full frame.

### Failure modes

| Bug | Image |
|-----|--------|
| Grad ignored / λ≤0 | Solid red (L0) |
| Wrong ρ | Green (L1) or white (L3) |

---

## Standalone programs (optional / special builds)

| Program | Why keep |
|---------|----------|
| `test_mipmap_phase1.c` | Extra layout edge cases; intentional `GL_INVALID_*` |
| `test_mipmap_lod.c` / `test_mipmap_auto.c` | Fast no-link-suite iteration |
| `test_mipmap_grad_lod.c` | Overlaps suite unit panel |
| `test_texsize_tex0.c` | Focused name-0 policy |
| `test_core_profile_mips.c` | Needs `-DPGL_CORE_PROFILE` (whole binary) |

Core incomplete-texture policy cannot live in default `run_tests` without a
separate suite binary; keep `test_core_profile_mips` standalone.

---

## Regenerating goldens

```bash
cd testing
./run_tests mipmap_unit mipmap_auto_vis mipmap_lod_bands mipmap_grad_vis
cp test_output/mipmap_unit.png test_output/mipmap_auto_vis.png \
   test_output/mipmap_lod_bands.png test_output/mipmap_grad_vis.png \
   expected_output/
```

After changing clear colors or layout, refresh goldens the same way.

---

## Mental checklist when touching mips

- [ ] Alloc not inside `PGL_ERR(...)` argument  
- [ ] λ ≤ 0 → MAG on auto / Lod / Grad / cube  
- [ ] Auto UV = first non-FLAT float pair only  
- [ ] `textureSize` / `pgl_lod_*` reject 0 in debug  
- [ ] Cube L0 = 6 faces; chain ×6  
- [ ] `./run_tests mipmap_unit mipmap_auto_vis mipmap_lod_bands mipmap_grad_vis`
