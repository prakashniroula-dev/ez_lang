#if !defined(ezy_parser_h)
#define ezy_parser_h

#include <ezy_ast.h>

ezy_ast_node_t* ezyparse_parse(const char* src);

#endif // ezy_parser_h
