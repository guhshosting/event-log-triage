#include "format_detail.h"
namespace triage {
const char *help_text() {
    return "event-log-triage 1.0.0 - read-only local Windows Event Log triage\n"
           "Usage: event-log-triage [--since <Nm|Nh|Nd>] [--log <channel>]\n"
           "       [--level <critical|error|warning|information|verbose|1..5>]\n"
           "       [--limit <1..100>] [--json] [--help] [--version]\n"
           "Defaults: 24h, System and Application, warning or more severe, 10 details.\n"
           "Level is an inclusive maximum; publisher level 0 is included.\n"
           "Limits: 31 days, 10000 events total, 30 seconds acquisition, 32 MiB retained text.\n"
           "JSON includes all retained events; --limit bounds human details only.\n"
           "Temporal proximity does not establish causality. No elevation or network.\n";
}

} // namespace triage
