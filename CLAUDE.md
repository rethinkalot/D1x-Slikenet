# CLAUDE.md
- ALWAYS use Caveman skill.
- ALWAYS use Ponytail skill.

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

D1X-Redux SLikeNet: a Descent 1 source port (C, with C++ for the SLikeNet bridge and `xmodel/`) that is moving multiplayer onto SLikeNet (a RakNet fork). A sibling D2 port lives in `C:/Games/D2s` with its own `claude.md`. Multiplayer changes here get re-based there, so `D1S_MULTI_PLUGIN_LOG.md` records them for hand-off.

## Working rules (from RETHINK,)

- Plan, confirm, execute, build, then wait for the next task. Raise likely problems and choices before deciding.
- Make only the code changes that were asked for. No unrequested refactors or style cleanups.
- If an attempt fails or you get stuck, stop and ask how to proceed.
- Don't run git commands or scripts on your own initiative. Don't commit untested builds; RETHINK tests first.
- Ask before destructive commands, and don't treat one approval as standing permission. No `rm -rf`-style cleans; builds stay incremental.
- Don't install or reconfigure toolchains.
- Keep the network telemetry console layout exactly as it is, including wording, colors and `CON_NET_*` priorities, and style new network messages to match: orange `[TX]`, blue `[RX]`, green status, red for deallocated or failed. All traffic, on either lane, must feed the same `[TX]` / `[RX]` lines (`main/net_udp_socket.c`).
- Use targeted reads and greps. Many files here are very large (`main/net_udp.c` is over 5,000 lines).

## Build

Windows 11, MSYS2 MinGW64 at `C:/Programs/msys64`, Ninja. RETHINK's standard build-and-deploy command, from Git Bash or an MSYS2 shell:

    cd /c/Games/D1s && export PATH=/c/Programs/msys64/mingw64/bin:$PATH && cmake -G Ninja -B build && cmake --build build --target main/clean 2>/dev/null || rm -f build/main/d1x-redux.exe && cmake --build build -- -j10 && if [ -f build/main/d1x-redux.exe ]; then mkdir -p /c/Games/D1x /c/Games/D1x2; taskkill //F //IM D1x-Redux-SLikeNet.exe 2>/dev/null || tskill D1x-Redux-SLikeNet 2>/dev/null; sleep 1; rm -f /c/Games/D1x/D1x-Redux-SLikeNet.exe /c/Games/D1x2/D1x-Redux-SLikeNet.exe; cp -v build/main/d1x-redux.exe /c/Games/D1x/D1x-Redux-SLikeNet.exe; cp -v build/main/d1x-redux.exe /c/Games/D1x2/D1x-Redux-SLikeNet.exe; echo 'Confirm Complete'; else echo 'Build completed, but no expected executable was found.' >&2; exit 1; fi

Use it exactly as written; it is also the "Build D1x-Redux-SLikeNet" task in `.vscode/tasks.json`. It closes any running game before copying. Output is `build/main/d1x-redux.exe`. Apart from this command's own relink step, don't remove it from `build/main/`.

A build-only run, with no deploy, is still:

    cmake --build build -- -j10

`CMakePresets.json` also defines `windows-mingw-debug` and `windows-mingw-release`, which build into `build/<preset>/`.

CMake options are at the top of the root `CMakeLists.txt` (`OPENGL`, `UDP`, `TRACKER`, `EDITOR`, `DXX_USE_SDL12_COMPAT`, ...). The game uses the SDL 1.2 API; `-DSDL2=ON` is a hard error.

There are no project tests or lint targets (test files under `thirdparty/` belong to those libraries). Verification is a successful build plus in-game testing by RETHINK.

## Deploy and run

Testbenches are `C:/Games/D1x` and `C:/Games/D1x2`, two installs for testing multiplayer. After a successful build, copy to both:

    taskkill //F //IM D1x-Redux-SLikeNet.exe 2>/dev/null; sleep 1
    cp -v build/main/d1x-redux.exe /c/Games/D1x/D1x-Redux-SLikeNet.exe
    cp -v build/main/d1x-redux.exe /c/Games/D1x2/D1x-Redux-SLikeNet.exe

The `taskkill` closes any running game, so check before using it during a test session. Report "Confirm Complete" when the sequence finishes.

Run from the testbench: `./D1x-Redux-SLikeNet.exe -window -notitles -nostickykeys`. The executable is a GUI-subsystem app, so a console only appears with these flags:

