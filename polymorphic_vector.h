#include "contiguous_allocator.h"
#include "polymorphic_vector_iterator.h"
#include <new>
#include <utility>

namespace gut
{
	template<class B, class D>
	using enable_if_derived_t = std::enable_if_t
	<
		std::is_base_of<B, std::decay_t<D>>::value, int
	>;
}

namespace gut
{
	template<class B>
	class polymorphic_vector
	{
	public:
		using byte = gut::contiguous_allocator::byte;

		using value_type = B;
		using reference = value_type&;
		using const_reference = value_type const&;
		using pointer = value_type*;
		using const_pointer = value_type const*;

		using iterator = gut::polymorphic_vector_iterator<B, false>;
		using const_iterator = gut::polymorphic_vector_iterator<B, true>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		using size_type = gut::contiguous_allocator::size_type;
		using difference_type = typename iterator::difference_type;

		// iterators
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

		// constructors
		polymorphic_vector(size_type const capacity = 0);

		polymorphic_vector(polymorphic_vector&&) = default;
		polymorphic_vector& operator=(polymorphic_vector&&) = default;

		polymorphic_vector(polymorphic_vector const&) = default;
		polymorphic_vector& operator=(polymorphic_vector const&) = default;

		// modifiers
		template<class D, gut::enable_if_derived_t<B, D> = 0>
		void push_back(D&& value);

		template<class D, class... Args, gut::enable_if_derived_t<B, D> = 0>
		void emplace_back(Args&&... value);

		iterator erase(const_iterator position);
		iterator erase(const_iterator begin, const_iterator end);
		
		void pop_back();
		void swap(polymorphic_vector& other) noexcept;
		void clear();

		// element access
		reference operator[](size_type const i) noexcept;
		const_reference operator[](size_type const i) const noexcept;

		reference at(size_type const i);
		const_reference at(size_type const i) const;

		reference front() noexcept;
		const_reference front() const noexcept;

		reference back() noexcept;
		const_reference back() const noexcept;

		// capacity - maybe add reserve() <---- think about this one more
		size_type size() const noexcept;
		bool empty() const noexcept;

	private:
		void ensure_index_bounds(size_type const i) const;

