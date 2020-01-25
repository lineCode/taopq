// Copyright (c) 2016-2019 Daniel Frey and Dr. Colin Hirsch
// Please see LICENSE for license or visit https://github.com/taocpp/taopq/

#ifndef TAO_PQ_PARAMETER_TRAITS_HPP
#define TAO_PQ_PARAMETER_TRAITS_HPP

#include <cmath>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include <byteswap.h>
#include <cstring>

#include <tao/pq/internal/gen.hpp>
#include <tao/pq/internal/printf.hpp>
#include <tao/pq/null.hpp>

namespace tao::pq
{
   template< typename T, typename = void >
   struct parameter_traits
   {
      static constexpr std::size_t columns = 1;

      static_assert( sizeof( T ) == 0, "data type not specialized for use as DB parameter" );

      template< std::size_t I >
      [[nodiscard]] static const char* c_str() noexcept
      {
         return nullptr;
      }

      template< std::size_t I >
      [[nodiscard]] static constexpr int size() noexcept
      {
         return 0;
      }

      template< std::size_t I >
      [[nodiscard]] static constexpr int format() noexcept
      {
         return 0;
      }
   };

   namespace internal
   {
      template< typename To, typename From >
      std::enable_if_t< ( sizeof( To ) == sizeof( From ) ) && std::is_trivially_copyable< From >::value && std::is_trivial< To >::value, To >
      bit_cast( const From& src ) noexcept
      {
         To dst;
         std::memcpy( &dst, &src, sizeof( To ) );
         return dst;
      }

      class char_pointer_helper
      {
      private:
         const char* m_p;

      protected:
         explicit char_pointer_helper( const char* p ) noexcept
            : m_p( p )
         {}

      public:
         static constexpr std::size_t columns = 1;

         template< std::size_t I >
         [[nodiscard]] const char* c_str() const noexcept
         {
            static_assert( I < columns );
            return m_p;
         }

         template< std::size_t I >
         [[nodiscard]] static constexpr int size() noexcept
         {
            static_assert( I < columns );
            return 0;
         }

         template< std::size_t I >
         [[nodiscard]] static constexpr int format() noexcept
         {
            static_assert( I < columns );
            return 0;
         }
      };

      class string_helper
      {
      private:
         std::string m_s;

      protected:
         template< typename... Ts >
         explicit string_helper( Ts&&... ts ) noexcept( noexcept( std::string( std::forward< Ts >( ts )... ) ) )
            : m_s( std::forward< Ts >( ts )... )
         {
         }

      public:
         static constexpr std::size_t columns = 1;

         template< std::size_t I >
         [[nodiscard]] const char* c_str() const noexcept
         {
            static_assert( I < columns );
            return m_s.c_str();
         }

         template< std::size_t I >
         [[nodiscard]] static constexpr int size() noexcept
         {
            static_assert( I < columns );
            return 0;
         }

         template< std::size_t I >
         [[nodiscard]] static constexpr int format() noexcept
         {
            static_assert( I < columns );
            return 0;
         }
      };

      template< typename T >
      [[nodiscard]] std::string printf_helper( const char* format, const T v )
      {
         if( std::isfinite( v ) ) {
            return internal::printf( format, v );
         }
         if( std::isnan( v ) ) {
            return "NAN";
         }
         return ( v < 0 ) ? "-INF" : "INF";
      }

   }  // namespace internal

   template<>
   struct parameter_traits< null_t >
   {
      explicit parameter_traits( const null_t& /*unused*/ ) noexcept {}

      static constexpr std::size_t columns = 1;

      template< std::size_t I >
      [[nodiscard]] constexpr const char* c_str() const noexcept
      {
         static_assert( I < columns );
         return nullptr;
      }

      template< std::size_t I >
      [[nodiscard]] static constexpr int size() noexcept
      {
         static_assert( I < columns );
         return 0;
      }

      template< std::size_t I >
      [[nodiscard]] static constexpr int format() noexcept
      {
         static_assert( I < columns );
         return 0;
      }
   };

   template<>
   struct parameter_traits< const char* >
      : internal::char_pointer_helper
   {
      explicit parameter_traits( const char* p ) noexcept
         : char_pointer_helper( p )
      {
      }
   };

   template<>
   struct parameter_traits< std::string >
      : internal::char_pointer_helper
   {
      explicit parameter_traits( const std::string& v ) noexcept
         : char_pointer_helper( v.c_str() )
      {
      }
   };

