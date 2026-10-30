import os

def apply_network_patches():
    print("--- INITIATING NETWORK WORKSPACE ALIGNMENT ---")
    
    # Using relative lookup inside C:/Games/D1s/main/
    multi_path = "multi.c"
    
    if os.path.exists(multi_path):
        with open(multi_path, "r", encoding="utf-8") as f:
            content = f.read()
            
        stub_multi = """

void multi_send_system_message(char *message)
{
    if (is_observer()) {
        return;
    }
}

void multi_interp_push_position(int pnum, object *objp, ubyte serial, int force)
{
    return;
}
"""
        if "void multi_send_system_message(char *message)" not in content:
            content = content.strip() + stub_multi
            with open(multi_path, "w", encoding="utf-8", newline="\n") as f:
                f.write(content)
            print("[SUCCESS] multi.c updated successfully.")
        else:
            print("[INFO] multi stubs already present in multi.c")
    else:
        print(f"[ERROR] Could not locate file: {os.path.abspath(multi_path)}")

    print("--- LOCALIZED ALIGNMENT PASS COMPLETE ---")

if __name__ == "__main__":
    apply_network_patches()
