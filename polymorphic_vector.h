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
	static std::size_t calculate_padding( void* p, std::size_t const align ) noexcept
	{
		std::size_t r{ reinterpret_cast<std::uintptr_t>( p ) % align };
		return r == 0 ? 0 : align - r;
	}

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

		// iterators - complete
		iterator begin() noexcept;
		const_iterator begin() const noexcept;
		iterator end() noexcept;
		const_iterator end() const noexcept;

		reverse_iterator rbegin() noexcept;
		const_reverse_iterator rbegin() const noexcept;
		reverse_iterator rend() noexcept;
		const_reverse_iterator rend() const noexcept;

		const_iterator cbegin() const noexcept;
		const_iterator cend() const noexcept;
		const_reverse_iterator crbegin() const noexcept;
		const_reverse_iterator crend() const noexcept;

		// destructor - complete
		~polymorphic_vector() noexcept;

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

		// modifiers
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
		
		// element access - complete
		reference operator[]( size_type const i ) noexcept;
		const_reference operator[]( size_type const i ) const noexcept;

		reference at( size_type const i );
		const_reference at( size_type const i ) const;

		reference front() noexcept;
		const_reference front() const noexcept;

		reference back() noexcept;
		const_reference back() const noexcept;

		// capacity - missing reserve
		void reserve( size_type const n );
		size_type size() const noexcept;
		bool empty() const noexcept;

		// specialized algorithms - complete
		void swap( polymorphic_vector& other ) noexcept;

	private:
		using byte = unsigned char;

		void copy( polymorphic_vector const& other )
		{
			gut::polymorphic_handle out_h;
			for ( gut::polymorphic_handle const& h : other.handles_ )
			{
				h->copy( data_ +
					( reinterpret_cast<byte*>( h->src() ) - other.data_ ), out_h );

				handles_.emplace_back( std::move( out_h ) );
			}
		}

		void erase_range( size_type const i, size_type const j )
		{
			gut::polymorphic_handle& hi{ handles_[ i ] };
			emplace_offset_ = reinterpret_cast<byte*>( hi->src() ) - data_;
			hi->destroy();

			for ( size_type k{ i + 1 }; k != j; ++k )
			{
				handles_[ k ]->destroy();
			}

			gut::polymorphic_handle& hj{ handles_[ j ] };
			hj->transfer( data_ + emplace_offset_ );
			sizeof_prev_ = hj->size();
			emplace_offset_ = sizeof_prev_;

			for ( size_type k{ j + 1 }, handle_count{ handles_.size() }; k != handle_count; ++k )
			{
				update_emplace_offset( handles_[ k ]->size() );
				handles_[ k ]->transfer( data_ + emplace_offset_ );
				sizeof_prev_ = handles_[ k ]->size();
			}

			auto handles_begin = handles_.begin();
			handles_.erase( handles_begin + i, handles_begin + j );
		}

		void update_emplace_offset( size_type const size ) noexcept
		{
			if ( size > sizeof_prev_ )
			{
				emplace_offset_ += size - sizeof_prev_;
				emplace_offset_ += gut::calculate_padding( data_ + emplace_offset_, alignof( std::max_align_t ) );
			}
			else
			{
				emplace_offset_ += gut::calculate_padding( data_ + emplace_offset_, alignof( std::max_align_t ) );
			}
		}

		template<class D> // TODO: extract transfer loop into function
		std::decay_t<D>* next_alloc_ptr()
		{
			using der_t = std::decay_t<D>;

			size_type previous_offset{ emplace_offset_ };
			update_emplace_offset( sizeof( der_t ) );
			size_type required_size{ sizeof( D ) + emplace_offset_ - previous_offset };

			if ( capacity_ - previous_offset < required_size )
			{
				size_type new_capacity{ ( alignof( der_t ) + required_size + capacity_ ) * 2 };
				byte* new_data{ reinterpret_cast<byte*>( std::malloc( new_capacity ) ) };

				if ( new_data )
				{
					capacity_ = new_capacity;
					sizeof_prev_ = 0;

					gut::polymorphic_handle& h{ handles_.front() };
					h->transfer( new_data );
					sizeof_prev_ = h->size();
					emplace_offset_ = sizeof_prev_;

					for ( size_type k{ 1 }, handle_count{ handles_.size() }; k != handle_count; ++k )
					{
						update_emplace_offset( handles_[ k ]->size() );
						handles_[ k ]->transfer( new_data + emplace_offset_ );
						sizeof_prev_ = handles_[ k ]->size();
					}

					update_emplace_offset( sizeof( der_t ) );
					std::free( data_ );
					data_ = new_data;
				}
				else
				{
					throw std::bad_alloc{};
				}
			}
			sizeof_prev_ = sizeof( der_t );
			return reinterpret_cast<der_t*>( data_ + emplace_offset_ );
		}

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
		size_type emplace_offset_;
		size_type capacity_;
		size_type sizeof_prev_;
	};
}

