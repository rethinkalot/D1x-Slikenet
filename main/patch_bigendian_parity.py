# C:/Games/D1s/main/patch_bigendian_parity.py
import os

filepath = "C:/Games/D1s/main/multibot.c"

with open(filepath, "r") as f:
    code = f.read()

# First: Inject the velocity clamp ceiling logic into the Little-Endian path
le_target = """			// Calculate pitch tilt slope to dampen over-corrections when looking heavily up or down
			fix vertical_tilt = abs(Objects[botnum].orient.fvec.y);
			int error_dampen = (vertical_tilt > F1_0 / 2) ? 2 : 4; // Drop multiplier to 2x during extreme vertical tracking

			Objects[botnum].mtype.phys_info.velocity.x = (pos_error.x * error_dampen);
			Objects[botnum].mtype.phys_info.velocity.y = (pos_error.y * error_dampen);
			Objects[botnum].mtype.phys_info.velocity.z = (pos_error.z * error_dampen);"""

le_patched = """			// Calculate pitch tilt slope to dampen over-corrections when looking heavily up or down
			fix vertical_tilt = abs(Objects[botnum].orient.fvec.y);
			int error_dampen = (vertical_tilt > F1_0 / 2) ? 2 : 4; // Drop multiplier to 2x during extreme vertical tracking

			// Kinetic Governor Clamp: prevent tracking spikes from causing visual warps
			fix max_correction = F1_0 * 5;
			fix vx = pos_error.x * error_dampen;
			fix vy = pos_error.y * error_dampen;
			fix vz = pos_error.z * error_dampen;

			Objects[botnum].mtype.phys_info.velocity.x = (vx > max_correction) ? max_correction : ((vx < -max_correction) ? -max_correction : vx);
			Objects[botnum].mtype.phys_info.velocity.y = (vy > max_correction) ? max_correction : ((vy < -max_correction) ? -max_correction : vy);
			Objects[botnum].mtype.phys_info.velocity.z = (vz > max_correction) ? max_correction : ((vz < -max_correction) ? -max_correction : vz);"""

# Second: Overwrite the broken Big-Endian branch section with synchronized math and the same clamp
be_target = """#else
	memcpy((ubyte *)(sp.bytemat), (ubyte *)(buf + loc), 9);		loc += 9;
	memcpy((ubyte *)&(sp.xo), (ubyte *)(buf + loc), 14);
	
	vms_vector client_pos = Objects[botnum].pos;
	vms_matrix client_orient = Objects[botnum].orient;
	
	extract_shortpos(&Objects[botnum], &sp, 1);
	
	if (Objects[botnum].ctype.ai_info.REMOTE_OWNER != Player_num)
	{
		vms_matrix target_orient = Objects[botnum].orient;
		Objects[botnum].pos = client_pos;
		
		turn_dot = vm_vec_dot(&client_orient.fvec, &target_orient.fvec);
		blend_shift = (turn_dot < F1_0 / 2) ? 1 : 2; // Smooth out high-delta tracking pivots
		
		Objects[botnum].orient.fvec.x = client_orient.fvec.x + ((target_orient.fvec.x - client_orient.fvec.x) >> blend_shift);
		Objects[botnum].orient.fvec.y = client_orient.fvec.y + ((target_orient.fvec.y - client_orient.fvec.y) >> blend_shift);
		Objects[botnum].orient.fvec.z = client_orient.fvec.z + ((target_orient.fvec.z - client_orient.fvec.z) >> blend_shift);
		
		Objects[botnum].orient.uvec.x = client_orient.uvec.x + ((target_orient.uvec.x - client_orient.uvec.x) >> blend_shift);
		Objects[botnum].orient.uvec.y = client_orient.uvec.y + ((target_orient.uvec.y - client_orient.uvec.y) >> blend_shift);
		Objects[botnum].orient.uvec.z = client_orient.uvec.z + ((target_orient.uvec.z - client_orient.uvec.z) >> blend_shift);
		
		Objects[botnum].orient.rvec.x = client_orient.rvec.x + ((target_orient.rvec.x - client_orient.rvec.x) >> blend_shift);
		Objects[botnum].orient.rvec.y = client_orient.rvec.y + ((target_orient.rvec.y - client_orient.rvec.y) >> blend_shift);
		Objects[botnum].orient.rvec.z = client_orient.rvec.z + ((target_orient.rvec.z - client_orient.rvec.z) >> blend_shift);
		
		vm_vec_normalize_quick(&Objects[botnum].orient.fvec);
		vm_vec_normalize_quick(&Objects[botnum].orient.uvec);
		vm_vec_normalize_quick(&Objects[botnum].orient.rvec);
		
		if (blend_shift == 1) { // Clear lingering rotvel tracking inertia during sharp snaps
			Objects[botnum].mtype.phys_info.rotvel.x = 0;
			Objects[botnum].mtype.phys_info.rotvel.y = 0;
			Objects[botnum].mtype.phys_info.rotvel.z = 0;
		}
		update_object_seg(&Objects[botnum]);
	}
#endif"""

