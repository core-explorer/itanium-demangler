#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
namespace swartalf {
template <typename T>
struct ReferenceTraits {
	using value_type = T;
	using reference = T&;
	using const_reference = const T&;
};

template <typename T>
struct IntrusiveRefCounted {
	size_t ref_count = 0;
	void add_ref() {
		ref_count++;
	}
	void release() {
		if (ref_count > 0) {
			ref_count--;
		}
		if (ref_count == 0) {
			delete static_cast<T*>(this);
		}
	}
};

template <typename T>
struct AtomicRefCounted {
	std::atomic<size_t> ref_count = 0;
	void add_ref() {
		ref_count++;
	}
	void release() {
		if (ref_count > 0) {
			ref_count--;
		}
		if (ref_count == 0) {
			delete static_cast<T*>(this);
		}
	}
};

template <typename T>
struct RefPointer {
	T* ptr = nullptr;
	constexpr RefPointer() noexcept = default;
	explicit RefPointer(T* p) :
	    ptr(p) {
		if (ptr) {
			ptr->add_ref();
		}
	}
	RefPointer(const RefPointer& other) :
	    ptr(other.ptr) {
		if (ptr) {
			ptr->add_ref();
		}
	}
	constexpr RefPointer(RefPointer&& other) noexcept :
	    ptr(other.ptr) {
		other.ptr = nullptr;
	}

	constexpr T* get() const {
		return ptr;
	}

	void reset() {
		if (ptr) {
			ptr->release();
			ptr = nullptr;
		}
	}

	template <typename Y>
	    requires std::is_base_of_v<T, Y>
	RefPointer(const RefPointer<Y>& other) :
	    ptr(other.ptr) {
		if (ptr) {
			ptr->add_ref();
		}
	}
	template <typename Y>
	    requires std::is_base_of_v<T, Y>
	RefPointer(RefPointer<Y>&& other) :
	    ptr(other.ptr) {
		other.ptr = nullptr;
	}

	RefPointer& operator=(const RefPointer& other) {
		if (ptr) {
			ptr->release();
		}
		ptr = other.ptr;
		if (ptr) {
			ptr->add_ref();
		}
		return *this;
	}

	void swap(RefPointer& other) noexcept {
		std::swap(ptr, other.ptr);
	}

	RefPointer& operator=(RefPointer&& other) noexcept {
		if (ptr) {
			ptr->release();
		}
		ptr = other.ptr;
		other.ptr = nullptr;
		return *this;
	}
	~RefPointer() {
		reset();
	}

	constexpr bool operator!() const {
		return ptr == nullptr;
	}
	explicit constexpr operator bool() const {
		return ptr != nullptr;
	}
	T* operator->() const {
		return ptr;
	}

	T& operator*() const {
		return *ptr;
	}
};

#if 0
template <typename T, typename C>
struct SubRefPointer {
	T* value = nullptr;
	C* container = nullptr;
	SubRefPointer() = default;
	SubRefPointer(T* v, C* c) :
	    value(v), container(c) {
		if (container) {
			container->add_ref();
		}
	}
	SubRefPointer(const SubRefPointer& other) :
	    value(other.value), container(other.container) {
		if (container) {
			container->add_ref();
		}
	}
	SubRefPointer(SubRefPointer&& other) noexcept :
	    value(other.value), container(other.container) {
		other.value = nullptr;
		other.container = nullptr;
	}
	SubRefPointer& operator=(const SubRefPointer& other) {
		if (container) {
			container->release();
		}
		value = other.value;
		container = other.container;
		if (container) {
			container->add_ref();
		}
		return *this;
	}
	SubRefPointer& operator=(SubRefPointer&& other) noexcept {
		if (container) {
			container->release();
		}
		value = other.value;
		container = other.container;
		other.value = nullptr;
		other.container = nullptr;
		return *this;
	}
	~SubRefPointer() {
		if (container) {
			container->release();
		}
	}
	T* operator->() {
		return value;
	}
	const T* operator->() const {
		return value;
	}
	T& operator*() {
		return *value;
	}
	const T& operator*() const {
		return *value;
	}
};

template <typename T>
struct WeakRefPointer {
	T* ptr = nullptr;
	WeakRefPointer() = default;
	WeakRefPointer(T* p) :
	    ptr(p) {
	}
	WeakRefPointer(const WeakRefPointer& other) :
	    ptr(other.ptr) {
	}
	WeakRefPointer(WeakRefPointer&& other) noexcept :
	    ptr(other.ptr) {
		other.ptr = nullptr;
	}
	WeakRefPointer& operator=(const WeakRefPointer& other) {
		ptr = other.ptr;

		return *this;
	}
	WeakRefPointer& operator=(WeakRefPointer&& other) noexcept {
		ptr = other.ptr;
		other.ptr = nullptr;
		return *this;
	}
	~WeakRefPointer() {
	}
	RefPointer<T> lock() const {
		return RefPointer<T>(ptr);
	}
};

template <typename T, typename C>
struct WeakSubRefPointer {
	T* value = nullptr;
	C* container = nullptr;
	WeakSubRefPointer() = default;
	WeakSubRefPointer(T* v, C* c) :
	    value(v), container(c) {
	}
	WeakSubRefPointer(const WeakSubRefPointer& other) :
	    value(other.value), container(other.container) {
	}
	WeakSubRefPointer(WeakSubRefPointer&& other) noexcept:
	    value(other.value), container(other.container) {
		other.value = nullptr;
		other.container = nullptr;
	}
	WeakSubRefPointer& operator=(const WeakSubRefPointer& other) {
		value = other.value;
		container = other.container;

		return *this;
	}
	WeakSubRefPointer& operator=(WeakSubRefPointer&& other) noexcept {
		value = other.value;
		container = other.container;
		other.value = nullptr;
		other.container = nullptr;
		return *this;
	}
	~WeakSubRefPointer() {
	}
	SubRefPointer<T, C> lock() const {
		return SubRefPointer<T, C>(value, container);
	}
};
#endif

template <typename T>
struct ReferenceTraits<IntrusiveRefCounted<T>> {
	using value_type = T;
	using reference = RefPointer<T>;
	using const_reference = RefPointer<const T>;
	static void addRef(T* ptr) {
		ptr->addRef();
	}
	static void release(T* ptr) {
		ptr->release();
	}
};

template <typename T, typename... Args>
RefPointer<T> make_ref_pointer(Args&&... args) {
	return RefPointer<T>(new T(std::forward<Args>(args)...));
}

template <typename New, typename Old>
constexpr RefPointer<New> cast_ref_pointer(const RefPointer<Old>& ptr) {
	RefPointer<New> result;
	result.ptr = static_cast<New*>(ptr.ptr);
	result.ptr->add_ref();
	return result;
}

template <typename New, typename Old>
constexpr RefPointer<New> cast_ref_pointer(RefPointer<Old>&& ptr) {
	RefPointer<New> result;
	result.ptr = static_cast<New*>(ptr.ptr);
	ptr.ptr = nullptr;
	return result;
}

template <typename T>
constexpr bool operator==(const RefPointer<T>& a, const RefPointer<T>& b) {
	return a.ptr == b.ptr;
}

template <typename T>
constexpr auto operator<=>(const RefPointer<T>& a, const RefPointer<T>& b) {
	return a.ptr <=> b.ptr;
}

}