   template<>
   struct parameter_traits< bool >
      : internal::char_pointer_helper
   {
      explicit parameter_traits( const bool v ) noexcept
         : char_pointer_helper( v ? "TRUE" : "FALSE" )
      {
      }
   };

   template<>
   struct parameter_traits< char >
      : internal::string_helper
   {
      explicit parameter_traits( const char v )
         : string_helper( 1, v )
      {
      }
   };

   template<>
   struct parameter_traits< signed char >
      : internal::string_helper
   {
      explicit parameter_traits( const signed char v )
         : string_helper( internal::printf( "%hhd", v ) )
      {
      }
   };

   template<>
   struct parameter_traits< unsigned char >
      : internal::string_helper
   {
      explicit parameter_traits( const unsigned char v )
         : string_helper( internal::printf( "%hhu", v ) )
      {
      }
   };

   template<>
   struct parameter_traits< short >
   {
      const short m_v;

      static_assert( sizeof( short ) == 2 );

      explicit parameter_traits( const short v ) noexcept
         : m_v( bswap_16( v ) )
      {
      }

      static constexpr std::size_t columns = 1;

      template< std::size_t I >
      [[nodiscard]] const char* c_str() const noexcept
      {
         static_assert( I < columns );
         return reinterpret_cast< const char* >( &m_v );
      }

      template< std::size_t I >
      [[nodiscard]] static constexpr int size() noexcept
      {
         static_assert( I < columns );
         return sizeof( short );
      }

      template< std::size_t I >
      [[nodiscard]] static constexpr int format() noexcept
      {
         static_assert( I < columns );
         return 1;
      }
   };

   template<>
   struct parameter_traits< unsigned short >
      : internal::string_helper
   {
      explicit parameter_traits( const unsigned short v )
         : string_helper( internal::printf( "%hu", v ) )
      {
      }
   };

   template<>
   struct parameter_traits< int >
   {
      const int m_v;

      static_assert( sizeof( int ) == 4 );

      explicit parameter_traits( const int v ) noexcept
         : m_v( bswap_32( v ) )
      {
      }

      static constexpr std::size_t columns = 1;

      template< std::size_t I >
      [[nodiscard]] const char* c_str() const noexcept
      {
         static_assert( I < columns );
         return reinterpret_cast< const char* >( &m_v );
      }

      template< std::size_t I >
      [[nodiscard]] static constexpr int size() noexcept
      {
         static_assert( I < columns );
         return sizeof( int );
      }

      template< std::size_t I >
      [[nodiscard]] static constexpr int format() noexcept
      {
         static_assert( I < columns );
         return 1;
      }
   };

   template<>
   struct parameter_traits< unsigned >
      : internal::string_helper
   {
      explicit parameter_traits( const unsigned v )
         : string_helper( internal::printf( "%u", v ) )
      {
      }
   };

   template<>
   struct parameter_traits< long >
   {
      const long m_v;

      static_assert( sizeof( long ) == 8 );

      explicit parameter_traits( const long v ) noexcept
         : m_v( bswap_64( v ) )
      {
      }

      static constexpr std::size_t columns = 1;

      template< std::size_t I >
      [[nodiscard]] const char* c_str() const noexcept
      {
         static_assert( I < columns );
         return reinterpret_cast< const char* >( &m_v );
      }

      template< std::size_t I >
      [[nodiscard]] static constexpr int size() noexcept
      {
         static_assert( I < columns );
         return sizeof( long );
      }

      template< std::size_t I >
      [[nodiscard]] static constexpr int format() noexcept
      {
         static_assert( I < columns );
         return 1;
      }
   };

   template<>
   struct parameter_traits< unsigned long >
      : internal::string_helper
   {
      explicit parameter_traits( const unsigned long v )
         : string_helper( internal::printf( "%lu", v ) )
      {
      }
   };

   template<>
   struct parameter_traits< long long >
      : internal::string_helper
   {
      explicit parameter_traits( const long long v )
         : string_helper( internal::printf( "%lld", v ) )
      {
      }
   };

   template<>
   struct parameter_traits< unsigned long long >
      : internal::string_helper
   {
      explicit parameter_traits( const unsigned long long v )
         : string_helper( internal::printf( "%llu", v ) )
      {
      }
   };

