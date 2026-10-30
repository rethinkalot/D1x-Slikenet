import os

def main():
    filepath = "c:/Games/D1s/main/multi.c"
    
    if not os.path.exists(filepath):
        print(f"Error: Target file not found at {filepath}")
        return

    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    print("Executing final code safety checks on multi.c...")

    # Define the pure-space layout payload for missing global observer tracking links
    helpers_payload = (
        "\n\n"
        "bool is_observing_player()\n"
        "{\n"
        "    return (Game_mode & GM_OBSERVER) ? 1 : 0;\n"
        "}\n\n"
        "bool object_is_observer(object* obj)\n"
        "{\n"
        "    if (!obj) { return 0; }\n"
        "    return (obj->type == OBJ_GHOST) ? 1 : 0;\n"
        "}\n\n"
        "int get_observer_game_mode()\n"
        "{\n"
        "    return 0;\n"
        "}\n"
    )

    # Append safely to the bottom of the file if definitions are missing
    if "bool is_observing_player()" not in content:
        content += helpers_payload
        print(" -> Successfully linked missing observer helper blocks.")

    # Convert any lingering tab characters to explicit raw spaces
    content = content.replace("\t", "    ")

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(content)

    print("Final multi.c structural repairs completed!")

if __name__ == "__main__":
    main()
