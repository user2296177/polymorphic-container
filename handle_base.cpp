#include "handle_base.h"
#include <stdexcept>

gut::handle_base::handle_base( void* src, std::size_t const padding ) noexcept
	: src_{ src }
	, padding_{ padding }
{}

gut::handle_base::handle_base( handle_base&& other ) noexcept
	: src_{ other.src_ }
	, padding_{ other.padding_ }
{
	other.src_ = nullptr;
}

gut::handle_base& gut::handle_base::operator=( handle_base&& other ) noexcept
{
	if ( this != &other )
	{
		src_ = other.src_;
		padding_ = other.padding_;
		other.src_ = nullptr;
	}
	return *this;
}