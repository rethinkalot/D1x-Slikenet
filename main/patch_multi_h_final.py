import os

def main():
    filepath = "c:/Games/D1s/main/multi.h"
    
    if not os.path.exists(filepath):
        print(f"Error: Target file not found at {filepath}")
        return

    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    print("Cleaning up conflicting layout macros from multi.h...")

    # Define the old block with conflicting macros
    old_macro_block = (
        "// Global tracking fallback configurations to prevent compiler drops\n"
        "#define is_observing_player() (Game_mode & GM_OBSERVER)\n"
        "#define object_is_observer(obj) ((obj)->type == OBJ_GHOST || (obj)->type == OBJ_CAMERA)\n"
        "#define get_observer_game_mode() (0)\n\n"
    )

    # Erase the conflicting macros so the compiler relies solely on real functions
    if old_macro_block in content:
        content = content.replace(old_macro_block, "")
        print(" -> Successfully removed macro expansion collisions.")
    else:
        # Fallback raw line strip to ensure cleanup coverage
        content = content.replace("#define is_observing_player() (Game_mode & GM_OBSERVER)\n", "")
        content = content.replace("#define object_is_observer(obj) ((obj)->type == OBJ_GHOST || (obj)->type == OBJ_CAMERA)\n", "")
        content = content.replace("#define get_observer_game_mode() (0)\n", "")
        print(" -> Applied individual line cleanup on macro entities.")

    # Convert tabs to pure formatting spaces
    content = content.replace("\t", "    ")

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(content)

    print("Header cleanup successfully completed!")

if __name__ == "__main__":
    main()
