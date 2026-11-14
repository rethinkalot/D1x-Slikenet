# D1X-Redux SLikeNet: Update News

## Multiplayer

- **Faster, steadier netcode.** In-game traffic now runs over SLikeNet for ship positions and combat actions. Lobby, joining, discovery and observers share the same single port (42424 by default), so there is nothing extra to open.
- **Real delivery modes.** Late or out-of-order position packets are dropped and the newest wins. Important game data is delivered reliably and in order, and the rest stays unreliable. The acknowledgement counter is now 32-bit, so long games don't wrap it.
- **Up to 90 packets per second.** The Packets Per Second setting (10-90) now holds its target even when frames run late.
- **Up to 360 FPS.** Higher frame rates are supported, with smoothing so motion stays fluid between network updates.
- **Better NAT behavior.** Joiners connect out to lower-numbered players and the host. Observers connect to the host only. Each connection is set up with a token-checked handshake.
- **Link Status In Everyone's Chat**
Each player's SLikeNet link results show on their HUD and in every player's chat, starting with "SLikeNet Link Success: name" when a link comes up.
- **Link Failure Reasons**
"Link Failure:name reason" says why a link didn't come up: no dial, no reply, no hello, full or no lane. Traffic with that player uses plain UDP.
- **Link Drop Warning**
"Peer Link Dropped:name" shows when a working link goes down mid-game while that player is still in it.
- **Clear failure if the port is busy.** SLikeNet retries the port 10 times, 50 ms apart. If it still can't bind, hosting or joining fails instead of quietly running without it.
- **Pausing is safe.** Nobody is dropped for going quiet. A host or player sitting in a menu, paused or inactive stays in the game for as long as they like. A player whose reliable-packet queue fills up without ever acknowledging can still be dropped.
- **Smoother ships.** Other players' ships are drawn between two real updates, with nothing guessed, about 1.5 packet intervals behind (16.7 ms at 90 PPS; co-op timing uses 60 PPS). They won't glide through walls or closed doors, and a ship crossing a segment border no longer snaps ahead.
- **Same feel as D2.** D1's ship smoothing, including the second pass after physics, send pacing and robot handling now match D2's code. The old experimental switches `-interp`, `-rawships`, `-smoothships`, `-sendeveryframe`, `-ease` and `-demoraw` are gone.
- **Robots in co-op.** Robots controlled by another player move only by network updates, sent 20 times per second, with no local drift, wobble or added delay. Robot AI also behaves more consistently between players.
- **Short Packets always on.** Observers need it to link over SLikeNet, so there is no menu item, and a saved game profile can't turn it off.
- **Anti-cheat.** Only the original Descent data files (checked by PIG hash) are allowed online. Unsupported files stop you hosting or joining.
- **Crash fix.** Leaving a game and rejoining manually no longer crashes.
- **Version bump.** Multiplayer protocol 30006. Older builds can't join games from this version.
- **Rejoining Players Link Again**
A player who rejoins into a lower player slot gets a SLikeNet link to every player again, instead of staying on plain UDP with the higher-numbered ones.
- **Separate Game, Host And Join Ports**
-udp_gameport is the port the LAN game list searches on and Join Manually's game port, -udp_hostport is the port you host on, and -udp_myport is the port you join from. All default to 42424.
- **Join Manually Remembers Game Port**
A game port you type in Join Manually is saved and used again next time, including for the LAN game list.
- **Host Can Change Map**
The host can press Esc, pick Change Map, then a mission and starting level. Everyone goes straight to Prepare for Descent on the new map, with scores reset and the same game settings.
- **Menus Keep The Game Smooth**
In a multiplayer game the Esc menu and other screens no longer slow the game down, so the host keeps sending and relaying updates at full rate and observers see smooth movement.
- **Co-op Start Points**
In co-op every player starts on their own spot. Levels without co-op starts use their other player starts, so ships no longer stack on the first start.

## Running several copies on one PC

