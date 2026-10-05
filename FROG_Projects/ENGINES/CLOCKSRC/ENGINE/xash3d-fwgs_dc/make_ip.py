# Precise byte-aligned Dreamcast IP.BIN generator
with open("IP.BIN", "wb") as f:
    # Initialize a blank 32KB sector filled with null bytes
    ip_sector = bytearray(32768)

    # Write strings at their strict hardware offset positions
    ip_sector[0:16]   = b"SEGA ENTERPRISES".ljust(16)
    ip_sector[16:32]  = b"SEGA KATANA     ".ljust(16)
    ip_sector[32:48]  = b"CD-ROM1/1       ".ljust(16)
    ip_sector[48:64]  = b"JUE             ".ljust(16)
    ip_sector[64:80]  = b"E000F10         ".ljust(16)
    ip_sector[80:96]  = b"T0000           ".ljust(16)
    ip_sector[96:112] = b"1ST_READ.BIN    ".ljust(16)
    ip_sector[112:128]= b"internets       ".ljust(16)
    ip_sector[128:144]= b"Xash3D DC       ".ljust(16)

    f.write(ip_sector)
print("IP.BIN successfully generated with exact hardware offsets!")
