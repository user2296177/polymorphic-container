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

		using container_type = typename polymorphic_vector<B>::container_type;
		using size_type = typename container_type::size_type;
		
		container_type& handles_;
		size_type iter_idx_;

		iterator( container_type& handles, size_type const iter_idx ) noexcept
			: handles_{ handles }
			, iter_idx_{ iter_idx }
		{}
	
	public:
		B& operator*() noexcept
		{
			return *reinterpret_cast<B*>( ( handles_[ iter_idx_ ] )->src() );
		}
		
		B const& operator*() const noexcept
		{
			return *reinterpret_cast<B const*>( ( handles_[ iter_idx_ ] )->src() );
		}

		B& operator[]( size_type const i )
		{
			return *reinterpret_cast<B*>( ( handles_[ i ] )->src() );
		}

		B const& operator[]( size_type const i ) const
		{
			return *reinterpret_cast<B*>( ( handles_[ i ] )->src() );
		}

		iterator& operator++() noexcept
		{
			++iter_idx_;
			return *this;
		}

		iterator operator++( int ) noexcept
		{
			iterator self{ *this };
			++iter_idx_;
			return self;
		}

		iterator& operator--() noexcept
		{
			--iter_idx_;
			return *this;
		}

		iterator operator--( int ) noexcept
		{
			iterator self{ *this };
			--iter_idx_;
			return self;
		}

		friend bool operator==( iterator const& lhs, iterator const& rhs )
		noexcept
		{
			return lhs.iter_idx_ == rhs.iter_idx_ &&
				&lhs.handles_ == &rhs.handles_;
		}

		friend bool operator!=( iterator const& lhs, iterator const& rhs )
		noexcept
		{
			return lhs.iter_idx_ != rhs.iter_idx_ ||
				&lhs.handles_ != &rhs.handles_;
		}

		friend bool operator<( iterator const& lhs, iterator const& rhs )
		noexcept
		{
			return lhs.iter_idx_ < rhs.iter_idx_ &&
				&lhs.handles_ == &rhs.handles_;
		}

		friend bool operator<=( iterator const& lhs, iterator const& rhs )
		noexcept
		{
			return lhs.iter_idx_ <= rhs.iter_idx_ &&
				&lhs.handles_ == &rhs.handles_;
		}

		friend bool operator>( iterator const& lhs, iterator const& rhs )
		noexcept
		{
			return lhs.iter_idx_ > rhs.iter_idx_ &&
				&lhs.handles_ == &rhs.handles_;
		}

		friend bool operator>=( iterator const& lhs, iterator const& rhs )
		noexcept
		{
			return lhs.iter_idx_ >= rhs.iter_idx_ &&
				&lhs.handles_ == &rhs.handles_;
		}
	};
}
#endif // GUT_POLYMORPHIC_VECTOR_ITERATOR_H
