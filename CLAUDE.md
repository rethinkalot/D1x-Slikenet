# CLAUDE.md
- ALWAYS use Caveman skill.
- ALWAYS use Ponytail skill.

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

D1X-Redux SLikeNet: a Descent 1 source port (C, with C++ for the SLikeNet bridge and `xmodel/`) that is moving multiplayer onto SLikeNet (a RakNet fork). A sibling D2 port lives in `C:/Games/D2s` with its own `CLAUDE.md`. 

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

## Deploy and run

Testbenches are `C:/Games/D1x` and `C:/Games/D1x2`, two installs for testing multiplayer. After a successful build, copy to both:

    taskkill //F //IM D1x-Redux-SLikeNet.exe 2>/dev/null; sleep 1
    cp -v build/main/d1x-redux.exe /c/Games/D1x/D1x-Redux-SLikeNet.exe
    cp -v build/main/d1x-redux.exe /c/Games/D1x2/D1x-Redux-SLikeNet.exe

The `taskkill` closes any running game, so check before using it during a test session. Report "Confirm Complete" when the sequence finishes.
