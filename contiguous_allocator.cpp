#include "contiguous_allocator.h"
#include <cassert>

using byte = gut::contiguous_allocator::byte;
using size_type = gut::contiguous_allocator::size_type;

#define as_byte_ptr(ptr) reinterpret_cast<byte*>(ptr)

//////////////////////////////////////////////////////////////////////////////////
// destructor
//////////////////////////////////////////////////////////////////////////////////
gut::contiguous_allocator::~contiguous_allocator() noexcept
{
	std::free(data_);
}
//////////////////////////////////////////////////////////////////////////////////
// constructors/assignment
//////////////////////////////////////////////////////////////////////////////////
gut::contiguous_allocator::contiguous_allocator(size_type const cap)
	: data_{ as_byte_ptr(std::malloc(cap)) }
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
	: data_{ as_byte_ptr(std::malloc(other.offset_)) }
	, offset_{ 0 }
	, cap_{ 0 }
{
	if (data_)
	{
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
			byte* new_data = as_byte_ptr(std::malloc(other.offset_));

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
		copy(other);
	}
	return *this;
}
//////////////////////////////////////////////////////////////////////////////////
// modifiers
//////////////////////////////////////////////////////////////////////////////////
void gut::contiguous_allocator::deallocate(size_type const i, size_type const j)
{
	assert(i < j);
	assert(j <= handles_.size());

	erase_inner_sections(i, j);

	auto block = destroy(i, j);
	auto end = next_section(j);
	
	// merge right adjacent section
	auto r_sec = to_section_index(end);
	if (r_sec != sections_.size())
	{
		sections_.erase(sections_.cbegin() + r_sec);
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
//////////////////////////////////////////////////////////////////////////////////
// private member functions
//////////////////////////////////////////////////////////////////////////////////
void gut::contiguous_allocator::copy(contiguous_allocator const& other)
{
	gut::polymorphic_handle out_h;
	byte* blk;
	byte* src;
	for (auto const& h : other.handles_)
	{
		blk = data_ + offset_;
		src = make_aligned(blk, h->align());
		h->copy(blk, src, out_h);
		handles_.emplace_back(std::move(out_h));
		offset_ += h->size() + (src - blk);
	}
}

byte* gut::contiguous_allocator::destroy(size_type i, size_type const j)
{
	auto& h_i = handles_[i++];

	auto block_address = as_byte_ptr(h_i->blk());

	// merge left adjacent section - maybe switch to iterator?
	auto l_sec = to_section_index(i);
	if (l_sec != sections_.size())
	{
		block_address -= sections_[l_sec].available_size;
		sections_.erase(sections_.begin() + l_sec);
	}

	h_i->destroy();

	for (; i != j; ++i)
	{
		handles_[i]->destroy();
	}

	return block_address;
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

void gut::contiguous_allocator::transfer(byte* block, size_type i, size_type const j)
{
	size_type offset = 0;

	for (byte* blk, *src; i != j; ++i)
	{
		blk = block + offset;
		src = make_aligned(blk, handles_[i]->align());

		if (src + handles_[i]->size() <= handles_[i]->src())
		{
			handles_[i]->transfer(blk, src);
			offset += handles_[i]->size() + (src - blk);
		}
		else if ( i != 0 )
		{
			sections_.emplace_back(i, blk - data_);
			return;
		}
	}

	if (j != handles_.size())
	{
		transfer(block + offset, j, next_section(j));
	}
	else
	{
		auto& last_handle = handles_.back();
		offset_ = (as_byte_ptr(last_handle->src()) + last_handle->size()) - data_;
	}
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

void swap(gut::contiguous_allocator& x, gut::contiguous_allocator& y)
noexcept(noexcept(x.swap(y)))
{
	x.swap(y);
}