//////////////////////////////////////////////////////////////////////////////////
// destructor
//////////////////////////////////////////////////////////////////////////////////
template<class B>
gut::polymorphic_vector<B>::~polymorphic_vector() noexcept
{
	for ( gut::polymorphic_handle& h : handles_ )
	{
		h->destroy();
	}
	std::free( data_ );
}
//////////////////////////////////////////////////////////////////////////////////
// constructors
//////////////////////////////////////////////////////////////////////////////////
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
	, emplace_offset_{ 0 }
	, capacity_{ 0 }
{
	using der_t = std::decay_t<D>;

	byte* new_data{ reinterpret_cast<byte*>(
		std::malloc( sizeof( der_t ) + alignof( der_t ) ) ) };

	if ( new_data )
	{
		data_ = new_data;
		emplace_offset_ = sizeof( der_t );
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
	, emplace_offset_{ other.emplace_offset_ }
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
		emplace_offset_ = other.emplace_offset_;
		capacity_ = other.capacity_;
	}
	return *this;
}

template<class B>
inline
gut::polymorphic_vector<B>::polymorphic_vector( polymorphic_vector const& other )
	: data_{ reinterpret_cast<byte*>( std::malloc( other.emplace_offset_ ) ) }
	, emplace_offset_{ 0 }
	, capacity_{ 0 }
{
	if ( data_ )
	{
		emplace_offset_ = other.emplace_offset_;
		capacity_ = other.emplace_offset_;

		gut::polymorphic_handle hnd;
		for ( gut::polymorphic_handle const& h : other.handles_ )
		{
			h->copy( data_ +
				( reinterpret_cast<byte*>( h->src() ) - other.data_ ), hnd );
			
			handles_.emplace_back( std::move( hnd ) );
		}
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
		if ( emplace_offset_ >= other.emplace_offset_ )
		{
			clear();
		}
		else
		{
			byte* new_data = reinterpret_cast<byte*>(
				std::malloc( other.emplace_offset_ ) );

			if ( new_data )
			{
				clear();
				data_ = new_data;
				capacity_ = other.emplace_offset_;
			}
			else
			{
				throw std::bad_alloc{};
			}
		}
		
		emplace_offset_ = other.emplace_offset_;
		sizeof_prev_ = other.sizeof_prev_;
		copy( other );
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////
// iterators
//////////////////////////////////////////////////////////////////////////////////
template<class B>
inline typename gut::polymorphic_vector<B>::iterator
gut::polymorphic_vector<B>::begin() noexcept
{
	return { handles_, 0 };
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_iterator
gut::polymorphic_vector<B>::begin() const noexcept
{
	return { handles_, 0 };
}

template<class B>
inline typename gut::polymorphic_vector<B>::iterator
gut::polymorphic_vector<B>::end() noexcept
{
	return { handles_, handles_.size() };
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_iterator
gut::polymorphic_vector<B>::end() const noexcept
{
	return { handles_, handles_.size() };
}

template<class B>
inline typename gut::polymorphic_vector<B>::reverse_iterator
gut::polymorphic_vector<B>::rbegin() noexcept
{
	return reverse_iterator{ end() };
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_reverse_iterator
gut::polymorphic_vector<B>::rbegin() const noexcept
{
	return const_reverse_iterator{ end() };
}

template<class B>
inline typename gut::polymorphic_vector<B>::reverse_iterator
gut::polymorphic_vector<B>::rend() noexcept
{
	return reverse_iterator{ begin() };
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_reverse_iterator
gut::polymorphic_vector<B>::rend() const noexcept
{
	return const_reverse_iterator{ begin() };
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_iterator
gut::polymorphic_vector<B>::cbegin() const noexcept
{
	return { handles_, 0 };
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_iterator
gut::polymorphic_vector<B>::cend() const noexcept
{
	return { handles_, handles_.size() };
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_reverse_iterator
gut::polymorphic_vector<B>::crbegin() const noexcept
{
	return const_reverse_iterator{ cend() };
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_reverse_iterator
gut::polymorphic_vector<B>::crend() const noexcept
{
	return const_reverse_iterator{ cbegin() };
}
//////////////////////////////////////////////////////////////////////////////////
// modifiers
//////////////////////////////////////////////////////////////////////////////////
template<class B>
template
<
	class D,
	std::enable_if_t<std::is_base_of<B, std::decay_t<D>>::value, int>
>
inline void
gut::polymorphic_vector<B>::push_back( D&& value )
{
	using der_t = std::decay_t<D>;

	der_t* p{ ::new ( next_alloc_ptr<der_t>() ) der_t{ std::forward<D>( value ) } };
	handles_.emplace_back( gut::handle<der_t>{ p } );

	emplace_offset_ += sizeof( der_t );
}

template<class B>
inline void gut::polymorphic_vector<B>::pop_back()
{
    gut::polymorphic_handle& h{ handles_.back() };
	emplace_offset_ = reinterpret_cast<byte*>( h->src() ) - data_;
	h->destroy();
	handles_.pop_back();
}

template<class B>
inline void gut::polymorphic_vector<B>::clear() noexcept
{
	for ( gut::polymorphic_handle& h : handles_ )
	{
		h->destroy();
	}
	emplace_offset_ = 0;
	handles_.erase( handles_.begin(), handles_.end() );
}
//////////////////////////////////////////////////////////////////////////////////
// element access
//////////////////////////////////////////////////////////////////////////////////
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
gut::polymorphic_vector<B>::front() noexcept
{
	return *reinterpret_cast<pointer>( handles_[ 0 ]->src() );
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_reference
gut::polymorphic_vector<B>::front() const noexcept
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
	return *reinterpret_cast<const_pointer>( handles_[ handles_.size() - 1 ]->src() );
}
//////////////////////////////////////////////////////////////////////////////////
// capacity
//////////////////////////////////////////////////////////////////////////////////
template<class B>
inline typename gut::polymorphic_vector<B>::size_type
gut::polymorphic_vector<B>::size() const noexcept
{
	return handles_.size();
}

template<class B>
inline bool gut::polymorphic_vector<B>::empty() const noexcept
{
	return handles_.empty();
}
//////////////////////////////////////////////////////////////////////////////////
// specialized algorithms
//////////////////////////////////////////////////////////////////////////////////
template<class B>
inline void
gut::polymorphic_vector<B>::swap( polymorphic_vector<B>& other ) noexcept
{
	std::swap( handles_, other.handles_ );
	std::swap( data_, other.data_ );
	std::swap( size_, other.size_ );
	std::swap( capacity_, other.capacity_ );
}
///////////////////////////////////////////////////////////////////////////////
// free functions
///////////////////////////////////////////////////////////////////////////////
template <class B>
void swap( gut::polymorphic_vector<B>& x, gut::polymorphic_vector<B>& y )
noexcept( noexcept( x.swap( y ) ) )
{
	x.swap( y );
}
#endif // GUT_POLYMORPHIC_STORAGE_H
