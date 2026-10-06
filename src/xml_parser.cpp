#include "windows_detail.h"
namespace triage {
using namespace detail;
Event parse_xml(const std::wstring &xml) {
    if (xml.size() * sizeof(wchar_t) > max_xml_bytes)
        throw std::runtime_error("XML exceeds hard limit");
    auto bytes = utf8(xml);
    Com<IStream> stream(SHCreateMemStream(reinterpret_cast<const BYTE *>(bytes.data()),
                                          static_cast<UINT>(bytes.size())));
    IXmlReader *raw = nullptr;
    if (!stream ||
        FAILED(CreateXmlReader(__uuidof(IXmlReader), reinterpret_cast<void **>(&raw), nullptr)))
        throw std::runtime_error("XML reader initialization failed");
    Com<IXmlReader> reader(raw);
    if (FAILED(reader->SetProperty(XmlReaderProperty_DtdProcessing, DtdProcessing_Prohibit)) ||
        FAILED(reader->SetInput(stream.get())))
        throw std::runtime_error("XML reader setup failed");
    Event e;
    std::vector<std::wstring> path;
    std::wstring data_name;
    bool root = false, system = false;
    XmlNodeType type{};
    HRESULT hr = S_OK;
    while ((hr = reader->Read(&type)) == S_OK) {
        if (type == XmlNodeType_Element) {
            const wchar_t *name = nullptr;
            UINT len = 0;
            if (FAILED(reader->GetLocalName(&name, &len)))
                throw std::runtime_error("XML name failure");
            std::wstring n(name, len);
            if (path.size() >= 32)
                throw std::runtime_error("XML depth limit");
            path.push_back(n);
            if (path.size() == 1) {
                if (n != L"Event")
                    throw std::runtime_error("unexpected XML root");
                root = true;
            }
            if (path.size() == 2 && n == L"System")
                system = true;
            if (path.size() == 3 && path[1] == L"System") {
                if (n == L"Provider")
                    e.provider = attribute(reader.get(), L"Name");
                if (n == L"TimeCreated") {
                    e.timestamp = attribute(reader.get(), L"SystemTime");
                    e.time = parse_time(e.timestamp);
                }
                if (n == L"Execution") {
                    e.pid = numeric(attribute(reader.get(), L"ProcessID"));
                    e.tid = numeric(attribute(reader.get(), L"ThreadID"));
                }
            }
            if (path.size() >= 3 && (path[1] == L"EventData" || path[1] == L"UserData")) {
                data_name = attribute(reader.get(), L"Name");
                if (data_name.empty())
                    data_name = n;
            }
            if (reader->IsEmptyElement())
                path.pop_back();
        } else if (type == XmlNodeType_EndElement) {
            if (path.empty())
                throw std::runtime_error("unexpected XML closing tag");
            path.pop_back();
        } else if (type == XmlNodeType_Text || type == XmlNodeType_CDATA ||
                   type == XmlNodeType_Whitespace) {
            const wchar_t *p = nullptr;
            UINT len = 0;
            if (FAILED(reader->GetValue(&p, &len)))
                throw std::runtime_error("XML value failure");
            std::wstring v(p, len);
            if (path.size() == 3 && path[1] == L"System") {
                const auto &n = path.back();
                if (n == L"EventID")
                    e.id = numeric(v);
                else if (n == L"Version")
                    e.version = numeric(v);
                else if (n == L"Level")
                    e.level = numeric(v);
                else if (n == L"Task")
                    e.task = numeric(v);
                else if (n == L"Opcode")
                    e.opcode = numeric(v);
                else if (n == L"Keywords")
                    e.keywords += v;
                else if (n == L"Channel")
                    e.channel += v;
                else if (n == L"Computer")
                    e.computer += v;
                else if (n == L"EventRecordID")
                    e.record_id += v;
            } else if (path.size() >= 3 && (path[1] == L"EventData" || path[1] == L"UserData") &&
                       e.data.size() < 128)
                e.data.emplace_back(data_name, v);
        }
    }
    if (FAILED(hr) || !root || !system || !path.empty() || e.provider.empty() ||
        e.timestamp.empty())
        throw std::runtime_error("malformed or incomplete event XML");
    return e;
}

} // namespace triage
