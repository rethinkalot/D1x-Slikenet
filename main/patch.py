import os
import re

filepath = "multibot.c"
if not os.path.exists(filepath):
    filepath = r"C:/Games/D1s/main/multibot.c"

print(f"[INFO] Targeting multibot.c to eliminate tracking jolts: {filepath}")

with open(filepath, "r", encoding="utf-8") as f:
    content = f.read()

# 1. Neutralize the sharp shift multiplier logic and keep it locked to a smooth linear glide scale
pattern_shift = r"(target_orient\.fvec;\s*\n\s*turn_dot\s*=\s*vm_vec_dot\s*\(\s*&client_orient\.fvec\s*,\s*&target_orient\.fvec\s*\)\s*;\s*\n\s*blend_shift\s*=\s*\(turn_dot\s*<\s*F1_0\s*/\s*2\)\s*\?\s*1\s*:\s*2\s*;)"

# 2. Safely capture and target the harsh rotvel zeroing blocks that trigger during high-delta snaps
pattern_rotvel = r"(\t*//\s*Damp\s*out\s*rogue\s*local\s*prediction\s*forces[^\n]*\n\s*if\s*\(\s*blend_shift\s*==\s*1\s*\)\s*\{.*?\}\s*\n\s*//\s*Kill\s*local\s*predicted\s*rotation[^\n]*\n\s*if\s*\(\s*blend_shift\s*==\s*1\s*\)\s*\{.*?\})"

if re.search(pattern_shift, content):
    # Lock blend_shift to a steady, smooth value (2) instead of jumping down to 1
    content = re.sub(pattern_shift, "target_orient.fvec;\n\t\t\t\tblend_shift = 2; // Locked to clean linear glide scale", content)
    
    # Strip out the tracking frame zeroing overrides if they exist in the file
    content = re.sub(pattern_rotvel, "/* Rotational tracking overrides cleaned for smooth gliding */", content, flags=re.DOTALL)
    
    with open(filepath, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)
    print("[SUCCESS] Jolt snapping gates stripped. Tracking is now configured for smooth gliding.")
else:
    print("[ERROR] Could not isolate the target tracking snap sequences inside multibot.c.")
