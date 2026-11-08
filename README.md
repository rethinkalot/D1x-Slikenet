# D1X-Redux SLikeNet: Update News

## Multiplayer

- **Faster, steadier netcode.** In-game traffic now runs over SLikeNet for ship positions and combat actions. Lobby, joining, discovery and observers share the same single port (42424 by default), so there is nothing extra to open.
- **Real delivery modes.** Late or out-of-order position packets are dropped and the newest wins. Important game data is delivered reliably and in order, and the rest stays unreliable. The acknowledgement counter is now 32-bit, so long games don't wrap it.
- **Up to 90 packets per second.** The Packets Per Second setting (10-90) now holds its target even when frames run late.
- **Up to 360 FPS.** Higher frame rates are supported, with smoothing so motion stays fluid between network updates.
- **Better NAT behavior.** Joiners connect out to lower-numbered players and the host. Observers connect to the host only. Each connection is set up with a token-checked handshake.
- **Link status on screen.** At the start of a match the HUD tells you, per player, "SLikeNet Peer Link Success" or "SLikeNet Peer Link Failure". A failure means that player's traffic is using plain UDP on the same port.
- **Clear failure if the port is busy.** SLikeNet retries the port 10 times, 50 ms apart. If it still can't bind, hosting or joining fails instead of quietly running without it.
- **Pausing is safe.** Nobody is dropped for going quiet. A host or player sitting in a menu, paused or inactive stays in the game for as long as they like. A player whose reliable-packet queue fills up without ever acknowledging can still be dropped.
- **Smoother ships.** Other players' ships are drawn between two real updates, with nothing guessed, about 1.5 packet intervals behind (16.7 ms at 90 PPS; co-op timing uses 60 PPS). They won't glide through walls or closed doors, and a ship crossing a segment border no longer snaps ahead.
- **Same feel as D2.** D1's ship smoothing, including the second pass after physics, send pacing and robot handling now match D2's code. The old experimental switches `-interp`, `-rawships`, `-smoothships`, `-sendeveryframe`, `-ease` and `-demoraw` are gone.
- **Robots in co-op.** Robots controlled by another player move only by network updates, sent 20 times per second, with no local drift, wobble or added delay. Robot AI also behaves more consistently between players.
- **Short Packets always on.** Observers need it to link over SLikeNet, so there is no menu item, and a saved game profile can't turn it off.
- **Anti-cheat.** Only the original Descent data files (checked by PIG hash) are allowed online. Unsupported files stop you hosting or joining.
- **Crash fix.** Leaving a game and rejoining manually no longer crashes.
- **Version bump.** Multiplayer protocol 30008. Older builds can't join games from this version.

## Running several copies on one PC

- **`proxy.cfg`.** Put a file named `proxy.cfg` next to the game with one line, `bind_ip=127.0.0.2`. That copy binds its game and broadcast sockets to that address instead of all addresses. An invalid address shows a red warning and falls back to all addresses.
- **One port for all.** Give each copy its own address (`127.0.0.2`, `127.0.0.3`, ...) and they can all use port 42424.
- **No background slowdown.** Windows 11 no longer slows down a game window sitting behind another one.

## Observers

- **Own controls.** Observers get their own custom controls.
- **Wall-aware camera.** The free-fly camera no longer passes through walls behind the ship.
- **Steady follow.** The camera follows the observed ship rigidly, with no ghosting or jitter.
- **Clean segment borders.** The first-person and free-view camera no longer jumps when crossing from one segment to the next.
- **Fading ships.** Ships that get close to the observer camera fade out so they don't block the view.
- **FPS counter.** The Graphics menu FPS Counter now works for observers and in observer demos.
- **Show Observer Messages.** A new Misc option, on by default, lets non-observers see or hide observer chat. The host relays observer chat to all players.
- **Observing host chat is blue too.** A host who is observing chats in the same all-blue style, name and text, and follows the same toggle.

## Demos

- **Recorded at your frame rate.** Demos store about one frame per rendered frame, set by your FPS cap (up to 360). They used to be fixed at 20 frames per second, which looked choppy on fast PCs. Higher rates make bigger files.
- **Played at your frame rate.** Playback runs in real time at whatever frame rate you are running, so speed is always correct, and blends between recorded frames.
- **Exact camera.** Demos store the camera's exact position and orientation, and ship smoothing runs while recording, so an observer demo plays back the way it looked live.
- **New demo format.** Demo version 14. Demos recorded by older builds no longer play.
- **Playback HUD.** `FPS` shows your live frame rate and `DEMO` shows the rate the demo was recorded at, so you can compare. The `DEMO` readout no longer jumps around during playback.
- **No seam at segment borders.** The playback view no longer pops when the camera crosses a segment.
- **Fixes.** The HUD no longer goes blank when fast-forwarding demos as an observer. Fast-forward and rewind steps scale with your FPS cap.

## HUD and options

- **Speedrun timers:** RTA covers gameplay and level transitions. IGT covers gameplay only and stays visible (frozen) over the pause screen. Loading a save or using cheats turns them white. Toggle them in the Misc menu. Timer issues are fixed.
- **"Suppress HUD Messages (SP)"** replaces "No redundant pickup messages". It hides all single-player HUD text except ammo/shield warnings.
- **HUD messages stay longer.** A single message shows for 4.5 seconds, and stacked messages 3 seconds each (up from 3 and 2). Repeats reset to 3 seconds.
- **Graphics toggles.** Turn custom graphics, textures and models on or off. The texture toggle now works correctly.
- **Flash toggle.** Reduced Flash is your own Graphics menu setting. Off is off everywhere. On in multiplayer also needs the host's Reduced Flash option.
- **Velocity meter.** New optional readout, toggled in the Misc menu. Velocity is recorded into demos whether or not the readout is shown.
- **Power-ups.** Fixed orientation and rotation for 3D models. Spin speeds are retuned and Quad Fire is tilted.

## BugFixes

- **No ship flash before respawn.** A dead player's ship no longer shows briefly at the spot they died just before they spawn.
- **No stray shots from the death spot.** Pressing fire to respawn no longer makes a shot appear where you died.
- **No flares while dead.** With Preview spawn, a dead player can no longer launch a flare from where they died.
- **Position updates go out sooner.** Ship positions are sent right away instead of waiting up to 10 ms. Positions stuck in the send queue for over 100 ms are dropped instead of arriving late in a burst.
- **Network clock fix.** The network clock no longer loses up to one frame every 65 seconds.
- **Quieter `-debug` console.** The per-packet `Packet OUT` and `Packet IN` lines are off. The traffic summary lines stay.


