#include "query_get_visitor.hpp"

bool QueryGetVisitor::VisitCallExpr(CallExpr *call) {
  const FunctionDecl *funcDecl = call->getDirectCallee();

  if (!funcDecl)
    return true;

  processCurrentCall(call, funcDecl);
  visitCalledFunctionBody(funcDecl);

  return true;
}

void QueryGetVisitor::processCurrentCall(CallExpr *call, const FunctionDecl *funcDecl) {
  std::string qualifiedName = funcDecl->getQualifiedNameAsString();

  if (qualifiedName.compare(0, 5, "std::") == 0 || qualifiedName.compare(0, 6, "rttr::") == 0) {
      return;
  }

  if (qualifiedName.find("Query::get") != std::string::npos) {
    detectVariantBaseQueryCalls(call, funcDecl, qualifiedName);
  }
}

void QueryGetVisitor::visitCalledFunctionBody(const FunctionDecl *funcDecl) {
  if (!funcDecl->hasBody() || !funcDecl->isUserProvided()) {
      return;
  }

  if (Visited.insert(funcDecl).second) {
      QueryGetVisitor subVisitor(Context, Visited);
      subVisitor.TraverseStmt(funcDecl->getBody());
  }
}

void QueryGetVisitor::detectVariantBaseQueryCalls(CallExpr *call, const FunctionDecl *funcDecl, const std::string &qualifiedName) {
  if (call->getNumArgs() < 1) { // no argument ?, suss
    return;
  }

  Expr *arg = call->getArg(0);

  // we assume that the client code will only use variantbase* overload of query function
  // in case of entity_id overload, its not possible to detect whether the query is for this entity variant or for another entity's variant
  if (!isVariantBasePointer(arg->getType())) { 
      return;
  }

  printTemplateArguments(funcDecl);
}

bool QueryGetVisitor::isVariantBasePointer(QualType type) {
  if (!type->isPointerType()) {
      return false;
  }

  QualType pointee = type->getPointeeType();
  const CXXRecordDecl *record = pointee->getAsCXXRecordDecl();

  return record && record->getNameAsString() == "VariantBase";
}

void QueryGetVisitor::printTemplateArguments(const FunctionDecl *funcDecl) {
  const auto *specInfo = funcDecl->getTemplateSpecializationInfo();
  if (!specInfo)
    return;

  const TemplateArgumentList *templateArgs = specInfo->TemplateArguments;
  if (!templateArgs || templateArgs->size() == 0)
    return;

  llvm::outs() << "      - Template argument(s): ";

  for (unsigned i = 0; i < templateArgs->size(); ++i) {
    printTemplateArgument(templateArgs->get(i));
  }

  llvm::outs() << "\n";
}

void QueryGetVisitor::printTemplateArgument(const TemplateArgument &arg) {
  if (arg.getKind() == TemplateArgument::Type) {
    llvm::outs() << arg.getAsType().getAsString() << " ";
  }
  else if (arg.getKind() == TemplateArgument::Pack) {
    for (const auto &packArg : arg.pack_elements()) {
      if (packArg.getKind() == TemplateArgument::Type) {
        llvm::outs() << packArg.getAsType().getAsString() << " ";
      }
    }
  }
}
