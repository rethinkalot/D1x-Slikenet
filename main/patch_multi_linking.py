import os

def main():
    filepath = "c:/Games/D1s/main/multi.c"
    
    if not os.path.exists(filepath):
        print(f"Error: Target file not found at {filepath}")
        return

    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    print("Targeting inclusions and missing linker symbols...")

    # 1. Structural Include: Ensure multibot.h is included for robot loops
    if '#include "multibot.h"' not in content:
        content = content.replace(
            '#include "multi.h"',
            '#include "multi.h"\n#include "multibot.h"'
        )
        print(" -> Verified robot definition headers")

    # 2. Prototype Declarations: Inject headers to satisfy the compiler
    if 'void multi_send_ship_status_for_frame();' not in content:
        content = content.replace(
            'void multi_send_gmode_update();',
            'void multi_send_gmode_update();\nvoid multi_send_ship_status_for_frame();'
        )
        print(" -> Injected linker prototype guards")

    # 3. Missing Linker Symbol Injection: Safe appending above the process dispatcher
    linker_payload = (
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

    if "void multi_send_ship_status_for_frame" not in content:
        content = content.replace("void multi_process_data", linker_payload)
        print(" -> Linked multi_send_ship_status_for_frame")

    # Final sanity check: force replace tabs with spaces across modified sectors
    content = content.replace("\t", "    ")

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(content)

    print("Repairs completed successfully!")

if __name__ == "__main__":
    main()
