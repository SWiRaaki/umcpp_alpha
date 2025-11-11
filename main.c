#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>

#include <libtcc.h>
#include "umka_api.h"

#define UMCPPVER               "v0.1.0"
#define UMCPPMAXLINEWIDTH      0xFFFFU
#define UMCPPDIRECTIVEMODULE   "#MODULE"
#define UMCPPDIRECTIVEFUNCTION "#FUNCTION"

#ifndef DEBUG
#define DEBUG
#endif

#ifdef DEBUG
#define PRINT( str, ... ) printf( str, __VA_ARGS__ )
#else
#define PRINT( str, ... )
#endif

enum {
	PROCESS_UNKNOWN,
	PROCESS_HEADER,
	PROCESS_PREPROCESS
};

enum {
	DIRECTIVE_NODIRECTIVE,
	DIRECTIVE_UNKNOWN,
	DIRECTIVE_INVALID,
	DIRECTIVE_MODULE,
	DIRECTIVE_FUNCTION
};

typedef struct um_module_state {
	char * name;
	char * content;
} ModuleState;

char const * infile;
char const * outfile;
char const * cccl;
ModuleState* mods;
size_t modcount;

char const * getUmkaStackType( char const * type );
char const * getUmkaCType( char const * type );
char * genUmkaFunction( char const * name, char const * rettype, char ** params, size_t params_cnt, size_t cur_line );
int processMethod( char * line );
int processFunction( char * line, size_t cur_line );
int processModule( char * line );
int processLine( char * line, size_t cur_line );
int preprocess( void );
char const * getExtension( char const * file );
int determineProcess( void );
bool parseArgs( int argc, char * argv[] );
void help( void );

int main( int argc, char * argv[] ) {
	PRINT( "Debug activated: %s\n", "true" );
	if ( !parseArgs( argc, argv ) ) {
		help();
		return -1;
	}

	int process = determineProcess();
	switch( process ) {
	case PROCESS_PREPROCESS:
		preprocess();
		break;
	case PROCESS_HEADER:
		printf( "Header parsing is not implemented yet!\n" );
		return 0;
	default:
		printf( "Input file '%s' is not a known format!\n", infile );
		return -1;
	}

	if ( cccl != NULL ) {
		PRINT( "Execute cmd: %s\n", cccl );
		int status = system( cccl );
		PRINT( "cccl result: %d\n", status );
	} else {
		printf( "No compile command provided, finished." );
	}

	return 0;
}

static inline bool isalpha_( int c ) {
	return isalpha( c ) || c == '_';
}

static inline bool isalnum_( int c ) {
	return isalnum( c ) || c == '_';
}

static inline bool _strbgn( char const * str, char const * begin ) {
	return strncmp( str, begin, strlen( begin ) ) == 0;
}

static inline bool _strapp( char * str, char const * append, size_t * cursor ) {
	if ( !cursor ) {
		return false;
	}

	char * begin = str + *cursor;
	size_t len = strlen( append );
	memcpy( begin, append, len );
	*cursor = *cursor + len;
	return true;
}

char const * getUmkaStackType( char const * type ) {
	if ( strcmp( type, "int" ) == 0 ) {
		return type;
	} else if ( strcmp( type, "uint" ) == 0 ) {
		return type;
	} else if ( strcmp( type, "real" ) == 0 ) {
		return type;
	} else if ( strcmp( type, "real32" ) == 0 ) {
		return type;
	} else {
		return "ptr";
	}
}

