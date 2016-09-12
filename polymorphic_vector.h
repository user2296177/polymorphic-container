#ifndef GUT_POLYMORPHIC_STORAGE_H
#define GUT_POLYMORPHIC_STORAGE_H

#include "handle_base.h"
#include "handle.h"
#include "polymorphic_handle.h"
#include "polymorphic_vector_iterator.h"
#include <new>
#include <vector>
#include <utility>
#include <iterator>
#include <cassert>
#include <cstdlib>

namespace gut
{
    template<class B>
    class polymorphic_vector final
    {
    public:
		using value_type = B;
		using reference = value_type&;
		using const_reference = value_type const&;
		using pointer = value_type*;
		using const_pointer = value_type const*;

		using container_type = std::vector<gut::polymorphic_handle>;
        using size_type = container_type::size_type;

		using iterator = gut::polymorphic_vector_iterator<B, false>;
		using const_iterator = gut::polymorphic_vector_iterator<B, true>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		iterator begin() noexcept
		{
			return iterator{ handles_, 0 };
		}

		iterator end() noexcept
		{
			return iterator{ handles_, handles_.size() };
		}

		const_iterator cbegin() const noexcept
		{
			return const_iterator{ handles_, 0 };
		}

		const_iterator cend() const noexcept
		{
			return const_iterator{ handles_, handles_.size() };
		}

		reverse_iterator rbegin() noexcept
		{
			return reverse_iterator{ end() };
		}

		reverse_iterator rend() noexcept
		{
			return reverse_iterator{ begin() };
		}

		const_reverse_iterator crbegin() const noexcept
		{
			return const_reverse_iterator{ cend() };
		}

		const_reverse_iterator crend() const noexcept
		{
			return const_reverse_iterator{ cbegin() };
		}

		~polymorphic_vector() noexcept;

		explicit polymorphic_vector( size_type const initial_capacity = 0 );

		template
		<
			class D,
			std::enable_if_t
			<
				!std::is_same<std::decay_t<D>, polymorphic_vector>::value, int
			> = 0,
			std::enable_if_t<std::is_base_of<B, std::decay_t<D>>::value, int> = 0
		>
		explicit polymorphic_vector( D&& value );

		polymorphic_vector( polymorphic_vector&& other ) noexcept;
		polymorphic_vector& operator=( polymorphic_vector&& other ) noexcept;

		polymorphic_vector( polymorphic_vector const& other );
		polymorphic_vector& operator=( polymorphic_vector const& other );

		template
		<
			class D,
			std::enable_if_t<std::is_base_of<B, std::decay_t<D>>::value, int> = 0
		>
		void push_back( D&& value );

		template
		<
			class D, class... Args,
			std::enable_if_t<std::is_base_of<B, std::decay_t<D>>::value, int> = 0,
			std::enable_if_t
			<
				std::is_constructible<std::decay_t<D>, Args&&...>::value, int
			> = 0
		>
		void emplace_back( Args&&... value );

		void pop_back();

		void clear() noexcept;
		void swap( polymorphic_vector& other ) noexcept;

		reference operator[]( size_type const i ) noexcept;
		const_reference operator[]( size_type const i ) const noexcept;

		reference at( size_type const i );
		const_reference at( size_type const i ) const;

		reference front();
		const_reference front() const;

		reference back() noexcept;
		const_reference back() const noexcept;

		size_type size() const noexcept;
		bool empty() const noexcept;

        template<class D>
		void ensure_capacity();

		void unchecked_erase( size_type const i, size_type const j )
		{
			gut::polymorphic_handle& h{ handles_[ i ] };
            size_ = reinterpret_cast<byte*>( h->src() ) - h->padding() - data_;

			for ( size_type k{ i }; k != j; ++k )
            {
				handles_[ k ]->destroy();
            }

            /*
			 * unimplemented fix for VC++, worth implementing?????
			 * this can occur more than once, so this check must be done for
			 * every type

			if ( destroyed_size < sizeof( next_value )
			{
				std::aligned_storage_t s;
				next_value.transfer( &s );
				next_value.transfer( data_ + size_, size_ );
			}

			 */

			auto handles_begin = handles_.begin();
			handles_.erase( handles_begin + i, handles_begin + j );

			for ( size_type k{ i }, sz{ handles_.size() }; k != sz; ++k )
            {
				handles_[ k ]->transfer( data_ + size_, size_ );
            }
		}

    private:
		using byte = unsigned char;

		void ensure_index_bounds( size_type const i ) const
		{
			if ( i >= handles_.size() )
			{
				throw std::out_of_range
				{
					"polymorphic_vector<B>::"
					"ensure_index_bounds( size_type const i );\n"
					"index out of range"
				};
			}
		}

