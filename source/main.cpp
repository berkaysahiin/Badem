#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "method_body_action.hpp"

int main(int argc, const char **argv) {
  if (argc > 1) {
    static llvm::cl::OptionCategory MethodAnalyzerCategory("method-analyzer options");

    auto ExpectedParser = clang::tooling::CommonOptionsParser::create(argc, argv, MethodAnalyzerCategory);
    if (!ExpectedParser) {
      llvm::errs() << ExpectedParser.takeError();
      return 1;
    }

    clang::tooling::CommonOptionsParser &OP = ExpectedParser.get();
    clang::tooling::ClangTool Tool(OP.getCompilations(), OP.getSourcePathList());

    return Tool.run(clang::tooling::newFrontendActionFactory<MethodBodyAction>().get());
  }

  return 0;
}
