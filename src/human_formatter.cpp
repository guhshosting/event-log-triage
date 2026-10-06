#include "format_detail.h"
namespace triage {
std::string human(const Report &r) {
    std::ostringstream s;
    s << "event-log-triage 1.0.0\nUTC range: " << utf8(r.start) << " to " << utf8(r.end) << "\n";
    for (const auto &c : r.channels)
        s << "Log " << safe(c.name) << ": " << safe(c.status) << ", retained " << c.count
          << ", error " << c.error << '\n';
    s << "Retained events: " << r.events.size() << "; groups: " << r.groups.size()
      << "; temporal clusters: " << r.clusters.size()
      << "\n\nRecurring provider/event ID groups:\n";
    unsigned shown = 0;
    for (const auto &g : r.groups)
        if (g.indices.size() > 1 && shown++ < r.options.limit)
            s << "  " << safe(g.provider) << " / " << g.id << " [" << safe(g.channel) << "] level "
              << g.level << ": " << g.indices.size() << " retained events\n";
    if (!shown)
        s << "  None.\n";
    s << "\nRecent events (critical/error first, publisher level 0 also retained):\n";
    std::vector<size_t> order;
    order.reserve(r.events.size());
    for (size_t i = 0; i < r.events.size(); ++i)
        order.push_back(i);
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        const auto rank = [](unsigned level) { return level == 0 ? 6U : level; };
        return rank(r.events[a].level) < rank(r.events[b].level);
    });
    for (size_t k = 0; k < std::min<size_t>(order.size(), r.options.limit); ++k) {
        const auto &e = r.events[order[k]];
        s << "  " << safe(e.timestamp) << ' ' << safe(e.channel) << " / " << safe(e.provider)
          << " ID " << e.id << " level " << e.level << " record " << safe(e.record_id) << "\n    "
          << (e.message.empty() ? "Message unavailable: " + safe(e.message_status)
                                : safe(e.message))
          << '\n';
        if (e.message.empty())
            for (size_t d = 0; d < std::min<size_t>(e.data.size(), 4); ++d)
                s << "    " << safe(e.data[d].first) << ": " << safe(e.data[d].second) << '\n';
    }
    s << "\nTemporally associated events (120 seconds; no causality established):\n";
    for (size_t i = 0; i < std::min<size_t>(r.clusters.size(), r.options.limit); ++i) {
        const auto &c = r.clusters[i];
        s << "  " << safe(r.events[c.indices.back()].timestamp) << " to "
          << safe(r.events[c.indices.front()].timestamp) << ": " << c.indices.size() << " events; ";
        for (size_t k = 0; k < std::min<size_t>(c.indices.size(), 4); ++k) {
            const auto &e = r.events[c.indices[k]];
            s << safe(e.provider) << '/' << e.id << " [" << safe(e.channel) << "] ";
        }
        s << '\n';
    }
    if (r.clusters.empty())
        s << "  None.\n";
    for (const auto &w : r.warnings)
        s << "Warning: " << safe(w) << '\n';
    s << "Limitations: " << limitations << '\n';
    return s.str();
}

} // namespace triage
