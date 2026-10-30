import os

def purify_udp_requests():
    print("--- PURIFYING NET_UDP RATE-LIMIT SYMBOLS ---")
    
    udp_path = "net_udp.c"
    
    if os.path.exists(udp_path):
        with open(udp_path, "r", encoding="utf-8") as f:
            content = f.read()

        # Target the entire cluttered loop area cleanly
        old_cluttered_block = """void net_udp_send_game_info(struct _sockaddr sender_addr, ubyte info_upid, ubyte send_to_observers, uint player_token)
{
    //static fix64 last_full_req_time = 0;
    //if (timer_query() < last_full_req_time+(F1_0/5)) // answer 5 times per second max
    //	break;
    //last_full_req_time = timer_query();

    //static fix64 last_lite_req_time = 0;
    //if (timer_query() < last_lite_req_time+(F1_0/8))// answer 8 times per second max
    //	break;
    //last_lite_req_time = timer_query();\t

    // Send game info to someone who requested it

    int len = 0;"""

        # Replace it with a pristine, fully clear function entrance layout
        new_clean_block = """void net_udp_send_game_info(struct _sockaddr sender_addr, ubyte info_upid, ubyte send_to_observers, uint player_token)
{
    /* Send game info to someone who requested it */
    int len = 0;"""

        if old_cluttered_block in content:
            content = content.replace(old_cluttered_block, new_clean_block)
            with open(udp_path, "w", encoding="utf-8", newline="\n") as f:
                f.write(content)
            print("[SUCCESS] Rogue statement blocks successfully scrubbed from net_udp.c.")
        else:
            # Flexible substring split lookup pass in case internal tabs/spaces match unevenly
            print("[INFO] Direct layout match missed. Initiating flexible split filter...")
            parts = content.split("void net_udp_send_game_info(struct _sockaddr sender_addr, ubyte info_upid, ubyte send_to_observers, uint player_token)")
            if len(parts) > 1:
                sub_parts = parts[1].split("int len = 0;")
                if len(sub_parts) > 1:
                    content = parts[0] + "void net_udp_send_game_info(struct _sockaddr sender_addr, ubyte info_upid, ubyte send_to_observers, uint player_token)\\n{\\n    int len = 0;" + sub_parts[1]
                    with open(udp_path, "w", encoding="utf-8", newline="\n") as f:
                        f.write(content)
                    print("[SUCCESS] net_udp.c rate loops fully aligned via fallback pass.")
    else:
        print(f"[ERROR] Could not find file: {os.path.abspath(udp_path)}")

if __name__ == "__main__":
    purify_udp_requests()
