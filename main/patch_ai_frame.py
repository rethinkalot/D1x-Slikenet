# Save this file as C:/Games/D1s/main/kill_rubber_band.py and execute it
import os

filepath = "C:/Games/D1s/main/multibot.c"

with open(filepath, "r") as f:
    lines = f.readlines()

output = []
skip_mode = False

for line in lines:
    stripped = line.strip()
    
    # Target the flood gate condition we added previously
    if stripped.startswith("if (last_processed_tick[botnum] != d_tick_count) {"):
        skip_mode = True
        continue
        
    # Skip the old clamp assignment inner boundaries completely
    if skip_mode and stripped == "}":
        skip_mode = False
        continue
        
    # Replace the legacy hardcoded clamp fallback if it exists independently
    if not skip_mode and stripped == "Objects[botnum].pos = client_pos;":
        indent = line[:len(line) - len(line.lstrip())]
        output.append(f"{indent}// Sanitized: Never snap absolute position backward mid-frame for observers\n")
        output.append(f"{indent}// Objects[botnum].pos = client_pos;\n")
        continue

    if not skip_mode:
        output.append(line)

with open(filepath, "w") as f:
    f.writelines(output)

print("SUCCESS: Positional rubber-band clamp successfully neutralized from multibot.c.")