        std::vector<gut::polymorphic_handle> handles_;
        byte* data_;
        size_type size_;
        size_type capacity_;
    };
}

template<class B>
gut::polymorphic_vector<B>::~polymorphic_vector() noexcept
{
	for ( gut::polymorphic_handle& h : handles_ )
	{
		h->destroy();
	}
	std::free( data_ );
}

template<class B>
inline
gut::polymorphic_vector<B>::polymorphic_vector( size_type const initial_capacity )
{
	byte* new_data{ reinterpret_cast<byte*>( std::malloc( initial_capacity ) ) };
	if ( new_data )
	{
		data_ = new_data;
		size_ = 0;
		capacity_ = initial_capacity;
	}
	else
	{
		throw std::bad_alloc{};
	}
}

template<class B>
template
<
	class D,
	std::enable_if_t
	<
		!std::is_same<std::decay_t<D>, gut::polymorphic_vector<B>>::value, int
	>,
	std::enable_if_t<std::is_base_of<B, std::decay_t<D>>::value, int>
>
gut::polymorphic_vector<B>::polymorphic_vector( D&& value )
	: data_{ nullptr }
	, size_{ 0 }
	, capacity_{ 0 }
{
	using der_t = std::decay_t<D>;

	byte* new_data{ reinterpret_cast<byte*>(
		std::malloc( sizeof( der_t ) + alignof( der_t ) ) ) };

	if ( new_data )
	{
		data_ = new_data;
		size_ = sizeof( der_t );
		capacity_ = sizeof( der_t ) + alignof( der_t );
		handles_.emplace_back( gut::handle<der_t>
		{
			::new ( data_ ) der_t{ std::forward<D>( value ) }
		} );
	}
	else
	{
		throw std::bad_alloc{};
	}
}

template<class B>
gut::polymorphic_vector<B>::polymorphic_vector( polymorphic_vector&& other )
noexcept
	: handles_{ std::move( other.handles_ ) }
	, data_{ other.data_ }
	, size_{ other.size_ }
	, capacity_{ other.capacity_ }
{
	other.data_ = nullptr;
}

template<class B>
inline gut::polymorphic_vector<B>&
gut::polymorphic_vector<B>::operator=( polymorphic_vector&& other ) noexcept
{
	if ( this != &other )
	{
		for ( gut::polymorphic_handle& h : handles_ )
		{
			h->destroy();
		}
		handles_ = std::move( other.handles_ );
		std::free( data_ );
		data_ = other.data_;
		other.data_ = nullptr;
		size_ = other.size_;
		capacity_ = other.capacity_;
	}
	return *this;
}

template<class B>
inline
gut::polymorphic_vector<B>::polymorphic_vector( polymorphic_vector const& other )
	: data_{ reinterpret_cast<byte*>( std::malloc( other.size_ ) ) }
	, size_{ 0 }
	, capacity_{ 0 }
{
	if ( data_ )
	{
		gut::polymorphic_handle hnd;
		for ( gut::polymorphic_handle const& h : other.handles_ )
		{
			h->copy( data_ + size_, hnd, size_ );
			handles_.emplace_back( std::move( hnd ) );
		}
		capacity_ = other.size_;
		assert( capacity_ == size_ );
	}
	else
	{
		throw std::bad_alloc{};
	}
}

template<class B>
inline gut::polymorphic_vector<B>&
gut::polymorphic_vector<B>::operator=( polymorphic_vector const& other )
{
	if ( this != &other )
	{
		byte* new_data = reinterpret_cast<byte*>( std::malloc( other.size_ ) );
		if ( new_data )
		{
			for ( gut::polymorphic_handle& h : handles_ )
			{
				h->destroy();
			}

			std::free( data_ );
			data_ = new_data;
			size_ = 0;
			capacity_ = other.size_;

			gut::polymorphic_handle hnd;
			for ( gut::polymorphic_handle const& h : other.handles_ )
			{
				h->copy( data_ + size_, hnd, size_ );
				handles_.emplace_back( std::move( hnd ) );
			}
			assert( capacity_ == size_ );
		}
		else
		{
			throw std::bad_alloc{};
		}
	}
	return *this;
}

template<class B>
template
<
	class D,
	std::enable_if_t<std::is_base_of<B, std::decay_t<D>>::value, int>
>
inline void
gut::polymorphic_vector<B>::push_back( D&& value )
{
	emplace_back<std::decay_t<D>>( std::forward<D>( value ) );
}

