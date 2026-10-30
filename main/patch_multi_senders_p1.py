import os

def main():
    filepath = "c:/Games/D1s/main/multi.c"
    
    if not os.path.exists(filepath):
        print(f"Error: Target file not found at {filepath}")
        return

    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    print("Injecting Page 1 of missing network sender definitions...")

    # Define the first block of missing sender bodies using pure space indents
    senders_payload_p1 = (
        "\n\n// Redux Linking Sender Implementations - Page 1\n"
        "void multi_send_ship_status()\n"
        "{\n"
        "    if (is_observer()) { return; }\n"
        "    multi_send_ship_status_for_frame();\n"
        "}\n\n"
        "void multi_send_gmode_update()\n"
        "{\n"
        "    if (!multi_i_am_master()) { return; }\n"
        "    multibuf[0] = MULTI_GMODE_UPDATE;\n"
        "    multibuf[1] = Netgame.team_vector;\n"
        "    multibuf[2] = Bounty_target;\n"
        "    multi_send_data(multibuf, 3, 2);\n"
        "}\n\n"
        "void multi_do_gmode_update(const ubyte *buf)\n"
        "{\n"
        "    if (multi_i_am_master()) { return; }\n"
        "    Netgame.team_vector = buf[1];\n"
        "    Bounty_target = buf[2];\n"
        "}\n\n"
        "void multi_send_bounty()\n"
        "{\n"
        "    if (!multi_i_am_master()) { return; }\n"
        "    multibuf[0] = MULTI_DO_BOUNTY;\n"
        "    multibuf[1] = Bounty_target;\n"
        "    multi_send_data(multibuf, 2, 2);\n"
        "}\n\n"
        "void multi_do_bounty(const ubyte *buf)\n"
        "{\n"
        "    Bounty_target = buf[1];\n"
        "}\n\n"
        "void multi_new_bounty_target(int pnum)\n"
        "{\n"
        "    Bounty_target = pnum;\n"
        "    multi_send_gmode_update();\n"
        "}\n\n"
        "void multi_send_repair(fix repair, fix shields, ubyte sourcetype)\n"
        "{\n"
        "    if (is_observer()) { return; }\n"
        "    multibuf[0] = MULTI_REPAIR;\n"
        "    multibuf[1] = Player_num;\n"
        "    PUT_INTEL_INT(multibuf + 2, repair);\n"
        "    PUT_INTEL_INT(multibuf + 6, shields);\n"
        "    multi_send_data_direct(multibuf, 11, multi_who_is_master(), 2);\n"
        "}\n"
    )

    if "void multi_send_ship_status()" not in content:
        content += senders_payload_p1
        print(" -> Appended Page 1 link definitions successfully.")

    # Convert hidden tabs to pure spaces
    content = content.replace("\t", "    ")

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(content)

    print("Page 1 pipeline completed successfully.")

if __name__ == "__main__":
    main()
