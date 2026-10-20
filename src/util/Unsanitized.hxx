#pragma once
#include <string>
#include <string_view>
#include <type_traits>
namespace swartalf {
template <typename T>
struct Unsanitized {
	using value_type = T;

	constexpr Unsanitized() = default;
	constexpr Unsanitized(const Unsanitized&) = default;
	constexpr Unsanitized(Unsanitized&&) noexcept = default;
	explicit constexpr Unsanitized(const T& v) : val(v) {}
	explicit constexpr Unsanitized(T&& v) noexcept : val(std::move(v)) {}
	constexpr Unsanitized& operator=(const Unsanitized&) = default;
	constexpr Unsanitized& operator=(Unsanitized&&) noexcept = default;
	constexpr Unsanitized& operator=(const T& v) {
		val = v;
		return *this;
	}
	constexpr Unsanitized& operator=(T&& v) noexcept {
		val = std::move(v);
		return *this;
	}

	constexpr bool empty() const {
		return val.empty();
	}
	constexpr size_t size() const {
		return val.size();
	}
	constexpr auto data() const {
		return val.data();
	}
	constexpr auto back() const -> decltype(auto) {
		return val.back();
	}
	constexpr auto operator[](size_t idx) const -> decltype(auto) {
		return val[idx];
	}
	template <typename... Args>
	auto find(Args&&... args) const -> decltype(auto) {
		return val.find(std::forward<Args>(args)...);
	}
	template <typename... Args>
	auto find_last_of(Args&&... args) const -> decltype(auto) {
		return val.find_last_of(std::forward<Args>(args)...);
	}
	template <typename... Args>
	auto find_first_of(Args&&... args) const -> decltype(auto) {
		return val.find_first_of(std::forward<Args>(args)...);
	}
	template <typename... Args>
	auto substr(Args&&... args) const -> decltype(auto) {
		auto found = val.substr(std::forward<Args>(args)...);
		return Unsanitized<std::decay_t<decltype(found)>>(found);
	}
	template <typename... Args>
	void remove_prefix(Args&&... args) {
		val.remove_prefix(std::forward<Args>(args)...);
	}

	constexpr const T& get_value() const {
		return val;
	}

	constexpr T& get_value() {
		return val;
	}

	void clear() {
		val.clear();
	}

	template <typename From>
	    requires std::is_convertible_v<std::remove_cvref_t<From>, T> and (not std::is_same_v<std::remove_cvref_t<From>, T>)
	Unsanitized& operator=(const Unsanitized<From>& arg) {
		val = arg.get_value();
		return *this;
	}

	template <typename... Args>
	auto starts_with(Args&&... args) const -> decltype(auto) {
		return val.starts_with(std::forward<Args>(args)...);
	}

	template <typename... Args>
	auto ends_with(Args&&... args) const -> decltype(auto) {
		return val.ends_with(std::forward<Args>(args)...);
	}
	friend constexpr auto operator<=>(const Unsanitized& lhs, const Unsanitized& rhs) {
		return lhs.val <=> rhs.val;
	}

	friend constexpr bool operator==(const Unsanitized& lhs, const Unsanitized& rhs) {
		return lhs.val == rhs.val;
	}

	friend constexpr auto operator==(const Unsanitized& lhs, std::string_view rhs) -> bool {
		return lhs.val == rhs;
	}

	template <typename T2>
	    requires(not std::is_same_v<T, T2>)
	friend constexpr auto operator==(const Unsanitized& lhs, const Unsanitized<T2>& rhs) -> bool {
		return lhs.val == rhs.get_value();
	}

private:
	T val;
};

template <typename T>
Unsanitized(T) -> Unsanitized<std::decay_t<T>>;

constexpr Unsanitized<std::string_view> operator""_usv(const char* p, size_t s) {
	return Unsanitized(std::string_view(p, s));
}

inline Unsanitized<std::string> from_view(const Unsanitized<std::string_view> v) {
	return Unsanitized(std::string(v.get_value()));
}

inline Unsanitized<std::string_view> to_view(const Unsanitized<std::string>& v) {
	return Unsanitized(std::string_view(v.get_value()));
}

}

namespace std {
template <typename Value>
struct hash<swartalf::Unsanitized<Value>> {
	size_t operator()(const swartalf::Unsanitized<Value>& offset) const {
		return hash<Value>()(offset.val);
	}
};
}