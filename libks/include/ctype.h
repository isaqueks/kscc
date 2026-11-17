// <ctype.h> da libks (doc 05 §2). Só ASCII ("C" locale): valores fora de
// 0..127, inclusive EOF, não pertencem a nenhuma classe.
#ifndef __CTYPE_H
#define __CTYPE_H

int isdigit(int c);
int isalpha(int c);
int isalnum(int c);
int isspace(int c);
int isupper(int c);
int islower(int c);
int isxdigit(int c);
int isprint(int c);
int toupper(int c);
int tolower(int c);

#endif
