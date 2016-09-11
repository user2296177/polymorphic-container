#ifndef GUT_POLYMORPHIC_VECTOR_ITERATOR_H
#define GUT_POLYMORPHIC_VECTOR_ITERATOR_H

#include <utility>
#include <type_traits>

namespace gut
{
	template<class> class polymorphic_vector;

	template<class B, bool is_const>
	class polymorphic_vector_iterator final
		: public std::iterator<std::random_access_iterator_tag, B>
	{
	private:
		friend class polymorphic_vector<B>;
		friend class polymorphic_vector_iterator<B, true>;

		using polymorphic_vector_t = typename polymorphic_vector<B>;

		using size_type = typename polymorphic_vector_t::size_type;
		
		using container_reference = std::conditional_t
		<
			is_const,
			typename polymorphic_vector_t::container_type const&,
			typename polymorphic_vector_t::container_type&
		>;

		container_reference handles_;
		size_type iter_idx_;

		polymorphic_vector_iterator( container_reference handles,
			size_type const iter_idx ) noexcept
			: handles_{ handles }
			, iter_idx_{ iter_idx }
		{}

	public:
		template
		<
			class PolymorphicVectorIterator,
			std::enable_if_t
			<
				std::is_same
				<
					PolymorphicVectorIterator,
					polymorphic_vector_iterator<B, false>
				>::value, int
			> = 0
		>
		polymorphic_vector_iterator( PolymorphicVectorIterator&& it )
			: handles_{ it.handles_ }
			, iter_idx_{ it.iter_idx_ }
		{}

		polymorphic_vector_iterator( polymorphic_vector_iterator&& ) = default;
		polymorphic_vector_iterator( polymorphic_vector_iterator const& ) = default;

		polymorphic_vector_iterator& operator=( polymorphic_vector_iterator&& ) = default;
		polymorphic_vector_iterator& operator=( polymorphic_vector_iterator const& ) = default;

		B& operator*() noexcept
		{
			return *reinterpret_cast<B*>( ( handles_[ iter_idx_ ] )->src() );
		}
		
		B const& operator*() const noexcept
		{
			return *reinterpret_cast<B const*>( ( handles_[ iter_idx_ ] )->src() );
		}

		B* operator->() noexcept
		{
			return reinterpret_cast<B*>( ( handles_[ iter_idx_ ] )->src() );
		}

		B const* operator->() const noexcept
		{
			return reinterpret_cast<B const*>( ( handles_[ iter_idx_ ] )->src() );
		}

		B& operator[]( size_type const i ) noexcept
		{
			return *reinterpret_cast<B*>( ( handles_[ i ] )->src() );
		}

		B const& operator[]( size_type const i ) const noexcept
		{
			return *reinterpret_cast<B*>( ( handles_[ i ] )->src() );
		}

		polymorphic_vector_iterator& operator++() noexcept
		{
			++iter_idx_;
			return *this;
		}

		polymorphic_vector_iterator operator++( int ) noexcept
		{
			polymorphic_vector_iterator self{ *this };
			++iter_idx_;
			return self;
		}

		polymorphic_vector_iterator& operator+=( size_type const n ) noexcept
		{
			iter_idx_ += n;
			return *this;
		}

		polymorphic_vector_iterator& operator--() noexcept
		{
			--iter_idx_;
			return *this;
		}

		polymorphic_vector_iterator operator--( int ) noexcept
		{
			polymorphic_vector_iterator self{ *this };
			--iter_idx_;
			return self;
		}

		polymorphic_vector_iterator& operator-=( size_type const n ) noexcept
		{
			iter_idx_ -= n;
			return *this;
		}

		friend bool operator==( polymorphic_vector_iterator const& lhs,
			polymorphic_vector_iterator const& rhs ) noexcept
		{
			return lhs.iter_idx_ == rhs.iter_idx_ &&
				&lhs.handles_ == &rhs.handles_;
		}

		friend bool operator!=( polymorphic_vector_iterator const& lhs,
			polymorphic_vector_iterator const& rhs ) noexcept
		{
			return lhs.iter_idx_ != rhs.iter_idx_ ||
				&lhs.handles_ != &rhs.handles_;
		}

		friend bool operator<( polymorphic_vector_iterator const& lhs,
			polymorphic_vector_iterator const& rhs ) noexcept
		{
			return lhs.iter_idx_ < rhs.iter_idx_ &&
				&lhs.handles_ == &rhs.handles_;
		}

		friend bool operator<=( polymorphic_vector_iterator const& lhs,
			polymorphic_vector_iterator const& rhs ) noexcept
		{
			return lhs.iter_idx_ <= rhs.iter_idx_ &&
				&lhs.handles_ == &rhs.handles_;
		}

		friend bool operator>( polymorphic_vector_iterator const& lhs,
			polymorphic_vector_iterator const& rhs ) noexcept
		{
			return lhs.iter_idx_ > rhs.iter_idx_ &&
				&lhs.handles_ == &rhs.handles_;
		}

		friend bool operator>=( polymorphic_vector_iterator const& lhs,
			polymorphic_vector_iterator const& rhs ) noexcept
		{
			return lhs.iter_idx_ >= rhs.iter_idx_ &&
				&lhs.handles_ == &rhs.handles_;
		}
	};
}
#endif // GUT_POLYMORPHIC_VECTOR_ITERATOR_H
