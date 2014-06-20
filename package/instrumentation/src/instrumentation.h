#ifndef __INSTRUMENTATION__
#define __INSTRUMENTATION__

#include <stdlib.h>

//_GNU_SOURCE should be defined to properly include link.h
#include <link.h>

#ifdef __cplusplus
extern "C" {
#endif

void sig_handler_setup( void ) __attribute__( ( constructor( 101 ), no_instrument_function ) );
//void sig_handler_cleanup( void ) __attribute__( ( destructor( 101 ), no_instrument_function ) );

void trace_begin( void ) __attribute__( ( constructor( 102 ), no_instrument_function ) );
void trace_end( void ) __attribute__( ( destructor( 102 ), no_instrument_function ) );

void __cyg_profile_func_enter( void* func, void* callsite ) __attribute__( ( no_instrument_function ) ); 
void __cyg_profile_func_exit( void* func, void* callsite ) __attribute__( ( no_instrument_function ) ); 

int dlib_callback( struct dl_phdr_info* info, size_t size, void* data ) __attribute__( ( no_instrument_function ) ); 
void sig_handler( int sig ) __attribute__( ( no_instrument_function ) ); 
void memory_status( void ) __attribute__( ( no_instrument_function ) ); 
void resolve( void* pointer ) __attribute__( ( no_instrument_function ) ); 

#ifdef __cplusplus
}
#endif

#endif
