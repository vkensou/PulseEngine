#include "codegen.h"
#include "schema.h"
#include "validate.h"

#include <cstdio>
#include <string>
#include <vector>

namespace {

std::string path_base_name(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string path_stem(const std::string& path) {
    std::string name = path_base_name(path);
    size_t dot = name.find_last_of('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}

void print_usage() {
    std::printf("usage: tablegen [--out-h <header> --out-cpp <source>] [--out-das <module>] <schema file>...\n");
}

void print_errors(const std::vector<std::string>& errors) {
    for (const std::string& error : errors) {
        std::fprintf(stderr, "tablegen: %s\n", error.c_str());
    }
}

} // namespace

int main(int argc, char** argv) {
    std::string header_path{};
    std::string source_path{};
    std::string das_path{};
    std::vector<std::string> schema_paths{};

    for (int i = 1; i < argc; ++i) {
        std::string argument(argv[i]);
        if (argument == "--out-h" && i + 1 < argc) {
            header_path = argv[++i];
        } else if (argument == "--out-cpp" && i + 1 < argc) {
            source_path = argv[++i];
        } else if (argument == "--out-das" && i + 1 < argc) {
            das_path = argv[++i];
        } else if (argument == "--help" || argument == "-h") {
            print_usage();
            return 0;
        } else {
            schema_paths.push_back(argument);
        }
    }

    if (header_path.empty() != source_path.empty() || (header_path.empty() && das_path.empty()) || schema_paths.empty()) {
        print_usage();
        return 1;
    }

    tablegen::Library library{};
    if (!tablegen::load_library(schema_paths, library)) {
        print_errors(library.errors);
        return 1;
    }

    std::vector<std::string> errors{};
    if (!tablegen::validate_library(library, errors)) {
        print_errors(errors);
        return 1;
    }

    tablegen::LayoutSet layouts{};
    if (!tablegen::compute_layouts(library, layouts, errors)) {
        print_errors(errors);
        return 1;
    }

    if (!tablegen::generate(library, layouts, header_path, source_path, path_base_name(header_path), das_path, path_stem(das_path), errors)) {
        print_errors(errors);
        return 1;
    }

    for (const tablegen::Schema& schema : library.schemas) {
        const char* kind = "struct";
        if (schema.kind == tablegen::Kind::Table) {
            kind = "table";
        } else if (schema.kind == tablegen::Kind::Enum) {
            kind = "enum";
        }
        std::printf("tablegen: %s -> %s\n", schema.name.c_str(), kind);
    }
    std::printf("tablegen: wrote");
    if (!header_path.empty()) {
        std::printf(" %s and %s", header_path.c_str(), source_path.c_str());
    }
    if (!das_path.empty()) {
        std::printf(" %s", das_path.c_str());
    }
    std::printf("\n");
    return 0;
}
