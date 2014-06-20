#include "instrumentation.h"

#include <assert.h>

#include <stdio.h>
#include <time.h>
#include <string.h>

#include <signal.h>
#include <execinfo.h>
#include <pthread.h>

#include <cxxabi.h>

void memory_status( void )
{
// Inaccurate memory information
   unsigned long total, resident, shared, text, stack, libs, dirty;
   
   FILE* fp = fopen( "/proc/self/statm", "r");

   if( fp != NULL )
   {
      if( 7 == fscanf( fp, "%lu %lu %lu %lu %lu %lu %lu", &total, &resident, &shared, &text, &stack, &libs, &dirty ) ) 
      {
         printf( "Memory information \n" );
         printf( "\tTotal program size %lu\n", total );
         printf( "\tResident set size  %lu\n", resident );
         printf( "\tShared pages       %lu\n", shared );
         printf( "\tText (code)        %lu\n", text );
         printf( "\tData/stack         %lu\n", stack );
         printf( "\tLibrary            %lu\n", libs );
         printf( "\tDirty pages        %lu\n", dirty );
      }

      fclose( fp );
   }
}

int dlib_callback( struct dl_phdr_info* info, size_t size, void* data )
{
// List the shared object

#ifdef __VERBOSE__
   int j;
#endif

   assert( info != NULL );
   assert( data == NULL );

#ifndef __VERBOSE__
   printf( "\t Object's name = %s\n", info->dlpi_name );
#else
   printf( "\t Object's name = %s has %d segments\n", info->dlpi_name, info->dlpi_phnum );
#endif

#ifdef __VERBOSE__
   for ( j = 0; j < info->dlpi_phnum; j++ )
      printf( "\t header %3d: segments' (virtual memory) address=%10p\n", j, ( void* ) ( info->dlpi_addr + info->dlpi_phdr[j].p_vaddr ) );
#endif

   return 0;
}

void sig_handler( int sig ) 
{
// This allows us to terminate the program and exit via the destructors

   printf( "Received signal " );

   switch ( sig )
   {
      case SIGINT  : printf ( "SIGINT" ); break;
      case SIGABRT : printf ( "SIGABRT" ); break;
      case SIGTERM : printf ( "SIGTERM" ); break;
      case SIGHUP  : printf ( "SIGHUP" ); break;
      default      : printf ( "%d", sig );
   }

   printf( ". Stoppping instrumentation.\n" );

   exit( EXIT_SUCCESS );
}

void sig_handler_setup( void )
{
   if( signal( SIGINT, sig_handler ) == SIG_ERR )
   {
      printf( "Cannot enable signal handling for %d (SIGINT)\n", SIGINT );
   }

   if( signal( SIGABRT, sig_handler ) == SIG_ERR )
   {
      printf( "Cannot enable signal handling for %d (SIGABRT)\n", SIGABRT );
   }

   if( signal( SIGTERM, sig_handler ) == SIG_ERR )
   {
      printf( "Cannot enable signal handling for %d (SIGTERM)\n", SIGTERM );
   }

   if( signal( SIGHUP, sig_handler ) == SIG_ERR )
   {
      printf( "Cannot enable signal handling for %d (SIGHUP)\n", SIGHUP );
   }
}

void trace_begin( void )
{
// Called before main()

   printf( "Listing loaded (at start-up) shared objects ...\n" );

   // Calls callback for every shared object
   dl_iterate_phdr( dlib_callback, NULL );

   // Initial memory information
   memory_status();
}
 
void trace_end( void )
{
// Called after main() or exit()

   printf( "Listing loaded (at clean-up) shared objects ...\n" );

   // Calls callback for every shared object
   dl_iterate_phdr( dlib_callback, NULL );

   // Final memory information
   memory_status();
}

void resolve( void* pointer )
{
// Get the dynamic (relocated) addresses
   printf("\t%p --> ", pointer);

   // Only resolves dynamic symbols; for static symbols use nm --demangle or other tool
   char** data = backtrace_symbols( &pointer, 1 );

   if( data != NULL)
   {
      // We only requested a single trace line
      printf( "%s --> ", *data );

      unsigned long length =  strlen( ( const char* ) *data );

      char* prefix = ( char* ) malloc ( sizeof ( char ) * length );
      if( prefix == NULL )
      {
         printf( "? [failed malloc prefix]" );
         free ( data );
         return;
      }

      char* numfix = ( char* ) malloc ( sizeof ( char ) * length );
      if( numfix == NULL )
      {
         printf( "? [failed malloc numfix]" );
         free ( data );
         free ( prefix );
         return;
      }

      int matches = 0;

      matches = sscanf ( *data, "%[^(](%[^)]", prefix, numfix );

      if( matches == 2 )
      {
         // Re-use allocated space
         char* symbol = prefix;

         matches = sscanf ( numfix, "%[^+]", symbol );

         if( matches == 1 )
         {
            char* buffer = NULL;
            int status = 0;

            char* demangled_name = __cxa_demangle( ( const char* ) symbol, buffer, &length, &status );

            switch( status )
            {
               case 0  : // Print the demangled name
                  printf( "%s", demangled_name );
                  break;
               case -2 : // Print the (probably C) symbol
                  printf( "%s", symbol);
                  break;
               case -1 :
               case -3 :
               default :
                  printf( "? [demangle error %d]", status );
            }
         }
      }
      else
         printf( "? [no symbol match]" ); // Try another tool, e.g. nm --demangle, to extract the static symbols

      free ( data );
      free ( prefix ); // Alias for symbol
      free ( numfix );
   }
   else
      printf( "? [no backtrace symbol]" );

   printf("\n");
} 

void __cyg_profile_func_enter( void* func, void* callsite )
{
   struct timespec ts;

   if( clock_gettime( CLOCK_MONOTONIC_RAW, &ts ) < 0 )
   {
      ts.tv_sec = 0;
      ts.tv_nsec = 0;
   }

   // typedef unsigned long int pthread_t;
   pthread_t id = pthread_self();

   printf( "--> \t%lu.%lu \t %lu \t %p \t %p \n", ts.tv_sec, ts.tv_nsec, id, callsite, func );

   resolve ( callsite );
   resolve ( func );
}
 
void __cyg_profile_func_exit( void* func, void* callsite )
{
   struct timespec ts;

   if( clock_gettime( CLOCK_MONOTONIC_RAW, &ts ) < 0 )
   {
      ts.tv_sec = 0;
      ts.tv_nsec = 0;
   }

   // typedef unsigned long int pthread_t;
   pthread_t id = pthread_self();

   printf( "<-- \t%lu.%lu \t %lu \t %p \t %p \n", ts.tv_sec, ts.tv_nsec, id, callsite, func );

   resolve ( callsite );
   resolve ( func );
}
