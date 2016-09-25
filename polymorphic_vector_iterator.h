#ifndef GUT_POLYMORPHIC_VECTOR_ITERATOR_H
#define GUT_POLYMORPHIC_VECTOR_ITERATOR_H

#include <cassert>
#include <utility>
#include <iterator>
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

		using difference_type = typename std::iterator<std::random_access_iterator_tag, B>::difference_type;
		using size_type = std::size_t;
		using container_reference = std::conditional_t
		<
			is_const,
			std::vector<gut::polymorphic_handle> const&,
			std::vector<gut::polymorphic_handle>&
		>;

		container_reference handles_;
		size_type iter_idx_;

		polymorphic_vector_iterator(container_reference handles,
			size_type const iter_idx) noexcept
			: handles_{ handles }
			, iter_idx_{ iter_idx }
		{
			assert(iter_idx_ <= handles_.size());
		}

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
			polymorphic_vector_iterator(PolymorphicVectorIterator const& it) noexcept
			: polymorphic_vector_iterator(it.handles_, it.iter_idx_)
		{}

		polymorphic_vector_iterator(polymorphic_vector_iterator&&) = default;
		polymorphic_vector_iterator(polymorphic_vector_iterator const&) = default;

		polymorphic_vector_iterator&
			operator=(polymorphic_vector_iterator&&) = default;

		polymorphic_vector_iterator&
			operator=(polymorphic_vector_iterator const&) = default;

		B& operator*() noexcept
		{
			return *reinterpret_cast<B*>((handles_[iter_idx_])->src());
		}

		B const& operator*() const noexcept
		{
			return *reinterpret_cast<B const*>((handles_[iter_idx_])->src());
		}

		B* operator->() noexcept
		{
			return reinterpret_cast<B*>((handles_[iter_idx_])->src());
		}

		B const* operator->() const noexcept
		{
			return reinterpret_cast<B const*>((handles_[iter_idx_])->src());
		}

		B& operator[](difference_type const i) noexcept
		{
			assert(i < 0 ? iter_idx_ + i < iter_idx_ : iter_idx_ + i > iter_idx_);
			return *reinterpret_cast<B*>((handles_[iter_idx_ + i])->src());
		}

		B const& operator[](difference_type const i) const noexcept
		{
			assert(i < 0 ? iter_idx_ + i < iter_idx_ : iter_idx_ + i > iter_idx_);
			return *reinterpret_cast<B*>((handles_[iter_idx_ + i])->src());
		}

		polymorphic_vector_iterator& operator++() noexcept
		{
			assert(iter_idx_ + 1 > iter_idx_);
			++iter_idx_;
			return *this;
		}

		polymorphic_vector_iterator& operator--() noexcept
		{
			assert(iter_idx_ - 1 < iter_idx_);
			--iter_idx_;
			return *this;
		}

		polymorphic_vector_iterator operator++(int) noexcept
		{
			assert(iter_idx_ + 1 > iter_idx_);
			return{ handles_, iter_idx_++ };
		}

		polymorphic_vector_iterator operator--(int) noexcept
		{
			assert(iter_idx_ - 1 < iter_idx_);
			return{ handles_, iter_idx_-- };
		}

		polymorphic_vector_iterator& operator+=(difference_type const n) noexcept
		{
			assert(iter_idx_ + n > iter_idx_);
			iter_idx_ += n;
			return *this;
		}

		polymorphic_vector_iterator& operator-=(difference_type const n) noexcept
		{
			assert(iter_idx_ - n < iter_idx_);
			iter_idx_ -= n;
			return *this;
		}

		friend polymorphic_vector_iterator operator+(
			polymorphic_vector_iterator const& lhs, difference_type const n)
			noexcept
		{
			assert(n < 0 ? lhs.iter_idx_ + n < lhs.iter_idx_ : lhs.iter_idx_ + n > lhs.iter_idx_);
			return{ lhs.handles_, lhs.iter_idx_ + n };
		}

		friend polymorphic_vector_iterator operator+(
			difference_type const n, polymorphic_vector_iterator const& rhs)
			noexcept
		{
			assert(n < 0 ? rhs.iter_idx_ + n < rhs.iter_idx_ : rhs.iter_idx_ + n > rhs.iter_idx_);
			return{ rhs.handles_, rhs.iter_idx_ + n };
		}

		friend polymorphic_vector_iterator operator-(
			polymorphic_vector_iterator const& lhs, difference_type const n)
			noexcept
		{
			assert(n < 0 ? lhs.iter_idx_ - n > lhs.iter_idx_ : lhs.iter_idx_ - n < lhs.iter_idx_);
			return{ lhs.handles_, lhs.iter_idx_ - n };
		}

		friend polymorphic_vector_iterator operator-(
			difference_type const n, polymorphic_vector_iterator const& rhs)
			noexcept
		{
			assert(n < 0 ? rhs.iter_idx_ - n > rhs.iter_idx_ : rhs.iter_idx_ - n < rhs.iter_idx_);
			return{ rhs.handles_, rhs.iter_idx_ - n };
		}

		friend bool operator==(polymorphic_vector_iterator const& lhs,
			polymorphic_vector_iterator const& rhs) noexcept
		{
			return lhs.iter_idx_ == rhs.iter_idx_ &&
				&lhs.handles_ == &rhs.handles_;
		}

		friend bool operator!=(polymorphic_vector_iterator const& lhs,
			polymorphic_vector_iterator const& rhs) noexcept
		{
			return lhs.iter_idx_ != rhs.iter_idx_ ||
				&lhs.handles_ != &rhs.handles_;
		}

		friend bool operator<(polymorphic_vector_iterator const& lhs,
			polymorphic_vector_iterator const& rhs) noexcept
		{
			return lhs.iter_idx_ < rhs.iter_idx_ &&
				&lhs.handles_ == &rhs.handles_;
		}

		friend bool operator<=(polymorphic_vector_iterator const& lhs,
			polymorphic_vector_iterator const& rhs) noexcept
		{
			return lhs.iter_idx_ <= rhs.iter_idx_ &&
				&lhs.handles_ == &rhs.handles_;
		}

		friend bool operator>(polymorphic_vector_iterator const& lhs,
			polymorphic_vector_iterator const& rhs) noexcept
		{
			return lhs.iter_idx_ > rhs.iter_idx_ &&
				&lhs.handles_ == &rhs.handles_;
		}

		friend bool operator>=(polymorphic_vector_iterator const& lhs,
			polymorphic_vector_iterator const& rhs) noexcept
		{
			return lhs.iter_idx_ >= rhs.iter_idx_ &&
				&lhs.handles_ == &rhs.handles_;
		}
	};
}
#endif // GUT_POLYMORPHIC_VECTOR_ITERATOR_H
