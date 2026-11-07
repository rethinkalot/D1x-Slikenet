# D1X-Redux SLikeNet: Update News

## Multiplayer

- **Faster, steadier netcode.** In-game traffic now runs over SLikeNet for ship positions and combat actions. Lobby, joining, discovery and observers share the same single port (42424 by default), so there is nothing extra to open.
- **Up to 90 packets per second.** The Packets Per Second setting (10-90) now holds its target even when frames run late.
- **Up to 360 FPS.** Higher frame rates are supported, with smoothing so motion stays fluid between network updates.
- **Better NAT behavior.** Joiners connect out to the host. Observers connect to the host only.
- **Link status on screen.** At the start of a match the HUD tells you, per player, "SLikeNet Peer Link Success" or "SLikeNet Peer Link Failure". A failure means that player's traffic is using plain UDP on the same port.
- **Pausing is safe.** Nobody is dropped for going quiet. A host or player sitting in a menu, paused or inactive stays in the game for as long as they like.
- **Smoother ships.** Other players' ships are drawn between two real updates, with nothing guessed. They won't glide through walls or closed doors.
- **Smoother robots in co-op.** Robots controlled by another player move fluidly with no jitter. Robot AI also behaves more consistently between players.
- **Anti-cheat.** Only the original Descent data files are allowed online. Unsupported files stop you hosting or joining.
- **Crash fix.** Leaving a game and rejoining manually no longer crashes.
- **Version bump.** Older builds can't join games from this version.

## Running several copies on one PC

- **`proxy.cfg`.** Put a file named `proxy.cfg` next to the game with one line, `bind_ip=127.0.0.2`. That copy binds that address instead of all addresses.
- **One port for all.** Give each copy its own address (`127.0.0.2`, `127.0.0.3`, ...) and they can all use port 42424.
- **No background slowdown.** Windows 11 no longer slows down a game window sitting behind another one.

## Observers

- **Own controls.** Observers get their own custom controls.
- **Wall-aware camera.** The free-fly camera no longer passes through walls behind the ship.
- **Steady follow.** The camera follows the observed ship rigidly, with no ghosting or jitter.
- **Fading ships.** Ships that get close to the observer camera fade out so they don't block the view.

## Demos

- **Recorded at your frame rate.** Demos store about one frame per rendered frame, set by your FPS cap (up to 360). They used to be fixed at 20 frames per second, which looked choppy on fast PCs. Higher rates make bigger files.
- **Played at your frame rate.** Playback runs in real time at whatever frame rate you are running, so speed is always correct. Older 20 FPS demos are smoothed between frames.
- **Playback HUD.** `FPS` shows your live frame rate and `DEMO` shows the rate the demo was recorded at, so you can compare.
- **Fixes.** Demo playback now plays the afterburner sound. The HUD no longer goes blank when fast-forwarding demos as an observer. Fast-forward and rewind steps scale with your FPS cap.

## HUD and options

- **Speedrun timers:** RTA covers gameplay and level transitions. IGT covers gameplay only and stays visible (frozen) over the pause screen. Loading a save or using cheats turns them white. Toggle them in the Misc menu. Timer issues are fixed.
- **"Suppress HUD Messages (SP)"** replaces "No redundant pickup messages". It hides all single-player HUD text except ammo/shield warnings and Guide-Bot.
- **Graphics toggles.** Turn custom graphics, textures and models on or off. The texture toggle now works correctly.
- **HUD messages stay longer.** A single message shows for 4.5 seconds, and stacked messages 3 seconds each (up from 3 and 2).
- **Flash toggle.** Reduced flash is a client-side setting.
- **Original D1 weapons toggle** for single-player D2.
- **Velocity meter.** New optional readout, toggled in the Misc menu.
- **Power-ups.** Fixed orientation and rotation for 3D models. Spin speeds are retuned and Quad Fire is tilted.

## For testers

- **Console traffic.** Orange `[TX]` and blue `[RX]` lines, a 5-second summary and a PData line. The log ends with lane and channel totals.

---

# Developer Notes (not for publishing)

## Ports

