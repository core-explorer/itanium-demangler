#pragma once
#include "Decoration.hxx"
#include <expected>
#include "../util/format.hxx"
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace swartalf {

struct DecoratedString {
	std::string decorated;
	std::string undecorated;
	constexpr bool empty() const {
		return undecorated.empty();
	}
	std::string_view get_raw() const {
		return undecorated;
	}
};

struct UndecoratedString {
	std::string undecorated;
	constexpr bool empty() const {
		return undecorated.empty();
	}
	std::string_view get_raw() const {
		return undecorated;
	}
};

enum class StackModifierType : uint8_t {
	name,
	ref,
	rval_ref,
	pointer,
	const_,
	volatile_,
	array,
	function,
	restrict_
};

constexpr StackModifierType to_stack(Decoration d) {
	switch (d) {
	case Decoration::pointer:
		return StackModifierType::pointer;
	case Decoration::reference:
		return StackModifierType::ref;
	case Decoration::rvalue_reference:
		return StackModifierType::rval_ref;
	case Decoration::const_keyword:
		return StackModifierType::const_;
	case Decoration::array_brackets:
		return StackModifierType::array;
	default:
		return StackModifierType::name;
	}
}

template <typename T>
struct Stacked {
	using value_type = T;
	struct Entry {
		value_type text;
		StackModifierType mod;

		Entry(value_type&& v, StackModifierType m) : text(std::move(v)), mod(m) {}
		Entry(const value_type& v, StackModifierType m) : text(v), mod(m) {}

		bool invalidated = false;
	};
	std::vector<Entry> stack;

	bool empty() const {
		return stack.empty();
	}

	void clear() {
		stack.clear();
	}

	bool contains_type(StackModifierType t) const {
		for (auto& [s, m, i] : stack) {
			if (m == t) {
				return true;
			}
		}
		return false;
	}
	bool contains_type(StackModifierType t0, StackModifierType t1) const {
		for (auto& [s, m, i] : stack) {
			if (m == t0 or m == t1) {
				return true;
			}
		}
		return false;
	}

	void append(value_type&& v, StackModifierType m) {
		stack.emplace_back(std::move(v), m);
	}
	void append(value_type const& v, StackModifierType m = StackModifierType::name) {
		stack.emplace_back(v, m);
	}
	template <typename W, typename Formatter>
	void append(Decoration m, Formatter& formatter, W&& w) {
		stack.emplace_back(formatter.decorate(m, std::forward<W>(w)), to_stack(m));
	}

	Entry pop() {
		auto r = std::move(stack.back());
		stack.pop_back();
		return r;
	}

	value_type partial_collapse(size_t idx) {
		value_type result;
		bool prev_pr = false;
		for (ssize_t i = ssize_t(idx) - 1; i >= 0; i--) {
			if (prev_pr and stack[i].mod == StackModifierType::const_ and not stack[i].invalidated) {
				result += ' ';
				result += stack[i].text;
				stack[i].invalidated = true;
			}
			if (stack[i].mod == StackModifierType::pointer or stack[i].mod == StackModifierType::ref or stack[i].mod == StackModifierType::rval_ref) {
				if (not stack[i].invalidated) {
					result += stack[i].text;
					stack[i].invalidated = true;
					prev_pr = true;
				}
			} else {
				prev_pr = false;
			}
		}
		return result;
	}

	value_type collapse() const {
		value_type result;
		for (size_t i = 0; i < stack.size(); i++) {
			if (not stack[i].invalidated) {
				result += stack[i].text;
			}
		}
		return result;
	}
};

template <typename StringType>
struct CompositeStack {
	using value_type = StringType;

	value_type head;
	value_type tail;

	CompositeStack(StringType&& h) :
	    head(std::move(h)) {}
	CompositeStack() = default;
	CompositeStack(const CompositeStack&) = default;
	CompositeStack(CompositeStack&&) = default;

	CompositeStack& operator=(const CompositeStack&) = default;
	CompositeStack& operator=(CompositeStack&&) = default;

	CompositeStack& operator+=(const StringType& sv) {
		head += sv;
		return *this;
	}
	CompositeStack& operator+=(std::string_view sv) {
		head += sv;
		return *this;
	}
	CompositeStack& operator+=(char sv) {
		head += sv;
		return *this;
	}

	friend CompositeStack operator+(const CompositeStack& a, std::string_view b) {
		auto c = a;
		c += b;
		return c;
	}
	friend CompositeStack operator+(CompositeStack&& a, std::string_view b) {
		CompositeStack c(std::move(a));
		c += b;
		return c;
	}

	bool empty() const {
		return head.empty() and tail.empty();
	}

	value_type collapse() const {
		if (tail.empty()) {
			return head;
		}
		value_type result = head;

		result += tail;
		return result;
	}
};

template <typename T>
auto compose(const CompositeStack<T>& c) {
	return c.collapse();
}

template <typename T, typename E>
auto compose(const std::expected<CompositeStack<T>, E>& c) -> std::expected<T, E> {
	if (not c) {
		return std::unexpected(c.error());
	}
	return c->collapse();
}

template <typename T>
auto expand(T&& c) -> CompositeStack<T> {
	return CompositeStack<T> {std::forward<T>(c)};
}

template <typename T, typename E>
auto expand(std::expected<T, E>&& c) -> std::expected<CompositeStack<T>, E> {
	if (not c) {
		return std::unexpected(c.error());
	}
	return expand(std::move(*c));
}

using StackedString = CompositeStack<std::string>;

}