- **`proxy.cfg`.** Put a file named `proxy.cfg` next to the game with one line, `bind_ip=127.0.0.2`. That copy binds its game and broadcast sockets to that address instead of all addresses. An invalid address shows a red warning and falls back to all addresses.
- **One port for all.** Give each copy its own address (`127.0.0.2`, `127.0.0.3`, ...) and they can all use port 42424.
- **No background slowdown.** Windows 11 no longer slows down a game window sitting behind another one.
- **Link Failure Test Switch**
Start a copy with -slikenet_nolink and it refuses every SLikeNet link while still playing over plain UDP, so the other players see Link Failure with a reason. Failures can't happen between copies on one PC otherwise.

## Observers

- **Own controls.** Observers get their own custom controls.
- **Wall-aware camera.** The free-fly camera no longer passes through walls behind the ship.
- **Steady follow.** The camera follows the observed ship rigidly, with no ghosting or jitter.
- **Clean segment borders.** The first-person and free-view camera no longer jumps when crossing from one segment to the next.
- **Fading ships.** Ships that get close to the observer camera fade out so they don't block the view.
- **FPS counter.** The Graphics menu FPS Counter now works for observers and in observer demos.
- **Show Observer Messages.** A new Misc option, on by default, lets non-observers see or hide observer chat. The host relays observer chat to all players.
- **Observing host chat is blue too.** A host who is observing chats in the same all-blue style, name and text, and follows the same toggle.
- **Centered gun sounds.** Following a ship in first person, its guns sound centered, the same as your own.
- **Respawn sound in place.** The observed ship's respawn sound plays where it respawns, not from where it died.
- **Observer List Above FPS**
The OBSERVERS list and streaks sit above the FPS line in every view, live and in observer demo playback, instead of at the bottom right over the weapon rows.
- **Observer Ping**
Observers now get the host's ping table, so with F6 network stats and the kill list on, the kill list shows real pings instead of 0. In a normal game they are from the followed player's point of view; with Retro Protocol they are the host's ping to each player.

## Demos

- **Recorded at your frame rate.** Demos store about one frame per rendered frame, set by your FPS cap (up to 360). They used to be fixed at 20 frames per second, which looked choppy on fast PCs. Higher rates make bigger files.
- **Played at your frame rate.** Playback runs in real time at whatever frame rate you are running, so speed is always correct, and blends between recorded frames.
- **Exact camera.** Demos store the camera's exact position and orientation, and ship smoothing runs while recording, so an observer demo plays back the way it looked live.
- **New demo format.** Demo version 14. Demos recorded by older builds no longer play.
- **Playback HUD.** `FPS` shows your live frame rate and `DEMO` shows the rate the demo was recorded at, so you can compare. The `DEMO` readout no longer jumps around during playback and, like `FPS`, updates once a second.
- **No seam at segment borders.** The playback view no longer pops when the camera crosses a segment.
- **Fixes.** The HUD no longer goes blank when fast-forwarding demos as an observer. Fast-forward and rewind steps scale with your FPS cap.
- **Bomb Count In Demos**
Prox bombs picked up or dropped while another secondary is selected now show in playback, rewind included. Older demos still play.

## HUD and options

- **Speedrun timers:** RTA covers gameplay and level transitions. IGT covers gameplay only and stays visible (frozen) over the pause screen. Loading a save or using cheats turns them white. Toggle them in the Misc menu. Timer issues are fixed.
- **"Suppress HUD Messages (SP)"** replaces "No redundant pickup messages". It hides all single-player HUD text except ammo/shield warnings.
- **HUD messages stay longer.** A single message shows for 4.5 seconds, and stacked messages 3 seconds each (up from 3 and 2). Repeats reset to 3 seconds.
- **Graphics toggles.** Turn custom graphics, textures and models on or off. The texture toggle now works correctly.
- **Flash toggle.** Reduced Flash is your own Graphics menu setting. Off is off everywhere. On in multiplayer also needs the host's Reduced Flash option.
- **Velocity meter.** New optional readout, toggled in the Misc menu. Velocity is recorded into demos whether or not the readout is shown.
- **Power-ups.** Fixed orientation and rotation for 3D models. Spin speeds are retuned and Quad Fire is tilted.
- **3D Prox Bomb**
A live proximity bomb now draws with when custom models are on.
- **Fixed HUD Rows**
Full screen keeps FPS, DEMO, RTA, IGT and velocity stacked above the weapon rows, so nothing overlaps and nothing moves when you switch HUD modes. Multiplayer and multiplayer demo playback use the same layout, and Alt HUD #1 now shows the bomb count.

