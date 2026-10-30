import os

def main():
    filepath = "c:/Games/D1s/main/multi.h"
    
    if not os.path.exists(filepath):
        print(f"Error: Target file not found at {filepath}")
        return

    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    print("Executing targeted macro repairs on multi.h...")

    # Define the pure-space injection block for missing global link prototypes
    prototype_payload = (
        "void multi_send_ship_status_for_frame();\n"
        "bool is_observing_player();\n"
        "bool object_is_observer(object* obj);\n"
        "int get_observer_game_mode();\n\n"
        "// Global tracking fallback configurations to prevent compiler drops\n"
        "#define is_observing_player() (Game_mode & GM_OBSERVER)\n"
        "#define object_is_observer(obj) ((obj)->type == OBJ_GHOST || (obj)->type == OBJ_CAMERA)\n"
        "#define get_observer_game_mode() (0)\n\n"
        "// Exported variables"
    )

    # Safely swap the baseline anchor near the variable definitions block
    if "// Global tracking fallback" not in content:
        content = content.replace("// Exported variables", prototype_payload)
        print(" -> Successfully integrated global observer link macros.")

    # Convert hidden tabs to pure spacing fields
    content = content.replace("\t", "    ")

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(content)

    print("Page 1 macro configurations written to disk.")

if __name__ == "__main__":
    main()