be_patched = """#else
	memcpy((ubyte *)(sp.bytemat), (ubyte *)(buf + loc), 9);		loc += 9;
	memcpy((ubyte *)&(sp.xo), (ubyte *)(buf + loc), 14);
	
	vms_vector client_pos = Objects[botnum].pos;
	vms_matrix client_orient = Objects[botnum].orient;
	
	if (Objects[botnum].ctype.ai_info.REMOTE_OWNER != Player_num)
	{
		Objects[botnum].mtype.phys_info.velocity.x = 0;
		Objects[botnum].mtype.phys_info.velocity.y = 0;
		Objects[botnum].mtype.phys_info.velocity.z = 0;
		
		// Force observers to strip away automatic vertical leveling and bank rolls from non-owned robots
		Objects[botnum].mtype.phys_info.flags &= ~(PF_LEVELLING | PF_TURNROLL);
	}

	extract_shortpos(&Objects[botnum], &sp, 1);
	
	if (Objects[botnum].ctype.ai_info.REMOTE_OWNER != Player_num)
	{
		vms_vector pos_error;
		vm_vec_sub(&pos_error, &Objects[botnum].pos, &client_pos);
		
		if (vm_vec_mag_quick(&pos_error) > F1_0 * 15) {
			update_object_seg(&Objects[botnum]);
		} else {
			vms_matrix target_orient = Objects[botnum].orient;
			Objects[botnum].pos = client_pos;
			
			turn_dot = vm_vec_dot(&client_orient.fvec, &target_orient.fvec);
			blend_shift = (turn_dot < F1_0 / 2) ? 1 : 2;
			
			Objects[botnum].orient.fvec.x = client_orient.fvec.x + ((target_orient.fvec.x - client_orient.fvec.x) >> blend_shift);
			Objects[botnum].orient.fvec.y = client_orient.fvec.y + ((target_orient.fvec.y - client_orient.fvec.y) >> blend_shift);
			Objects[botnum].orient.fvec.z = client_orient.fvec.z + ((target_orient.fvec.z - client_orient.fvec.z) >> blend_shift);
			
			Objects[botnum].orient.uvec.x = client_orient.uvec.x + ((target_orient.uvec.x - client_orient.uvec.x) >> blend_shift);
			Objects[botnum].orient.uvec.y = client_orient.uvec.y + ((target_orient.uvec.y - client_orient.uvec.y) >> blend_shift);
			Objects[botnum].orient.uvec.z = client_orient.uvec.z + ((target_orient.uvec.z - client_orient.uvec.z) >> blend_shift);
			
			Objects[botnum].orient.rvec.x = client_orient.rvec.x + ((target_orient.rvec.x - client_orient.rvec.x) >> blend_shift);
			Objects[botnum].orient.rvec.y = client_orient.rvec.y + ((target_orient.rvec.y - client_orient.rvec.y) >> blend_shift);
			Objects[botnum].orient.rvec.z = client_orient.rvec.z + ((target_orient.rvec.z - client_orient.rvec.z) >> blend_shift);
			
			vm_vec_normalize_quick(&Objects[botnum].orient.fvec);
			vm_vec_normalize_quick(&Objects[botnum].orient.uvec);
			vm_vec_normalize_quick(&Objects[botnum].orient.rvec);

			if (blend_shift == 1) {
				Objects[botnum].mtype.phys_info.rotvel.x = 0;
				Objects[botnum].mtype.phys_info.rotvel.y = 0;
				Objects[botnum].mtype.phys_info.rotvel.z = 0;
			}

			// Calculate pitch tilt slope to dampen over-corrections when looking heavily up or down
			fix vertical_tilt = abs(Objects[botnum].orient.fvec.y);
			int error_dampen = (vertical_tilt > F1_0 / 2) ? 2 : 4;

			// Kinetic Governor Clamp: prevent tracking spikes from causing visual warps
			fix max_correction = F1_0 * 5;
			fix vx = pos_error.x * error_dampen;
			fix vy = pos_error.y * error_dampen;
			fix vz = pos_error.z * error_dampen;

			Objects[botnum].mtype.phys_info.velocity.x = (vx > max_correction) ? max_correction : ((vx < -max_correction) ? -max_correction : vx);
			Objects[botnum].mtype.phys_info.velocity.y = (vy > max_correction) ? max_correction : ((vy < -max_correction) ? -max_correction : vy);
			Objects[botnum].mtype.phys_info.velocity.z = (vz > max_correction) ? max_correction : ((vz < -max_correction) ? -max_correction : vz);
			
			update_object_seg(&Objects[botnum]);
		}
	}
#endif"""

patched_count = 0
if le_target in code:
    code = code.replace(le_target, le_patched)
    patched_count += 1
if be_target in code:
    code = code.replace(be_target, be_patched)
    patched_count += 1

if patched_count == 2:
    with open(filepath, "w") as f:
        f.write(code)
    print("SUCCESS: Big-Endian parity restored AND kinetic velocity clamp deployed globally across both calculation paths.")
else:
    print(f"ERROR: Only replaced {patched_count}/2 blocks. Check anchor alignments.")
