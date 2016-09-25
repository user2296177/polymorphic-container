#ifndef GUT_CONTIGUOUS_ALLOCATOR_H
#define GUT_CONTIGUOUS_ALLOCATOR_H

#include "polymorphic_handle.h"
#include <cstddef>
#include <vector>

namespace gut
{
	class polymorphic_handle;
	template<class B> class polymorphic_vector;

	class contiguous_allocator
	{
	public:
		using byte = unsigned char;
		using size_type = std::size_t;

		~contiguous_allocator() noexcept;

		explicit contiguous_allocator(size_type const cap = 0);

		contiguous_allocator(contiguous_allocator&& other) noexcept;
		contiguous_allocator& operator=(contiguous_allocator&& other) noexcept;

		contiguous_allocator(contiguous_allocator const& other);
		contiguous_allocator& operator=(contiguous_allocator const& other);

		template<class T>
		T* allocate();

		void deallocate(size_type const i, size_type const j);

		void swap(contiguous_allocator& other) noexcept;
		void clear();

	public:
		struct section
		{
			section(size_type const idx, size_type const sz) noexcept
				: handle_index{ idx }
				, available_size{ sz }
			{}

			size_type handle_index;
			size_type available_size;
		};

		void copy(contiguous_allocator const& other);

		void erase_inner_sections(size_type const i, size_type const j);

		byte* destroy(size_type i, size_type const j);

		size_type to_section_index(size_type const handle_index) const;

		size_type next_section(size_type const handle_index) const;

		void transfer(byte* block, size_type i, size_type const j);

		std::vector<section> sections_;
		std::vector<gut::polymorphic_handle> handles_;
		byte* data_;
		size_type offset_;
		size_type cap_;
	};
}

#define make_aligned(block, align)\
(byte*)(((std::uintptr_t)block + align - 1) & ~(align - 1))

template<class T>
T* gut::contiguous_allocator::allocate()
{
	byte* blk = data_ + offset_;
	byte* src = make_aligned(blk, alignof(T));

	size_type available_size = cap_ - offset_;
	size_type required_size = sizeof(T) + (src - blk);

	if (available_size < required_size)
	{
		size_type ncap = (cap_ + required_size) * 2;
		byte* ndata = reinterpret_cast<byte*>(std::malloc(ncap));

		if (ndata)
		{
			sections_.clear();
			offset_ = 0;
			cap_ = ncap;

			for (gut::polymorphic_handle& h : handles_)
			{
				blk = ndata + offset_;
				src = make_aligned(blk, h->align());

				h->transfer(blk, src);
				offset_ += h->size() + (src - blk);
			}

			blk = ndata + offset_;
			src = make_aligned(blk, alignof(T));

			std::free(data_);
			data_ = ndata;
		}
		else
		{
			throw std::bad_alloc{};
		}
	}

	handles_.emplace_back(gut::handle<T>{ blk, src });
	offset_ += sizeof(T) + (src - blk);

	return reinterpret_cast<T*>( src );
}

void swap(gut::contiguous_allocator& x, gut::contiguous_allocator& y)
noexcept(noexcept(x.swap(y)));

#endif // GUT_CONTIGUOUS_ALLOCATOR_H