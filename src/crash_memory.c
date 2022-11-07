
enum MemArea {
    MEMAREA_RCP,
    MEMAREA_PI1,
    MEMAREA_PI2,
    MEMAREA_SI,
    MEMAREA_PI3,
    MEMAREA_PI4,
    MEMAREA_UNMAPPED,
};

#define RAND_INTERVAL(min, max) ((min) + RANDN((max) - (min) + 1))

uint32_t rand_phys_address(enum MemArea area) {
    switch (area) {
    case MEMAREA_RCP:      return RAND_INTERVAL(0x04000000, 0x04FFFFFF) & ~7;
    case MEMAREA_PI1:      return RAND_INTERVAL(0x05000000, 0x0FFFFFFF) & ~7;
    case MEMAREA_PI2:      return RAND_INTERVAL(0x10000000, 0x1FBFFFFF) & ~7;
    case MEMAREA_SI:       return RAND_INTERVAL(0x1FC00000, 0x1FCFFFFF) & ~7;
    case MEMAREA_PI3:      return RAND_INTERVAL(0x1FD00000, 0x1FFFFFFF) & ~7;
    case MEMAREA_PI4:      return RAND_INTERVAL(0x20000000, 0x7FFFFFFF) & ~7;
    case MEMAREA_UNMAPPED: return RAND_INTERVAL(0x80000000, 0xFFFFFFFF) & ~7;
    default: assert(0);
    }
}

uint64_t rand_uncached_vaddr(enum MemArea area) {
    uint32_t addr = rand_phys_address(area);
    // Convert to virtual address using the uncached direct-mapped segment.
    // Use a 32-bit compatible addressing mode when possible so that the
    // test also runs on emulators that don't yet support 64-bit addressing.
    if (addr < 0x20000000)
        return addr | 0xFFFFFFFFA0000000ull;
    else
        return addr | 0x9000000000000000ull;
}

uint64_t rand_cached_vaddr(enum MemArea area) {
    uint32_t addr = rand_phys_address(area);
    if (addr < 0x20000000)
        return addr | 0xFFFFFFFF80000000ull;
    else
        return addr | 0x8000000000000000ull;
}

uint64_t read_64bit(uint64_t vaddr)
{
    // Perform a 64-bit read from the given 64-bit virtual address.
    // In the general case (that is, when the 64-bit vaddr isn't
    // a sign-extended 32-bit vaddr), we cannot do this in C, casting
    // to a uint64_t* pointer, because the libdragon toolchain uses 32-bit
    // pointers. So we need to use inline assembly to perform the read.
    uint64_t value;
    asm volatile (
        "ld %[value], 0(%[vaddr])  \n" :
        [value] "=r" (value):
        [vaddr] "r" (vaddr)
    );
    return value;
}

void crash_ld_rcp(void)      { (void)read_64bit(rand_uncached_vaddr(MEMAREA_RCP)); }
void crash_ld_pi1(void)      { (void)read_64bit(rand_uncached_vaddr(MEMAREA_PI1)); }
void crash_ld_pi2(void)      { (void)read_64bit(rand_uncached_vaddr(MEMAREA_PI2)); }
void crash_ld_si(void)       { (void)read_64bit(rand_uncached_vaddr(MEMAREA_SI)); }
void crash_ld_pi3(void)      { (void)read_64bit(rand_uncached_vaddr(MEMAREA_PI3)); }
void crash_ld_pi4(void)      { (void)read_64bit(rand_uncached_vaddr(MEMAREA_PI4)); }
void crash_ld_unmapped(void) { (void)read_64bit(rand_uncached_vaddr(MEMAREA_UNMAPPED)); }

void crash_rcp_cached(void)      { (void)read_64bit(rand_cached_vaddr(MEMAREA_RCP)); }
void crash_pi1_cached(void)      { (void)read_64bit(rand_cached_vaddr(MEMAREA_PI1)); }
void crash_pi2_cached(void)      { (void)read_64bit(rand_cached_vaddr(MEMAREA_PI2)); }
void crash_si_cached(void)       { (void)read_64bit(rand_cached_vaddr(MEMAREA_SI)); }
void crash_pi3_cached(void)      { (void)read_64bit(rand_cached_vaddr(MEMAREA_PI3)); }
void crash_pi4_cached(void)      { (void)read_64bit(rand_cached_vaddr(MEMAREA_PI4)); }
void crash_unmapped_cached(void) { (void)read_64bit(rand_cached_vaddr(MEMAREA_UNMAPPED)); }
