import os
import re

def main():
    filepath = "c:/Games/D1s/main/multi.c"
    
    if not os.path.exists(filepath):
        print(f"Error: Target file not found at {filepath}")
        return

    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    print("Executing automated pure-space layout repairs on multi.c...")

    # 1. Structural Fix: Ensure multibot.h is included for robot identifiers
    if '#include "multibot.h"' not in content:
        content = content.replace(
            '#include "multi.h"',
            '#include "multi.h"\n#include "multibot.h"'
        )
        print(" -> Injected missing robot definitions header (multibot.h)")

    # 2. Compilation Fix: Inject function prototypes to prevent implicit declaration build failures
    if 'void multi_do_obs_update(const ubyte *buf);' not in content:
        content = content.replace(
            'void multi_do_gmode_update(const ubyte *buf);',
            'void multi_do_gmode_update(const ubyte *buf);\nvoid multi_do_obs_update(const ubyte *buf);'
        )
        print(" -> Added explicit prototype declaration for multi_do_obs_update")

    # 3. Structural Fix: Map protocol layout references to direct fully-qualified addresses
    old_dump_pattern = r"Netgame\.players\[i\]\.addr"
    if re.search(old_dump_pattern, content):
        content = re.sub(old_dump_pattern, "Netgame.players[i].protocol.udp.addr", content)
        print(" -> Repaired structural protocol layout pathing to matching network properties (.protocol.udp.addr)")

    # 4. Pure Space Status & Linker Senders Code Block Definition
    status_injection = (
        "void multi_do_damage( const ubyte *buf )\n"
        "{\n"
        "    if (1) {\n"
        "        fix old_shields = Players[(int)buf[1]].shields;\n"
        "        fix new_shields = GET_INTEL_INT(buf + 6);\n"
        "        fix shields_delta = GET_INTEL_INT(buf + 2);\n"
        "        Players[(int)buf[1]].shields_certain = (new_shields <= 0 || Players[(int)buf[1]].shields - shields_delta == new_shields) ? 1 : 0;\n"
        "        Players[(int)buf[1]].shields = (new_shields > 0) ? new_shields : 0;\n"
        "        Players[(int)buf[1]].shields_delta -= shields_delta;\n"
        "    }\n"
        "}\n\n"
        "void multi_do_repair( const ubyte *buf )\n"
        "{\n"
        "    if (1) {\n"
        "        fix old_shields = Players[(int)buf[1]].shields;\n"
        "        fix new_shields = GET_INTEL_INT(buf + 6);\n"
        "        fix shields_delta = GET_INTEL_INT(buf + 2);\n"
        "        Players[(int)buf[1]].shields_certain = (Players[(int)buf[1]].shields + shields_delta == new_shields) ? 1 : 0;\n"
        "        Players[(int)buf[1]].shields = (new_shields > 0) ? new_shields : 0;\n"
        "    }\n"
        "}\n\n"
        "void multi_do_ship_status( const ubyte *buf )\n"
        "{\n"
        "    if (1) {\n"
        "        Players[(int)buf[1]].laser_level = buf[2];\n"
        "        Players[(int)buf[1]].flags = GET_INTEL_SHORT(buf + 3);\n"
        "        Players[(int)buf[1]].primary_ammo[1] = GET_INTEL_SHORT(buf + 5);\n"
        "        Players[(int)buf[1]].primary_weapon_flags = buf[7];\n"
        "        Players[(int)buf[1]].primary_weapon = (sbyte)buf[8];\n"
        "        Players[(int)buf[1]].secondary_ammo[0] = GET_INTEL_SHORT(buf + 9);\n"
        "        Players[(int)buf[1]].secondary_ammo[1] = GET_INTEL_SHORT(buf + 11);\n"
        "        Players[(int)buf[1]].secondary_ammo[2] = GET_INTEL_SHORT(buf + 13);\n"
        "        Players[(int)buf[1]].secondary_ammo[3] = GET_INTEL_SHORT(buf + 15);\n"
        "        Players[(int)buf[1]].secondary_ammo[4] = GET_INTEL_SHORT(buf + 17);\n"
        "        Players[(int)buf[1]].secondary_weapon_flags = buf[19];\n"
        "        Players[(int)buf[1]].secondary_weapon = (sbyte)buf[20];\n"
        "        Players[(int)buf[1]].energy = GET_INTEL_INT(buf + 21);\n"
        "        Players[(int)buf[1]].homing_object_dist = GET_INTEL_INT(buf + 25);\n"
        "    }\n"
        "}\n\n"
        "void multi_send_obs_update(ubyte event, ubyte event_data)\n"
        "{\n"
        "    if (!multi_i_am_master()) { return; }\n"
        "    multibuf[0] = MULTI_OBS_UPDATE;\n"
        "    multibuf[1] = event;\n"
        "    multibuf[2] = event_data;\n"
        "    multibuf[3] = Netgame.numobservers;\n"
        "    for(int i = 0; i < Netgame.max_numobservers; i++) {\n"
        "        if (Netgame.observers[i].connected == 1) {\n"
        "            memcpy(&multibuf[4 + i*8], &Netgame.observers[i].callsign, 8);\n"
        "        } else {\n"
        "            memset(&multibuf[4 + i*8], 0, 8);\n"
        "        }\n"
        "    }\n"
        "    multi_send_data( multibuf, 4 + 8*MAX_OBSERVERS, 2 );\n"
        "    if (event == 0 && !is_observer())\n"
        "    {\n"
        "        multi_do_request_status();\n"
        "    }\n"
        "}\n\n"
        "void multi_send_ship_status_for_frame()\n"
        "{\n"
        "    multibuf[0] = MULTI_SHIP_STATUS;\n"
        "    multibuf[1] = Player_num;\n"
        "    multibuf[2] = Players[Player_num].laser_level;\n"
        "    PUT_INTEL_SHORT(multibuf + 3, Players[Player_num].flags);\n"
        "    PUT_INTEL_SHORT(multibuf + 5, Players[Player_num].primary_ammo[1]);\n"
        "    multibuf[7] = Players[Player_num].primary_weapon_flags;\n"
        "    multibuf[8] = (ubyte)Players[Player_num].primary_weapon;\n"
        "    PUT_INTEL_SHORT(multibuf + 9, Players[Player_num].secondary_ammo[0]);\n"
        "    PUT_INTEL_SHORT(multibuf + 11, Players[Player_num].secondary_ammo[1]);\n"
        "    PUT_INTEL_SHORT(multibuf + 13, Players[Player_num].secondary_ammo[2]);\n"
        "    PUT_INTEL_SHORT(multibuf + 15, Players[Player_num].secondary_ammo[3]);\n"
        "    PUT_INTEL_SHORT(multibuf + 17, Players[Player_num].secondary_ammo[4]);\n"
        "    multibuf[19] = Players[Player_num].secondary_weapon_flags;\n"
        "    multibuf[20] = (ubyte)Players[Player_num].secondary_weapon;\n"
        "    PUT_INTEL_INT(multibuf + 21, Players[Player_num].energy);\n"
        "    PUT_INTEL_INT(multibuf + 25, Players[Player_num].homing_object_dist);\n"
        "    multi_send_data_direct( multibuf, 29, multi_who_is_master(), 2);\n"
        "}\n\n"
        "void multi_process_data"
    )

    # Clean array indexing expression matches
    legacy_functions_pattern = r"void\s+multi_do_damage\s*\([^)]*\)\s*\{.*?\}\s*void\s+multi_do_repair\s*\([^)]*\)\s*\{.*?\}\s*void\s+multi_do_ship_status\s*\([^)]*\)\s*\{.*?\}\s*void\s+multi_process_data"
    
    if re.search(legacy_functions_pattern, content, re.DOTALL):
        content = re.sub(legacy_functions_pattern, status_injection, content, flags=re.DOTALL)
        print(" -> Injected space-insulated packet handlers and missing sender definitions.")
    else:
        fallback_pattern = r"void\s+multi_do_damage.*?(?=void\s+multi_process_data)"
        if re.search(fallback_pattern, content, re.DOTALL):
            content = re.sub(fallback_pattern, status_injection.replace("\nvoid multi_process_data", ""), content, flags=re.DOTALL)
            print(" -> Applied fallback anchored space injection patch successfully.")
        else:
            print(" ! Warning: Could not locate the precise location of legacy handlers at the file bottom.")

    # Final sanity check: permanently erase any stray literal tab codes inside the file modification space
    content = content.replace("\t", "    ")

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(content)

    print("Fixes complete! Run your workspace Ninja build pipeline to verify compiling stability.")

if __name__ == "__main__":
    main()
