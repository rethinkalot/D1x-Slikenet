import os

target_udp = r"C:\games\d1s\main\net_udp.c"

def apply_unconditional_baseline_revert():
    if not os.path.exists(target_udp):
        return False
        
    with open(target_udp, "r", encoding="utf-8", errors="ignore") as f:
        code = f.read()

    # 1. Force revert the timeout condition unconditionally
    if "else if (0)" in code or "Disabled timeout kicks" in code:
        # Locate the exact line boundaries for the timeout section
        start_idx = code.find("                else if (")
        if start_idx != -1:
            end_idx = code.find("\n", start_idx)
            code = code[:start_idx] + "                else if ((time - Netgame.players[i].LastPacketTime) > UDP_TIMEOUT)" + code[end_idx:]

    # 2. Force wipe out any handshake unlock modifications inside the pdata read function
    old_pdata_segment = """    pd->serial = packet_num;
    
    // HANDSHAKE UNLOCK: Instantly release verification state machine loops for internal loopback channels
    extern int VerifyPlayerJoined;
    if (VerifyPlayerJoined != -1 && pd->Player_num == VerifyPlayerJoined) {
        VerifyPlayerJoined = -1;
    }"""
    
    if old_pdata_segment in code:
        code = code.replace(old_pdata_segment, "    pd->serial = packet_num;")

    # 3. Clean slice rewrite of the resend sequence if still modified
    if "Forced clear to prevent the" in code:
        start_resend = code.find("    // Forced clear to prevent the")
        if start_resend != -1:
            # Look for the matching closing bracket block
            end_resend = code.find("}", start_resend)
            if end_resend != -1:
                clean_resend = """    if (VerifyPlayerJoined!=-1 && time >= last_resync_time+F1_0)
    {
        last_resync_time = time;
        net_udp_resend_sync_due_to_packet_loss(); // This will resend to UDP_sync_player
    }"""
                code = code[:start_resend] + clean_resend + code[end_resend+1:]

    with open(target_udp, "w", encoding="utf-8") as f:
        f.write(code)
    return True

if apply_unconditional_baseline_revert():
    print("SUCCESS")
