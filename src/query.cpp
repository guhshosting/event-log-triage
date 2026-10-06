#include "format_detail.h"
namespace triage {
std::wstring query(const Options &o, const std::wstring &start, const std::wstring &end) {
    return L"*[System[(Level <= " + std::to_wstring(o.level) +
           L") and TimeCreated[@SystemTime >= '" + start + L"' and @SystemTime <= '" + end +
           L"']]]";
}

} // namespace triage
