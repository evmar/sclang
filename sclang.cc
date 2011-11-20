#include <clang-c/Index.h>
#include <string>
#include <vector>

struct ScopedClangString {
  ScopedClangString(CXString str) : str_(str) {}
  ~ScopedClangString() { clang_disposeString(str_); }
  const char* c_str() const { return clang_getCString(str_); }

  CXString str_;
};

void sexp_type(CXType type) {
  switch (type.kind) {
  case CXType_Void:   printf("void"); break;
  case CXType_UInt:   printf("uint"); break;
  case CXType_Int:    printf("int"); break;
  case CXType_Char_S: printf("char"); break;
  case CXType_Double: printf("double"); break;
  case CXType_Pointer: {
    sexp_type(clang_getPointeeType(type));
    printf("*");
    break;
  }
  case CXType_Typedef: {
    CXCursor decl = clang_getTypeDeclaration(type);
    ScopedClangString name(clang_getCursorSpelling(decl));
    printf("%s", name.c_str());
    break;
  }
  default: printf("[unhandled type %d]", type.kind); break;
  }
}

static CXChildVisitResult visit_func(CXCursor cursor, CXCursor parent,
                                     CXClientData client_data) {
  CXCursorKind kind = clang_getCursorKind(cursor);
  if (kind == CXCursor_TypeRef) {
    // It appears typerefs can be extracted via other means... ?
    return CXChildVisit_Continue;
  }

  printf(" ");
  ScopedClangString name(clang_getCursorSpelling(cursor));
  switch (clang_getCursorKind(cursor)) {
  case CXCursor_ParmDecl:
    printf("(");
    sexp_type(clang_getCursorType(cursor));
    printf(" %s)", name.c_str());
    break;
  default:
    printf("visit %s %d\n", name.c_str(), cursor.kind);
  }
  return CXChildVisit_Continue;
}

static CXChildVisitResult visit_struct(CXCursor cursor, CXCursor parent,
                                       CXClientData client_data) {
  ScopedClangString name(clang_getCursorSpelling(cursor));
  switch (clang_getCursorKind(cursor)) {
  case CXCursor_FieldDecl:
    printf(" (");
    sexp_type(clang_getCursorType(cursor));
    printf(" %s)", name.c_str());
    break;
  default:
    printf("(XXX struct %d)", clang_getCursorKind(cursor));
  }

  return CXChildVisit_Continue;
}

static CXChildVisitResult visit_enum(CXCursor cursor, CXCursor parent,
                                     CXClientData client_data) {
  ScopedClangString name(clang_getCursorSpelling(cursor));
  switch (clang_getCursorKind(cursor)) {
  case CXCursor_EnumConstantDecl:
    printf(" %s", name.c_str());
    break;
  default:
    printf("(XXX enum %d)", clang_getCursorKind(cursor));
  }

  return CXChildVisit_Continue;
}

static CXChildVisitResult visit(CXCursor cursor, CXCursor parent,
                                CXClientData client_data) {
  CXSourceLocation loc = clang_getCursorLocation(cursor);

  CXFile file = NULL;
  unsigned line, column;
  clang_getSpellingLocation(loc, &file, &line, &column, NULL);
  if (!file)
    return CXChildVisit_Continue;

  if (clang_isPreprocessing(clang_getCursorKind(cursor)))
    return CXChildVisit_Continue;

  ScopedClangString filename = clang_getFileName(file);
  printf("; %s:%d:%d\n", filename.c_str(), line, column);

  ScopedClangString name(clang_getCursorSpelling(cursor));

  switch (clang_getCursorKind(cursor)) {
  case CXCursor_FunctionDecl: {
    printf("(func ");
    sexp_type(clang_getResultType(clang_getCursorType(cursor)));
    printf(" %s", name.c_str());
    clang_visitChildren(cursor, visit_func, NULL);
    printf(")\n");
    break;
  }
  case CXCursor_TypedefDecl: {
    printf("(typedef %s)\n", name.c_str());
    break;
  }
  case CXCursor_StructDecl: {
    printf("(struct %s", name.c_str());
    clang_visitChildren(cursor, visit_struct, NULL);
    printf(")\n");
    break;
  }
  case CXCursor_UnionDecl:
    printf("(union %s)\n", name.c_str());
    break;
  case CXCursor_EnumDecl:
    printf("(enum %s", name.c_str());
    clang_visitChildren(cursor, visit_enum, NULL);
    printf(")\n");
    break;
  case CXCursor_MacroDefinition:
  case CXCursor_MacroExpansion:
  case CXCursor_InclusionDirective:
    // ignore
    break;
  default:
    printf("visit %d\n", cursor.kind);
  }

  return CXChildVisit_Continue;
}

int main(int argc, char* argv[]) {
  CXIndex index = clang_createIndex(/* excludeDeclsFromPCH */ false,
                                    /* displayDiagnostics */ true);
  CXTranslationUnit tu =
    clang_createTranslationUnitFromSourceFile(index,
                                              /* source_file */ NULL,
                                              argc, argv,
                                              0, NULL);

  printf("(\n");
  clang_visitChildren(clang_getTranslationUnitCursor(tu),
                      visit, NULL);
  printf(")\n");

  return 0;
}
