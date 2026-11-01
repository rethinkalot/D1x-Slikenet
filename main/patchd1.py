import os

file_path = r"C:\Games\D2s\build\main"
if not os.path.exists(file_path):
    print(f"Error: File not found at {file_path}")
    exit(1)

with open(file_path, "r", encoding="utf-8") as f:
    content = f.read()

# Target render_gauges logic block precisely
old_block = """    if (Newdemo_state == ND_STATE_RECORDING) {
        if (0) {
            int _op;
            for (_op = 0; _op < MAX_PLAYERS; _op++) {
                if (!Players[_op].connected) continue;
                newdemo_record_player_energy(f2ir(Players[_op].energy));
                newdemo_record_player_shields(f2ir(Players[_op].shields));
                newdemo_record_player_flags(Players[_op].flags);
                newdemo_record_multi_score(_op, Players[_op].score);
            }
        } else {
            newdemo_record_player_energy(energy);
        }
    }"""

new_block = """        if (Newdemo_state == ND_STATE_RECORDING) {
            if (is_observer()) {
                int _op;
                for (_op = 0; _op < MAX_PLAYERS; _op++) {
                    if (!Players[_op].connected) continue;
                    newdemo_record_player_stats(
                        f2ir(Players[_op].shields),
                        f2ir(Players[_op].energy),
                        Players[_op].score
                    );
                    newdemo_record_multi_kill(_op, Players[_op].num_kills_total);
                    newdemo_record_multi_death(_op);
                }
            } else {
                newdemo_record_player_energy(energy);
            }
        }"""

if old_block in content:
    content = content.replace(old_block, new_block)
    with open(file_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)
    print("Successfully patched CM_FULL_COCKPIT recording block in gauges.c!")
else:
    # Backup lookup check for different indentation/spacing forms
    if "newdemo_record_multi_score" in content:
        print("Target code structural pattern matches partially, but code may have slightly modified line spaces.")
    else:
        print("Block already patched or missing standard layout target signatures.")
