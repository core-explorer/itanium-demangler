#pragma once
#include "Hyperlink.hxx"
#include <string>
#include <string_view>
#include <vector>
namespace swartalf {

enum class Resource {
	external,
	internal
};

enum class Envelope {
	fullPage,
	body,
	content
};

struct PageConfig {
	Resource resource = Resource::external;
	Navigation navigation = Navigation::hyperlink;
	Envelope envelope = Envelope::fullPage;
};

struct NavigationLevel {
	std::string name;
	std::string url;
	NavigationLevel() = default;
	NavigationLevel(std::string_view n, std::string_view u) :
	    name(n), url(u) {}
};

struct NavigationData {
	std::vector<NavigationLevel> levels;
	std::string title;

	bool empty() const {
		return levels.empty();
	}
	template <typename... Args>
	void emplace_back(Args&&... args) {
		levels.emplace_back(std::forward<Args>(args)...);
	}
	void clear() {
		levels.clear();
	}
	void pop_back() {
		levels.pop_back();
	}
	void push_back(const NavigationLevel& nl) {
		levels.push_back(nl);
	}
	size_t size() const {
		return levels.size();
	}
	NavigationLevel& operator[](size_t idx) {
		return levels[idx];
	}
	const NavigationLevel& operator[](size_t idx) const {
		return levels[idx];
	}
	NavigationLevel& back() {
		return levels.back();
	}
	const NavigationLevel& back() const {
		return levels.back();
	}
};
}
