#pragma once
#include <vector>
#include <unordered_set>

template<typename T>
bool has_changed(const std::vector<T>& old_list, const std::vector<T>& new_list) {
	if (old_list.size() != new_list.size()) return true;

	std::unordered_set<T> old_set(old_list.begin(), old_list.end());
	for (const T& item : new_list) {
		if (old_set.find(item) == old_set.end()) {
			return true;
		}
	}
	return false;
}