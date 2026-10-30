# Save this file as C:/Games/D1s/main/apply_dual_parity.py and execute it
import os

filepath = "C:/Games/D1s/main/multibot.c"

with open(filepath, "r") as f:
    code = f.read()

# Target the receiving interpolation block to drop the over-engineered velocity equations
old_sync_logic = """		if (迫Netgame.RetroProtocol) {
			// Extract pristine uncompressed metrics directly into client memory bounds
			memcpy(&Objects[botnum].orient, buf + loc, sizeof(vms_matrix)); loc += sizeof(vms_matrix);
			memcpy(&Objects[botnum].pos, buf + loc, sizeof(vms_vector));    loc += sizeof(vms_vector);
			memcpy(&Objects[botnum].mtype.phys_info.velocity, buf + loc, sizeof(vms_vector)); loc += sizeof(vms_vector);
		} else {
			shortpos temp_sp;
			memcpy((ubyte *)(temp_sp.bytemat), (ubyte *)(buf + loc), 9);
			memcpy((ubyte *)&(temp_sp.xo), (ubyte *)(buf + loc + 9), 14);
			extract_shortpos(&Objects[botnum], &temp_sp, 0);
			loc += sizeof(shortpos);
		}"""

# If the code was partially modified or shifted, let's target the inner calculation landmarks instead to be safe:
old_velocity_block = """// Inertial Blending: Calculate the target snap velocity
			fix target_vel_x = (pos_error.x * 4);
			fix target_vel_y = (pos_error.y * 4);
			fix target_vel_z = (pos_error.z * 4);
			
			// Apply a 75/25 physical momentum filter to absorb violent multi-axis trajectory snaps
			Objects[botnum].mtype.phys_info.velocity.x = fixmul(Objects[botnum].mtype.phys_info.velocity.x, F1_0 * 3 / 4) + fixmul(target_vel_x, F1_0 / 4);
			Objects[botnum].mtype.phys_info.velocity.y = fixmul(Objects[botnum].mtype.phys_info.velocity.y, F1_0 * 3 / 4) + fixmul(target_vel_y, F1_0 / 4);
			Objects[botnum].mtype.phys_info.velocity.z = fixmul(Objects[botnum].mtype.phys_info.velocity.z, F1_0 * 3 / 4) + fixmul(target_vel_z, F1_0 / 4);"""

new_velocity_block = """// Dual-State Parity: Directly apply incoming spatial positions to prevent rubber-banding
			// Let the high 90 PPS uncompressed stream anchor the robot coordinates natively
			if (vm_vec_mag_quick(&pos_error) < F1_0 * 5) {
				Objects[botnum].pos.x += (pos_error.x >> 1);
				Objects[botnum].pos.y += (pos_error.y >> 1);
				Objects[botnum].pos.z += (pos_error.z >> 1);
			} else {
				Objects[botnum].pos = Objects[botnum].last_pos;
			}

			// Re-align native D1 physics forces to zero out tracking drift between frame ticks
			Objects[botnum].mtype.phys_info.velocity.x = 0;
			Objects[botnum].mtype.phys_info.velocity.y = 0;
			Objects[botnum].mtype.phys_info.velocity.z = 0;"""

if old_velocity_block in code:
    code = code.replace(old_velocity_block, new_velocity_block)
    with open(filepath, "w") as f:
        f.write(code)
    print("SUCCESS: Dual-State Parity Engine successfully deployed.")
else:
    # Fallback to check if previous raw velocity multipliers are present instead
    fallback_block = """Objects[botnum].mtype.phys_info.velocity.x = (pos_error.x * 4);
			Objects[botnum].mtype.phys_info.velocity.y = (pos_error.y * 4);
			Objects[botnum].mtype.phys_info.velocity.z = (pos_error.z * 4);"""
    if fallback_block in code:
        code = code.replace(fallback_block, new_velocity_block)
        with open(filepath, "w") as f:
            f.write(code)
        print("SUCCESS: Dual-State Parity Engine deployed via fallback landmarks.")
    else:
        print("ERROR: Formatting drifts prevented landmark anchoring. Check code layout.")
