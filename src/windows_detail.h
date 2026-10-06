#pragma once
#include "triage.h"
#include <objbase.h>
#include <windows.h>
#include <winevt.h>
// SDK 10.0.26100 XmlLite SetProperty carries an invalid SAL value annotation.
#pragma warning(push)
#pragma warning(disable : 6553)
#include <xmllite.h>
#pragma warning(pop)
#include <algorithm>
#include <iomanip>
#include <memory>
#include <shlwapi.h>
#include <sstream>
#include <stdexcept>

namespace triage {
namespace detail {
struct EvtCloser {
    void operator()(void *p) const {
        if (p)
            EvtClose(p);
    }
};
using Handle = std::unique_ptr<void, EvtCloser>;
template <class T> struct ComCloser {
    void operator()(T *p) const {
        if (p)
            p->Release();
    }
};
template <class T> using Com = std::unique_ptr<T, ComCloser<T>>;
inline unsigned numeric(const std::wstring &v) {
    if (v.empty())
        return 0;
    if (v.find_first_not_of(L"0123456789") != std::wstring::npos)
        throw std::runtime_error("invalid event number");
    unsigned n = 0;
    for (auto c : v) {
        const auto digit = static_cast<unsigned>(c - L'0');
        if (n > (MAXDWORD - digit) / 10U)
            throw std::runtime_error("invalid event number");
        n = n * 10U + digit;
    }
    return n;
}
inline std::wstring attribute(IXmlReader *r, const wchar_t *name) {
    std::wstring v;
    if (r->MoveToAttributeByName(name, nullptr) == S_OK) {
        const wchar_t *p = nullptr;
        UINT n = 0;
        if (SUCCEEDED(r->GetValue(&p, &n)) && p)
            v.assign(p, n);
        r->MoveToElement();
    }
    return v;
}
inline std::wstring render(EVT_HANDLE event) {
    DWORD needed = 0, properties = 0;
    if (!EvtRender(nullptr, event, EvtRenderEventXml, 0, nullptr, &needed, &properties) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        throw std::runtime_error("EvtRender failed");
    if (!needed || needed > max_xml_bytes)
        throw std::runtime_error("event XML exceeds size limit");
    std::vector<wchar_t> buffer((needed + sizeof(wchar_t) - 1) / sizeof(wchar_t));
    if (!EvtRender(nullptr, event, EvtRenderEventXml, needed, buffer.data(), &needed, &properties))
        throw std::runtime_error("EvtRender failed");
    return std::wstring(buffer.data());
}
void format_message(Event &e, EVT_HANDLE event);
} // namespace detail

} // namespace triage