- `-debug`: SLikeNet/network telemetry (`CON_NET_*` in `include/console.h`) and `CON_DEBUG`
- `-verbose`: event, input and window diagnostics (`CON_VERBOSE`)

The testbench folders also hold `gamelog.txt` and `memlog.txt`.

## Architecture

- `main/` builds the `d1x-redux` executable and holds nearly all game logic.
- `2d`, `3d`, `iff`, `maths`, `mem`, `misc`, `texmap` are static engine libraries. `arch/` has the platform layers (`sdl`, `ogl`, `win32`, `x11`, `cocoa`). `xmodel/` is the OpenGL model loader. `editor/` and `ui/` only build with `EDITOR=ON`.
- `thirdparty/` is vendored dependencies, built by default (`DXX_USE_BUNDLED_DEPS`). Treat it as upstream code. `vcpkg.json` only applies with bundled deps off.

### Multiplayer layers

1. `main/multi.c`, `main/multibot.c`: game-level messages and robot sync. Robot positions are `shortpos` (`MULTI_ROBOT_POSITION`). For a robot another player owns, `multi_do_robot_position()` zeroes its velocity, clears `PF_LEVELLING | PF_TURNROLL`, places it at the packet pose and rebuilds the orientation, and `do_ai_frame()` / `do_physics_sim()` return early for it, so only packets move it. `multi_blend_robot_positions()` (called from `do_ai_frame_all()`) only refreshes segments; there is no per-frame blend and no `quaternionpos` robot sync. Robots owned locally (`REMOTE_OWNER == Player_num`) are skipped.
2. `main/net_udp.c`, `main/net_udp_ui.c`: session, join, sync and tracker logic.
3. `main/net_udp_socket.c`: raw sockets and receive (`udp_receive_packet`).
4. `main/slikenet_bridge.cpp` / `.h`: the C-callable wrapper around `SLNet::RakPeerInterface`, linked to `slikenet_modern` (built in `thirdparty/CMakeLists.txt`) as a whole archive.

### SLikeNet migration is unfinished

Check these against the current code before relying on them:

- Two ports: plain UDP on the game port (42424) for discovery, lobby, tracker, join and sync; SLikeNet on game port + 1000 for in-game traffic (`SLIKENET_PORT_OFFSET` in `slikenet_bridge.cpp`). Hosts forward both. In `d1x.ini`, `-udp_hostport` is 42424 on both copies, and `-udp_myport` is 42424 for `D1x` (SLikeNet 43424) and 42425 for `D1x2` (SLikeNet 43425).
- The peer starts and stops with game socket 0 in `udp_open_socket` / `udp_close_socket`. If it can't bind, the game stays on plain UDP.
- Only 4 packet types ride SLikeNet; `dxx_sendto` in `net_udp_socket.c` picks the transport by packet ID (`udp_send_slikenet`), falling back to plain UDP when the peer isn't ready:
  - `UPID_PDATA` (position/velocity/orientation): sequenced, newest wins.
  - `UPID_OBJECT_DATA` (object sync for a joining player): reliable ordered, its own channel. Joiners connect to SLikeNet only once playing, so this normally falls back to plain UDP.
  - `UPID_MDATA_PNORM` (multi buffer, priority 0-1): unreliable, and a lost packet is not resent.
  - `UPID_MDATA_PNEEDACK` (multi buffer, priority 2, including weapon fire): reliable, unordered. The game's ACK layer still runs, and `net_udp_noloss_validate_mdata()` drops duplicates.
  Everything else — lobby/session state (`UPID_SYNC`, discovery, join, tracker), observers, `UPID_MDATA_ACK`, `UPID_PING`/`PONG`, `UPID_ENDLEVEL_*`, `UPID_P2P_PING`/`PONG`, `UPID_PROXY`/`UPID_REATTEMPT_DIRECT` — stays on native plain UDP; it's unaffected by whether the SLikeNet lane is connected.
- In-game, each player connects to lower-numbered players, then a token-checked hello binds the SLikeNet address to the player's address (`net_udp_slikenet_frame` in `net_udp.c`). Received packets are reported with that player address, so existing sender checks still apply.
- Not on SLikeNet: observers, and IPv6 builds (compiled out).

## Repo notes

- Work branch: `D1-SlikeNet-WorkBranch`. Remote: `rethinkalot/D1x-Slikenet`.
- `main/patch.py` and `main/patchd1.py` are one-off scripts that rewrite source files in place. Read one before running it, and only run it if asked.
- `.clinerules` is RETHINK's rule file for Cline and contains unresolved merge markers. Where its two sides differ, follow this file and ask if unsure.
