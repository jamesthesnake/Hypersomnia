#pragma once
#include <algorithm>
#include "augs/templates/container_traits.h"

template <class Container, class F>
void erase_if(Container& v, F f) {
	if constexpr(can_access_data_v<Container>) {
		v.erase(std::remove_if(v.begin(), v.end(), f), v.end());
	}
	else {
		for (auto it = v.begin(); it != v.end(); ) {
			if (f(*it)) {
				it = v.erase(it);
			}
			else {
				++it;
			}
		}
	}
}

template <class Container, class T>
void erase_element(Container& v, const T& l) {
	static_assert(!std::is_same_v<decltype(v.begin()), T>, "erase_element serves to erase keys or values, not iterators!");

	if constexpr(can_access_data_v<Container>) {
		v.erase(std::remove(v.begin(), v.end(), l), v.end());
	}
	else {
		v.erase(l);
	}
}

template<class Container>
void remove_duplicates_from_sorted(Container& v, std::enable_if_t<can_access_data_v<Container>>* dummy = nullptr) {
	v.erase(std::unique(v.begin(), v.end()), v.end());
}

template<class A, class B>
A& concatenate(A& a, const B& b) {
	if constexpr(can_access_data_v<A>) {
		a.insert(a.end(), b.begin(), b.end());
	}
	else {
		a.insert(b.begin(), b.end());
	}

	return a;
}

template<class Container, class K>
auto find_in(Container& v, const K& key) {
	static_assert(!has_member_find_v<Container, K>, "Use mapped_or_nullptr instead of find_in.");
	
	return std::find(v.begin(), v.end(), key);
}

template<class Container, class K>
bool found_in(Container& v, const K& l) {
	if constexpr(has_member_find_v<Container, K>) {
		return v.find(l) != v.end();
	}
	else {
		return find_in(v, l) != v.end();
	}
}

template <class T, class... Args>
auto default_or_invalid_enum(Args&&... args) {
	if constexpr(std::is_enum_v<T>) {
		return T::INVALID;
	}
	else {
		static_assert(
			!std::is_arithmetic_v<T>,
			"Default value for arithmetic types is not well-defined."
		);

		return T { std::forward<Args>(args)... };
	}
}

template <class Container, class Key, class... Args>
auto mapped_or_default(
	const Container& container, 
	const Key& key,
	Args&&... args
) {
	using M = typename std::decay_t<Container>::mapped_type;

	if (const auto it = container.find(key);
		it != container.end()
	) {
		return (*it).second;
	}

	return default_or_invalid_enum<M>(std::forward<Args>(args)...);
}

template <class Container, class Key>
auto mapped_or_nullptr(
	Container& container,
	const Key& key
) -> decltype(std::addressof((*container.begin()).second)) {
	if (const auto it = container.find(key);
		it != container.end()
	) {
		return std::addressof((*it).second);
	}

	return nullptr;
}

template <class Container, class Value>
auto key_or_default(
	const Container& container, 
	const Value& searched_value
) {
	using K = typename std::decay_t<Container>::key_type;

	for (const auto& [key, tested_value] : container) {
		if (tested_value == searched_value) {
			return key;
		}
	}

	return default_or_invalid_enum<K>();
}


template <class Container>
auto first_free_key(const Container& in) {
	for (std::size_t candidate = 0;;++candidate) {
		const auto key = static_cast<typename Container::key_type>(candidate);
		const auto it = in.find(key);

		if (it == in.end()) {
			return key;
		}
	}
}

template <class Container, class F>
void for_each_in(const Container& in, F callback) {
	for (auto& element : in) {
		callback(element);
	}
}

/* Thanks to https://stackoverflow.com/a/28139075/503776 */

template <typename T>
struct reversion_wrapper { T& iterable; };

template <typename T>
auto begin(reversion_wrapper<T> w) { return rbegin(w.iterable); }

template <typename T>
auto end(reversion_wrapper<T> w) { return rend(w.iterable); }

template <typename T>
reversion_wrapper<T> reverse(T&& iterable) { return { iterable }; }
