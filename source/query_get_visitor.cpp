#include "query_get_visitor.hpp"

bool QueryGetVisitor::VisitCallExpr(CallExpr *call) {
  const FunctionDecl *funcDecl = call->getDirectCallee();
  if (!funcDecl)
    return true;

  std::string qualifiedName = funcDecl->getQualifiedNameAsString();

  if (qualifiedName.compare(0, 5, "std::") == 0 || qualifiedName.compare(0, 6, "rttr::") == 0)
    return true;

  if (qualifiedName.find("Query::get") != std::string::npos) {
    if (call->getNumArgs() == 1) {
      Expr *arg = call->getArg(0);
      QualType argType = arg->getType();

      if (argType->isPointerType()) {
        QualType pointee = argType->getPointeeType();
        if (const CXXRecordDecl *record = pointee->getAsCXXRecordDecl()) {
          if (record->getNameAsString() == "VariantBase") {
            llvm::outs() << "    - Detected Query::get with VariantBase* at: "
                       << qualifiedName << "\n";

            if (const auto *specInfo = funcDecl->getTemplateSpecializationInfo()) {
              const TemplateArgumentList *templateArgs = specInfo->TemplateArguments;
              if (templateArgs && templateArgs->size() > 0) {
                llvm::outs() << "      - Template argument(s): ";
                for (unsigned i = 0; i < templateArgs->size(); ++i) {
                  const TemplateArgument &arg = templateArgs->get(i);
                  if (arg.getKind() == TemplateArgument::Type) {
                    llvm::outs() << arg.getAsType().getAsString() << " ";
                  } else if (arg.getKind() == TemplateArgument::Pack) {
                    for (const auto &packArg : arg.pack_elements()) {
                      if (packArg.getKind() == TemplateArgument::Type) {
                        llvm::outs() << packArg.getAsType().getAsString() << " ";
                      }
                    }
                  }
                }
                llvm::outs() << "\n";
              }
            }
          }
        }
      }
    }
  }

  if (funcDecl->hasBody() && funcDecl->isUserProvided()) {
    if (Visited.insert(funcDecl).second) {
      Stmt *calleeBody = funcDecl->getBody();
      QueryGetVisitor subVisitor(Context, Visited);
      subVisitor.TraverseStmt(calleeBody);
    }
  }

  return true;
}
