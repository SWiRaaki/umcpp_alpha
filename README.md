# What is umcpp
The name 'umcpp' stands for **<ins>um</ins>**ka **<ins>c</ins>** **<ins>p</ins>**re**<ins>p</ins>**rocessor
The preprocessor is intented to generate interfaces between umka scripts and c functions. It will either pregenerate APIs from headers or generate interface functions and scripts from within c source files.

# What does it 'process'

Imagine the following input:
`test.umc`
```c
#include <stdlib.h>
#include <stdio.h>

#include <umka_api.h>

typedef struct div_result {
    long quot, rem;
} div_result;

static inline int foo( void );
static inline div_result * divide( long a, long b );
static inline void hello( void );

#MODULE   bar
#FUNCTION foo    int        void
#FUNCTION divide div_result a:int, b:int
#FUNCTION hello  void       void

int foo( void ) {
    return 69;
}

div_result * divide( long a, long b ) {
    div_result * res = malloc( sizeof( div_result ) );
    *res = (div_result){
        a / b,
        a % b
    };
    return res;
}

void hello( void ) {
    div_result * res = divide( 100, foo() );
    printf( "Hello, World!\n" );
}
```

This file, when run through the preprocessor as `umcpp test.umc`, results in a new file:
`test.um.c`
```c
#line 1 "test.umc"
#include <stdlib.h>
#include <stdio.h>

#include <umka_api.h>

typedef struct div_result {
    long quot, rem;
} div_result;

static inline int foo( void );
static inline div_result * divide( long a, long b );
static inline void hello( void );

#line 14
//#MODULE   bar
#line 15
UMKA_EXPORT void umc_foo( UmkaStackSlot * params, UmkaStackSlot * result ) {
    Umka * umka = umkaGetInstance( result );
    UmkaAPI * api = umkaGetAPI( umka );
    (void)(params);
    result->intVal = foo( );
}
#line 16
UMKA_EXPORT void umc_divide( UmkaStackSlot * params, UmkaStackSlot * result ) {
    Umka * umka = umkaGetInstance( result );
    UmkaAPI * api = umkaGetAPI( umka );
    int64_t a = umkaGetParam( params, 0 )->intVal;
    int64_t b = umkaGetParam( params, 1 )->intVal;
    result->ptrVal = divide( a,  b );
}
#line 17
UMKA_EXPORT void umc_hello( UmkaStackSlot * params, UmkaStackSlot * result ) {
    Umka * umka = umkaGetInstance( result );
    UmkaAPI * api = umkaGetAPI( umka );
    (void)(params);
    (void)(result);
}
#line 18

int foo( void ) {
    return 69;
}

div_result * divide( long a, long b ) {
    div_result * res = malloc( sizeof( div_result ) );
    *res = (div_result){
        a / b,
        a % b
    };
    return res;
}

void hello( void ) {
    div_result * res = divide( 100, foo() );
    printf( "Hello, World!\n" );
}
```

It reads custom '#'-directives to generate source that is callable from umka scripts. The resulting source is compilable, and should be even with stronger warnings and errors:
```sh
~/dev/git/umcpp_alpha$ tcc -isystem /home/user/pkg/umka/include -nostdlib -fno-exceptions -pedantic -Wall -Wextra -Wconversion -Wreturn-type -Werror=all -Werror=extra -Werror=conversion -Werror=return-type -Wno-unused-function -DRELEASE -O3 -std=gnu99 -MD -o test.um.o -c test.um.c
~/dev/git/umcpp_alpha$ ll
total 44
...
-rw-r--r-- 1 user user 1289 Nov 11 10:25 test.um.c
-rw-r--r-- 1 user user  643 Nov 11 08:53 test.umc
-rw-r--r-- 1 user user   25 Nov 11 10:26 test.um.d
-rw-r--r-- 1 user user 1792 Nov 11 10:26 test.um.o
...
```
