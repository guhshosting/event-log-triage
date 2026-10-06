#include "format_detail.h"
namespace triage {
Options parse_cli(const std::vector<std::wstring> &args) {
    Options o;
    std::map<std::wstring, bool> seen;
    for (size_t i = 0; i < args.size(); ++i) {
        const auto &a = args[i];
        if (seen[a])
            throw std::invalid_argument("duplicate option");
        seen[a] = true;
        if (a == L"--json")
            o.json = true;
        else if (a == L"--help")
            o.help = true;
        else if (a == L"--version")
            o.version = true;
        else {
            if (a != L"--since" && a != L"--log" && a != L"--level" && a != L"--limit")
                throw std::invalid_argument("unknown option");
            if (++i == args.size())
                throw std::invalid_argument("missing option value");
            const auto &v = args[i];
            if (a == L"--since") {
                if (v.size() < 2)
                    throw std::invalid_argument("duration requires m, h, or d suffix");
                unsigned factor = v.back() == L'm'   ? 60
                                  : v.back() == L'h' ? 3600
                                  : v.back() == L'd' ? 86400
                                                     : 0;
                if (!factor)
                    throw std::invalid_argument("duration requires m, h, or d suffix");
                o.seconds = number(v.substr(0, v.size() - 1), 31 * 86400 / factor) * factor;
            } else if (a == L"--limit")
                o.limit = number(v, max_details);
            else if (a == L"--level") {
                if (v == L"critical")
                    o.level = 1;
                else if (v == L"error")
                    o.level = 2;
                else if (v == L"warning")
                    o.level = 3;
                else if (v == L"information")
                    o.level = 4;
                else if (v == L"verbose")
                    o.level = 5;
                else
                    o.level = number(v, 5);
            } else {
                if (v.empty() || v.size() > 256 ||
                    v.find_first_of(L"\\:*?'\"<>|\r\n") != std::wstring::npos ||
                    v.find(L"..") != std::wstring::npos ||
                    std::any_of(v.begin(), v.end(), [](wchar_t c) { return c < 32 || c == 127; }))
                    throw std::invalid_argument("invalid local channel name");
                o.logs = {v};
                o.explicit_log = true;
            }
        }
    }
    if ((o.help || o.version) && args.size() != 1)
        throw std::invalid_argument("help and version must be used alone");
    return o;
}

} // namespace triage
