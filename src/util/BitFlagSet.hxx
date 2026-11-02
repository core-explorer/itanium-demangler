#pragma once
#include <bit>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace swartalf {

template <typename T, typename U, size_t num_bits>
struct BitFlagSetIterator {
	using value_type = T;
	using storage_type = U;
	U stor_value = U(0);
	U it_value = U(0);

	constexpr BitFlagSetIterator() = default;
	constexpr BitFlagSetIterator(U v, U i) :
	    stor_value(v), it_value(i) {}

	void advance_to_next_bit() {
		while ((stor_value & (U(1) << it_value)) == U(0)) {
			if (it_value == (U(num_bits))) {
				break;
			}
			it_value <<= 1;
		}
	}

	constexpr T operator*() const {
		return static_cast<T>(it_value);
	}
	BitFlagSetIterator& operator++() {
		if (it_value < U(num_bits)) {
			it_value <<= 1;
			advance_to_next_bit();
		}
		return *this;
	}
};
template <typename T, typename U, size_t num_bits>
constexpr bool operator==(const BitFlagSetIterator<T, U, num_bits>& a, const BitFlagSetIterator<T, U, num_bits>& b) {
	return a.it_value == b.it_value;
}

template <typename T, typename U, size_t num_bits>
struct BitFlagSet {
	using value_type = T;
	using key_type = T;
	using storage_type = U;

	storage_type val = U {};

	using iterator = BitFlagSetIterator<T, U, num_bits>;

	constexpr iterator begin() const {
		auto it = iterator(val, U(0));
		it.advance_to_next_bit();
		return it;
	}
	constexpr iterator end() const {
		return iterator(val, U(num_bits));
	}
	constexpr bool contains(T v) const {
		return ((U(1) << U(v)) & U(val)) != U(0);
	}
	constexpr bool empty() const {
		return val == U(0);
	}
	size_t size() const {
		return std::popcount(val);
	}

	std::pair<iterator, bool> insert(T v) {
		bool is_new = (val & (U(1) << U(v))) == U(0);
		val |= U(1) << U(v);
		auto it = iterator(val, U(v));
		return std::make_pair(it, is_new);
	}

	std::pair<iterator, bool> emplace(T v) {
		bool is_new = val & (U(1) << U(v)) == 0;
		val |= U(1) << U(v);
		auto it = iterator(val, U(v));
		return std::make_pair(it, is_new);
	}

	void clear() {
		val = U(0);
	}

	constexpr void insert_one(T v) {
		val |= U(1) << U(v);
	}

	template <typename It>
	void insert(It first, It last) {
		while (first != last) {
			insert_one(*first);
			++first;
		}
	}

	void insert(std::initializer_list<value_type> ilist) {
		insert(ilist.begin(), ilist.end());
	}

	constexpr BitFlagSet() = default;
	constexpr BitFlagSet(std::initializer_list<value_type> ilist) {
		insert(ilist.begin(), ilist.end());
	}

	constexpr static BitFlagSet from(U v) {
		BitFlagSet f;
		f.val = v;
		return f;
	}

	size_t erase(T v) {
		size_t num_removed = (val & (U(1) << U(v))) != U(0);
		val &= ~(U(1) << U(v));
		return num_removed;
	}

	iterator erase(iterator pos) {
		val &= ~(U(1) << U(pos.it_value));
		pos.stor_value = val;
		pos.it_value++;
		pos.advance_to_next_bit;
		return pos;
	}

	iterator find(T v) {
		if ((val & U(v)) == 0) {
			return end();
		}
		return iterator(val, U(v));
	}

	template <typename It>
	BitFlagSet(It first, It last) {
		insert(first, last);
	}
};

template <typename T, typename U, size_t num_bits>
constexpr BitFlagSet<T, U, num_bits> operator|(BitFlagSet<T, U, num_bits> a, BitFlagSet<T, U, num_bits> b) {
	return BitFlagSet<T, U, num_bits>::from(U(a.val | b.val));
}

}