   template<>
   struct parameter_traits< float >
      : parameter_traits< int >
   {
      explicit parameter_traits( const float v ) noexcept
         : parameter_traits< int >( internal::bit_cast< int >( v ) )
      {
      }
   };

   template<>
   struct parameter_traits< double >
      : parameter_traits< long >
   {
      explicit parameter_traits( const double v )
         : parameter_traits< long >( internal::bit_cast< long >( v ) )
      {
      }
   };

   template<>
   struct parameter_traits< long double >
      : internal::string_helper
   {
      explicit parameter_traits( const long double v )
         : string_helper( internal::printf_helper( "%.21Lg", v ) )
      {
      }
   };

   template< typename T >
   struct parameter_traits< std::optional< T > >
   {
   private:
      using U = parameter_traits< std::decay_t< T > >;
      std::optional< U > m_forwarder;

   public:
      explicit parameter_traits( const std::optional< T >& v )
      {
         if( v ) {
            m_forwarder.emplace( *v );
         }
      }

      explicit parameter_traits( std::optional< T >&& v )
      {
         if( v ) {
            m_forwarder.emplace( std::move( *v ) );
         }
      }

      static constexpr std::size_t columns = 1;
      static_assert( U::columns == 1 );

      template< std::size_t I >
      [[nodiscard]] constexpr const char* c_str() const noexcept
      {
         static_assert( I < columns );
         return m_forwarder ? m_forwarder->template c_str< I >() : nullptr;
      }

      template< std::size_t I >
      [[nodiscard]] static constexpr int size() noexcept
      {
         static_assert( I < columns );
         return U::template size< I >();
      }

      template< std::size_t I >
      [[nodiscard]] static constexpr int format() noexcept
      {
         static_assert( I < columns );
         return U::template format< I >();
      }
   };

   template< typename... Ts >
   struct parameter_traits< std::tuple< Ts... > >
   {
   private:
      using tuple_t = std::tuple< parameter_traits< std::decay_t< Ts > >... >;
      tuple_t m_tuple;

      using gen = internal::gen< parameter_traits< std::decay_t< Ts > >::columns... >;

   public:
      explicit parameter_traits( const std::tuple< Ts... >& tuple ) noexcept( noexcept( tuple_t( tuple ) ) )
         : m_tuple( tuple )
      {}

      explicit parameter_traits( std::tuple< Ts... >&& tuple ) noexcept( noexcept( tuple_t( std::move( tuple ) ) ) )
         : m_tuple( std::move( tuple ) )
      {}

      static constexpr std::size_t columns = ( 0 + ... + parameter_traits< std::decay_t< Ts > >::columns );

      template< std::size_t I >
      [[nodiscard]] const char* c_str() const noexcept
      {
         static_assert( I < columns );
         return std::get< gen::template outer< I > >( m_tuple ).template c_str< gen::template inner< I > >();
      }

      template< std::size_t I >
      [[nodiscard]] static constexpr int size() noexcept
      {
         static_assert( I < columns );
         return std::decay_t< std::tuple_element_t< gen::template outer< I >, tuple_t > >::template size< gen::template inner< I > >();
      }

      template< std::size_t I >
      [[nodiscard]] static constexpr int format() noexcept
      {
         static_assert( I < columns );
         return std::decay_t< std::tuple_element_t< gen::template outer< I >, tuple_t > >::template format< gen::template inner< I > >();
      }
   };

   // detect free function to_taopq_param() found via ADL, simplify user-defined traits
   template< typename T >
   struct parameter_traits< T, std::void_t< decltype( to_taopq_param( std::declval< const T& >() ) ) > >
      : parameter_traits< decltype( to_taopq_param( std::declval< const T& >() ) ) >
   {
      explicit parameter_traits( const T& t ) noexcept( noexcept( parameter_traits< decltype( to_taopq_param( std::declval< const T& >() ) ) >( to_taopq_param( t ) ) ) )
         : parameter_traits< decltype( to_taopq_param( std::declval< const T& >() ) ) >( to_taopq_param( t ) )
      {}
   };

}  // namespace tao::pq

template< typename T >
auto to_taopq_param( const T& t ) noexcept( noexcept( t.to_taopq_param() ) )
   -> decltype( t.to_taopq_param() )
{
   return t.to_taopq_param();
}

#endif
