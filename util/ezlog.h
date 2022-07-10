#pragma once

#ifdef DISABLE_EZLOG

#define LOGI ::ezlog::log_null()
#define LOGD ::ezlog::log_null()
#define LOGW ::ezlog::log_null()
#define LOGE ::ezlog::log_null()

#else

#ifndef EZLOG_GET_FILE
  #define EZLOG_GET_FILE __FILE__
#endif

#ifndef EZLOG_GET_LINE
  #define EZLOG_GET_LINE __LINE__
#endif

#ifndef EZLOG_GET_FUNCTION
  #define EZLOG_GET_FUNCTION __PRETTY_FUNCTION__
#endif

#ifndef EZLOG_INFO_PREFIX
  #define EZLOG_INFO_PREFIX  " [ info  ]\t"
#endif

#ifndef EZLOG_DEBUG_PREFIX
  #define EZLOG_DEBUG_PREFIX " [ debug ]\t"
#endif

#ifndef EZLOG_WARN_PREFIX
  #define EZLOG_WARN_PREFIX  " [ warn  ]\t"
#endif

#ifndef EZLOG_ERROR_PREFIX
  #define EZLOG_ERROR_PREFIX " [ error ]\t"
#endif


#define LOGI ::ezlog::log_intermediate::make_log \
  ( EZLOG_GET_FILE \
  , EZLOG_GET_LINE \
  , EZLOG_GET_FUNCTION \
  ) << EZLOG_INFO_PREFIX
#define LOGD ::ezlog::log_intermediate::make_log\
  ( EZLOG_GET_FILE \
  , EZLOG_GET_LINE \
  , EZLOG_GET_FUNCTION \
  ) << EZLOG_DEBUG_PREFIX
#define LOGW ::ezlog::log_intermediate::make_log \
  ( EZLOG_GET_FILE \
  , EZLOG_GET_LINE \
  , EZLOG_GET_FUNCTION \
  ) << EZLOG_WARN_PREFIX
#define LOGE ::ezlog::log_intermediate::make_log \
  ( EZLOG_GET_FILE \
  , EZLOG_GET_LINE \
  , EZLOG_GET_FUNCTION \
  ) << EZLOG_ERROR_PREFIX

#endif

#ifndef EZLOG_OUT
  #define EZLOG_OUT ::std::cout
#endif

#ifndef EZLOG_FLUSH
  #define EZLOG_FLUSH ::std::flush
#endif

#include <string>
#include <iostream>
#include <iomanip>

namespace ezlog
{
  using namespace std;

#ifdef DISABLE_EZLOG
  
  struct log_null
  {
    template < typename T >
    decltype( auto ) operator<<( const T& ) { return *this; }
  };
  
#else
    
  struct log_intermediate
  {
    stringstream buffer;
    
    const char* source;
    const size_t line;
    const char* function;
           
    log_intermediate( log_intermediate&& a )
      : buffer( move( a.buffer ) )
      , source( a.source )
      , line( a.line )
      , function( a.function )
    { }
    
    log_intermediate( const char* source_, const size_t line_, const char* function_ )
      : source( source_ )
      , line( line_ )
      , function( function_ )
    { }
    
    static auto make_log ( const char* s, const size_t l, const char* f ) { return log_intermediate( s, l, f ); }
    
    template < typename T >
    decltype( auto ) operator<<( const T& in ) noexcept
    {
      try
      { return buffer << in; }
      catch ( const exception& e )
      { return buffer << "<<<<<exception on " << __PRETTY_FUNCTION__ << " what=" << e.what() << ">>>>>"; }
      catch ( ... )
      { return buffer << "<<<<<exception on " << __PRETTY_FUNCTION__ << " uknown>>>>>"; }
    }
    
    ~log_intermediate() noexcept
    {
      try
      {
        stringstream s;
        s << buffer.str()
          << "\t"
          << source << ':' << line << '\t' << function
          << endl
          ;
        EZLOG_OUT << s.str() << EZLOG_FLUSH;
      }
      catch ( const exception& e )
      { cerr << "\n\n<<<<<\nexception on " << __PRETTY_FUNCTION__ << "\nwhat=" << e.what() << "\n>>>>>\n\n"; }
      catch ( ... )
      { cerr << "\n\n<<<<<\nexception on " << __PRETTY_FUNCTION__ << "\nunknown\n>>>>>\n\n"; }
    }
  };

#endif

}