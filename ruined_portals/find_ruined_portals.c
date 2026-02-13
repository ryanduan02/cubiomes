#include "finders.h"
#include "util.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static inline int64_t sqr64(int64_t a) { return a * a; }

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <mc_version> <seed> [radius_chunks]\n", argv[0]);
        fprintf(stderr, "  mc_version examples: 1.16, 1.18, 1.20, 1.20.6\n");
        fprintf(stderr, "  Example: %s 1.20 12345 1000\n", argv[0]);
        return 1;
    }

    int mc = str2mc(argv[1]);
    if (mc <= 0)
    {
        fprintf(stderr, "Could not parse mc_version='%s'\n", argv[1]);
        return 2;
    }

    uint64_t seed = (uint64_t)strtoull(argv[2], NULL, 10);
    int radius_chunks = (argc >= 4) ? atoi(argv[3]) : 1000;
    int64_t R = (int64_t)radius_chunks * 16; // blocks

    Generator g;
    setupGenerator(&g, mc, 0);
    applySeed(&g, DIM_OVERWORLD, seed);

    // Spawn finder expects an Overworld generator with applied seed.
    Pos spawn = getSpawn(&g);

    const int st = Ruined_Portal;

    StructureConfig sc;
    if (!getStructureConfig(st, mc, &sc))
    {
        fprintf(stderr, "getStructureConfig failed for %s in %s\n",
                struct2str(st), mc2str(mc));
        return 3;
    }

    // Regions are measured in "regionSize" chunks for this structure type.
    const int regChunks = sc.regionSize;
    const int64_t regBlocks = (int64_t)regChunks * 16;

    // Conservative region-coordinate bounds around spawn-centered radius.
    int rx0 = (int)((spawn.x - R) / regBlocks) - 2;
    int rx1 = (int)((spawn.x + R) / regBlocks) + 2;
    int rz0 = (int)((spawn.z - R) / regBlocks) - 2;
    int rz1 = (int)((spawn.z + R) / regBlocks) + 2;

    int found = 0;

    for (int rz = rz0; rz <= rz1; rz++)
    for (int rx = rx0; rx <= rx1; rx++)
    {
        Pos p;
        if (!getStructurePos(st, mc, seed, rx, rz, &p))
            continue;

        // Distance filter in blocks.
        int64_t dx = (int64_t)p.x - spawn.x;
        int64_t dz = (int64_t)p.z - spawn.z;
        if (sqr64(dx) + sqr64(dz) > sqr64(R))
            continue;

        // Biome viability check. For ruined portals, flags=0 is correct.
        if (!isViableStructurePos(st, &g, p.x, p.z, 0))
            continue;

        printf("%s x=%d z=%d\n", struct2str(st), p.x, p.z);
        found++;
    }

    fprintf(stderr,
            "mc=%s seed=%" PRIu64 " spawn=(%d,%d) radius=%d chunks -> %d portals\n",
            mc2str(mc), seed, spawn.x, spawn.z, radius_chunks, found);

    return 0;
}