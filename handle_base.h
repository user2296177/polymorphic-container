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
		using size_type = std::size_t;

		virtual ~handle_base() = default;

		handle_base( handle_base&& other ) noexcept;
		handle_base& operator=( handle_base&& other ) noexcept;

		handle_base( handle_base const& ) = delete;
		handle_base& operator=( handle_base const& ) = delete;

		virtual size_type size() const noexcept = 0;
		virtual void destroy() = 0;
		virtual void transfer( void* dst ) = 0;
		virtual void copy( void* dst, polymorphic_handle& out_handle ) const = 0;

		void* src() const noexcept;

	protected:
		handle_base( void* src ) noexcept;

		void* src_;
	};
}
//////////////////////////////////////////////////////////////////////////////////
// constructors
//////////////////////////////////////////////////////////////////////////////////
inline gut::handle_base::handle_base( handle_base&& other ) noexcept
	: src_{ other.src_ }
{
	other.src_ = nullptr;
}

inline gut::handle_base& gut::handle_base::operator=( handle_base&& other ) noexcept
{
	if ( this != &other )
	{
		src_ = other.src_;
		other.src_ = nullptr;
	}
	return *this;
}

inline gut::handle_base::handle_base( void* src ) noexcept
	: src_{ src }
{}
//////////////////////////////////////////////////////////////////////////////////
// member access
//////////////////////////////////////////////////////////////////////////////////
inline void* gut::handle_base::src() const noexcept
{
	return src_;
}

#endif // GUT_HANDLE_BASE_H