char const * getUmkaCType( char const * type ) {
	if ( strcmp( type, "int" ) == 0 ) {
		return "int64_t";
	} else if ( strcmp( type, "uint" ) == 0 ) {
		return "uint64_t";
	} else if ( strcmp( type, "real" ) == 0 ) {
		return "double";
	} else if ( strcmp( type, "real32" ) == 0 ) {
		return "float";
	} else {
		return "void *";
	}
}
char * genUmkaFunction( char const * name, char const * rettype, char ** params, size_t params_cnt, size_t cur_line ) {
	size_t curs = 0;
	char * umc = calloc( UMCPPMAXLINEWIDTH, 1 );
	char lnum[128] = {0};
	sprintf( lnum, "#line %zu\n", cur_line );
	_strapp( umc, "UMKA_EXPORT void umc_", &curs );
	_strapp( umc, name, &curs );
	_strapp( umc, "( UmkaStackSlot * params, UmkaStackSlot * result ) {\n", &curs );
	_strapp( umc, lnum, &curs);
	_strapp( umc, "\tUmka * umka = umkaGetInstance( result );\n", &curs );
	_strapp( umc, lnum, &curs);
	_strapp( umc, "\tUmkaAPI * api = umkaGetAPI( umka );\n", &curs );
	_strapp( umc, lnum, &curs);

	bool has_params = false;
	if ( strcmp( params[0], "void" ) != 0 ) {
		has_params = true;
		for ( size_t i = 0; i < params_cnt; ++i ) {
			char * sep = strchr( params[i], ':' );
			_strapp( umc, "\t", &curs );
			_strapp( umc, getUmkaCType( sep ? sep + 1 : "" ), &curs );
			_strapp( umc, " ", &curs );
			strncat( umc + curs, params[i], (unsigned)(sep - params[i]) );
			curs += (unsigned)(sep - params[i]);
			_strapp( umc, " = api->umkaGetParam( params, ", &curs );
			char num[32] = {0};
			sprintf( num, "%zu", i );
			_strapp( umc, num, &curs );
			_strapp( umc, " )->", &curs );
			_strapp( umc, getUmkaStackType( sep+1 ), &curs );
			_strapp( umc, "Val;\n", &curs );
			_strapp( umc, lnum, &curs);
		}
	} else {
		_strapp( umc, "\t(void)(params);\n", &curs );
		_strapp( umc, lnum, &curs);
	}

	bool has_return = false;
	if ( strcmp( rettype, "void" ) != 0 ) {
		has_return = true;
		_strapp( umc, "\tapi->umkaGetResult(params, result)->", &curs );
		_strapp( umc, getUmkaStackType( rettype ), &curs );
		_strapp( umc, "Val = ", &curs );
		_strapp( umc, name, &curs );
		_strapp( umc, "(", &curs );

		if ( strcmp( params[0], "void" ) != 0 ) {
			for ( size_t i = 0; i < params_cnt; ++i ) {
				if ( i != 0 ) {
					_strapp( umc, ", ", &curs );
				}
				char * sep = strchr( params[i], ':' );
				_strapp( umc, " ", &curs );
				strncat( umc + curs, params[i], (unsigned)(sep - params[i]) );
				curs += (unsigned)(sep - params[i]);
			}
		}
		_strapp( umc, " );\n", &curs );
	} else {
		_strapp( umc, "\t(void)(result);\n", &curs );
	}

	_strapp( umc, lnum, &curs);

	if ( !has_params && !has_return ) {
		_strapp( umc, "\t(void)(api);\n", &curs );
		_strapp( umc, lnum, &curs);
	}

	_strapp( umc, "}\n", &curs );

	return umc;
}

int processMethod( char * line ) {
	(void)line;
	return DIRECTIVE_UNKNOWN;
}

int processFunction( char * line, size_t cur_line ) {
	PRINT( "Found Function in line:\n%s\n", line );
	ModuleState * mod = mods + modcount - 1;
	(void)mod;
	char * origin = line;
	line += strlen( UMCPPDIRECTIVEFUNCTION" " );
	char * first;
	char * last;

	while ( isspace( *line ) ) ++line;
	if ( !isalpha_( *line ) ) {
		return DIRECTIVE_INVALID;
	}

	first = line;
	while ( isalnum_( *line ) ) ++line;
	last = line;

	char * name = strndup( first, (unsigned)(last - first) );
	PRINT( "Function name: %s\n", name );

	while ( isspace( *line ) ) ++line;
	if ( !isalpha_( *line ) ) {
		return DIRECTIVE_INVALID;
	}

	first = line;
	while ( isalnum_( *line ) ) ++line;
	last = line;

	char * rettype = strndup( first, (unsigned)(last - first) );
	PRINT( "Function rettype: %s\n", rettype );

	size_t idx = 0;
	char ** params = NULL;

	bool sep_found = false;
	do {
		params = realloc( params, sizeof( char * ) * (idx + 1) );

		while ( isspace( *line ) ) ++line;
		if ( !isalpha_( *line ) ) {
			return DIRECTIVE_INVALID;
		}

		first = line;
		while ( isalnum_( *line ) || *line == ':' ) {
			++line;
		}
		last = line;

		params[idx++] = strndup( first, (unsigned)(last - first) );
		PRINT( "Function parameter %zu: %s\n", idx, params[idx-1] );

		sep_found = *line == ',';
		++line;
	} while ( sep_found );

	char * func = genUmkaFunction( name, rettype, params, idx, cur_line );
	memmove( origin, func, UMCPPMAXLINEWIDTH );
	free( func );
	return DIRECTIVE_FUNCTION;
}

int processModule( char * line ) {
	PRINT( "Found Module in line\n%s\n", line );
	char * origin = line;
	line += strlen( UMCPPDIRECTIVEMODULE" " );
	char * first;
	char * last;
	while ( isspace( *line ) ) ++line;
	if ( !isalpha_( *line ) ) {
		return DIRECTIVE_INVALID;
	}

	first = line;
	while ( isalnum_( *line ) ) ++line;
	last = line;

	ModuleState * ptr = realloc( mods, sizeof( ModuleState ) * (modcount + 1) );
	if ( !ptr ) {
		return DIRECTIVE_INVALID;
	}
	mods = ptr;
	mods[modcount++].name = strndup( first, (unsigned)(last - first) );

	memmove( origin + 2, origin, UMCPPMAXLINEWIDTH - 2 );
	memset( origin, '/', 2 );
	return DIRECTIVE_MODULE;
}

