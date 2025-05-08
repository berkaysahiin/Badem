#include "query_visitor.hpp"

bool QueryVisitor::VisitCXXMethodDecl(CXXMethodDecl *methodDecl) {
    if (!methodDecl->hasBody() || !methodDecl->isUserProvided()) {
        return true;
    }

    std::string methodName = methodDecl->getNameInfo().getName().getAsString();

    if (methodName != "on_init" && methodName != "on_post_init" &&
        methodName != "on_update" && methodName != "on_play_update" &&
        methodName != "on_play_start" && methodName != "on_play_late_start") {
        return true;
    }

    CXXRecordDecl *parentClass = methodDecl->getParent();

    bool isVariant = false;
    for (const auto &base : parentClass->bases()) {
        const Type *baseType = base.getType().getTypePtrOrNull();
        if (!baseType) {
            continue;
        }

        const CXXRecordDecl *baseDecl = baseType->getAsCXXRecordDecl();
        if (!baseDecl) {
            continue;
        }

        if (baseDecl->getNameAsString() == "VariantBase") {
            isVariant = true;
            break;
        }
    }

    if (!isVariant)
        return true;

    if (Stmt* body = methodDecl->getBody()) {
        analyzeMethodBody(body);
    }

    return true;
}

void QueryVisitor::analyzeMethodBody(Stmt *body) {
    TraverseStmt(body);
}

bool QueryVisitor::VisitCallExpr(CallExpr *call) {
  const FunctionDecl *funcDecl = call->getDirectCallee();

  if (!funcDecl)
    return true;

  processCurrentCall(call, funcDecl);
  visitCalledFunctionBody(funcDecl);

  return true;
}

void QueryVisitor::processCurrentCall(CallExpr *call, const FunctionDecl *funcDecl) {
  std::string qualifiedName = funcDecl->getQualifiedNameAsString();

  if (qualifiedName.compare(0, 5, "std::") == 0 || qualifiedName.compare(0, 6, "rttr::") == 0) {
      return;
  }

  if (qualifiedName.find("Query::get") != std::string::npos) {
    detectVariantBaseQueryCalls(call, funcDecl, qualifiedName);
  }
}

void QueryVisitor::visitCalledFunctionBody(const FunctionDecl *funcDecl) {
  if (!funcDecl->hasBody() || !funcDecl->isUserProvided()) {
      return;
  }

  if (Visited.insert(funcDecl).second) {
      TraverseStmt(funcDecl->getBody());
  }
}

void QueryVisitor::detectVariantBaseQueryCalls(CallExpr *call, const FunctionDecl *funcDecl, const std::string &qualifiedName) {
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

bool QueryVisitor::isVariantBasePointer(QualType type) {
  if (!type->isPointerType()) {
      return false;
  }

  QualType pointee = type->getPointeeType();
  const CXXRecordDecl *record = pointee->getAsCXXRecordDecl();

  return record && record->getNameAsString() == "VariantBase";
}

void QueryVisitor::printTemplateArguments(const FunctionDecl *funcDecl) {
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

void QueryVisitor::printTemplateArgument(const TemplateArgument &arg) {
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
