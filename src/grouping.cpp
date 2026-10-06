#include "format_detail.h"
namespace triage {
void analyze(Report &r) {
    r.groups.clear();
    r.clusters.clear();
    std::sort(r.events.begin(), r.events.end(), [](const Event &a, const Event &b) {
        return std::tie(a.time, a.channel, a.record_id) > std::tie(b.time, b.channel, b.record_id);
    });
    std::map<std::tuple<std::wstring, std::wstring, unsigned, unsigned>, size_t> keys;
    for (size_t i = 0; i < r.events.size(); ++i) {
        const auto &e = r.events[i];
        auto key = std::make_tuple(e.provider, e.channel, e.id, e.level);
        auto it = keys.find(key);
        if (it == keys.end()) {
            keys[key] = r.groups.size();
            r.groups.push_back({e.provider, e.channel, e.id, e.level, {i}});
        } else
            r.groups[it->second].indices.push_back(i);
    }
    std::stable_sort(r.groups.begin(), r.groups.end(), [](const Group &a, const Group &b) {
        return a.indices.size() > b.indices.size();
    });
    correlate(r);
}
} // namespace triage
