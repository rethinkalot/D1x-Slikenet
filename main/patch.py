import os

# Define files from local project folder paths
newdemo_c_path = r"C:\games\d1s\main\newdemo.c"
newdemo_h_path = r"C:\games\d1s\main\newdemo.h"

if not os.path.exists(newdemo_c_path) or not os.path.exists(newdemo_h_path):
    print("Error: Missing project files inside directory structure.")
    exit(1)

# ---- Part 1: Optimize seeking intervals inside newdemo.c ----
with open(newdemo_c_path, "r", encoding="utf-8") as f:
    c_content = f.read()

# Target standard non-optimized static layout segment
old_seeking_block = """        if (Newdemo_vcr_state == ND_STATE_REWINDING)
            frames_back = 10;
        else
            frames_back = 1;"""

# Dynamic framerate adaptive seeking layout implementation
new_seeking_block = """        if (Newdemo_vcr_state == ND_STATE_REWINDING) {
            // Scale step dynamically to provide rapid seeking regardless of high frame ceilings
            int target_fps = PlayerCfg.maxFps > 0 ? PlayerCfg.maxFps : 60;
            frames_back = (target_fps > 60) ? (target_fps / 3) : 10;
            if (frames_back < 10) frames_back = 10;
        }
        else
            frames_back = 1;"""

old_ff_block = """    else if (Newdemo_vcr_state == ND_STATE_FASTFORWARD) {
        if (!nd_playback_v_at_eof)
        {
            for (i = 0; i < 10; i++)
            {
                if (newdemo_read_frame_information(0) == -1)
                {
                    if (nd_playback_v_at_eof)
                        Newdemo_vcr_state = ND_STATE_PAUSED;
                    else
                        newdemo_stop_playback();
                    break;
                }
            }
        }
        else
            Newdemo_vcr_state = ND_STATE_PAUSED;
    }"""

new_ff_block = """    else if (Newdemo_vcr_state == ND_STATE_FASTFORWARD) {
        if (!nd_playback_v_at_eof)
        {
            int target_fps = PlayerCfg.maxFps > 0 ? PlayerCfg.maxFps : 60;
            int ff_steps = (target_fps > 60) ? (target_fps / 3) : 10;
            if (ff_steps < 10) ff_steps = 10;
            
            for (i = 0; i < ff_steps; i++)
            {
                if (newdemo_read_frame_information(0) == -1)
                {
                    if (nd_playback_v_at_eof)
                        Newdemo_vcr_state = ND_STATE_PAUSED;
                    else
                        newdemo_stop_playback();
                    break;
                }
            }
        }
        else
            Newdemo_vcr_state = ND_STATE_PAUSED;
    }"""

if old_seeking_block in c_content and old_ff_block in c_content:
    c_content = c_content.replace(old_seeking_block, new_seeking_block)
    c_content = c_content.replace(old_ff_block, new_ff_block)
    with open(newdemo_c_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(c_content)
    print("Successfully injected dynamic seeking logic into newdemo.c!")
else:
    print("Error: Target pattern matches not found in newdemo.c or already patched.")


# ---- Part 2: Clean up remaining newdemo_record_player_stats reference inside newdemo.h ----
with open(newdemo_h_path, "r", encoding="utf-8") as f:
    h_content = f.read()

old_header_decl = "extern void newdemo_record_player_stats(int shields, int energy, int score );"

if old_header_decl in h_content:
    # Remove old tracking reference since D1X uses discrete channels natively
    h_content = h_content.replace(old_header_decl, "")
    with open(newdemo_h_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(h_content)
    print("Successfully removed invalid signature tracking symbol from newdemo.h!")
else:
    print("Header declaration symbol was already cleared out.")
