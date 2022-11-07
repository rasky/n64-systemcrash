#include <stdio.h>
#include <stdalign.h>
#include <libdragon.h>

#define SYS_RESET_TYPE  ((uint32_t*)0x8000030C)

#define SRAM_BASE_ADDRESS 0x08000000
#define SRAM_STATE_OFFSET 0x100

#define STATE_ID          0xA7B4C3D2

enum {
    PAGE_INTRO,
    PAGE_TEST,
    PAGE_RESULTS,
    PAGE_ERROR_SRAM,
    PAGE_ERROR_RESET,
};

struct {
    uint32_t id;
    uint32_t curtest;
    uint32_t rand_state;
    struct {
        bool passed;
    } tests[256];
} State alignas(8);

static uint32_t randx(void) {
	uint32_t x = State.rand_state;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 5;
	return State.rand_state = x;
}

#define SRAND(n) ({ State.rand_state = (n); if (!State.rand_state) State.rand_state = 1; })

// RANDN(n): generate a random number from 0 to n-1
#define RANDN(n) ({ \
	__builtin_constant_p((n)) ? \
		(randx()%(n)) : \
		(uint32_t)(((uint64_t)randx() * (n)) >> 32); \
})

#include "crash_memory.c"
#include "crash_rdp.c"

typedef struct {
    void (*crash)(void);
    const char *name, *desc;
} crasher_t;

crasher_t crashers[] = {
    { crash_rcp_cached,      "Cached read RCP",      "Doing a cached read from RCP MMIO area" },
    { crash_pi1_cached,      "Cached read PI #1",    "Doing a cached read from PI area #1" },
    { crash_pi2_cached,      "Cached read PI #2",    "Doing a cached read from PI area #2" },
    { crash_si_cached,       "Cached read PI #2",    "Doing a cached read from SI area" },
    { crash_pi3_cached,      "Cached read PI #2",    "Doing a cached read from PI area #3" },
    { crash_pi4_cached,      "Cached read PI #2",    "Doing a cached read from PI area #4" },
    { crash_unmapped_cached, "Cached read PI #2",    "Doing a cached read from unmapped area" },

    { crash_ld_rcp,      "LD RCP",       "Doing a 64-bit from RCP area" },
    { crash_ld_pi1,      "LD PI #1",     "Doing a 64-bit from PI area #1"  },
    { crash_ld_pi2,      "LD PI #2",     "Doing a 64-bit from PI area #2"  },
    { crash_ld_si,       "LD PI #2",     "Doing a 64-bit from SI area"  },
    { crash_ld_pi3,      "LD PI #3",     "Doing a 64-bit from PI area #3"  },
    { crash_ld_pi4,      "LD PI #4",     "Doing a 64-bit from PI area #4"  },
    { crash_ld_unmapped, "LD Unmapped",  "Doing a 64-bit from unmapped area"  },

    { crash_rdp_loadtile_4bpp, "RDP: LOAD_TILE 4bpp", "Loading a 4bpp texture via LOAD_TILE will crash the RDP" },
    { crash_rdp_fill_4bpp, "RDP: FILL on 4bpp", "FILL mode is not supported on a 4bpp framebuffer" },
};

#define NUM_CRASHERS (sizeof(crashers) / sizeof(crashers[0]))

void reset_state(void)
{
    memset(&State, 0, sizeof(State));
    State.id = STATE_ID;
}

void load_state(void)
{
    data_cache_hit_writeback_invalidate(&State, sizeof(State));
    dma_read_raw_async(&State, SRAM_BASE_ADDRESS+SRAM_STATE_OFFSET, sizeof(State));
    dma_wait();

    if (State.id == STATE_ID)
        return;
    reset_state();
}

void save_state(void)
{
    data_cache_hit_writeback_invalidate(&State, sizeof(State));
    dma_write_raw_async(&State, SRAM_BASE_ADDRESS+SRAM_STATE_OFFSET, sizeof(State));
    dma_wait();
}

void wait_vblank(void)
{
    while (*(volatile uint32_t*)(0xA4400010) != 0x2) {}
}

bool run_next_crasher(void)
{
    if (State.curtest >= NUM_CRASHERS) return false;

    crasher_t *c = &crashers[State.curtest];

    // Save state before running the crasher, with incremented index. This allows
    // next test to run after the crash (if the crash happens). We also pretend
    // the test passed.
    State.tests[State.curtest].passed = true;
    State.curtest++;
    randx(); // Advance the random state
    save_state();

    debugf("Running crasher #%ld: %s\n", State.curtest-1, c->name);

    c->crash();

    // If we get here, the crash didn't happen. Mark the test as failed.
    debugf("    FAILED (no crash happened)\n");
    State.tests[State.curtest-1].passed = false;
    return true;
}

void header(surface_t *disp)
{
    char sbuf[32];
    graphics_fill_screen(disp, 0);
    graphics_draw_text(disp, 320-70, 10, "n64-systemcrash");
    graphics_draw_text(disp, 320-70, 20, "v1.0 - by Rasky");
    sprintf(sbuf, "Platform: %s", sys_bbplayer() ? "iQue" : "N64");
    graphics_draw_text(disp, 270-15, 40, sbuf);
}

