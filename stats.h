#ifndef HOKUYO_STATS_H
#define HOKUYO_STATS_H

namespace hokuyo {
    struct Statistics {
        uint32_t count;
        uint32_t max;
        uint32_t min;
        uint32_t mean;
        uint32_t dev;
    };
}
#endif

