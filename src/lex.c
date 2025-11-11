#include "lex.h"

#include <stdlib.h>

static inline CToken _read_identify( char const * str, size_t * row, size_t * col, size_t * cur ) {
	return (CToken){0};
}

static inline CToken _read_number( void ) {
	return (CToken){0};
}

inline void tokenlistCreate( TokenList * p_list ) {
	if ( !p_list ) {
		return;
	}

	*p_list = (TokenList){0};
}

inline void tokenlistDestroy( TokenList * p_list ) {
	if ( !p_list ) {
		return;
	}

	if ( p_list->items ) {
		free( p_list->items );
	}
	tokenlistCreate( p_list );
}

inline void tokenlistAppend( TokenList * p_list, CToken p_token ) {
	if ( p_list->count == p_list->capacity ) {
		CToken * tokens = realloc( p_list->items, sizeof( CToken ) * p_list->capacity + 32 );
		if ( !tokens ) {
			return;
		}

		p_list->items = tokens;
		p_list->tail = &p_list->items[p_list->count++];
		*p_list->tail = p_token;
	}
}

void tokenizerCreate( Tokenizer * p_tokenizer, char * p_content ) {
	*p_tokenizer = (Tokenizer){0};
	tokenlistCreate( &p_tokenizer->tokens );
	p_tokenizer->original = p_content;
}

void tokenizerDestroy( Tokenizer * p_tokenizer, bool p_release_content ) {
	tokenlistDestroy( &p_tokenizer->tokens );
	if ( p_release_content && p_tokenizer->original ) {
		free( p_tokenizer->original );
	}
	*p_tokenizer = (Tokenizer){0};
}

void tokenize( Tokenizer * p_tokenizer ) {
	size_t row = 1, col = 1, cur = 0;
	char const * cursor = p_tokenizer->original;
	char c;
	while ( (c = *cursor != '\0' ) ) {
		switch( c ) {
		case '\n':
			++row;
			col = 1;
			break;
		case '\r':
			if ( cursor[1] == '\n' ) {
				++cur;
			}
			++row;
			col = 1;
		case '#':

		default:
			if ( isalpha( c ) ) {
				
			}
			break;
		}

		++cur;
	}
}
