#include "windows_detail.h"
namespace triage {
using namespace detail;
std::string utf8(const std::wstring &s) {
    if (s.empty())
        return {};
    int length = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0,
                                     nullptr, nullptr);
    if (!length)
        throw std::runtime_error("Unicode conversion failed");
    std::string out(static_cast<size_t>(length), '\0');
    if (!WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), length,
                             nullptr, nullptr))
        throw std::runtime_error("Unicode conversion failed");
    return out;
}
std::wstring iso_time(std::uint64_t ticks) {
    ULARGE_INTEGER u{};
    u.QuadPart = ticks;
    FILETIME f{u.LowPart, u.HighPart};
    SYSTEMTIME t{};
    if (!FileTimeToSystemTime(&f, &t))
        throw std::runtime_error("invalid timestamp");
    std::wostringstream s;
    s << std::setfill(L'0') << std::setw(4) << t.wYear << L'-' << std::setw(2) << t.wMonth << L'-'
      << std::setw(2) << t.wDay << L'T' << std::setw(2) << t.wHour << L':' << std::setw(2)
      << t.wMinute << L':' << std::setw(2) << t.wSecond << L'.' << std::setw(3) << t.wMilliseconds
      << L'Z';
    return s.str();
}
std::uint64_t parse_time(const std::wstring &v) {
    if (v.size() < 20 || v[4] != L'-' || v[7] != L'-' || v[10] != L'T' || v[13] != L':' ||
        v[16] != L':' || v.back() != L'Z')
        throw std::runtime_error("invalid timestamp");
    SYSTEMTIME t{};
    t.wYear = static_cast<WORD>(numeric(v.substr(0, 4)));
    t.wMonth = static_cast<WORD>(numeric(v.substr(5, 2)));
    t.wDay = static_cast<WORD>(numeric(v.substr(8, 2)));
    t.wHour = static_cast<WORD>(numeric(v.substr(11, 2)));
    t.wMinute = static_cast<WORD>(numeric(v.substr(14, 2)));
    t.wSecond = static_cast<WORD>(numeric(v.substr(17, 2)));
    std::uint64_t fraction = 0;
    if (v.size() > 20) {
        if (v[19] != L'.' || v.size() > 28)
            throw std::runtime_error("invalid timestamp fraction");
        auto f = v.substr(20, v.size() - 21);
        if (f.empty())
            throw std::runtime_error("empty timestamp fraction");
        for (auto c : f) {
            if (c < L'0' || c > L'9')
                throw std::runtime_error("invalid timestamp fraction");
            fraction = fraction * 10 + static_cast<unsigned>(c - L'0');
        }
        for (size_t i = f.size(); i < 7; ++i)
            fraction *= 10;
    }
    FILETIME ft{};
    if (!SystemTimeToFileTime(&t, &ft))
        throw std::runtime_error("invalid timestamp");
    ULARGE_INTEGER u{};
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    return u.QuadPart + fraction;
}

} // namespace triage