int processLine( char * line, size_t cur_line ) {
	if ( *line != '#' ) {
		return DIRECTIVE_NODIRECTIVE;
	} else if ( _strbgn( line, UMCPPDIRECTIVEMODULE" " ) ) {
		return processModule( line );
	} else if ( _strbgn( line, UMCPPDIRECTIVEFUNCTION" " ) ) {
		return processFunction( line, cur_line );
	} else if ( _strbgn( line, "#METHOD" ) ) {
		return processMethod( line );
	} else {
		PRINT( "Found unknown directive: %s\n", line );
		return DIRECTIVE_UNKNOWN;
	}
}

int preprocess( void ) {
	int ret = 0;
	FILE * fin = fopen( infile, "r" );
	if ( !fin ) {
		printf( "Failed to open input file: %s\n", strerror( errno ) );
		ret = errno;
		goto finish;
	}

	FILE * fout = fopen( outfile, "w+" );
	if ( !fout ) {
		ret = errno;
		printf( "Failed to open output file: %s\n", strerror( ret ) );
		goto finish_in;
	}

	size_t line_cur = 1;
	fprintf( fout, "#line %zu \"%s\"\n", line_cur, infile );
	char line[UMCPPMAXLINEWIDTH];
	int proc_last = DIRECTIVE_NODIRECTIVE;
	while( fgets( line, UMCPPMAXLINEWIDTH, fin ) ) {
		int proc = processLine( line, line_cur );
		switch( proc ) {
		case DIRECTIVE_MODULE: case DIRECTIVE_FUNCTION:
			fprintf( fout, "#line %zu\n", line_cur );
			break;
		default:
			if ( proc_last == DIRECTIVE_MODULE || proc_last == DIRECTIVE_FUNCTION ) {
				fprintf( fout, "#line %zu\n", line_cur );
			}
			break;
		}
		proc_last = proc;
		if ( fputs( line, fout ) < 0 ) {
			ret = ferror( fout );
			printf( "Failed to read line from input file: %s\n", strerror( ret ) );
			goto finish_out;
		}
		++line_cur;
	}

	if ( (ret = ferror( fin )) ) {
		printf( "Failed to read line from input file: %s\n", strerror( ret ) );
		goto finish_out;
	}

	finish_out:
	fclose( fout );
	finish_in:
	fclose( fin );

	finish:
	return 0;
}

char const * getExtension( char const * file ) {
	char const * cur = file + strlen( file );
	while( --cur > file ) {
		if ( *cur == '.' ) {
			return cur;
		}
	}

	return NULL;
}

int determineProcess( void ) {
	char const * ext = getExtension( infile );
	if ( !ext ) {
		return PROCESS_UNKNOWN;
	}

	if ( strcmp( ext, ".h" ) == 0 ) {
		if ( !outfile ) {
			size_t len = strlen( infile );
			char * out = malloc( len + 4 );
			outfile = strcpy( out, infile );
			memcpy( out + len - 2, "um.c", 5);
		}
		return PROCESS_HEADER;
	}

	if ( strcmp( ext, ".umc" ) == 0 ) {
		if ( !outfile ) {
			size_t len = strlen( infile );
			char * out = malloc( len + 2 );
			outfile = strcpy( out, infile );
			memcpy( out + len - 1, ".c", 3);
		}
		return PROCESS_PREPROCESS;
	}

	return PROCESS_UNKNOWN;
}

bool parseArgs( int argc, char * argv[] ) {
	if ( argc < 2 ) {
		return false;
	}

	int args = 1;
	if ( strcmp( argv[args], "-o" ) == 0 ) {
		if ( args + 1 == argc ) {
			return false;
		}

		outfile = strdup( argv[++args] );

		if ( args + 1 == argc ) {
			return false;
		}

		infile = argv[++args];
	} else {
		infile = argv[args];
	}
	++args;
	PRINT( "Argument: %s", argv[args]);

	if ( args < argc ) {
		cccl = argv[args];
	}

	return true;
}

void help( void ) {
	printf( "Umka c preprocessor " UMCPPVER "\n" );
	printf( "Create a c file compilable into an api module\n" );
	printf( "If infile is a header, it will just generate an interface by the given header.\n" );
	printf( "If infile is a .umc c source file, it will be preprocessed for further compilation\n" );
	printf( "If a compile cl is provided, the compiler will be called to compile the processed source file\n" );
	printf( "Usage: umcpp [-o outfile] infile [cc-cl]\n" );
	printf( "       -o     specifiy output filename, optional\n" );
	printf( "       cc-cl  full command line for the compiler, {src} as placeholder for processed infile\n" );
}
