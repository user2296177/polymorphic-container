#ifndef GUT_POLYMORPHIC_HANDLE_H
#define GUT_POLYMORPHIC_HANDLE_H

#include "handle_base.h"
#include "handle.h"
#include <new>
#include <algorithm>
#include <stdexcept>

namespace gut
{
	class polymorphic_handle
	{
	public:
		using value_type = gut::handle_base;
		using reference = value_type&;
		using const_reference = value_type const&;
		using pointer = value_type*;
		using const_pointer = value_type const*;

		~polymorphic_handle() noexcept
		{
			if (is_initialized_)
			{
				reinterpret_cast<pointer>(&h_)->~handle_base();
			}
		}

		polymorphic_handle() noexcept
			: is_initialized_{ false }
		{}

		polymorphic_handle(polymorphic_handle&& other) = default;
		polymorphic_handle& operator=(polymorphic_handle&& other) = default;

		polymorphic_handle(polymorphic_handle const&) = delete;
		polymorphic_handle& operator=(polymorphic_handle const&) = delete;

		template<class T>
		explicit polymorphic_handle(gut::handle<T>&& h) noexcept
			: is_initialized_{ true }
		{
			::new (&h_) gut::handle<T>{ std::move(h) };
		}

		pointer operator->()
		{
			ensure_initialized_handle();
			return reinterpret_cast<pointer>(&h_);
		}

		const_pointer operator->() const
		{
			ensure_initialized_handle();
			return reinterpret_cast<const_pointer>(&h_);
		}

	private:
		using storage_t = std::aligned_storage_t
		<
			sizeof(value_type), alignof(value_type)
		>;

		void ensure_initialized_handle() const
		{
			if (!is_initialized_)
			{
				throw std::logic_error
				{
					"polymorphic_handle::ensure_initialized_handle();\n"
					"attempted to dereference uninitialized handle"
				};
			}
		}

		storage_t h_;
		bool is_initialized_;
	};
}
#endif // GUT_POLYMORPHIC_HANDLE_H