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
		virtual ~handle_base() = default;
		
		handle_base( handle_base&& ) noexcept;
		handle_base& operator=( handle_base&& ) noexcept;
		handle_base( handle_base const& ) = delete;
		handle_base& operator=( handle_base const& ) = delete;

		virtual void destroy() = 0;
		virtual void destroy( std::size_t& out_size ) = 0;
		virtual void transfer( void* dst, std::size_t& out_size ) = 0;
		virtual void copy( void* dst,
			polymorphic_handle& out_handle, std::size_t& out_size ) const = 0;

		void* src() const noexcept
		{
			return src_;
		}

		std::size_t padding() const noexcept
		{
			return padding_;
		}

	protected:
		handle_base( void* src, std::size_t const padding ) noexcept;
		
		void* src_;
		mutable std::size_t padding_;
	};
}
#endif // GUT_HANDLE_BASE_H