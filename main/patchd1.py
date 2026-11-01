import os

filepath = "gameseq.c"

if not os.path.exists(filepath):
    print(f"ERROR: Could not find {filepath} in the current folder execution path.")
else:
    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        lines = f.readlines()

    found = False
    for i, line in enumerate(lines):
        if "action == OBSERVER_CONTROL_TOGGLE_MODE" in line:
            # Dynamically look ahead for the nested statements regardless of layout brackets
            start_idx = i - 1 if "{" in lines[i-1] else i
            
            # Find the true outer trailing bracket ending index safely
            end_idx = None
            for offset in range(3, 12):
                if "}" in lines[i + offset]:
                    # Match the closing segment brace safely
                    if "return 1;" in lines[i + offset - 1] or "return 1;" in lines[i + offset - 2]:
                        end_idx = i + offset + 1
                        break
            
            if end_idx is not None:
                new_logic = [
                    "    if (action == OBSERVER_CONTROL_TOGGLE_MODE)\n",
                    "    {\n",
                    "        if (is_observing_player())\n",
                    "        {\n",
                    "            if (Obs_at_distance == 0)\n",
                    "            {\n",
                    "                HUD_init_message_literal(HM_MULTI, \"Observing third person.\");\n",
                    "                Obs_at_distance = 1;\n",
                    "                init_cockpit();\n",
                    "            }\n",
                    "            else\n",
                    "            {\n",
                    "                reset_obs();\n",
                    "            }\n",
                    "        }\n",
                    "        else\n",
                    "        {\n",
                    "            cycle_observed_player(1, 0);\n",
                    "            HUD_init_message_literal(HM_MULTI, \"Observing first person.\");\n",
                    "            Obs_at_distance = 0;\n",
                    "            init_cockpit();\n",
                    "        }\n",
                    "        return 1;\n",
                    "    }\n"
                ]
                
                lines[start_idx:end_idx] = new_logic
                found = True
                break

    if found:
        with open(filepath, "w", encoding="utf-8") as f:
            f.writelines(lines)
        print("SUCCESS: gameseq.c camera cycle logic updated successfully.")
    else:
        print("ERROR: Target structural toggle configuration block not found in gameseq.c.")