- Gameplay uses one port. SLikeNet opens the socket on the game port (42424 by default) and the game's plain UDP shares it (`main/net_udp_socket.c`, `udp_open_socket`). SLikeNet's receive thread claims its own traffic and queues the game's plain datagrams for the game (`main/slikenet_bridge.cpp`).
- Other sockets: the tracker uses its own port (socket 2). Socket 1 opens only for a custom port, to keep 42424 open for LAN broadcast discovery.
- No plain-UDP fallback. If SLikeNet can't bind, it retries 10 times at 50 ms intervals, then host or join fails. Per-peer fallback remains: until a peer finishes the SLikeNet handshake, its packets go out as plain UDP on the same socket.
- The earlier "game port + 1000" lane was removed in commit `ba084aab`.

## Lane lifecycle (leave and rejoin)

- Every open path (host start, manual join, game list) goes through `udp_open_socket`, which closes the old socket first. That calls `slikenet_shutdown()`, and `slikenet_startup()` shuts down again before starting, so a stale lane cannot survive a reopen.
- Closing happens on a clean leave (`multi_leave_game` → `net_udp_leave_game` → `net_udp_close`), on cancelling a join or host setup screen, and on a failed wait for sync.
- Leaving waits at most 100 ms (`peer->Shutdown(100)`). The last test showed the host dropping the player in the same second.
- The no-packet timeout is disabled: `net_udp_timeout_check` returns early (`main/net_udp.c`), so the 15 s `UDP_TIMEOUT` no longer drops anyone. A player who vanishes without leaving (crash, unplugged) keeps their slot until the host ends the game. The 15 s unacknowledged-packet drop in `net_udp_noloss_process_queue` (players and observers) is commented out too: the packet is just discarded from the queue. One path remains: a full reliable-packet queue still dumps a player who never acknowledges, and a client then leaves with a "failed sending important packets (queue full)" message.
- **Weak spot:** if the port stays busy through all 10 retries, host start and join return silently. Only the console shows "NOT STARTED". Worth adding a message box.
- Not traced: being kicked, a host crash, and the end-of-level path.

## proxy.cfg

- One line, `bind_ip=<ipv4>`, in the working directory. It makes that copy bind its game and broadcast sockets to that one local address. The tracker socket is not affected. An invalid IP prints a red warning and falls back to all addresses.
- Not the same as the game's relay feature (`UPID_PROXY`, `CONNT_PROXY`), which sends packets through the host when two players can't reach each other directly. That code is untouched.

## Why SLikeNet is better than the old code

Design improvements read from the code, not measurements. The one local test lost 2 of about 2400 packets, which says nothing about the internet.

- **Sequenced positions.** A late or out-of-order position packet is dropped and the newest wins.
- **Real delivery modes.** Reliable packets are tracked by SLikeNet, with the game's ACK layer still on top. Non-critical ones stay unreliable. The ACK sequence is now 32-bit, so long games don't wrap it.
- **Handshake.** Connections are established by a token-checked hello, with a connect state per peer.
- **NAT.** Joiners dial out to lower-numbered players and the host. The bridge remembers each peer's real address even if NAT changes it.
- **One socket.** One port to forward, and no second port to collide with.
- **Diagnostics.** Per-lane totals and a PData rate line in the log and `memlog.txt`.

## How robot smoothing works

Code: `main/multibot.c`, lines 673-862.

**Old behavior.** Each robot-position packet nudged the robot toward the new pose and also fed an invented velocity into the physics. That overshot, the next packet pulled it back, and the robot vibrated.

**New behavior, per frame** (`multi_interp_update_robots`, after the network is read):

1. **Record.** `robot_interp_push` stores position, orientation and local arrival time for the last 4 poses of each robot, plus a smoothed gap between that robot's poses (3/4 old, 1/4 new).
2. **Zero velocity.** On every pose packet the robot's velocity is set to 0, so physics never moves a remote robot between packets.
3. **Delay.** Draw time is "now minus delay". Delay is 1.25x the robot's own pose interval, clamped to 62-125 ms. The default interval is 50 ms.
4. **Blend.** Find the two received poses on either side of the draw time. Position is a straight line between them. The facing and right vectors are blended and re-normalized.
5. **No extrapolation.** If the draw time is past the newest pose, the robot is held there.

**Limits:**

- **No new pose for 0.75 s:** the robot is left alone.
- **Silence over 1 s, time going backwards, or a jump over 80 units** (teleport or reused object slot): history restarts.
- **Skipped:** exploding robots and robots you own. Two poses read in one frame keep only the newer.

**Cost:** other players' robots show about 60-125 ms behind their real position. Same idea as the ship interpolation.


