#include "format_detail.h"
namespace triage {
std::string json(const Report &r) {
    std::ostringstream s;
    s << "{\"schema_version\":\"1.0\",\"application_version\":\"1.0.0\",\"query\":{\"start\":"
      << quote(r.start) << ",\"end\":" << quote(r.end) << ",\"since_seconds\":" << r.options.seconds
      << ",\"maximum_level\":" << r.options.level << ",\"detail_limit\":" << r.options.limit
      << ",\"maximum_events\":" << max_events << ",\"logs\":[";
    for (size_t i = 0; i < r.channels.size(); ++i) {
        const auto &c = r.channels[i];
        if (i)
            s << ',';
        s << "{\"channel\":" << quote(c.name) << ",\"status\":" << quote(c.status)
          << ",\"retained_count\":" << c.count << ",\"win32_error\":" << c.error << '}';
    }
    s << "]},\"summary\":{\"retained_events\":" << r.events.size()
      << ",\"groups\":" << r.groups.size() << ",\"truncated\":" << (r.truncated ? "true" : "false")
      << ",\"exit_code\":" << r.exit_code << "},\"event_groups\":[";
    for (size_t i = 0; i < r.groups.size(); ++i) {
        const auto &g = r.groups[i];
        if (i)
            s << ',';
        s << "{\"provider\":" << quote(g.provider) << ",\"channel\":" << quote(g.channel)
          << ",\"event_id\":" << g.id << ",\"level\":" << g.level
          << ",\"count\":" << g.indices.size() << ",\"event_indices\":[";
        for (size_t k = 0; k < g.indices.size(); ++k) {
            if (k)
                s << ',';
            s << g.indices[k];
        }
        s << "]}";
    }
    s << "],\"events\":[";
    for (size_t i = 0; i < r.events.size(); ++i) {
        const auto &e = r.events[i];
        if (i)
            s << ',';
        s << "{\"timestamp\":" << quote(e.timestamp) << ",\"channel\":" << quote(e.channel)
          << ",\"provider\":" << quote(e.provider) << ",\"event_id\":" << e.id
          << ",\"version\":" << e.version << ",\"level\":" << e.level << ",\"task\":" << e.task
          << ",\"opcode\":" << e.opcode << ",\"keywords\":" << quote(e.keywords)
          << ",\"computer\":" << quote(e.computer) << ",\"record_id\":" << quote(e.record_id)
          << ",\"process_id\":" << e.pid << ",\"thread_id\":" << e.tid
          << ",\"message_status\":" << quote(e.message_status)
          << ",\"message\":" << quote(e.message) << ",\"data\":[";
        for (size_t k = 0; k < e.data.size(); ++k) {
            if (k)
                s << ',';
            s << "{\"name\":" << quote(e.data[k].first) << ",\"value\":" << quote(e.data[k].second)
              << '}';
        }
        s << "]}";
    }
    s << "],\"correlations\":[";
    for (size_t i = 0; i < r.clusters.size(); ++i) {
        if (i)
            s << ',';
        s << "{\"kind\":\"temporal_proximity\",\"window_seconds\":120,\"causality_established\":"
             "false,\"event_indices\":[";
        for (size_t k = 0; k < r.clusters[i].indices.size(); ++k) {
            if (k)
                s << ',';
            s << r.clusters[i].indices[k];
        }
        s << "]}";
    }
    s << "],\"warnings\":[";
    for (size_t i = 0; i < r.warnings.size(); ++i) {
        if (i)
            s << ',';
        s << quote(r.warnings[i]);
    }
    s << "],\"limitations\":[\"" << limitations << "\"]}\n";
    return s.str();
}

} // namespace triage
