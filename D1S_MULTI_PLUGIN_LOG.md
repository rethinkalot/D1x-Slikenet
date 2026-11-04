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

## SLikeNet transport (2026-11-02, for D2 hand-off)

Builds clean; not yet play-tested. `MULTI_PROTO_VERSION` is now 30008, so older builds can't join.

- **Ports:** plain UDP on 42424 (discovery, lobby, tracker, join, sync); SLikeNet on game port + 1000, so 43424. Hosts forward both.
- `main/slikenet_bridge.cpp` / `.h`: rewritten. Frames game packets behind `ID_USER_PACKET_ENUM` and maps SLikeNet addresses to player addresses.
- `main/net_udp_socket.c`: the peer starts and stops with socket 0. `dxx_sendto` sends exactly 4 packet types over SLikeNet when the peer is ready, otherwise plain UDP: `UPID_PDATA` (sequenced), `UPID_OBJECT_DATA` (reliable ordered, channel 1; joiners usually aren't connected yet, so this normally falls back to plain UDP), `UPID_MDATA_PNORM` (unreliable), `UPID_MDATA_PNEEDACK` (reliable, with the game's ACK layer still running).
- Same-PC testing: `-udp_hostport` is 42424 on both copies, and `-udp_myport` is 42424 and 42425. The + 1000 offset exists because game port + 1 collided on those adjacent ports (`NOT STARTED (error 5)`).
- Narrowed from an earlier pass that also routed `UPID_SYNC`, `UPID_OBSDATA`, `UPID_MDATA_ACK`, `UPID_PING`/`PONG` and `UPID_ENDLEVEL_*` over SLikeNet; those are back on plain UDP as "native DXX lobby state."
- `main/net_udp.c`: `net_udp_listen()` drains SLikeNet and runs the connect-and-hello handshake at the same cadence as `net_udp_send_pdata()` (`Netgame.PacketsPerSec`), not a separate hardcoded timer.
- Observers, tracker, proxy, P2P ping/pong and reattempt-direct all stay on plain UDP.
## Readout alignment with D2s (2026-11-03)

Builds clean; not yet play-tested. Console/log output only; no network behaviour changed.

- `main/net_udp_socket.c`: the per packet `[TX]` / `[RX]` lines now sample one packet per second instead of printing every packet (the counters still count every packet), so `-debug` doesn't cost a console write per packet. The 5 s `[TX]` / `[RX]` Traffic lines now divide by the elapsed time; before, they printed the 5 s totals labelled KB/s and PPS.
- New `PData x/y updates/s TX/RX (target N)` status line in the 5 s summary (`udp_record_pdata_tx()` / `udp_record_pdata_rx()`, called from `net_udp_send_pdata()` and `net_udp_process_pdata()`).
- `main/net_udp.c`: the debug footer now also prints "Player scores at leave", and a joining client now captures a footer too (before, only the host did). `memlog.txt` gets the same game/score header.
- Known, not fixed here: `net_udp_process_mdata` reads the 4-byte packet number as 16 bits (`GET_INTEL_SHORT(&data[6])`), so after 65,535 priority-2 packets in one session ACKs stop matching. Fixed in D2s (see D2S_MULTI_PLUGIN_LOG.md).

## Observer custom binds fix (2026-11-03)

- `main/gamecntl.c` `handle_observer_control_event`: the Previous / Next player branch was missing (lost when the Cycle camera toggle was rewritten), so the custom Next and Previous player binds did nothing and the function fell off the end without returning. Restored; it only acts while following a player. The stock Ctrl binds are unchanged.
- Found, not changed: `do_obs_menu` (`menu.c`) uses `if (i < 0) break;`, which exits on "Switch Game Mode" and "Share Settings" and discards edits on ESC (should be `i == -1`); `multi.c` `!PlayerCfg.ObsChat` lacks its `[mode]` index; the advanced netgame menu sets `ShortPackets` from the Difficulty slider (`net_udp_ui.c:273`).
