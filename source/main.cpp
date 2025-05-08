#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include <llvm-14/llvm/Support/raw_ostream.h>

#include <ranges>
#include <algorithm>

#include "method_body_action.hpp"

static llvm::cl::OptionCategory MethodAnalyzerCategory("Badem Method Analyzer Options");

static constexpr std::string_view game = "game";

static llvm::cl::opt<std::string> ProjectDir(
    "dir",
    llvm::cl::desc("Project directory containing compile_commands.json"),
    llvm::cl::Required,
    llvm::cl::cat(MethodAnalyzerCategory));

int main(int argc, const char **argv) {
    llvm::cl::HideUnrelatedOptions(MethodAnalyzerCategory);
    
    if (!llvm::cl::ParseCommandLineOptions(argc, argv, "Badem Method Analyzer\n")) {
        return 1;
    }
    
    std::string ErrorMessage;
    auto CompilationDB = clang::tooling::CompilationDatabase::loadFromDirectory(
        ProjectDir, ErrorMessage);
    
    if (!CompilationDB) {
        llvm::errs() << "Error loading compilation database from " << ProjectDir << ":\n"
                     << ErrorMessage << "\n";
        return 1;
    }
    
    std::vector<std::string> Sources = CompilationDB->getAllFiles();

    if (Sources.empty()) {
        llvm::errs() << "No source files found to analyze.\n";
        return 1;
    }

    std::vector<std::string> FilteredSources;

    std::copy_if(Sources.begin(), Sources.end(), std::back_inserter(FilteredSources),
        [](const std::string& s) {
            return s.find(game) != std::string::npos;
        });
    
    for(const auto& source: FilteredSources) {
        llvm::outs() << "Source: " << source << "\n";
    }

    clang::tooling::ClangTool Tool(*CompilationDB, FilteredSources);
    
    return Tool.run(clang::tooling::newFrontendActionFactory<MethodBodyAction>().get());
}
