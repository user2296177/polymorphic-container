#include "handle_base.h"
#include <stdexcept>

static void throw_not_implemented_exception( char const* msg )
{
	throw std::logic_error{ msg };
}

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

void gut::handle_base::copy( void* dst,
	gut::polymorphic_handle& out_handle, std::size_t& out_size ) const
{
	throw_not_implemented_exception(
		"handle_base::copy( void* dst, std::size_t& out_size );\n"
		"attempted to call base class member function" );
};

void gut::handle_base::transfer( void* dst, std::size_t& out_size )
{
	throw_not_implemented_exception(
		"handle_base::transfer( void* dst, std::size_t& out_size );\n"
		"attempted to call base class member function" );
}

void gut::handle_base::destroy()
{
	throw_not_implemented_exception(
		"handle_base::destroy();\n"
		"attempted to call base class member function" );
}

void gut::handle_base::destroy( std::size_t& out_size )
{
	throw_not_implemented_exception(
		"destroy( std::size_t& out_size );\n"
		"attempted to call base class member function" );
}