#pragma once
#include "triage.h"
#include <algorithm>
#include <map>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace triage {
namespace {
inline unsigned number(const std::wstring &s, unsigned maximum) {
    if (s.empty())
        throw std::invalid_argument("expected a positive integer");
    unsigned n = 0;
    for (auto c : s) {
        if (c < L'0' || c > L'9' || n > maximum / 10)
            throw std::invalid_argument("invalid or excessive integer");
        n = n * 10 + static_cast<unsigned>(c - L'0');
        if (n > maximum)
            throw std::invalid_argument("integer exceeds hard limit");
    }
    if (!n)
        throw std::invalid_argument("integer must be positive");
    return n;
}
inline std::string quote(const std::wstring &s) {
    std::string result = "\"";
    for (unsigned char c : utf8(s)) {
        switch (c) {
        case '"':
            result += "\\\"";
            break;
        case '\\':
            result += "\\\\";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            if (c < 32) {
                const char *h = "0123456789abcdef";
                result += "\\u00";
                result += h[c >> 4U];
                result += h[c & 15U];
            } else
                result += static_cast<char>(c);
        }
    }
    return result + '"';
}
inline std::string safe(const std::wstring &s, size_t limit = 1200) {
    std::wstring out;
    for (auto c : s) {
        if (out.size() >= limit) {
            out += L" [truncated]";
            break;
        }
        out += (c < 32 || c == 127 || (c >= 0x202a && c <= 0x202e) || (c >= 0x2066 && c <= 0x2069))
                   ? L' '
                   : c;
    }
    return utf8(out);
}
inline const char *limitations =
    "Analysis covers retained events only; counts are not total log counts. Missing resources can "
    "prevent message formatting. Temporal proximity does not establish causality. Severity is "
    "assigned by the publisher and does not prove root cause. No remediation is performed.";
} // namespace

} // namespace triage
