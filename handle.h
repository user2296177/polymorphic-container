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
		static_assert( sizeof( void* ) == sizeof( T* ),
			"incompatible pointer sizes" );

		static constexpr std::integral_constant
		<
			bool, std::is_move_constructible<T>::value
		> is_moveable{};

		~handle() = default;

		handle( void* blk, T* src ) noexcept;

		handle( handle&& other ) noexcept;
		handle& operator=( handle&& other ) noexcept;

		handle( handle const& ) = delete;
		handle& operator=( handle const& ) = delete;

		virtual size_type align() const noexcept;
		virtual size_type size() const noexcept;

		virtual void destroy()
		noexcept( std::is_nothrow_destructible<T>::value )
		override;

		virtual void transfer( void* blk, void* dst )
		noexcept( noexcept( std::declval<handle>().transfer( is_moveable, dst ) ) )
		override;

		virtual void copy( void* blk, void* dst, gut::polymorphic_handle& out_handle ) const
		noexcept( std::is_nothrow_copy_constructible<T>::value )
		override;

	private:
		void transfer( std::true_type, void* dst )
		noexcept( std::is_nothrow_move_constructible<T>::value );

		void transfer( std::false_type, void* dst )
		noexcept( std::is_nothrow_copy_constructible<T>::value );
	};
}
//////////////////////////////////////////////////////////////////////////////////
// constructors
//////////////////////////////////////////////////////////////////////////////////
template<class T>
inline gut::handle<T>::handle( void* block, T* src ) noexcept
	: handle_base( block, src )
{}

template<class T>
inline gut::handle<T>::handle( handle&& other ) noexcept
	: handle_base( other.blk_, other.src_ )
{
	other.src_ = nullptr;
}

template<class T>
inline gut::handle<T>& gut::handle<T>::operator=( handle&& other ) noexcept
{
	if ( this != &other )
	{
		src_ = other.src_;
		blk_ = other.blk_;
		other.src_ = nullptr;
	}
}
//////////////////////////////////////////////////////////////////////////////////
// overriden virtual functions
//////////////////////////////////////////////////////////////////////////////////
template<class T>
inline typename gut::handle<T>::size_type gut::handle<T>::align() const noexcept
{
	return alignof( T );
}

template<class T>
inline typename gut::handle<T>::size_type gut::handle<T>::size() const noexcept
{
	return sizeof( T );
}

template<class T>
inline void gut::handle<T>::destroy()
noexcept( std::is_nothrow_destructible<T>::value )
{
	reinterpret_cast<T*>( src_ )->~T();
	src_ = nullptr;
}

template<class T>
inline void gut::handle<T>::transfer( void* blk, void* dst )
noexcept( noexcept( std::declval<handle>().transfer( is_moveable, dst ) ) )
{
	blk_ = blk;
	transfer( is_moveable, dst );
}

template<class T>
inline void gut::handle<T>::copy( void* blk, void* dst, gut::polymorphic_handle& out_handle )
const noexcept( std::is_nothrow_copy_constructible<T>::value )
{
	T* p{ ::new ( dst ) T{ *reinterpret_cast<T*>( src_ ) } };
	out_handle = gut::polymorphic_handle{ gut::handle<T>{ blk, p } };
}
//////////////////////////////////////////////////////////////////////////////////
// private functions
//////////////////////////////////////////////////////////////////////////////////
template <class T>
inline void gut::handle<T>::transfer( std::true_type, void* dst )
noexcept( std::is_nothrow_move_constructible<T>::value )
{
	T* old_src{ reinterpret_cast<T*>( src_ ) };
	src_ = ::new ( dst ) T{ std::move( *old_src ) };
	old_src->~T();
}

template <class T>
inline void gut::handle<T>::transfer( std::false_type, void* dst )
noexcept( std::is_nothrow_copy_constructible<T>::value )
{
	T* old_src{ reinterpret_cast<T*>( src_ ) };
	src_ = ::new ( dst ) T{ *old_src };
	old_src->~T();
}
#endif // GUT_HANDLE_H
