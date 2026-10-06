#include "triage.h"
#include "windows_detail.h"
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <windows.h>

void check(bool ok, const char *name) {
    if (!ok) {
        std::fputs("Check failed: ", stderr);
        std::fputs(name, stderr);
        std::fputc('\n', stderr);
        std::exit(1);
    }
}
template <class F> void rejects(F f) {
    bool caught = false;
    try {
        f();
    } catch (const std::exception &) {
        caught = true;
    }
    check(caught, "expected rejection");
}
int main(int argc, char **argv) {
    try {
        if (argc > 1 && std::string(argv[1]) == "--nonadmin-child") {
            SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
            PSID admin = nullptr;
            check(AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                           DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                           &admin) != FALSE,
                  "admin SID");
            BOOL member = TRUE;
            BOOL ok = CheckTokenMembership(nullptr, admin, &member);
            FreeSid(admin);
            check(ok && !member, "child must lack enabled admin SID");
            auto r = triage::acquire(triage::parse_cli({L"--since", L"2h"}));
            check(r.exit_code == 0 && r.channels.size() == 2, "nonadmin local queries");
            return 0;
        }
        if (argc > 1 && std::string(argv[1]) == "--bounds") {
            auto o = triage::parse_cli(
                {L"--since", L"31d", L"--log", L"System", L"--level", L"verbose"});
            auto r = triage::acquire(o, {2, 30000, size_t{32} * 1024 * 1024});
            check(r.exit_code == 0 && r.events.size() <= 2 && r.truncated, "event count budget");
            auto memory_sample = triage::acquire(o, {2, 30000, 1});
            check(memory_sample.events.empty() && memory_sample.truncated, "text memory budget");
            auto empty =
                triage::acquire(triage::parse_cli({L"--since", L"1m", L"--log", L"Application"}));
            check(empty.exit_code == 0, "empty recent query");
            return 0;
        }
        if (argc > 1 && std::string(argv[1]) == "--unavailable") {
            auto o = triage::parse_cli({L"--log", L"event-log-triage-nonexistent-channel-947208"});
            auto r = triage::acquire(o);
            check(r.exit_code == 3, "missing channel status");
            return 0;
        }
        check(triage::parse_cli({}).seconds == 86400, "default window");
        check(triage::parse_cli({L"--since", L"7d", L"--level", L"error"}).seconds == 604800,
              "duration");
        for (auto v : {L"0h", L"32d", L"999999999999h", L"2x", L"-2h"})
            rejects([&] { triage::parse_cli({L"--since", v}); });
        rejects([] { triage::parse_cli({L"--limit", L"101"}); });
        rejects([] { triage::parse_cli({L"--log", L"\\\\remote\\System"}); });
        rejects([] { triage::parse_cli({L"--unknown"}); });
        rejects([] { triage::parse_cli({L"--log"}); });
        rejects([] { triage::parse_cli({L"--json", L"--json"}); });
        const std::wstring xml =
            LR"(<Event xmlns="http://schemas.microsoft.com/win/2004/08/events/event"><System><Provider Name="テスト"/><EventID>42</EventID><Version>2</Version><Level>2</Level><Task>3</Task><Opcode>4</Opcode><Keywords>0x80</Keywords><TimeCreated SystemTime="2026-10-01T12:00:00.1234567Z"/><EventRecordID>999</EventRecordID><Execution ProcessID="12" ThreadID="34"/><Channel>Application</Channel><Computer>local</Computer></System><EventData><Data Name="理由">失敗 &amp; context</Data></EventData></Event>)";
        auto e = triage::parse_xml(xml);
        auto missing = e;
        missing.provider = L"event-log-triage-nonexistent-publisher-947208";
        triage::detail::format_message(missing, nullptr);
        check(missing.message.empty() &&
                  missing.message_status.find(L"publisher_metadata_unavailable:") == 0 &&
                  missing.id == e.id,
              "missing publisher fallback");
        check(e.id == 42 && e.provider == L"テスト" && e.pid == 12 &&
                  e.data[0].second == L"失敗 & context",
              "XML structured and Unicode");
        check(triage::iso_time(e.time) == L"2026-10-01T12:00:00.123Z", "timestamp");
        rejects([] { triage::parse_xml(L"<Event><System>"); });
        rejects([] { triage::parse_xml(L"<Other/>"); });
        rejects([] {
            triage::parse_xml(L"<!DOCTYPE Event [<!ENTITY x 'bad'>]><Event><System/></Event>");
        });
        rejects([] { triage::parse_xml(std::wstring(triage::max_xml_bytes, L'x')); });
        triage::Report r;
        triage::analyze(r);
        check(r.groups.empty() && r.clusters.empty(), "empty");
        r.events = {e};
        triage::analyze(r);
        check(r.groups.size() == 1 && r.clusters.empty(), "single event");
        r.events.push_back(e);
        triage::analyze(r);
        check(r.groups[0].indices.size() == 2 && r.clusters.empty(), "recurrence alone");
        auto other = e;
        other.provider = L"different";
        other.channel = L"System";
        other.level = 1;
        other.time += 10000000;
        r.events.push_back(other);
        triage::analyze(r);
        check(r.groups.size() == 2 && r.clusters.size() == 1, "cross-channel temporal cluster");
        other.time += 121ULL * 10000000;
        r.events.push_back(other);
        triage::analyze(r);
        check(r.clusters.size() == 1, "fixed cluster span");
        r.events[0].message = L"\"\\\n\t\x1b Unicode Ω";
        auto j = triage::json(r);
        check(j.find("\\u001b") != std::string::npos &&
                  j.find("causality_established\":false") != std::string::npos,
              "JSON escaping and causality");
        check(triage::human(r).find('\x1b') == std::string::npos, "terminal control sanitization");
        auto q = triage::query(r.options, L"start", L"end");
        check(q.find(L"@SystemTime >= 'start'") != std::wstring::npos &&
                  q.find(L"@SystemTime <= 'end'") != std::wstring::npos,
              "bounded query");
        if (argc > 1 && std::string(argv[1]) == "--fixture-json")
            std::cout << triage::json(r);
        else
            std::cout << "Synthetic checks passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::fputs(e.what(), stderr);
        std::fputc('\n', stderr);
        return 1;
    } catch (...) {
        return 1;
    }
}
