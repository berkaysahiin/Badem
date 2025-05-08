#include "method_body_visitor.hpp"
#include "query_get_visitor.hpp"

/* filtering methods to process
 * NOTE: this can be futher optimized 
*/
bool MethodBodyVisitor::VisitCXXMethodDecl(CXXMethodDecl *methodDecl) {
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

    std::string className = parentClass->getNameAsString();

    if (Stmt* body = methodDecl->getBody()) {
        analyzeMethodBody(body);
    }

    return true;
}

void MethodBodyVisitor::analyzeMethodBody(Stmt *body) {
    std::set<const FunctionDecl *> visited;
    QueryGetVisitor visitor(Context, visited);
    visitor.TraverseStmt(body);
}
