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
#include <cstdlib>
#include <cassert>

namespace gut
{
	template<class B, class D>
	using enable_if_derived_t = std::enable_if_t
	<
		std::is_base_of<B, std::decay_t<D>>::value, int
	>;

	template<class B>
	class polymorphic_vector final
	{
	public:
		using container_type = std::vector<gut::polymorphic_handle>;
		
		using value_type = B;
		using reference = value_type&;
		using const_reference = value_type const&;
		using pointer = value_type*;
		using const_pointer = value_type const*;

		using iterator = gut::polymorphic_vector_iterator<B, false>;
		using const_iterator = gut::polymorphic_vector_iterator<B, true>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		using size_type = container_type::size_type;
		using difference_type = typename iterator::difference_type;

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

		// constructors - complete
		explicit polymorphic_vector( size_type const initial_capacity = 0 );

		polymorphic_vector( polymorphic_vector&& other ) noexcept;
		polymorphic_vector& operator=( polymorphic_vector&& other ) noexcept;
		
		polymorphic_vector( polymorphic_vector const& other );
		polymorphic_vector& operator=( polymorphic_vector const& other );

		// modifiers - missing insert operations; consider insert(pos, beg, end)
		template<class D, enable_if_derived_t<B, D> = 0>
		void push_back( D&& value );

		template<class D, class... Args, enable_if_derived_t<B, D> = 0>
		void emplace_back( Args&&... value );

		template<class D, class... Args, enable_if_derived_t<B, D> = 0>
		iterator emplace( const_iterator position, Args&&... args );

		template<class D, enable_if_derived_t<B, D> = 0>
		iterator insert( const_iterator pos, D&& value );

		template<class D, enable_if_derived_t<B, D> = 0>
		iterator insert( const_iterator pos, size_type count, D const& value );

		template<class D, enable_if_derived_t<B, D> = 0>
		iterator insert( const_iterator pos, std::initializer_list<D> list );

		template
		<
			class InputIt,
			enable_if_derived_t
			<
				B, typename std::iterator_traits<InputIt>::value_type
			> = 0
		>
		iterator insert( const_iterator pos, InputIt first, InputIt last );

		iterator erase( const_iterator position );
		iterator erase( const_iterator begin, const_iterator end );

		void pop_back();
		void swap( polymorphic_vector& other ) noexcept;
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

		// capacity - maybe add reserve() <---- think about this one more
		size_type size() const noexcept;
		bool empty() const noexcept;

	private:
		using byte = unsigned char;

		void ensure_index_bounds( size_type const i ) const;
		void copy( polymorphic_vector const& other );
		void erase_range( size_type const i, size_type const j );
		
		size_type next_emplace_offset( size_type const align,
			size_type const size ) noexcept;

		template<class D> // TODO: extract transfer loop into function
		std::pair<void*, void*> next_alloc();

		std::vector<gut::polymorphic_handle> handles_;
		byte* data_;
		size_type emplace_offset_;
		size_type capacity_;
		size_type sizeof_prev_;
	};
}

