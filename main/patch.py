import os
import re

filepath = "multibot.c"
if not os.path.exists(filepath):
    filepath = r"C:/Games/D1s/main/multibot.c"

print(f"[INFO] Eliminating tracking snaps and locking smooth glide scale: {filepath}")

with open(filepath, "r", encoding="utf-8") as f:
    content = f.read()

# 1. Target the sharp blend_shift scaling gate and lock it to a fluid linear glide factor (2)
shift_pattern = r"(vms_matrix\s+target_orient\s*=\s*Objects\[botnum\]\.orient;\s*\n\s*fix\s+turn_dot\s*=\s*vm_vec_dot\s*\(\s*&client_orient\.fvec\s*,\s*&target_orient\.fvec\s*\)\s*;\s*\n\s*)int\s+blend_shift\s*=\s*\(turn_dot\s*<\s*F1_0\s*/\s*2\)\s*\?\s*1\s*:\s*2\s*;"

# 2. Target the harsh rotvel momentum zeroing overrides that trigger sharp pops
rotvel_pattern = r"(\t*//\s*Damp\s*out\s*rogue\s*local\s*prediction\s*forces[^\n]*\n\s*if\s*\(\s*blend_shift\s*==\s*1\s*\)\s*\{.*?\}\s*\n\s*//\s*Kill\s*local\s*predicted\s*rotation[^\n]*\n\s*if\s*\(\s*blend_shift\s*==\s*1\s*\)\s*\{.*?\})"

if re.search(shift_pattern, content):
    content = re.sub(shift_pattern, r"\1int blend_shift = 2; // Locked to continuous fluid linear glide scale", content)
    content = re.sub(rotvel_pattern, "/* Rotational momentum overrides cleaned for smooth gliding */", content, flags=re.DOTALL)
    
    with open(filepath, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)
    print("[SUCCESS] Tracking snaps smoothed out. Bots are configured for absolute linear glide.")
else:
    print("[ERROR] Could not isolate the target tracking snap structures inside multibot.c.")