bool check_sram_present(void)
{
    uint8_t test[16] alignas(8) = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 } ;
    uint8_t res[16] alignas(8) = { 0 };

    data_cache_hit_writeback_invalidate(test, sizeof(test));
    data_cache_hit_writeback_invalidate(res, sizeof(res));

    dma_write_raw_async(test, SRAM_BASE_ADDRESS, sizeof(test));
    dma_wait();

    dma_read_raw_async(res, SRAM_BASE_ADDRESS, sizeof(res));
    dma_wait();

    return memcmp(test, res, sizeof(test)) == 0;
}

int page_intro(void)
{
    display_context_t disp;
    while (!(disp = display_lock())) {}
    header(disp);
    graphics_draw_text(disp, 40, 70, "This ROM contains a testsuite that verifies several different");
    graphics_draw_text(disp, 40, 80, "ways to crash a Nintendo 64.");

    graphics_draw_text(disp, 40, 100, "After each test, it will ask you to power-cycle the console to");
    graphics_draw_text(disp, 40, 110, "continue to next test, until the testsuite is finished.");

    graphics_draw_text(disp, 40, 160, "Press START to begin the testsuite.");
    display_show(disp);

    struct controller_data cont = get_keys_down();
    if (cont.c[0].start)
    {
        SRAND(TICKS_READ());
        debugf("Starting testsuite (random seed: %08lx)\n", State.rand_state);
        return PAGE_TEST;
    }
    return PAGE_INTRO;
}

int page_test(void)
{
    if (State.curtest == NUM_CRASHERS) {
        return PAGE_RESULTS;
    }

    display_context_t disp;
    while (!(disp = display_lock())) {}

    char sbuf[32];
    graphics_fill_screen(disp, 0);
    sprintf(sbuf, "Running test [%ld / %d]", State.curtest+1, NUM_CRASHERS);
    graphics_draw_text(disp, 40, 110, sbuf);
    graphics_draw_text(disp, 40, 120, crashers[State.curtest].desc);

    graphics_draw_text(disp, 40, 160, "Turn off and on the console to continue.");
    graphics_draw_text(disp, 40, 170, "Keep Z pressed while resetting to abort the testsuite.");
    display_show(disp);
    wait_vblank();

    // Now run the crasher
    run_next_crasher();

    return PAGE_TEST;
}

int page_results()
{
    static bool first_run = true;
    static int passed = 0;

    if (first_run) {
        for (int i=0; i<NUM_CRASHERS; i++) {
            if (State.tests[i].passed)
                passed++;
        }
    }

    reset_state();
    save_state();

    display_context_t disp;
    while (!(disp = display_lock())) {}
    header(disp);

    graphics_draw_text(disp, 40, 70, "RESULTS:");

    char sbuf[128];
    sprintf(sbuf, "Tests: %d, Passed: %d. Success rate: %d%%", NUM_CRASHERS, passed, (passed * 100) / NUM_CRASHERS);
    if (first_run) debugf("%s\n", sbuf);
    graphics_draw_text(disp, 40, 80, sbuf);

    display_show(disp);

    first_run = false;
    return PAGE_RESULTS;
}

int page_error_sram()
{
    display_context_t disp;
    while (!(disp = display_lock())) {}
    header(disp);

    graphics_draw_text(disp, 150, 70, "No SRAM detected!");
    graphics_draw_text(disp, 40, 90, "This means that your console is not able to access the SRAM.");
    graphics_draw_text(disp, 40, 100, "This is a fatal error, and the testsuite cannot continue.");
    display_show(disp);

    return PAGE_ERROR_SRAM;
}

int page_error_reset()
{
    display_context_t disp;
    while (!(disp = display_lock())) {}

    graphics_draw_text(disp, 150, 70, "Do not press the RESET button!");
    graphics_draw_text(disp, 40, 90, "Unfortunately, the RESET button cannot recover from all crash situations.");
    graphics_draw_text(disp, 40, 100, "Always power-cycle the console, by turning it off and on.");
    display_show(disp);

    return PAGE_ERROR_RESET;
}

int main(void)
{
    debug_init_isviewer();
    debug_init_usblog();
    debugf("n64-systemcrash is alive\n");

#if 0
    rdpq_init();
    rdpq_debug_start();
    rdpq_debug_log(true);

    surface_t surf = surface_alloc(FMT_RGBA32, 32, 32);
    rdpq_set_color_image(&surf);
    rdpq_set_mode_copy(false);
    rdpq_texture_rectangle(TILE0, 0, 0, surf.width, surf.height, 0, 0, 1, 1);
    rspq_wait();
#endif

    dfs_init(DFS_DEFAULT_LOCATION);
    controller_init();
    display_init(RESOLUTION_640x240, DEPTH_32_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);

    int page = PAGE_INTRO;
    if (!check_sram_present())
        page = PAGE_ERROR_SRAM;
    else if (*SYS_RESET_TYPE != 0)
        page = PAGE_ERROR_RESET;
    else {
        struct controller_data cont;
        controller_read(&cont);

        if (cont.c->Z)
            reset_state();
        else 
        {
            load_state();
            debugf("Loaded state: curtest=%ld\n", State.curtest);
            if (State.curtest > 0)
                page = PAGE_TEST;
        }
    }

    while(1) {
        controller_scan();

        switch (page) {
            case PAGE_INTRO:
                page = page_intro();
                break;
            case PAGE_TEST:
                page = page_test();
                break;
            case PAGE_RESULTS:
                page = page_results();
                break;
            case PAGE_ERROR_SRAM:
                page = page_error_sram();
                break;
            case PAGE_ERROR_RESET:
                page = page_error_reset();
                break;
        }
    }
}
