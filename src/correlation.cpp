#include "triage.h"
namespace triage {
void correlate(Report &r) {
    r.clusters.clear();
    // Fixed-span clusters avoid transitive chains stretching across hours.
    for (size_t i = 0; i < r.events.size();) {
        size_t j = i + 1;
        while (j < r.events.size() && r.events[i].time && r.events[j].time &&
               r.events[i].time - r.events[j].time <= 120ULL * 10000000)
            ++j;
        if (j - i >= 2) {
            bool distinct = false;
            for (size_t k = i + 1; k < j; ++k)
                if (r.events[k].provider != r.events[i].provider ||
                    r.events[k].id != r.events[i].id || r.events[k].channel != r.events[i].channel)
                    distinct = true;
            if (distinct) {
                Cluster c;
                for (size_t k = i; k < j; ++k)
                    c.indices.push_back(k);
                r.clusters.push_back(std::move(c));
            }
        }
        i = j;
    }
}
} // namespace triage
