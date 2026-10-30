import os

def main():
    filepath = "c:/Games/D1s/main/multi.c"
    
    if not os.path.exists(filepath):
        print(f"Error: Target file not found at {filepath}")
        return

    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    print("Injecting Page 2 of missing network sender definitions...")

    # Define the second block of missing sender bodies using pure space indents
    senders_payload_p2 = (
        "\n\n// Redux Linking Sender Implementations - Page 2\n"
        "void multi_send_msgsend_state(int state)\n"
        "{\n"
        "    if (is_observer()) { return; }\n"
        "    multibuf[0] = MULTI_TYPING_STATE;\n"
        "    multibuf[1] = Player_num;\n"
        "    multibuf[2] = (char)state;\n"
        "    multi_send_data(multibuf, 3, 2);\n"
        "}\n\n"
        "void multi_do_msgsend_state(const ubyte *buf)\n"
        "{\n"
        "    int pnum = buf[1];\n"
        "    if (pnum >= 0 && pnum < MAX_PLAYERS) {\n"
        "        multi_sending_message[pnum] = buf[2];\n"
        "    }\n"
        "}\n\n"
        "void multi_initiate_save_game()\n"
        "{\n"
        "    // Stub implementation to satisfy the linker\n"
        "}\n\n"
        "void multi_initiate_restore_game()\n"
        "{\n"
        "    // Stub implementation to satisfy the linker\n"
        "}\n\n"
        "void multi_do_save_game(const ubyte *buf)\n"
        "{\n"
        "    (void)buf;\n"
        "}\n\n"
        "void multi_do_restore_game(const ubyte *buf)\n"
        "{\n"
        "    (void)buf;\n"
        "}\n\n"
        "void multi_save_game(ubyte slot, uint id, char *desc)\n"
        "{\n"
        "    (void)slot; (void)id; (void)desc;\n"
        "}\n\n"
        "void multi_restore_game(ubyte slot, uint id)\n"
        "{\n"
        "    (void)slot; (void)id;\n"
        "}\n"
    )

    if "void multi_send_msgsend_state(int state)" not in content:
        content += senders_payload_p2
        print(" -> Appended Page 2 link definitions successfully.")

    # Convert hidden tabs to pure spaces
    content = content.replace("\t", "    ")

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(content)

    print("Page 2 pipeline completed successfully.")

if __name__ == "__main__":
    main()
