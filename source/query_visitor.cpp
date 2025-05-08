#include "query_visitor.hpp"
#include "clang/Basic/SourceManager.h"
#include <llvm-14/llvm/Support/raw_ostream.h>

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
    currentVariantClass = parentClass->getNameAsString();

    analyzeMethod(methodDecl);

    return true;
}

void QueryVisitor::analyzeMethod(CXXMethodDecl *md) {
    TraverseStmt(md->getBody());
}

bool QueryVisitor::VisitCallExpr(CallExpr *call) {
  const FunctionDecl *funcDecl = call->getDirectCallee();

  if (!funcDecl) {
    return true;
  }

  bool foundQuery = processCurrentCall(call, funcDecl);

  if(!foundQuery && !skipFunction(funcDecl)) {
      visitCalledFunctionBody(funcDecl);
  }

  return true;
}

bool QueryVisitor::processCurrentCall(CallExpr *call, const FunctionDecl *funcDecl) {
  std::string qualifiedName = funcDecl->getQualifiedNameAsString();

  if (qualifiedName.find("Query::get") != std::string::npos) {
    detectVariantBaseQueryCalls(call, funcDecl, qualifiedName);
    return true;
  }

  return false;
}

bool QueryVisitor::skipFunction(const FunctionDecl* fd) {
  SourceLocation loc = fd->getLocation();
  
  if (loc.isInvalid()) {
      return true;
  }
  
  if (Context->getSourceManager().isInSystemHeader(loc)) {
      return true;
  }
  
  FileID fileID = Context->getSourceManager().getFileID(loc);
  const FileEntry* fileEntry = Context->getSourceManager().getFileEntryForID(fileID);
  
  if (!fileEntry) {
      return true;
  }
  
  llvm::StringRef filePath = fileEntry->getName();

  if (filePath.find("/game/") != llvm::StringRef::npos) {
      return false;  
  }
  
  return true;
}

void QueryVisitor::visitCalledFunctionBody(const FunctionDecl *funcDecl) {
  if(skipFunction(funcDecl)) {
    return;
  }

  std::string qualifiedName = funcDecl->getQualifiedNameAsString();

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
  if (!templateArgs || templateArgs->size() == 0) {
      return;
  }

  for (unsigned i = 0; i < templateArgs->size(); ++i) {
    printTemplateArgument(templateArgs->get(i));
  }
}

void QueryVisitor::printTemplateArgument(const TemplateArgument &arg) {
  if (arg.getKind() == TemplateArgument::Type) {
      llvm::outs() << "Variant: " << currentVariantClass << ", Dependency: " << arg.getAsType().getAsString() << "\n";;
  }
  else if (arg.getKind() == TemplateArgument::Pack) {
    for (const auto &packArg : arg.pack_elements()) {
      if (packArg.getKind() == TemplateArgument::Type) {
          llvm::outs() << "Variant: " << currentVariantClass << ", Dependency: " << packArg.getAsType().getAsString() << "\n";;
      }
    }
  }
}
