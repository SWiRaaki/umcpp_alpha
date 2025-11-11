#ifndef UMCPP_LEX_H
#define UMCPP_LEX_H

#include <stdbool.h>

typedef struct {
	unsigned type;
	unsigned line;
	unsigned col;
	unsigned start;
	unsigned end;
	unsigned length;
} CToken;

typedef struct {
	CToken * items;
	CToken * tail;
	unsigned capacity;
	unsigned count;
} TokenList;

typedef struct {
	TokenList tokens;
	char * original;
} Tokenizer;

void tokenlistCreate( TokenList * list );
void tokenlistDestroy( TokenList * list );
void tokenlistAppend( TokenList * list, CToken token );

void tokenizerCreate( Tokenizer * tokenizer, char * content );
void tokenizerDestroy( Tokenizer * tokenizer, bool release_content );
void tokenize( Tokenizer * tokenizer );


#endif // UMCPP_LEX_H
