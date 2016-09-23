#ifndef GUT_CONTIGUOUS_ALLOCATOR_H
#define GUT_CONTIGUOUS_ALLOCATOR_H

#include "polymorphic_handle.h"
#include <cstddef>
#include <vector>

namespace gut
{
	template<class B> class polymorphic_vector;

	class contiguous_allocator
	{
	public:
		using byte = unsigned char;
		using size_type = std::size_t;

		explicit contiguous_allocator(size_type const cap = 0);

		contiguous_allocator(contiguous_allocator&& other) noexcept;
		contiguous_allocator& operator=(contiguous_allocator&& other) noexcept;

		template<class T>
		T* allocate();

		void deallocate(size_type const i, size_type const j);

		void clear();

	public:
		template<class B> friend class polymorphic_vector;

		struct section
		{
			constexpr section(size_type const hnd_idx, size_type const avail_sz)
				noexcept
				: handle_index{ hnd_idx }
				, available_size{ avail_sz }
			{}

			size_type handle_index;
			size_type available_size;
		};

		void erase_inner_sections(size_type const i, size_type const j);

		std::pair<byte*, size_type> destroy(size_type i, size_type const j);

		size_type to_section_index(size_type const handle_index) const;

		size_type next_section(size_type const handle_index) const;

		void transfer(std::pair<byte*, size_type> block,
			size_type i, size_type const j);

		bool is_overwrite(byte* dst, gut::polymorphic_handle& h) const;

		inline byte* make_aligned(byte* blk, size_type const align)
			const noexcept;

		struct section;

		std::vector<section> sections_;
		std::vector<gut::polymorphic_handle> handles_;
		byte* data_;
		size_type offset_;
		size_type cap_;
	};
}

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
#endif // GUT_CONTIGUOUS_ALLOCATOR_H