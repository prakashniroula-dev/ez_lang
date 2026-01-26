#include <stdio.h>

int main() {
  // char x[] = "Hello, World!";
  const char* x = "Hello, World!";
  float n = 12.2;
  printf("%s %s\n", "string:", x);
  printf("%s %g\n", "float:", n);
  return 0;
}