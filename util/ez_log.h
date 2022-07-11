#pragma once

#ifdef DISABLE_EZ_LOG

#define LOGI ::ez::log::log_null()
#define LOGD ::ez::log::log_null()
#define LOGW ::ez::log::log_null()
#define LOGE ::ez::log::log_null()

#else

#ifndef EZ_LOG_GET_FILE
  #define EZ_LOG_GET_FILE __FILE__
#endif

#ifndef EZ_LOG_GET_LINE
  #define EZ_LOG_GET_LINE __LINE__
#endif

#ifndef EZ_LOG_GET_FUNCTION
  #define EZ_LOG_GET_FUNCTION __PRETTY_FUNCTION__
#endif

#ifndef EZ_LOG_INFO_PREFIX
  #define EZ_LOG_INFO_PREFIX  " [ info  ]\t"
#endif

#ifndef EZ_LOG_DEBUG_PREFIX
  #define EZ_LOG_DEBUG_PREFIX " [ debug ]\t"
#endif

#ifndef EZ_LOG_WARN_PREFIX
  #define EZ_LOG_WARN_PREFIX  " [ warn  ]\t"
#endif

#ifndef EZ_LOG_ERROR_PREFIX
  #define EZ_LOG_ERROR_PREFIX " [ error ]\t"
#endif


#define LOGI ::ez::log::log_intermediate::make_log \
  ( EZ_LOG_GET_FILE \
  , EZ_LOG_GET_LINE \
  , EZ_LOG_GET_FUNCTION \
  ) << EZ_LOG_INFO_PREFIX
#define LOGD ::ez::log::log_intermediate::make_log\
  ( EZ_LOG_GET_FILE \
  , EZ_LOG_GET_LINE \
  , EZ_LOG_GET_FUNCTION \
  ) << EZ_LOG_DEBUG_PREFIX
#define LOGW ::ez::log::log_intermediate::make_log \
  ( EZ_LOG_GET_FILE \
  , EZ_LOG_GET_LINE \
  , EZ_LOG_GET_FUNCTION \
  ) << EZ_LOG_WARN_PREFIX
#define LOGE ::ez::log::log_intermediate::make_log \
  ( EZ_LOG_GET_FILE \
  , EZ_LOG_GET_LINE \
  , EZ_LOG_GET_FUNCTION \
  ) << EZ_LOG_ERROR_PREFIX

#endif

#ifndef EZ_LOG_OUT
  #define EZ_LOG_OUT ::std::cout
#endif

#ifndef EZ_LOG_FLUSH
  #define EZ_LOG_FLUSH ::std::flush
#endif

#include <string>
#include <iostream>
#include <iomanip>

namespace ez::log
{
  using namespace std;

#ifdef DISABLE_EZ_LOG
  
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
        EZ_LOG_OUT << s.str() << EZ_LOG_FLUSH;
      }
      catch ( const exception& e )
      { cerr << "\n\n<<<<<\nexception on " << __PRETTY_FUNCTION__ << "\nwhat=" << e.what() << "\n>>>>>\n\n"; }
      catch ( ... )
      { cerr << "\n\n<<<<<\nexception on " << __PRETTY_FUNCTION__ << "\nunknown\n>>>>>\n\n"; }
    }
  };

#endif

}