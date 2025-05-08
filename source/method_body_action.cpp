#include "method_body_action.hpp"
#include "method_body_consumer.hpp"

std::unique_ptr<ASTConsumer> MethodBodyAction::CreateASTConsumer(CompilerInstance& compilerInstance, StringRef file) {
    return std::make_unique<MethodBodyConsumer>(&compilerInstance.getASTContext());
}