## BugFixes

- **No ship flash before respawn.** A dead player's ship no longer shows briefly at the spot they died just before they spawn.
- **No stray shots from the death spot.** Pressing fire to respawn no longer makes a shot appear where you died.
- **No flares while dead.** With Preview spawn, a dead player can no longer launch a flare from where they died.
- **Position updates go out sooner.** Ship positions are sent right away instead of waiting up to 10 ms. Positions stuck in the send queue for over 100 ms are dropped instead of arriving late in a burst.
- **Network clock fix.** The network clock no longer loses up to one frame every 65 seconds.
- **Quieter `-debug` console.** The per-packet `Packet OUT` and `Packet IN` lines are off. The traffic summary lines stay.
- **Velocity meter clear of FPS.** In the death camera's letterbox view the velocity meter sits in the bottom bar instead of on top of the FPS line.
- **HUD modes in demos.** Alt+F7 now cycles the HUD modes during demo playback too, and the demo F1 screen lists it.
- **Cruise survives respawn.** Die with cruise control on and you respawn with it still on, at the same speed.
- **Last Input Wins.** New Controls option under Sticky Rearview, off by default. With both opposite slide or bank directions held, the one pressed last moves the ship instead of the two cancelling out.
- **`frametime` toggles the FPS counter.** Type it in game or during demo playback. It is not a cheat: no cheat flag, and your score is kept.
- **Shield orb tilt.** The 3D shield orb now spins at a 44-degree tilt.
- **JinX Mode Options Keep Toggles**
Toggles changed with in Observer Options save with Esc.
- **Autoselect Once Switches When Dry**
With Autoselect only once on, picking up a missile while sitting on prox bombs with no missiles left switches to it again.
- **Cheat Codes Work Again**
Typed single-player cheat codes work again. The frametime check was eating every typed letter before the cheat check saw it.
- **Bad Packets Can't Corrupt Memory**
Network messages with an impossible player number are ignored instead of writing outside the game's player lists.
- **Failed Joins Clear Themselves**
A player who starts joining but never arrives (wrong level file, crash, closed game) is removed for everyone after 20 seconds, so they don't stay listed and the next player can join.
- **No Crash On Exit**
Closing the game no longer crashes in OpenGL, so the last gamelog lines are kept.
- **Right Names In The Memlog**
Closing the game window after leaving no longer writes a second memlog with your own name in place of the host's.
- **Robot Messages Checked**
Robot messages with an impossible robot type, gun or segment are ignored, an update for an unknown robot no longer moves a different one, and a full object list no longer breaks robot respawns.
- **Boss Death Roll For Everyone**
The boss spins, explodes and starts the countdown on every screen, observers included, not only for the player who controlled it.
- **No Crash On Rejoin**
Leaving a game and rejoining right away no longer crashes while the game waits for the host's settings.
- **Observer Stays Listed**
An observer stays on the HUD list, shown once, across map changes, gets every packet once, and can rejoin without "game is full".
- **Robot Drops Land Together**
Items a robot drops start out the same on every screen, so they land in the same place for everyone.
- **Observer Ignores Blasts**
Explosions no longer push or damage the observer's camera.
- **No Weapon Crash After Loading**
Loading a co-op save no longer crashes with "Secondary Weapon is not in order list". Everyone keeps the weapons they had selected.
- **Crashed Players Removed**
A player whose game crashed or was closed is removed for everyone about 10 to 15 seconds later, so observers can't watch them and their slot frees up.
- **Backing Out Of A Join**
A player who backs out of the waiting box while joining is taken out at once, so nobody keeps sending to them.
- **Observers See Joins**
Observers now see players who join or rejoin while they watch, and a game that switches from observing to playing no longer stays on the observer list.
