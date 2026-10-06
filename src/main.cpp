#include "triage.h"
#include <iostream>
#include <cstdio>
#include <stdexcept>
#include <windows.h>

int wmain(int argc, wchar_t **argv) {
    try {
        std::vector<std::wstring> args;
        for (int i = 1; i < argc; ++i)
            args.emplace_back(argv[i]);
        triage::Options options;
        try {
            options = triage::parse_cli(args);
        } catch (const std::invalid_argument &e) {
            std::cerr << "Usage error: " << e.what() << '\n';
            return 2;
        }
        if (options.help) {
            std::cout << triage::help_text();
            return 0;
        }
        if (options.version) {
            std::cout << "event-log-triage 1.0.0\n";
            return 0;
        }
        auto report = triage::acquire(options);
        triage::analyze(report);
        SetConsoleOutputCP(CP_UTF8);
        std::cout << (options.json ? triage::json(report) : triage::human(report));
        return report.exit_code;
    } catch (...) {
        std::fputs("Unexpected internal failure.\n", stderr);
        return 5;
    }
}
