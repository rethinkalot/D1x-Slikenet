# Save this file as C:/Games/D1s/main/isolate_remote_steering.py and execute it
import os

filepath = "C:/Games/D1s/main/ai.c"

with open(filepath, "r") as f:
    lines = f.readlines()

output = []
for line in lines:
    stripped = line.strip()
    
    # Target 1: Line 737 - Blind tracking switch during chase sequences
    if stripped == "ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);":
        indent = line[:len(line) - len(line.lstrip())]
        output.append(f"{indent}if (obj->ctype.ai_info.REMOTE_OWNER == -1 || obj->ctype.ai_info.REMOTE_OWNER == Player_num) {{\n")
        output.append(f"{indent}    ai_turn_towards_vector(&vec_to_player, obj, robptr->turn_time[Difficulty_level]);\n")
        output.append(f"{indent}}}\n")
        continue
        
    # Target 2: Line 1261 & 1264 - Blind tracking/looking matrix overrides during boss teleports
    if stripped == "vm_vector_2_matrix(&objp->orient, &boss_dir, NULL, NULL);":
        indent = line[:len(line) - len(line.lstrip())]
        output.append(f"{indent}if (objp->ctype.ai_info.REMOTE_OWNER == -1 || objp->ctype.ai_info.REMOTE_OWNER == Player_num) {{\n")
        output.append(f"{indent}    vm_vector_2_matrix(&objp->orient, &boss_dir, NULL, NULL);\n")
        output.append(f"{indent}}}\n")
        continue
        
    # Target 3: Line 1317 - Blind tracking switches inside actual firing routines
    if stripped == "vm_vector_2_matrix( &obj->orient, &direction, &obj->orient.uvec, NULL);":
        indent = line[:len(line) - len(line.lstrip())]
        output.append(f"{indent}if (obj->ctype.ai_info.REMOTE_OWNER == -1 || obj->ctype.ai_info.REMOTE_OWNER == Player_num) {{\n")
        output.append(f"{indent}    vm_vector_2_matrix( &obj->orient, &direction, &obj->orient.uvec, NULL);\n")
        output.append(f"{indent}}}\n")
        continue
        
    # Target 4: Line 1341 & 1345 - Blind matrix manipulations inside final look sweeps
    if stripped == "vm_vector_2_matrix(&obj->orient, &goal_vector, NULL, &obj->orient.rvec);":
        indent = line[:len(line) - len(line.lstrip())]
        output.append(f"{indent}if (obj->ctype.ai_info.REMOTE_OWNER == -1 || obj->ctype.ai_info.REMOTE_OWNER == Player_num) {{\n")
        output.append(f"{indent}    vm_vector_2_matrix(&obj->orient, &goal_vector, NULL, &obj->orient.rvec);\n")
        output.append(f"{indent}}}\n")
        continue

    output.append(line)

with open(filepath, "w") as f:
    f.writelines(output)

print("SUCCESS: Remote steering loops successfully isolated across all ai.c update sectors.")
