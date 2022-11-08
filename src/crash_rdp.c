#include <stdlib.h>

uint64_t sync_full = 0xe900000000000000ull;

void crash_rdp_packets(const char *fn)
{
    uint64_t *buf = malloc_uncached_aligned(16, 1024);
    int numcmds = 0;

    FILE *f = fopen(fn, "r");
    assertf(f, "Failed to open %s", fn);

    while (!feof(f)) {
        char line[256] = {0};
        if (!fgets(line, sizeof(line), f))
            break;
        if (line[0] == '#') continue;
        char *end;
        buf[numcmds++] = strtoull(line, &end, 16);
    }
    fclose(f);

    while (*DP_STATUS & (DP_STATUS_DMA_BUSY | DP_STATUS_START_VALID | DP_STATUS_END_VALID)) {}

    // Now execute the RDP buffer
    *DP_START = PhysicalAddr(buf);
    *DP_END = PhysicalAddr(buf + numcmds);

    while (*DP_STATUS & (DP_STATUS_DMA_BUSY | DP_STATUS_START_VALID | DP_STATUS_END_VALID)) {}

    // Execute the SYNC_FULL buffer. We force a sync_full from a separate buffer
    // to allow emulators to handle RDP crashes asynchronously at the end of the
    // offending command buffer, rather than really crashing the RDP at the right point.
    data_cache_hit_writeback(&sync_full, 8);
    *DP_START = PhysicalAddr(&sync_full);
    *DP_END = PhysicalAddr(&sync_full + 1);

    // Wait for the RDP to finish. This will trigger an endless loop
    // if the RDP crashes
    while (*DP_STATUS & DP_STATUS_BUSY) {}

    free_uncached(buf);
}

void crash_rdp_loadtile_4bpp(void) { crash_rdp_packets("rom:/loadtile_4bpp.rdp"); }
void crash_rdp_fill_4bpp(void)     { crash_rdp_packets("rom:/fill_4bpp.rdp"); }
void crash_rdp_copy_32bpp(void)    { crash_rdp_packets("rom:/copy_32bpp.rdp"); }
void crash_rdp_fill_readen(void)   { crash_rdp_packets("rom:/fill_readen.rdp"); }
void crash_rdp_fill_zcmp(void)     { crash_rdp_packets("rom:/fill_zcmp.rdp"); }
void crash_rdp_fill_zupd(void)     { crash_rdp_packets("rom:/fill_zupd.rdp"); }
void crash_rdp_fill_zprim(void)    { crash_rdp_packets("rom:/fill_zprim.rdp"); }
