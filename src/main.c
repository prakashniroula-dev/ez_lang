#include <ezy_lexer.h>
#include <stdlib.h>
#include <ezy_log.h>

int main(int argc, const char** argv) {
  ezy_log("start of program");
  if ( argc < 2 ) {
    ezy_log_error("no input file specified");
    return 1;
  }
  const char* filename = argv[1];
  FILE* f = fopen(filename, "rb");
  if ( f == NULL ) {
    ezy_log_error("failed to open input file: %s", filename);
    return 1;
  }
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  rewind(f);
  char* buffer = (char*)malloc(fsize + 1);
  if (buffer == NULL) {
    ezy_log_error("failed to allocate memory for file buffer");
    fclose(f);
    return 1;
  }
  fread(buffer, 1, fsize, f);
  buffer[fsize] = '\0';
  fclose(f);

  ezylex_start(buffer);
  ezy_tkn_t tkn;
  do {
    tkn = ezylex_peek_tkn(0);
    switch ( tkn.type ) {
      case ezy_tkn_eof:
        printf("\nToken: EOF");
        break;
      case ezy_tkn_invalid:
        printf("\nToken: INVALID");
        break;
      case ezy_tkn_int64:
        printf("\nToken: INT64 (%lld)", tkn.data.t_int64);
        break;
      case ezy_tkn_uint64:
        printf("\nToken: UINT64 (%llu)", tkn.data.t_uint64);
        break;
      case ezy_tkn_float64:
        printf("\nToken: FLOAT64 (%f)", tkn.data.t_float64);
        break;
      case ezy_tkn_char:
        printf("\nToken: CHAR ('%c')", tkn.data.t_char);
        break;
      case ezy_tkn_string:
        printf("\nToken: STRING (\"%.*s\")", (int)tkn.data.t_string.len, tkn.data.t_string.ptr);
        break;
      case ezy_tkn_identifier:
        printf("\nToken: IDENTIFIER (\"%.*s\")", (int)tkn.data.t_identifier.len, tkn.data.t_identifier.ptr);
        break;
      case ezy_tkn_keyword:
        printf("\nToken: KEYWORD (%d)", tkn.data.t_keyword);
        break;
      case ezy_tkn_operator:
        printf("\nToken: OPERATOR (%d)", tkn.data.t_operator);
        break;
      case ezy_tkn_dummy:
        // skip
        break;
      default:
        printf("\nToken: UNKNOWN TYPE (%d)", tkn.type);
        break;
    }
    ezylex_consume_tkn(1);
  } while ( tkn.type != ezy_tkn_eof );

  free(buffer);
  return 0;
}