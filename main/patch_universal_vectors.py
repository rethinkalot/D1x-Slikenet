# C:/games/d1s/main/patch_universal_vectors.py
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
skip_until_blank_bracket = False
in_else_branch = False
patched_le = False
patched_be = False

# Definitive D2x Style Uniform Slashing Payload
le_payload = """				// D2x Parity: Inject uniform, time-sliced positional tracking velocities all at once
				// This completely stops local physics from over-correcting, shuddering, or storing axial tension
				if (FrameTime > 0) {
					Objects[botnum].mtype.phys_info.velocity.x = fixdiv(pos_error.x, FrameTime);
					Objects[botnum].mtype.phys_info.velocity.y = fixdiv(pos_error.y, FrameTime);
					Objects[botnum].mtype.phys_info.velocity.z = fixdiv(pos_error.z, FrameTime);
				} else {
					Objects[botnum].mtype.phys_info.velocity.x = 0;
					Objects[botnum].mtype.phys_info.velocity.y = 0;
					Objects[botnum].mtype.phys_info.velocity.z = 0;
				}
"""

be_payload = """			// D2x Parity: Inject uniform, time-sliced positional tracking velocities all at once
			if (FrameTime > 0) {
				Objects[botnum].mtype.phys_info.velocity.x = fixdiv(pos_error.x, FrameTime);
				Objects[botnum].mtype.phys_info.velocity.y = fixdiv(pos_error.y, FrameTime);
				Objects[botnum].mtype.phys_info.velocity.z = fixdiv(pos_error.z, FrameTime);
			} else {
				Objects[botnum].mtype.phys_info.velocity.x = 0;
				Objects[botnum].mtype.phys_info.velocity.y = 0;
				Objects[botnum].mtype.phys_info.velocity.z = 0;
			}
"""

for line in lines:
    if "#else" in line:
        in_else_branch = True

    # Detect the tracking comment marker landmark
    if "// Calculate pitch tilt slope" in line:
        skip_until_blank_bracket = True
        new_lines.append(line) # Keep the comment layout intact
        continue

    if skip_until_blank_bracket:
        # Erase and skip all intermediate velocity assignment expressions until we hit the ending statement boundary
        if "update_object_seg" in line or "#endif" in line:
            skip_until_blank_bracket = False
            # Drop the direct velocity injection block layout-intact
            if not in_else_branch:
                new_lines.append(le_payload)
                patched_le = True
            else:
                new_lines.append(be_payload)
                patched_be = True
            new_lines.append(line)
        continue

    new_lines.append(line)

if patched_le and patched_be:
    with open(filepath, "w") as f:
        f.writelines(new_lines)
    print("SUCCESS: Erased all legacy vector math and injected true D2x Direct Vector Blending on both branches!")
else:
    print(f"ERROR: Landmark parsing failed. LE patched: {patched_le}, BE patched: {patched_be}")
