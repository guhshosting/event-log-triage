#include "windows_detail.h"
namespace triage {
using namespace detail;
Report acquire(const Options &o, Budget budget) {
    if (o.logs.empty() || o.logs.size() > 2 || !budget.events || budget.events > max_events ||
        !budget.milliseconds || budget.milliseconds > 30000 || !budget.text_bytes ||
        budget.text_bytes > size_t{32} * 1024 * 1024)
        throw std::invalid_argument("invalid acquisition budget");
    Report r;
    r.options = o;
    FILETIME now{};
    GetSystemTimeAsFileTime(&now);
    ULARGE_INTEGER u{};
    u.LowPart = now.dwLowDateTime;
    u.HighPart = now.dwHighDateTime;
    r.end = iso_time(u.QuadPart);
    r.start = iso_time(u.QuadPart - static_cast<std::uint64_t>(o.seconds) * 10000000);
    auto xpath = query(o, r.start, r.end);
    auto deadline = GetTickCount64() + budget.milliseconds;
    size_t text_bytes = 0;
    unsigned attempted = 0, failures = 0, formatted = 0;
    const unsigned per_channel = budget.events / static_cast<unsigned>(o.logs.size());
    for (const auto &log : o.logs) {
        Channel c;
        c.name = log;
        c.status = L"complete";
        if (GetTickCount64() >= deadline) {
            c.status = L"budget_exhausted";
            r.truncated = true;
            r.channels.push_back(c);
            continue;
        }
        Handle q(EvtQuery(nullptr, log.c_str(), xpath.c_str(),
                          static_cast<DWORD>(EvtQueryChannelPath) |
                              static_cast<DWORD>(EvtQueryReverseDirection)));
        if (!q) {
            c.error = GetLastError();
            c.status = L"unavailable";
            r.warnings.push_back(L"Cannot query " + log + L" (Win32 " + std::to_wstring(c.error) +
                                 L").");
            if (o.explicit_log)
                r.exit_code = (c.error == ERROR_EVT_CHANNEL_NOT_FOUND ||
                               c.error == ERROR_ACCESS_DENIED || c.error == ERROR_FILE_NOT_FOUND)
                                  ? 3
                                  : 4;
            r.channels.push_back(c);
            continue;
        }
        unsigned channel_attempts = 0;
        while (channel_attempts < per_channel && attempted < budget.events &&
               GetTickCount64() < deadline && text_bytes < budget.text_bytes) {
            EVT_HANDLE raw_event = nullptr;
            DWORD returned = 0;
            if (!EvtNext(q.get(), 1, &raw_event, 100, 0, &returned)) {
                auto err = GetLastError();
                // A slow local service may exceed one polling interval. Retry only
                // while the acquisition deadline permits; no continuous subscription.
                if (err == ERROR_TIMEOUT)
                    continue;
                if (err != ERROR_NO_MORE_ITEMS) {
                    c.error = err;
                    c.status = L"query_failed";
                    r.exit_code = 4;
                    r.warnings.push_back(L"Event iteration failed for " + log + L" (Win32 " +
                                         std::to_wstring(err) + L").");
                }
                break;
            }
            Handle event(raw_event);
            ++attempted;
            ++channel_attempts;
            try {
                auto xml = render(event.get());
                auto e = parse_xml(xml);
                format_message(e, event.get());
                // Account generously for all retained UTF-16 text plus containers.
                size_t bytes = xml.size() * 4 + e.message.size() * 2 + 2048;
                if (bytes > budget.text_bytes - text_bytes) {
                    r.truncated = true;
                    c.status = L"budget_exhausted";
                    break;
                }
                text_bytes += bytes;
                if (e.message_status == L"formatted")
                    ++formatted;
                r.events.push_back(std::move(e));
                ++c.count;
            } catch (const std::runtime_error &) {
                ++failures;
            }
        }
        if (channel_attempts == per_channel || attempted == budget.events ||
            GetTickCount64() >= deadline || text_bytes >= budget.text_bytes) {
            r.truncated = true;
            if (c.status == L"complete")
                c.status = L"budget_exhausted";
        }
        r.channels.push_back(c);
    }
    if (!o.explicit_log && std::none_of(r.channels.begin(), r.channels.end(), [](const Channel &c) {
            return c.status == L"complete" || c.status == L"budget_exhausted";
        }))
        r.exit_code = 4;
    if (failures) {
        r.warnings.push_back(std::to_wstring(failures) +
                             L" events could not be rendered/parsed or exceeded XML limits.");
        if (r.events.empty())
            r.exit_code = 4;
    }
    if (formatted < r.events.size())
        r.warnings.push_back(
            std::to_wstring(r.events.size() - formatted) +
            L" retained events lack locally formatted messages; inspect structured fields.");
    if (r.truncated)
        r.warnings.push_back(
            L"Acquisition budget reached; recurrence and clusters describe a partial sample. "
            L"Per-channel event quotas prevent one channel consuming the full count budget.");
    return r;
}

} // namespace triage
