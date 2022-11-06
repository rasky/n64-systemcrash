
void crash_ld_rcp(void)
{
    // Doing a 64-bit from RCP area will crash the CPU
    (void)*(volatile uint64_t *)0xA4300000;
}

void crash_ld_pi1(void)
{
    // Doing a 64-bit from PI area will crash the CPU
    (void)*(volatile uint64_t *)0xB0000000;
}

void crash_ld_pi2(void)
{
    // Doing a 64-bit from PI area will crash the CPU
    (void)*(volatile uint64_t *)0xA7000000;
}
