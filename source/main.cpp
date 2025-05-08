#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "llvm/Support/CommandLine.h"

#include "method_body_action.hpp"

static llvm::cl::OptionCategory MethodAnalyzerCategory("Badem Method Analyzer Options");

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
    
    clang::tooling::ClangTool Tool(*CompilationDB, Sources);
    
    return Tool.run(clang::tooling::newFrontendActionFactory<MethodBodyAction>().get());
}