template<class B>
template
<
	class D, class... Args,
	std::enable_if_t<std::is_base_of<B, std::decay_t<D>>::value, int>,
	std::enable_if_t
	<
		std::is_constructible<std::decay_t<D>, Args&&...>::value, int
	>
>
inline void
gut::polymorphic_vector<B>::emplace_back( Args&&... args )
{
	using der_t = std::decay_t<D>;
	ensure_capacity<der_t>();
	der_t* p{ ::new ( data_ + size_ ) der_t{ std::forward<Args&&>( args )... } };
	size_ += sizeof( der_t );
	handles_.emplace_back( gut::handle<der_t>{ p } );
}

template<class B>
inline void
gut::polymorphic_vector<B>::pop_back()
{
    gut::polymorphic_handle& h{ handles_.back() };
    size_ = reinterpret_cast<byte*>( h->src() ) - h->padding() - data_;
	h->destroy();
	handles_.pop_back();
}

template<class B>
inline void
gut::polymorphic_vector<B>::clear() noexcept
{
	for ( gut::polymorphic_handle& h : handles_ )
	{
		h->destroy();
	}
	size_ = 0;
	handles_.clear();
}

template<class B>
inline void
gut::polymorphic_vector<B>::swap( polymorphic_vector<B>& other ) noexcept
{
	std::swap( handles_, other.handles_ );
	std::swap( data_, other.data_ );
	std::swap( size_, other.size_ );
	std::swap( capacity_, other.capacity_ );
}

template<class B>
inline typename gut::polymorphic_vector<B>::reference
gut::polymorphic_vector<B>::operator[]( size_type const i ) noexcept
{
	return *reinterpret_cast<pointer>( handles_[ i ]->src() );
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_reference
gut::polymorphic_vector<B>::operator[]( size_type const i ) const noexcept
{
	return *reinterpret_cast<const_pointer>( handles_[ i ]->src() );
}

template<class B>
inline typename gut::polymorphic_vector<B>::reference
gut::polymorphic_vector<B>::at( size_type const i )
{
	ensure_index_bounds( i );
	return *reinterpret_cast<pointer>( handles_[ i ]->src() );
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_reference
gut::polymorphic_vector<B>::at( size_type const i ) const
{
	ensure_index_bounds( i );
	return *reinterpret_cast<const_pointer>( handles_[ i ]->src() );
}

template<class B>
inline typename gut::polymorphic_vector<B>::reference
gut::polymorphic_vector<B>::front()
{
	return *reinterpret_cast<pointer>( handles_[ 0 ]->src() );
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_reference
gut::polymorphic_vector<B>::front() const
{
	return *reinterpret_cast<const_pointer>( handles_[ 0 ]->src() );
}

template<class B>
inline typename gut::polymorphic_vector<B>::reference
gut::polymorphic_vector<B>::back() noexcept
{
	return *reinterpret_cast<pointer>( handles_[ handles_.size() - 1 ]->src() );
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_reference
gut::polymorphic_vector<B>::back() const noexcept
{
	return *reinterpret_cast<pointer>( handles_[ handles_.size() - 1 ]->src() );
}

template<class B>
inline typename gut::polymorphic_vector<B>::size_type
gut::polymorphic_vector<B>::size() const noexcept
{
	return handles_.size();
}

template<class B>
inline bool
gut::polymorphic_vector<B>::empty() const noexcept
{
	return handles_.empty();
}

template<class B>
template<class D>
inline void
gut::polymorphic_vector<B>::ensure_capacity()
{
	using der_t = std::decay_t<D>;

	size_type padding{ gut::calculate_padding<der_t>( data_ + size_ ) };
	if ( capacity_ - size_ < sizeof( der_t ) + padding )
	{
		auto new_capacity = ( sizeof( der_t ) + alignof( der_t ) + capacity_ ) * 2;
		auto new_data = reinterpret_cast<byte*>( std::malloc( new_capacity ) );
		if ( new_data )
		{
			size_ = 0;
			capacity_ = new_capacity;
			for ( auto& h : handles_ )
			{
				h->transfer( new_data + size_, padding );
				size_ += padding;
			}
			std::free( data_ );
			data_ = new_data;
		}
		else
		{
			throw std::bad_alloc{};
		}
	}
	else
	{
		size_ += padding;
	}
}

template <class B>
void swap( gut::polymorphic_vector<B>& x, gut::polymorphic_vector<B>& y )
noexcept( noexcept( x.swap( y ) ) )
{
	x.swap( y );
}
#endif // GUT_POLYMORPHIC_STORAGE_H
