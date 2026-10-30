# Save this file as C:/Games/D1s/main/fix_inertial_blend.py and execute it
import os

filepath = "C:/Games/D1s/main/multibot.c"

with open(filepath, "r") as f:
    code = f.read()

# Target our previous raw velocity assignments
old_velocity_block = """// Restored responsive world-space error tracking scaled by 4
			Objects[botnum].mtype.phys_info.velocity.x = (pos_error.x * 4);
			Objects[botnum].mtype.phys_info.velocity.y = (pos_error.y * 4);
			Objects[botnum].mtype.phys_info.velocity.z = (pos_error.z * 4);"""

new_velocity_block = """// Inertial Blending: Calculate the target snap velocity
			fix target_vel_x = (pos_error.x * 4);
			fix target_vel_y = (pos_error.y * 4);
			fix target_vel_z = (pos_error.z * 4);
			
			// Apply a 75/25 physical momentum filter to absorb violent multi-axis trajectory snaps
			Objects[botnum].mtype.phys_info.velocity.x = fixmul(Objects[botnum].mtype.phys_info.velocity.x, F1_0 * 3 / 4) + fixmul(target_vel_x, F1_0 / 4);
			Objects[botnum].mtype.phys_info.velocity.y = fixmul(Objects[botnum].mtype.phys_info.velocity.y, F1_0 * 3 / 4) + fixmul(target_vel_y, F1_0 / 4);
			Objects[botnum].mtype.phys_info.velocity.z = fixmul(Objects[botnum].mtype.phys_info.velocity.z, F1_0 * 3 / 4) + fixmul(target_vel_z, F1_0 / 4);"""

if old_velocity_block in code:
    code = code.replace(old_velocity_block, new_velocity_block)
    with open(filepath, "w") as f:
        f.write(code)
    print("SUCCESS: Inertial momentum filter successfully integrated into the receiving loop.")
else:
    print("ERROR: Target velocity tracking block landmarks mismatched. Check current state.")
