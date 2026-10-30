import os
import sys

def apply_patches():
    print("--- INITIATING LOCALIZED WORKSPACE ALIGNMENT ---")
    
    # Using relative lookups since we are running inside C:/Games/D1s/main/
    controls_path = "controls.c"
    kconfig_path = "kconfig.c"

    # 1. PATCH controls.c
    if os.path.exists(controls_path):
        with open(controls_path, "r", encoding="utf-8") as f:
            content = f.read()
        
        old_block = """    if (Player_is_dead) {
        //vm_vec_zero(&obj->mtype.phys_info.rotthrust); //let dead players rotate, changed by WraithX
        vm_vec_zero(&obj->mtype.phys_info.thrust);  //don't let dead players move, changed by WraithX
        return;
    }//end if"""
        
        new_block = """    if (Player_is_dead && !is_observer()) {
        //vm_vec_zero(&obj->mtype.phys_info.rotthrust); //let dead players rotate, changed by WraithX
        vm_vec_zero(&obj->mtype.phys_info.thrust);  //don't let dead players move, changed by WraithX
        return;
    }//end if

    if (is_observer() && obj == ConsoleObject) {
        update_object_seg(obj);
    }"""
        
        if old_block in content:
            content = content.replace(old_block, new_block)
            with open(controls_path, "w", encoding="utf-8", newline="\n") as f:
                f.write(content)
            print("[SUCCESS] controls.c updated successfully.")
        else:
            print("[WARNING] Target anchor block not found in controls.c")
    else:
        print(f"[ERROR] Could not locate file: {os.path.abspath(controls_path)}")

    # 2. PATCH kconfig.c
    if os.path.exists(kconfig_path):
        with open(kconfig_path, "r", encoding="utf-8") as f:
            content = f.read()
        
        stub_kconfig = """

void kconfig_observer_action(void)
{
    if (is_observer() && ConsoleObject != NULL) {
        update_object_seg(ConsoleObject);
    }
}
"""
        if "void kconfig_observer_action(void)" not in content:
            content = content.strip() + stub_kconfig
            with open(kconfig_path, "w", encoding="utf-8", newline="\n") as f:
                f.write(content)
            print("[SUCCESS] kconfig.c updated successfully.")
        else:
            print("[INFO] kconfig_observer_action already present in kconfig.c")
    else:
        print(f"[ERROR] Could not locate file: {os.path.abspath(kconfig_path)}")

if __name__ == "__main__":
    apply_patches()