namespace gut
{
	static std::size_t calculate_padding( void* p, std::size_t const align )
	noexcept
	{
		std::size_t r{ reinterpret_cast<std::uintptr_t>( p ) % align };
		return r == 0 ? 0 : align - r;
	}
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
// constructors/assignment
//////////////////////////////////////////////////////////////////////////////////
template<class B>
gut::polymorphic_vector<B>::polymorphic_vector( size_type const initial_capacity )
	: data_{ reinterpret_cast<byte*>( std::malloc( initial_capacity ) ) }
	, emplace_offset_{ 0 }
	, capacity_{ 0 }
	, sizeof_prev_{ 0 }
{
	if ( data_ )
	{
		capacity_ = initial_capacity;
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
		capacity_ = other.capacity_;
		emplace_offset_ = other.emplace_offset_;
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
		copy( other );
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
			byte* new_data = reinterpret_cast<byte*>( std::malloc(
				other.emplace_offset_ ) );

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
template<class D, gut::enable_if_derived_t<B, D>>
inline void gut::polymorphic_vector<B>::push_back( D&& value )
{
	emplace_back<D>( std::forward<D>( value ) );
}

template<class B>
template<class D, class... Args, gut::enable_if_derived_t<B, D>>
inline void gut::polymorphic_vector<B>::emplace_back( Args&&... args )
{
	using der_t = std::decay_t<D>;
	std::pair<void*, void*> alloc{ next_alloc<der_t>() };
	handles_.emplace_back( gut::handle<der_t>
	{
		alloc.first, ::new ( alloc.second ) der_t{ std::forward<Args>( args )... }
	} );
}

template<class B>
inline typename gut::polymorphic_vector<B>::iterator
gut::polymorphic_vector<B>::erase( const_iterator position )
{
	erase_range( position.iter_idx_, position.iter_idx_ + 1 );
	return { handles_, position.iter_idx_ };
}

template<class B>
inline typename gut::polymorphic_vector<B>::iterator
gut::polymorphic_vector<B>::erase( const_iterator begin, const_iterator end )
{
	erase_range( begin.iter_idx_, end.iter_idx_ );
	return { handles_ , begin.iter_idx_ };
}

template<class B>
inline void gut::polymorphic_vector<B>::pop_back()
{
    gut::polymorphic_handle& h{ handles_.back() };
	emplace_offset_ = reinterpret_cast<byte*>( h->blk() ) - data_;
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
///////////////////////////////////////////////////////////////////////////////
// private member functions
///////////////////////////////////////////////////////////////////////////////
template <class B>
void gut::polymorphic_vector<B>::ensure_index_bounds( size_type const i ) const
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

template <class B>
void gut::polymorphic_vector<B>::copy( polymorphic_vector const& other )
{
	gut::polymorphic_handle out_h;
	for ( gut::polymorphic_handle const& h : other.handles_ )
	{
		h->copy(
			data_ + ( reinterpret_cast<byte*>( h->blk() ) - other.data_ ),
			data_ + ( reinterpret_cast<byte*>( h->src() ) - other.data_ ),
			out_h );

		handles_.emplace_back( std::move( out_h ) );
	}
}

template <class B>
void gut::polymorphic_vector<B>::erase_range( size_type const i, size_type const j )
{
	assert( i < j && j <= handles_.size() );

	emplace_offset_ =
		reinterpret_cast<byte*>( handles_[ i ]->blk() ) - data_;

	sizeof_prev_ = emplace_offset_ == 0 ? 0 : handles_[ i - 1 ]->size();

	for ( size_type k{ i }; k != j; ++k )
	{
		handles_[ k ]->destroy();
	}

	void* blk;
	void* src;
	for ( size_type k{ j }, sz{ handles_.size() }; k != sz; ++k )
	{
		blk = data_ + emplace_offset_;
		emplace_offset_ = next_emplace_offset( handles_[ k ]->align(), handles_[ k ]->size() );
		src = data_ + emplace_offset_;

		handles_[ k ]->transfer( blk, src );
		sizeof_prev_ = handles_[ k ]->size();
		emplace_offset_ += sizeof_prev_;
	}

	auto handles_begin = handles_.begin();
	handles_.erase( handles_begin + i, handles_begin + j );
}

template <class B>
typename gut::polymorphic_vector<B>::size_type
gut::polymorphic_vector<B>::next_emplace_offset( size_type const align,
	size_type const size ) noexcept
{
	size_type next_offset{ emplace_offset_ };
	if ( sizeof_prev_ == 0 )
	{
		return next_offset;
	}
	else if ( size > sizeof_prev_ )
	{
		next_offset += size - sizeof_prev_;
		next_offset += gut::calculate_padding( data_ + emplace_offset_, align );
	}
	else
	{
		next_offset += gut::calculate_padding( data_ + emplace_offset_, align );
	}
	return next_offset;
}

template<class B>
template<class D>
std::pair<void*, void*> gut::polymorphic_vector<B>::next_alloc()
{
	using der_t = std::decay_t<D>;

	// transform next 3 lines into function/macro?
	byte* blk{ data_ + emplace_offset_ };
	size_type new_emplace_offset = next_emplace_offset( alignof( der_t ), sizeof( der_t ) );
	byte* src{ data_ + new_emplace_offset };

	size_type required_size{ sizeof( D ) + ( src - blk ) };

	if ( capacity_ - emplace_offset_ < required_size )
	{
		size_type new_capacity{ ( alignof(der_t) +required_size + capacity_ ) * 2 };
		byte* new_data{ reinterpret_cast<byte*>( std::malloc( new_capacity ) ) };

		if ( new_data )
		{
			emplace_offset_ = 0;
			capacity_ = new_capacity;
			sizeof_prev_ = 0;

			for ( size_type k{ 0 }, size{ handles_.size() }; k != size; ++k )
			{
				blk = new_data + emplace_offset_;
				emplace_offset_ = next_emplace_offset( handles_[ k ]->align(), handles_[ k ]->size() );
				src = new_data + emplace_offset_;

				handles_[ k ]->transfer( blk, src );
				sizeof_prev_ = handles_[ k ]->size();
				emplace_offset_ += sizeof_prev_;
			}
			std::free( data_ );

			blk = new_data + emplace_offset_;
			emplace_offset_ = next_emplace_offset( alignof( der_t ), sizeof( der_t ) );
			src = new_data + emplace_offset_;

			data_ = new_data;
		}
		else
		{
			throw std::bad_alloc{};
		}
	}
	emplace_offset_ = new_emplace_offset + sizeof( der_t );
	sizeof_prev_ = sizeof( der_t );
	return{ blk, src };
}
///////////////////////////////////////////////////////////////////////////////
// specialized algorithms
///////////////////////////////////////////////////////////////////////////////
template <class B>
void swap( gut::polymorphic_vector<B>& x, gut::polymorphic_vector<B>& y )
noexcept( noexcept( x.swap( y ) ) )
{
	x.swap( y );
}
#endif // GUT_POLYMORPHIC_STORAGE_H
