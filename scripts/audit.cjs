// Read-only source/import audit. Complements review; it is not a packet capture.
const fs = require('node:fs');
const assert = require('node:assert/strict');
const files = fs.readdirSync('src').filter(n => /\.(cpp|h)$/.test(n));
const source = files.map(n => fs.readFileSync('src/'+n,'utf8')).join('\n');
const prohibited = /\b(?:EvtClearLog|EvtExportLog|EvtSetChannelConfigProperty|EvtSaveChannelConfig|EvtOpenSession|EvtSubscribe|EvtOpenChannelEnum|EventRegister|RegSetValue\w*|RegCreateKey\w*|RegDelete\w*|CreateProcess\w*|ShellExecute\w*|WinHttp\w*|InternetOpen\w*|WSAStartup|socket|connect|OpenSCManager\w*|StartService\w*|ControlService)\s*\(/;
assert.ok(!prohibited.test(source), 'Prohibited application API');
assert.match(source,/EvtQuery\(nullptr,/);
assert.match(source,/EvtOpenPublisherMetadata\(nullptr,/);
assert.match(source,/DtdProcessing_Prohibit/);
const imports = fs.readFileSync('artifacts/imports.txt','utf8');
for (const name of ['EvtQuery','EvtNext','EvtRender','EvtFormatMessage','EvtOpenPublisherMetadata','EvtClose']) assert.ok(imports.includes(name));
assert.ok(!/EvtClearLog|EvtExportLog|EvtOpenSession|EvtSubscribe|WS2_32\.dll|WINHTTP\.dll|WININET\.dll|CreateProcess|ShellExecute|RegSetValue|RegCreateKey|RegDelete|StartService|ControlService/i.test(imports));
console.log(`Read-only/local-only audit passed for ${files.length} source/header files and release imports. Test-only process-launch helper is excluded from the installed application.`);
