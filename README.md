# event-log-triage 1.0.0

> Runtime prerequisite for the exact qualified EXE: Microsoft Visual C++ x64 Redistributable is required when the imported runtime DLLs are absent. This supersedes any static-CRT/no-redistributable description of a source build below.


> Release context: see [qualified platforms, runtime requirements and package scope](RELEASE-QUALIFICATION.md).


A native, read-only Windows command-line diagnostic utility. It reduces recent
System/Application noise into recurring provider/event-ID groups, recent severe
events, and nearby events. Event Viewer remains the tool for browsing full logs.
This utility does not diagnose root cause from a warning or timestamp alone.

## Use

```text
event-log-triage
event-log-triage --since 2h
event-log-triage --since 7d --log Application --level error --limit 20
event-log-triage --json
event-log-triage --help
event-log-triage --version
```

Defaults: last 24 hours, System and Application, publisher levels 0–3,
10 human details. `--level` selects an inclusive maximum: critical=1, error=2,
warning=3, information=4, verbose=5; level 0 (LogAlways) remains included.
Severity reflects the publisher's classification, not impact or causality.
`--since` accepts positive whole minutes (`m`), hours (`h`), days (`d`), up to
31 days. `--log` selects one local channel; remote sessions and exported files
are unsupported. No channels are enabled or discovered automatically.

## Evidence and limits

Groups use provider, channel, event ID, and level. Every retained record stays
available in JSON; duplicates are grouped, not discarded. Recurrence counts
describe the retained sample. No event ID is classified as universally benign
or dangerous. Fixed 120-second clusters require distinct provider/ID/channel
combinations and never extend through transitive proximity chains. Temporal
correlation does **not** establish causality. There is no health score, machine
learning, causal engine, or remediation.

Queries use a fixed UTC start/end and reverse record direction. Event Log
reverse direction follows record order; timestamps can be out of order after
clock changes. Hard limits: 10,000 attempted records (5,000 per default channel),
1 MiB rendered XML per record, 16,384 UTF-16 units per message, 128 event-data
text segments, XML depth 32, 32 MiB conservatively accounted retained text,
30-second acquisition scheduling budget, 100 human details. XmlLite prohibits
DTDs. Events are processed incrementally. OS query/render/format calls are
synchronous and cannot be interrupted; the scheduling budget is checked between
records and channels, not a strict wall-clock cancellation guarantee. Output
and analysis have bounded size but are outside that acquisition budget.
Budget exhaustion is visible; counts and clusters may be incomplete.

Local publisher metadata provides messages when available. Missing publisher
metadata, DLLs, resources, or insertion formatting produces a per-event status
and structured identifiers instead of failing the audit. Human fallback shows
up to four data segments. JSON includes available System fields and up to 128
EventData/UserData text segments; it is not a lossless XML export. Human text is
shortened and terminal control characters are sanitized. Message and data text
can contain sensitive local information; consider that before sharing reports.

## Permissions and safety

Run unelevated. Inaccessible default channels produce warnings; accessible logs
are still analyzed. An unavailable explicitly requested channel exits 3. No
auto-elevation, channel changes, log clearing/export, Registry changes, provider
registration, service control, subscriptions, or intentional test events.
Only local Event Log handles are used. No intentional network, remote computer
resolution, downloads, telemetry, update checking, or shell execution exists.
Windows itself manages its local Event Log service and publisher resources.

Exit codes: 0 completed (including findings and a bounded partial sample),
2 invalid usage, 3 requested channel unavailable, 4 API failure preventing
analysis or iteration failure, 5 unexpected internal failure.

## JSON

`--json` emits UTF-8 JSON only on stdout, including partial reports on acquisition
failure; usage/internal errors go to stderr. Schema `1.0` includes
`schema_version`, `application_version`, `query`, `summary`, `event_groups`,
`events`, `correlations`, `warnings`, `limitations`. Group/cluster indices refer
to the `events` array in the same report. All retained events are included;
`--limit` bounds human output only. `--help` and `--version` must be used alone.
The schema is provided in `docs/schema-1.0.json`. IDs, levels and execution IDs are numeric;
record IDs and keywords are strings to avoid large-integer precision loss.
Times are UTC. Absent text is empty; absent numeric fields are zero. No event
messages are treated as instructions or interpreted as code.

## Build, test, install

Requires Windows, MSVC, CMake 3.24+, Windows SDK, C++17. With Ninja installed,
in an x64 VS developer shell:

```text
cmake -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
ctest --test-dir build-release --output-on-failure
cmake --install build-release --prefix install
```

Visual Studio generators may also be used with the matching installed version.
`/W4 /WX` is always enabled.
`-DTRIAGE_ANALYZE=ON` enables `/analyze`; `-DTRIAGE_ASAN=ON` enables MSVC ASan
(use RelWithDebInfo). The portable executable uses the normal MSVC runtime;
install the matching supported Visual C++ Redistributable where needed. Tests
query existing records without creating failures. See VALIDATION.md for checks,
tool versions, exact tested platform and remaining limitations.

`scripts/validate.cmd` reproduces the build/test/analysis matrix in an x64 VS
developer command prompt with Ninja, Node.js, clang-tidy and Cppcheck available.
Node.js is used only for independent validation; it is not a runtime dependency.

Intended compatibility: Windows 10, Windows 11, Windows Server 2016 and newer
supported generations, x64. Windows 8.1 and older are excluded. Intended
compatibility is not a claim of runtime validation on each version.
