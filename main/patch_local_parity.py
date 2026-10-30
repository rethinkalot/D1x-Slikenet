# C:/games/d1s/main/patch_local_parity.py
import os

filepath = "multibot.c"
if not os.path.exists(filepath):
    filepath = "../main/multibot.c"

if not os.path.exists(filepath):
    print("ERROR: Cannot find multibot.c in local paths.")
    exit(1)

with open(filepath, "r") as f:
    lines = f.readlines()

new_lines = []
skip_mode = False
in_else_branch = False
le_patched = False
be_patched = False

# Robust payload with explicit variable initialization inside the execution logic block
le_payload = """				// Calculate pitch tilt slope to dampen over-corrections when looking heavily up or down
				fix vertical_tilt = abs(Objects[botnum].orient.fvec.y);
				int error_dampen = (vertical_tilt > F1_0 / 2) ? 2 : 4;

				// Convert absolute world pos_error into local robot coordinate vectors via dot products
				fix local_r = vm_vec_dot(&pos_error, &Objects[botnum].orient.rvec);
				fix local_u = vm_vec_dot(&pos_error, &Objects[botnum].orient.uvec);
				fix local_f = vm_vec_dot(&pos_error, &Objects[botnum].orient.fvec);

				// Scale the localized positional tracking error naturally
				local_r *= error_dampen;
				local_u *= error_dampen;
				// Invert the forward vector coefficient to completely cure the local nose-line sign drift
				local_f *= -error_dampen;

				// Transform back to world velocity components based layout-intact on local axes orientation
				Objects[botnum].mtype.phys_info.velocity.x = fixmul(local_r, Objects[botnum].orient.rvec.x) + fixmul(local_u, Objects[botnum].orient.uvec.x) + fixmul(local_f, Objects[botnum].orient.fvec.x);
				Objects[botnum].mtype.phys_info.velocity.y = fixmul(local_r, Objects[botnum].orient.rvec.y) + fixmul(local_u, Objects[botnum].orient.uvec.y) + fixmul(local_f, Objects[botnum].orient.fvec.y);
				Objects[botnum].mtype.phys_info.velocity.z = fixmul(local_r, Objects[botnum].orient.rvec.z) + fixmul(local_u, Objects[botnum].orient.uvec.z) + fixmul(local_f, Objects[botnum].orient.fvec.z);
"""

be_payload = """			// Calculate pitch tilt slope to dampen over-corrections when looking heavily up or down
			fix vertical_tilt = abs(Objects[botnum].orient.fvec.y);
			int error_dampen = (vertical_tilt > F1_0 / 2) ? 2 : 4;

			// Convert absolute world pos_error into local robot coordinate vectors via dot products
			fix local_r = vm_vec_dot(&pos_error, &Objects[botnum].orient.rvec);
			fix local_u = vm_vec_dot(&pos_error, &Objects[botnum].orient.uvec);
			fix local_f = vm_vec_dot(&pos_error, &Objects[botnum].orient.fvec);

			// Scale the localized positional tracking error naturally
			local_r *= error_dampen;
			local_u *= error_dampen;
			// Apply identical sign inversion to Big Endian branch for global structural synchronization
			local_f *= -error_dampen;

			// Transform back to world velocity components based layout-intact on local axes orientation
			Objects[botnum].mtype.phys_info.velocity.x = fixmul(local_r, Objects[botnum].orient.rvec.x) + fixmul(local_u, Objects[botnum].orient.uvec.x) + fixmul(local_f, Objects[botnum].orient.fvec.x);
			Objects[botnum].mtype.phys_info.velocity.y = fixmul(local_r, Objects[botnum].orient.rvec.y) + fixmul(local_u, Objects[botnum].orient.uvec.y) + fixmul(local_f, Objects[botnum].orient.fvec.y);
			Objects[botnum].mtype.phys_info.velocity.z = fixmul(local_r, Objects[botnum].orient.rvec.z) + fixmul(local_u, Objects[botnum].orient.uvec.z) + fixmul(local_f, Objects[botnum].orient.fvec.z);
"""

for line in lines:
    if "#else" in line:
        in_else_branch = True

    # Catch the comment landmark or any broken variable residue lines
    if "// Calculate pitch tilt slope" in line or "local_r *=" in line:
        skip_mode = True
        continue

    if skip_mode:
        # Move forward past legacy velocity code blocks safely
        if "update_object_seg" in line or "#endif" in line:
            skip_mode = False
            if not in_else_branch:
                new_lines.append(le_payload)
                le_patched = True
            else:
                new_lines.append(be_payload)
                be_patched = True
            new_lines.append(line)
        continue

    new_lines.append(line)

if le_patched or be_patched:
    with open(filepath, "w") as f:
        f.writelines(new_lines)
    print(f"SUCCESS: Declaration variables restored. LE Patched: {le_patched}, BE Patched: {be_patched}")
else:
    print("ERROR: Structural landmarks not found inside the active source file.")
