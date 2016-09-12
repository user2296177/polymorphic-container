#ifndef GUT_HANDLE_H
#define GUT_HANDLE_H

#include "handle_base.h"
#include <cstddef>
#include <cstdint>
#include <utility>
#include <type_traits>

namespace gut
{
	template <class T>
	class handle final : public handle_base
	{
	public:
		using byte = unsigned char;

		static_assert( sizeof( void* ) == sizeof( T* ),
			"incompatible pointer sizes" );

		static constexpr std::integral_constant
		<
			bool, std::is_move_constructible<T>::value
		> is_moveable{};

		handle( T* src ) noexcept
			: handle_base( src )
		{}

		handle( handle&& other ) noexcept
			: handle_base( other.src_ )
		{
			other.src_ = nullptr;
		}

		handle& operator=( handle&& other ) noexcept
		{
			if ( this != &other )
			{
				src_ = other.src_;
				other.src_ = nullptr;
			}
		}

		handle( handle const& ) = delete;
		handle& operator=( handle const& ) = delete;

		virtual size_type size() const noexcept
		{
			return sizeof( T );
		}

		virtual void destroy()
		noexcept( std::is_nothrow_destructible<T>::value ) override
		{
			reinterpret_cast<T*>( src_ )->~T();
			src_ = nullptr;
		}

		virtual void transfer( void* dst )
		noexcept( noexcept( std::declval<handle>().transfer( is_moveable, dst ) ) ) override
		{
			transfer( is_moveable, dst );
		}

		virtual void copy( void* dst, gut::polymorphic_handle& out_handle, std::size_t& out_size ) const
		noexcept( std::is_nothrow_copy_constructible<T>::value ) override
		{
			T* p{ ::new ( dst ) T{ *reinterpret_cast<T*>( src_ ) } };
			out_handle = gut::polymorphic_handle{ gut::handle<T>{ p } };
			out_size += sizeof( T );
		}

	private:
		void transfer( std::true_type, void* dst )
		noexcept( std::is_nothrow_move_constructible<T>::value )
		{
			T* old_src{ reinterpret_cast<T*>( src_ ) };
			src_ = ::new ( dst ) T{ std::move( *old_src ) };
			old_src->~T();
		}

		void transfer( std::false_type, void* dst )
		noexcept( std::is_nothrow_copy_constructible<T>::value )
		{
		    T* old_src{ reinterpret_cast<T*>( src_ ) };
			src_ = ::new ( dst ) T{ *old_src };
			old_src->~T();
		}
	};
}
#endif // GUT_HANDLE_H
