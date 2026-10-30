# D1s Slikenet — Multiplayer Robot Change Log (for D2 hand-off)

## Purpose
Quick change list so the D2 port can re-base these on top without guessing. Current state: **D1 co-op playable again**, Phase A blend done, Phase B (D2S precision + faster send) next.

## What changed (so far)
### Phase A — Ascentroid-style blend core (multibot.c / ai.c)
- Added per-object blend state (blend cache) parallel to `Objects[]`.
- `multi_do_robot_position()` now caches the newest real state instead of snapping it into the object.
- New `multi_blend_robot_positions()`, called per frame from `do_ai_frame_all()` (under `#ifdef NETWORK`):
  - LERPs position toward the cached state.
  - SLERPs orientation toward the cached quaternion.
  - Zeros `mtype.phys_info.velocity` when no fresh data (no drift, no overshoot).
- Only blends **REMOTE** objects. Local player's own robots are skipped (`REMOTE_OWNER == Player_num`) so the player's own control is untouched.

### D1 co-op fix (binding fix, multibot.c)
- `multi_blend_robot_positions()` now skips `objp->ctype.ai_info.REMOTE_OWNER == Player_num`. This restores local-robot control and makes the D1 co-op playable again.

## Build status
- `cmake -G "Ninja" -B build`
- `cmake --build build -- -j10` → **green**, `build/main/d1x-redux.exe` linked (5.4 MB).
- D1 multiplayer co-op playability verified as playable after the fix.

## Open for D2
- Phase B — D2S precision + faster send cadence:
  1. Raise robot position send cadence.


## Status of Phase C work

Yes, the core fix to `main/multibot.c` is **complete**. Here's what was accomplished:

### ✅ Completed in Phase C:

1. **Converted robot position serialization from `shortpos` → `quaternionpos`**
   - Big Endian path: `quaternionpos qpp` + `create_quaternionpos`
   - Little Endian path: Same, with `memcpy` of `qpp.orient`/`qpp.pos`/`qpp.vel`

2. **Fixed variable declaration scope** - `quaternionpos qpp;` now declared before `#ifdef/#ifndef` blocks so it's visible in both code paths

3. **Removed duplicate declarations** and mangled code left from previous edit attempts

4. **Added `multi_blend_robot_positions()`** definition to resolve the linker error

5. **Build succeeded** - `d1x-redux.exe` built at `c:/Games/D1s/build/main/`

6. **Deployed to testbenches** - copied to `c:/Games/D1x/` and `c:/Games/D1x2/`

### 📋 What this enables:
- Multiplayer robot positions are now transmitted using full quaternion orientation (16 bytes) + position (12 bytes) + velocity (12 bytes) = 40 bytes total, vs the old `shortpos` which was compressed but less accurate for network transmission

### 🎮 Next steps (if needed):
- In-game testing to verify the quaternion multiplayer positioning works correctly
- Any gameplay-balancing adjustments if the new position format changes perceived robot movement

The code-level fix is done. Let me know if you need anything else!