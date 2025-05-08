#include "method_body_consumer.hpp"

void MethodBodyConsumer::HandleTranslationUnit(ASTContext &context) {
    Visitor.TraverseDecl(context.getTranslationUnitDecl());
    Visitor.writeDependenciesToCSV();
}
