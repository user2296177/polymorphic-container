#ifndef GUT_HANDLE_H
#define GUT_HANDLE_H

#include "handle_base.h"
#include <type_traits>

namespace gut
{
	template<class T>
	static std::size_t calculate_padding( void* p ) noexcept
	{
		std::size_t r{ reinterpret_cast<std::uintptr_t>( p ) % alignof( T ) };
		return r == 0 ? 0 : alignof( T ) - r;
	}

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
			: handle_base( src, 0 )
		{}

		handle( handle&& other ) noexcept
			: handle_base( other.src_, other.padding_ )
		{
			other.src_ = nullptr;
		}

		handle& operator=( handle&& other ) noexcept
		{
			src_ = other.src_;
			other.src_ = nullptr;
		}

		virtual void copy( void* dst,
			gut::polymorphic_handle& out_handle, std::size_t& out_size ) const
		noexcept( std::is_nothrow_copy_constructible<T>::value ) override
		{
			padding_ = gut::calculate_padding<T>( dst );
			T* p{ ::new ( reinterpret_cast<byte*>( dst ) + padding_ ) T
			{
				*reinterpret_cast<T*>( src_ )
			} };
			out_handle = gut::polymorphic_handle{ gut::handle<T>{ p } };
			out_size += sizeof( T ) + padding_;
		}

		virtual void transfer( void* dst, std::size_t& out_size )
		noexcept( noexcept(
			std::declval<handle>().transfer( is_moveable, dst ) ) ) override
		{
			padding_ = gut::calculate_padding<T>( dst );
			transfer( is_moveable, reinterpret_cast<byte*>( dst ) + padding_ );
			out_size += sizeof( T ) + padding_;
		}

		virtual void destroy()
		noexcept( std::is_nothrow_destructible<T>::value )
		{
			reinterpret_cast<T*>( src_ )->~T();
			src_ = nullptr;
		}

		virtual void destroy( std::size_t& out_size )
		noexcept( std::is_nothrow_destructible<T>::value )
		{
			reinterpret_cast<T*>( src_ )->~T();
			src_ = nullptr;
			out_size -= sizeof( T ) + padding_;
		}

	private:
		void transfer( std::true_type, void* dst )
			noexcept( std::is_nothrow_move_assignable<T>::value )
		{
			src_ = ::new ( dst ) T{ std::move( *reinterpret_cast<T*>( src_ ) ) };
		}

		void transfer( std::false_type, void* dst )
			noexcept( std::is_nothrow_copy_assignable<T>::value )
		{
			T* old_src{ src_ };
			src_ = ::new ( dst ) T{ *reinterpret_cast<T*>( src_ ) };
			old_src->~T();
		}
	};
}
#endif // GUT_HANDLE_H