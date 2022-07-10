/// @file
/// @brief すぐ使える実用的な簡易ロガー
/// @description LOGI, LOGD, LOGW, LOGE の4つのマクロが定義され、それぞれをログ出力ストリームと見做して LOGI << "hoge"; のように使う。
/// LOGI ... info  レベルのログメッセージ出力（白色）
/// LOGD ... debug レベルのログメッセージ出力（緑色）
/// LOGW ... warn  レベルのログメッセージ出力（黄色）
/// LOGE ... error レベルのログメッセージ出力（赤色）
/// @note
/// (a): USE_EMB_LOG_BOOST_CPU_TIMER が定義されている場合、時間計測に boost::timer::cpu_timer を使用する（ boost.timer 関連のライブラリーのリンクが必要 ）
/// (b): USE_EMB_LOG_BOOST_CHRONO が定義されている場合、時間計測に boost::chrono::steady_clock を使用する ( boost.chrono 関連のライブラリーのリンクが必要)
/// (c): USE_EMB_LOG_STD_CHRONO が定義されている場合、時間計測に std::chrono::steady_clock を使用する（ 外部ライブラリーのリンクは不要だが、処理系によっては分解能が不足する ）
/// (d): (a), (b), (c) の何れも定義されていない場合、時間計測に usagi::chrono::default_clock を使用する（ 外部ライブラリーは不要、windows処理系でもQueryPerformanceCounterを内部的に使用する ）
/// (f): DISABLE_EMB_LOG が定義されている場合、全てのログ出力は事実上無効になる。
/// EMB_LOG_GET_{ FUNCTION | FILE | LINE } を事前に定義しておくとユーザー定義の何かそういうものに置き換え可能。（ "" をユーザー定義すれば出力から消す事も可能。 ）
/// EMB_LOG_{INFO|DEBUG|WARN|ERROR}_PREFIX を事前に定義しておくと [ info ] 的な部分をユーザー定義に置き換え可能。（ "" をユーザー定義すれば出力から消す事も可能。 ）

#pragma once

#ifdef DISABLE_EMB_LOG

#define LOGI ::emb::log::log_null()
#define LOGD ::emb::log::log_null()
#define LOGW ::emb::log::log_null()
#define LOGE ::emb::log::log_null()

#else

#ifndef EMB_LOG_GET_FILE
  #define EMB_LOG_GET_FILE __FILE__
#endif

#ifndef EMB_LOG_GET_LINE
  #define EMB_LOG_GET_LINE __LINE__
#endif

#ifndef EMB_LOG_GET_FUNCTION
  #define EMB_LOG_GET_FUNCTION __PRETTY_FUNCTION__
#endif

#ifndef EMB_LOG_INFO_PREFIX
  #define EMB_LOG_INFO_PREFIX  " [ info  ]\t"
#endif

#ifndef EMB_LOG_DEBUG_PREFIX
  #define EMB_LOG_DEBUG_PREFIX " [ debug ]\t"
#endif

#ifndef EMB_LOG_WARN_PREFIX
  #define EMB_LOG_WARN_PREFIX  " [ warn  ]\t"
#endif

#ifndef EMB_LOG_ERROR_PREFIX
  #define EMB_LOG_ERROR_PREFIX " [ error ]\t"
#endif


#define LOGI ::emb::log::log_intermediate::make_log \
  ( EMB_LOG_GET_FILE \
  , EMB_LOG_GET_LINE \
  , EMB_LOG_GET_FUNCTION \
  ) << EMB_LOG_INFO_PREFIX
#define LOGD ::emb::log::log_intermediate::make_log\
  ( EMB_LOG_GET_FILE \
  , EMB_LOG_GET_LINE \
  , EMB_LOG_GET_FUNCTION \
  ) << EMB_LOG_DEBUG_PREFIX
#define LOGW ::emb::log::log_intermediate::make_log \
  ( EMB_LOG_GET_FILE \
  , EMB_LOG_GET_LINE \
  , EMB_LOG_GET_FUNCTION \
  ) << EMB_LOG_WARN_PREFIX
#define LOGE ::emb::log::log_intermediate::make_log \
  ( EMB_LOG_GET_FILE \
  , EMB_LOG_GET_LINE \
  , EMB_LOG_GET_FUNCTION \
  ) << EMB_LOG_ERROR_PREFIX

#endif

#ifndef EMB_LOG_OUT
  #define EMB_LOG_OUT ::std::cout
#endif

#ifndef EMB_LOG_FLUSH
  #define EMB_LOG_FLUSH ::std::flush
#endif

#include <string>
#include <iostream>
#include <iomanip>

namespace emb::log
{
  using namespace std;

#ifdef DISABLE_EMB_LOG
  
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
        EMB_LOG_OUT << s.str() << EMB_LOG_FLUSH;
      }
      catch ( const exception& e )
      { cerr << "\n\n<<<<<\nexception on " << __PRETTY_FUNCTION__ << "\nwhat=" << e.what() << "\n>>>>>\n\n"; }
      catch ( ... )
      { cerr << "\n\n<<<<<\nexception on " << __PRETTY_FUNCTION__ << "\nunknown\n>>>>>\n\n"; }
    }
  };

#endif

}