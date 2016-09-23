#include "contiguous_allocator.h"
#include <cassert>

using byte = gut::contiguous_allocator::byte;
using size_type = gut::contiguous_allocator::size_type;

/*
#define make_aligned( block, align )\
(unsigned char*)( ( (std::uintptr_t)block + align - 1 ) & ~( align - 1 ) )
//*/

#define as_byte_ptr( ptr ) reinterpret_cast<byte*>( ptr )

gut::contiguous_allocator::~contiguous_allocator() noexcept
{
	std::free(data_);
}

gut::contiguous_allocator::contiguous_allocator(size_type const cap)
	: data_{ reinterpret_cast<byte*>(std::malloc(cap)) }
	, offset_{ 0 }
	, cap_{ 0 }
{
	if (data_)
	{
		cap_ = cap;
	}
	else
	{
		throw std::bad_alloc{};
	}
}

gut::contiguous_allocator::contiguous_allocator(contiguous_allocator&& other) noexcept
	: sections_{ std::move(other.sections_) }
	, handles_{ std::move(other.handles_) }
	, data_{ other.data_ }
	, offset_{ other.offset_ }
	, cap_{ other.cap_ }
{
	other.data_ = nullptr;
}

gut::contiguous_allocator& gut::contiguous_allocator::operator=(contiguous_allocator&& other) noexcept
{
	if (this != &other)
	{
		for (auto& h : handles_)
		{
			h->destroy();
		}
		sections_ = std::move(other.sections_);
		handles_ = std::move(other.handles_);
		std::free(data_);
		data_ = other.data_;
		other.data_ = nullptr;
		cap_ = other.cap_;
		offset_ = other.offset_;
	}
	return *this;
}

gut::contiguous_allocator::contiguous_allocator(contiguous_allocator const& other)
	: data_{ reinterpret_cast<byte*>(std::malloc(other.offset_)) }
	, offset_{ 0 }
	, cap_{ 0 }
{
	if (data_)
	{
		sections_ = other.sections_;
		offset_ = other.offset_;
		cap_ = other.offset_;
		copy(other);
	}
	else
	{
		throw std::bad_alloc{};
	}
}

gut::contiguous_allocator& gut::contiguous_allocator::operator=(contiguous_allocator const& other)
{
	if (this != &other)
	{
		if (offset_ >= other.offset_)
		{
			clear();
		}
		else
		{
			byte* new_data = reinterpret_cast<byte*>(std::malloc(
				other.offset_));

			if (new_data)
			{
				clear();
				data_ = new_data;
				cap_ = other.offset_;
			}
			else
			{
				throw std::bad_alloc{};
			}
		}
		offset_ = other.offset_;
		copy(other);
	}
	return *this;
}

void gut::contiguous_allocator::deallocate(size_type const i, size_type const j)
{
	assert(i < j);
	assert(j <= handles_.size());

	erase_inner_sections(i, j);

	auto block = destroy(i, j);
	auto end = next_section(j);

	// merge right adjacent section, remove section if adjacent section 
	auto adjacent_section_idx = to_section_index(end);
	if (adjacent_section_idx != sections_.size())
	{
		block.second += sections_[adjacent_section_idx].available_size;
		// need to update index of a handle...
		//if (handles_[sections_[adjacent_section_idx].handle_index]->size() <= block.second)
		{
			sections_.erase(sections_.cbegin() + adjacent_section_idx);
		}
	}

	transfer(block, j, end);

	auto handles_cbegin = handles_.cbegin();
	handles_.erase(handles_cbegin + i, handles_cbegin + j);
}

void gut::contiguous_allocator::swap(contiguous_allocator& other) noexcept
{
	std::swap(sections_, other.sections_);
	std::swap(handles_, other.handles_);
	std::swap(data_, other.data_);
	std::swap(offset_, other.offset_);
	std::swap(cap_, other.cap_);
}

void gut::contiguous_allocator::clear()
{
	for (auto& h : handles_)
	{
		h->destroy();
	}
	sections_.clear();
	handles_.clear();
	offset_ = 0;
}

void gut::contiguous_allocator::copy(contiguous_allocator const& other)
{
	gut::polymorphic_handle out_h;
	for (auto const& h : other.handles_)
	{
		h->copy(
			data_ + (reinterpret_cast<byte*>(h->blk()) - other.data_),
			data_ + (reinterpret_cast<byte*>(h->src()) - other.data_),
			out_h);

		handles_.emplace_back(std::move(out_h));
	}
}

void gut::contiguous_allocator::erase_inner_sections(size_type const i, size_type const j)
{
	if (sections_.empty())
	{
		return;
	}

	auto b = sections_.cbegin();
	auto e = sections_.cend();

	for (; b != e && b->handle_index <= i; ++b);

	for (--e; e != b; --e)
	{
		if (e->handle_index < j)
		{
			++e;
			break;
		}
		else if (e->handle_index == j)
		{
			break;
		}
	}

	sections_.erase(b, e);
}

std::pair<byte*, size_type> gut::contiguous_allocator::destroy(size_type i, size_type const j)
{
	auto& h_i = handles_[i++];
	auto& h_j = handles_[j - 1];

	auto block_address = as_byte_ptr(h_i->blk());
	auto block_size = (as_byte_ptr(h_j->src()) + h_j->size()) - block_address;

	// merge left adjacent section
	auto self = to_section_index(i);
	if (self != sections_.size())
	{
		block_address -= sections_[self].available_size;
		block_size += sections_[self].available_size;
	}

	h_i->destroy();

	for (; i != j; ++i)
	{
		handles_[i]->destroy();
	}

	return{ block_address, block_size };
}

size_type gut::contiguous_allocator::to_section_index(size_type const handle_index) const
{
	auto sz{ sections_.size() };
	for (size_type i{ 0 }; i != sz; ++i)
	{
		if (sections_[i].handle_index == handle_index)
		{
			return i;
		}
	}
	return sz;
}

void gut::contiguous_allocator::transfer(std::pair<byte*, size_type> block, size_type i, size_type const j)
{
	size_type offset = 0;

	for (byte* blk, *src; i != j; ++i)
	{
		blk = block.first + offset;
		src = make_aligned(blk, handles_[i]->align());

		if (is_overwrite(src, handles_[i]))
		{
			/*
			emplace back section index and required size, where:
			- section index is a handle index
			- required size is size + padding - freed size
			*/
			sections_.emplace_back(i, block.second);

			return;
		}
		else
		{
			handles_[i]->transfer(blk, src);
			offset += handles_[i]->size() + (src - blk);
		}
	}

	if (j != handles_.size())
	{// ?
		transfer({ block.first + offset, offset }, j, next_section(j));
	}
	else
	{
		auto& last_handle = handles_.back();
		offset_ = (as_byte_ptr(last_handle->src()) + last_handle->size()) - data_;
	}
}

bool gut::contiguous_allocator::is_overwrite(byte* dst, gut::polymorphic_handle& h) const
{
	return (dst + h->size()) > h->src();
}

size_type gut::contiguous_allocator::next_section(size_type const handle_index) const
{
	for (auto s : sections_)
	{
		if (handle_index < s.handle_index)
		{
			return s.handle_index;
		}
	}
	return handles_.size();
}

inline byte* gut::contiguous_allocator::make_aligned(byte* blk, size_type const align) const noexcept
{
	return as_byte_ptr((reinterpret_cast<std::uintptr_t>(blk) + align - 1) & ~(align - 1));
}

void swap(gut::contiguous_allocator& x, gut::contiguous_allocator& y)
noexcept(noexcept(x.swap(y)))
{
	x.swap(y);
}