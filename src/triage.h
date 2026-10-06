#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace triage {
constexpr unsigned max_events = 10000;
constexpr unsigned max_details = 100;
constexpr unsigned max_xml_bytes = 1024 * 1024;
struct Options {
    unsigned seconds = 86400;
    unsigned limit = 10;
    unsigned level = 3;
    bool json = false, help = false, version = false, explicit_log = false;
    std::vector<std::wstring> logs{L"System", L"Application"};
};
struct Event {
    std::wstring timestamp, channel, provider, computer, message, message_status;
    std::wstring keywords, record_id;
    unsigned id = 0, version = 0, level = 0, task = 0, opcode = 0, pid = 0, tid = 0;
    std::uint64_t time = 0;
    std::vector<std::pair<std::wstring, std::wstring>> data;
};
struct Group {
    std::wstring provider, channel;
    unsigned id = 0, level = 0;
    std::vector<size_t> indices;
};
struct Cluster {
    std::vector<size_t> indices;
};
struct Channel {
    std::wstring name, status;
    unsigned count = 0, error = 0;
};
struct Report {
    Options options;
    std::wstring start, end;
    std::vector<Event> events;
    std::vector<Group> groups;
    std::vector<Cluster> clusters;
    std::vector<Channel> channels;
    std::vector<std::wstring> warnings;
    bool truncated = false;
    int exit_code = 0;
};
// Testable acquisition budgets can only narrow the production hard limits.
struct Budget {
    unsigned events = max_events;
    unsigned milliseconds = 30000;
    size_t text_bytes = size_t{32} * 1024 * 1024;
};
Options parse_cli(const std::vector<std::wstring> &args);
std::wstring query(const Options &options, const std::wstring &start, const std::wstring &end);
Event parse_xml(const std::wstring &xml);
void analyze(Report &report);
void correlate(Report &report);
Report acquire(const Options &options, Budget budget = {});
std::string utf8(const std::wstring &value);
std::string json(const Report &report);
std::string human(const Report &report);
std::wstring iso_time(std::uint64_t ticks);
std::uint64_t parse_time(const std::wstring &value);
const char *help_text();
} // namespace triage
