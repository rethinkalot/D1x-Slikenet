# Save this file as C:/Games/D1s/main/neutralize_local_drag.py and execute it
import os

filepath = "C:/Games/D1s/main/multibot.c"

with open(filepath, "r") as f:
    code = f.read()

# Target where the receiving branches zero out local velocity variables
old_drag_target = """if (Objects[botnum].ctype.ai_info.REMOTE_OWNER != Player_num)
		{
			Objects[botnum].mtype.phys_info.velocity.x = 0;
			Objects[botnum].mtype.phys_info.velocity.y = 0;
			Objects[botnum].mtype.phys_info.velocity.z = 0;
			
			// Force observers to strip away automatic vertical leveling and bank rolls from non-owned robots
			Objects[botnum].mtype.phys_info.flags &= ~(PF_LEVELLING | PF_TURNROLL);
		}"""

new_drag_target = """if (Objects[botnum].ctype.ai_info.REMOTE_OWNER != Player_num)
		{
			Objects[botnum].mtype.phys_info.velocity.x = 0;
			Objects[botnum].mtype.phys_info.velocity.y = 0;
			Objects[botnum].mtype.phys_info.velocity.z = 0;
			
			// Neutralize local simulation drag so it cannot truncate network tracking velocities
			Objects[botnum].mtype.phys_info.drag = 0;
			
			// Force observers to strip away automatic vertical leveling and bank rolls from non-owned robots
			Objects[botnum].mtype.phys_info.flags &= ~(PF_LEVELLING | PF_TURNROLL);
		}"""

if old_drag_target in code:
    # Replace both instances (Little-Endian and Big-Endian match this block pattern)
    code = code.replace(old_drag_target, new_drag_target)
    with open(filepath, "w") as f:
        f.write(code)
    print("SUCCESS: Remote physics drag successfully neutralized for observer frames.")
else:
    print("ERROR: Target velocity setup landmarks mismatched. Check formatting.")
