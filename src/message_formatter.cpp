#include "windows_detail.h"
namespace triage::detail {
void format_message(Event &e, EVT_HANDLE event) {
    Handle metadata(EvtOpenPublisherMetadata(nullptr, e.provider.c_str(), nullptr, 0, 0));
    if (!metadata) {
        e.message_status = L"publisher_metadata_unavailable:" + std::to_wstring(GetLastError());
        return;
    }
    DWORD needed = 0;
    if (!EvtFormatMessage(metadata.get(), event, 0, 0, nullptr, EvtFormatMessageEvent, 0, nullptr,
                          &needed) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        e.message_status = L"format_failed:" + std::to_wstring(GetLastError());
        return;
    }
    if (!needed || needed > 16384) {
        e.message_status = L"message_size_limit";
        return;
    }
    std::vector<wchar_t> buffer(needed);
    if (!EvtFormatMessage(metadata.get(), event, 0, 0, nullptr, EvtFormatMessageEvent, needed,
                          buffer.data(), &needed)) {
        e.message_status = L"format_failed:" + std::to_wstring(GetLastError());
        return;
    }
    e.message.assign(buffer.data());
    e.message_status = L"formatted";
}

} // namespace triage::detail
