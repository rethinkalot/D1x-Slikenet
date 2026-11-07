# D1X-Redux SLikeNet: Changelog

Everything since the clean baseline (tag `D1-SlikeNet-clean-baseline`, 2026-10-10). Player-facing text lives in `UPDATE_NEWS.md`; this file is the full list for the repo.

## Multiplayer and networking

- SLikeNet carries in-game traffic (ship positions, combat actions) on the game port, alongside plain UDP. Both share one socket on port 42424 by default (`main/slikenet_bridge.cpp`, `main/net_udp_socket.c`).
- Datagram filter on the receive thread: SLikeNet takes its own packets and queues the game's plain datagrams (128 slots) for the game.
- Delivery modes: positions sequenced, object data reliable ordered, other game data unreliable or reliable as before. ACK sequence is 32-bit.
- Token-checked hello handshake. Joiners dial lower-numbered players and the host; observers dial the host only. Per-peer fallback to plain UDP until a peer finishes the handshake.
- No plain-UDP fallback if the lane cannot bind: 10 retries at 50 ms, then host or join fails.
- Packets Per Second (10-90) holds its target when frames run late. Frame rates up to 360 FPS.
- Anti-cheat: only original Descent data files are allowed online.
- HUD link status at match start: "SLikeNet Peer Link Success" or "SLikeNet Peer Link Failure" per player.
- Handshake diagnostics in the console: `SLikeNet Handshake OUT` on each connect attempt, `SLikeNet Handshake IN` every 5 s while handshakes arrive. Lane totals added to `memlog.txt`.
- The 15 s no-packet player timeout is disabled (`net_udp_timeout_check` returns early). The 15 s unacknowledged-packet kick in `net_udp_noloss_process_queue` is commented out for players and observers. A full reliable-packet queue still dumps a non-acknowledging player.
- Fixed a crash when leaving a game and rejoining manually.
- Multiplayer protocol version bumped; older builds cannot join.
- Removed the earlier "game port + 1000" lane.

## Running several copies on one PC

- `proxy.cfg` with `bind_ip=<ipv4>` binds that copy's game and broadcast sockets to one local address, so several copies can share port 42424.
- Windows 11 EcoQoS opt-out (`arch/sdl/timer.c`): a window behind another one is no longer slowed down.

## Ships and robots

- Remote ships are drawn between two real updates with no extrapolation, and cannot pass through walls or closed doors.
- Robot smoothing for co-op (`main/multibot.c`): record the last 4 poses, zero velocity on every pose packet, draw 1.25x the pose interval behind (62-125 ms), blend position and orientation, hold at the newest pose, reset on 0.75 s age, 1 s silence or an 80-unit jump.

## Observers

- Own custom controls.
- Wall-aware free-fly camera.
- Rigid third-person follow, no ghosting or jitter.
- Ships close to the observer camera fade out.
- Added Misc option "Show Observer Messages", a toggle for non-observers, under "No Player Chat Sound" (default on, saved as `showobsmessages`). The host relays observer chat to all players (`forward_obs_chat_to_players` in `main/net_udp.c`). No protocol change.
- Chat from a host that is observing shows all blue (name and text), the same blue as observer chat, and follows the same toggle.

## Demos

- Recorded at your frame rate (up to 360 FPS) instead of a fixed 20 FPS. Played back at your frame rate; older 20 FPS demos are smoothed.
- Playback HUD shows live `FPS` and the recorded `DEMO` rate.
- Afterburner sound plays in playback. HUD no longer blanks when fast-forwarding as an observer. Fast-forward and rewind steps scale with the FPS cap.
- Removed a leftover `ND FF: cockpit` debug line.

## HUD and options

- HUD messages stay 1.5x longer: single message 4.5 s (was 3 s), stacked 3 s each (was 2 s), repeats reset to 3 s (`main/hud.c`).
- Speedrun timers RTA and IGT, toggled in the Misc menu. Timer issues fixed.
- "Suppress HUD Messages (SP)" replaces "No redundant pickup messages".
- Graphics toggles for custom graphics, textures and models; the texture toggle works.
- Reduced flash is client-side.
- Optional velocity meter in the Misc menu.
- Power-ups: fixed 3D model orientation and rotation, retuned spin speeds, tilted Quad Fire.
- Console traffic lines: orange `[TX]`, blue `[RX]`, 5-second summary, PData line.

## Build and repository

- Release build with `-O3`; SLikeNet linked with `--whole-archive`.
- Removed stray root files (`_renum.ps1`, `.clinerules`, empty `new/`) and the workspace `skills/` copies; Ponytail and caveman now come from user-level plugins.
- Fixed a `const` warning in `multibot.c`.
