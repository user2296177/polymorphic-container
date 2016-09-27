#ifndef GUT_HANDLE_BASE_H
#define GUT_HANDLE_BASE_H

#include <cstddef>

namespace gut
{
	template<class T> class handle;

	class polymorphic_handle;

	class handle_base
	{
	public:
		using byte = unsigned char;
		using size_type = std::size_t;

		virtual ~handle_base() = default;

		handle_base(handle_base&& other) noexcept;
		handle_base& operator=(handle_base&& other) noexcept;

		handle_base(handle_base const&) = delete;
		handle_base& operator=(handle_base const&) = delete;

		virtual size_type align() const noexcept = 0;
		virtual size_type size() const noexcept = 0;

		virtual void destroy() = 0;
		virtual void transfer(void* nblk, void* nsrc) = 0;
		virtual void copy(void* blk, void* dst, polymorphic_handle& out_handle) const = 0;

		void* blk() const noexcept;
		void* src() const noexcept;

	protected:
		handle_base(void* blk, void* src) noexcept;

		void* blk_;
		void* src_;
	};
}
//////////////////////////////////////////////////////////////////////////////////
// constructors
//////////////////////////////////////////////////////////////////////////////////
inline gut::handle_base::handle_base(handle_base&& other) noexcept
	: blk_{ other.blk_ }
	, src_{ other.src_ }
{
	other.src_ = nullptr;
}

inline gut::handle_base& gut::handle_base::operator=(handle_base&& other) noexcept
{
	if (this != &other)
	{
		blk_ = other.blk_;
		src_ = other.src_;
		other.src_ = nullptr;
	}
	return *this;
}

inline gut::handle_base::handle_base(void* blk, void* src) noexcept
	: blk_{ blk }
	, src_{ src }
{}
//////////////////////////////////////////////////////////////////////////////////
// member access
//////////////////////////////////////////////////////////////////////////////////
inline void* gut::handle_base::blk() const noexcept
{
	return blk_;
}

inline void* gut::handle_base::src() const noexcept
{
	return src_;
}
#endif // GUT_HANDLE_BASE_H