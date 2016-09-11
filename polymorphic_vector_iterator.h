#ifndef GUT_POLYMORPHIC_VECTOR_ITERATOR_H
#define GUT_POLYMORPHIC_VECTOR_ITERATOR_H

#include "polymorphic_vector.h"
#include <utility>

namespace gut
{
	template<class B>
	class polymorphic_vector<B>::iterator final
	{
	private:
		friend class polymorphic_vector<B>;

		using iter_t = typename polymorphic_vector<B>::container_type::iterator;
		iter_t iterator_;

		iterator( iter_t&& iterator )
			noexcept( std::is_nothrow_move_constructible<iter_t>::value )
			: iterator_{ std::move( iterator ) }
		{}
	
	public:
		decltype( auto ) operator*()
		noexcept( noexcept( *std::declval<iter_t>() ) )
		{
			return *reinterpret_cast<B*>( ( *iterator_ )->src() );
		}

		iterator& operator++()
		noexcept( noexcept( ++std::declval<iter_t>() ) )
		{
			++iterator_;
			return *this;
		}

		iterator operator++( int )
		noexcept( noexcept( std::declval<iter_t>()++ ) )
		{
			iterator self{ *this };
			++iterator_;
			return self;
		}

		iterator& operator--()
		noexcept( noexcept( --std::declval<iter_t>() ) )
		{
			--iterator_;
			return *this;
		}

		iterator operator--( int )
		noexcept( noexcept( std::declval<iter_t>()-- ) )
		{
			iterator self{ *this };
			--iterator_;
			return self;
		}

		friend bool operator==( iterator const& lhs, iterator const& rhs )
		noexcept( noexcept( std::declval<iter_t>() == std::declval<iter_t>() ) )
		{
			return lhs.iterator_ == rhs.iterator_;
		}

		friend bool operator!=( iterator const& lhs, iterator const& rhs )
		noexcept( noexcept( std::declval<iter_t>() == std::declval<iter_t>() ) )
		{
			return lhs.iterator_ != rhs.iterator_;
		}
	};
}
#endif // GUT_POLYMORPHIC_VECTOR_ITERATOR_H