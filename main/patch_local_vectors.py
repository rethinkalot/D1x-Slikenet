# C:/games/d1s/main/patch_universal_vectors.py
import os

filepath = "multibot.c"
if not os.path.exists(filepath):
    filepath = "../main/multibot.c"

with open(filepath, "r") as f:
    code = f.read()

# Target the entire legacy calculation block inside the Little-Endian path
le_target = """				// Convert absolute world pos_error into local robot coordinate vectors via dot products
				fix local_r = vm_vec_dot(&pos_error, &Objects[botnum].orient.rvec);
				fix local_u = vm_vec_dot(&pos_error, &Objects[botnum].orient.uvec);
				fix local_f = vm_vec_dot(&pos_error, &Objects[botnum].orient.fvec);

				// Scale the localized positional tracking error naturally
				local_r *= error_dampen;
				local_u *= error_dampen;
				local_f *= error_dampen;

				// Transform back to world velocity components based layout-intact on local axes orientation
				Objects[botnum].mtype.phys_info.velocity.x = fixmul(local_r, Objects[botnum].orient.rvec.x) + fixmul(local_u, Objects[botnum].orient.uvec.x) + fixmul(local_f, Objects[botnum].orient.fvec.x);
				Objects[botnum].mtype.phys_info.velocity.y = fixmul(local_r, Objects[botnum].orient.rvec.y) + fixmul(local_u, Objects[botnum].orient.uvec.y) + fixmul(local_f, Objects[botnum].orient.fvec.y);
				Objects[botnum].mtype.phys_info.velocity.z = fixmul(local_r, Objects[botnum].orient.rvec.z) + fixmul(local_u, Objects[botnum].orient.uvec.z) + fixmul(local_f, Objects[botnum].orient.fvec.z);"""

# The Direct Vector Blending injection (D2x Style): Uniformly eliminates desync steps on both branches
universal_patch = """				// D2x Parity: Inject uniform, time-sliced positional tracking velocities all at once
				// This completely stops local physics from over-correcting, shuddering, or storing axial tension
				if (FrameTime > 0) {
					Objects[botnum].mtype.phys_info.velocity.x = fixdiv(pos_error.x, FrameTime);
					Objects[botnum].mtype.phys_info.velocity.y = fixdiv(pos_error.y, FrameTime);
					Objects[botnum].mtype.phys_info.velocity.z = fixdiv(pos_error.z, FrameTime);
				} else {
					Objects[botnum].mtype.phys_info.velocity.x = 0;
					Objects[botnum].mtype.phys_info.velocity.y = 0;
					Objects[botnum].mtype.phys_info.velocity.z = 0;
				}"""

# Target the calculation block inside the Big-Endian path
be_target = """			// Convert absolute world pos_error into local robot coordinate vectors via dot products
			fix local_r = vm_vec_dot(&pos_error, &Objects[botnum].orient.rvec);
			fix local_u = vm_vec_dot(&pos_error, &Objects[botnum].orient.uvec);
			fix local_f = vm_vec_dot(&pos_error, &Objects[botnum].orient.fvec);

			// Scale the localized positional tracking error naturally
			local_r *= error_dampen;
			local_u *= error_dampen;
			local_f *= error_dampen;

			// Transform back to world velocity components based layout-intact on local axes orientation
			Objects[botnum].mtype.phys_info.velocity.x = fixmul(local_r, Objects[botnum].orient.rvec.x) + fixmul(local_u, Objects[botnum].orient.uvec.x) + fixmul(local_f, Objects[botnum].orient.fvec.x);
			Objects[botnum].mtype.phys_info.velocity.y = fixmul(local_r, Objects[botnum].orient.rvec.y) + fixmul(local_u, Objects[botnum].orient.uvec.y) + fixmul(local_f, Objects[botnum].orient.fvec.y);
			Objects[botnum].mtype.phys_info.velocity.z = fixmul(local_r, Objects[botnum].orient.rvec.z) + fixmul(local_u, Objects[botnum].orient.uvec.z) + fixmul(local_f, Objects[botnum].orient.fvec.z);"""

be_universal_patch = """			// D2x Parity: Inject uniform, time-sliced positional tracking velocities all at once
			if (FrameTime > 0) {
				Objects[botnum].mtype.phys_info.velocity.x = fixdiv(pos_error.x, FrameTime);
				Objects[botnum].mtype.phys_info.velocity.y = fixdiv(pos_error.y, FrameTime);
				Objects[botnum].mtype.phys_info.velocity.z = fixdiv(pos_error.z, FrameTime);
			} else {
				Objects[botnum].mtype.phys_info.velocity.x = 0;
				Objects[botnum].mtype.phys_info.velocity.y = 0;
				Objects[botnum].mtype.phys_info.velocity.z = 0;
			}"""

patched = 0
if le_target in code:
    code = code.replace(le_target, universal_patch)
    patched += 1
if be_target in code:
    code = code.replace(be_target, be_universal_patch)
    patched += 1

if patched == 2:
    with open(filepath, "w") as f:
        f.write(code)
    print("SUCCESS: Rewrote tracking vectors uniformly on all architectures simultaneously!")
else:
    print(f"ERROR: Matched {patched}/2 targets. Verify local workspace text file synchronization.")