		gut::contiguous_allocator alloc_;
	};
}
//////////////////////////////////////////////////////////////////////////////////
// iterators
//////////////////////////////////////////////////////////////////////////////////
template<class B>
inline typename gut::polymorphic_vector<B>::iterator
gut::polymorphic_vector<B>::begin() noexcept
{
	return{ alloc_.handles_, 0 };
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_iterator
gut::polymorphic_vector<B>::begin() const noexcept
{
	return{ alloc_.handles_, 0 };
}

template<class B>
inline typename gut::polymorphic_vector<B>::iterator
gut::polymorphic_vector<B>::end() noexcept
{
	return{ alloc_.handles_, alloc_.handles_.size() };
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_iterator
gut::polymorphic_vector<B>::end() const noexcept
{
	return{ alloc_.handles_, alloc_.handles_.size() };
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
	return{ alloc_.handles_, 0 };
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_iterator
gut::polymorphic_vector<B>::cend() const noexcept
{
	return{ alloc_.handles_, alloc_.handles_.size() };
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
// constructors/assignment
//////////////////////////////////////////////////////////////////////////////////
template<class B>
inline gut::polymorphic_vector<B>::polymorphic_vector(size_type const capacity)
	: alloc_{ capacity }
{}
//////////////////////////////////////////////////////////////////////////////////
// modifiers
//////////////////////////////////////////////////////////////////////////////////
template<class B>
template<class D, gut::enable_if_derived_t<B, D>>
inline void gut::polymorphic_vector<B>::push_back(D&& value)
{
	emplace_back<D>(std::forward<D>(value));
}

template<class B>
template<class D, class... Args, gut::enable_if_derived_t<B, D>>
inline void gut::polymorphic_vector<B>::emplace_back(Args&&... args)
{
	::new (alloc_.allocate<D>()) D{ std::forward<Args>(args)... };
}

template<class B>
inline typename gut::polymorphic_vector<B>::iterator
gut::polymorphic_vector<B>::erase(const_iterator position)
{
	alloc_.deallocate(position.iter_idx_, position.iter_idx_ + 1);
	return{ alloc_.handles_, position.iter_idx_ };
}

template<class B>
inline typename gut::polymorphic_vector<B>::iterator
gut::polymorphic_vector<B>::erase(const_iterator begin, const_iterator end)
{
	alloc_.deallocate(begin.iter_idx_, end.iter_idx_);
	return{ alloc_.handles_ , begin.iter_idx_ };
}

template<class B>
inline void gut::polymorphic_vector<B>::pop_back()
{
	gut::polymorphic_handle& h{ alloc_.handles_.back() };
	alloc_.offset_ = reinterpret_cast<byte*>(h->blk()) - alloc_.data_;
	h->destroy();
	alloc_.handles_.pop_back();
}

template<class B>
inline void gut::polymorphic_vector<B>::swap(polymorphic_vector& other) noexcept
{
	alloc_.swap(other.alloc_);
}

template<class B>
void gut::polymorphic_vector<B>::clear()
{
	alloc_.clear();
}
//////////////////////////////////////////////////////////////////////////////////
// element access
//////////////////////////////////////////////////////////////////////////////////
template<class B>
inline typename gut::polymorphic_vector<B>::reference
gut::polymorphic_vector<B>::operator[](size_type const i) noexcept
{
	return *reinterpret_cast<pointer>(alloc_.handles_[i]->src());
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_reference
gut::polymorphic_vector<B>::operator[](size_type const i) const noexcept
{
	return *reinterpret_cast<const_pointer>(alloc_.handles_[i]->src());
}

template<class B>
inline typename gut::polymorphic_vector<B>::reference
gut::polymorphic_vector<B>::at(size_type const i)
{
	ensure_index_bounds(i);
	return *reinterpret_cast<pointer>(alloc_.handles_[i]->src());
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_reference
gut::polymorphic_vector<B>::at(size_type const i) const
{
	ensure_index_bounds(i);
	return *reinterpret_cast<const_pointer>(alloc_.handles_[i]->src());
}

template<class B>
inline typename gut::polymorphic_vector<B>::reference
gut::polymorphic_vector<B>::front() noexcept
{
	return *reinterpret_cast<pointer>(alloc_.handles_[0]->src());
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_reference
gut::polymorphic_vector<B>::front() const noexcept
{
	return *reinterpret_cast<const_pointer>(alloc_.handles_[0]->src());
}

template<class B>
inline typename gut::polymorphic_vector<B>::reference
gut::polymorphic_vector<B>::back() noexcept
{
	return *reinterpret_cast<pointer>(alloc_.handles_[alloc_.handles_.size() - 1]->src());
}

template<class B>
inline typename gut::polymorphic_vector<B>::const_reference
gut::polymorphic_vector<B>::back() const noexcept
{
	return *reinterpret_cast<const_pointer>(alloc_.handles_[alloc_.handles_.size() - 1]->src());
}
//////////////////////////////////////////////////////////////////////////////////
// capacity
//////////////////////////////////////////////////////////////////////////////////
template<class B>
inline typename gut::polymorphic_vector<B>::size_type
gut::polymorphic_vector<B>::size() const noexcept
{
	return alloc_.handles_.size();
}

template<class B>
inline bool gut::polymorphic_vector<B>::empty() const noexcept
{
	return alloc_.handles_.empty();
}
///////////////////////////////////////////////////////////////////////////////
// private member functions
///////////////////////////////////////////////////////////////////////////////
template<class B>
inline void gut::polymorphic_vector<B>::ensure_index_bounds(size_type const i) const
{
	if (i >= alloc_.handles_.size())
	{
		throw std::out_of_range
		{
			"polymorphic_vector<B>::ensure_index_bounds( size_type const i );\n"
			"index out of range"
		};
	}
}
///////////////////////////////////////////////////////////////////////////////
// specialized algorithms
///////////////////////////////////////////////////////////////////////////////
template <class B>
void swap(gut::polymorphic_vector<B>& x, gut::polymorphic_vector<B>& y)
noexcept(noexcept(x.swap(y)))
{
	x.swap(y);
}