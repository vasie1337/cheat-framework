#pragma once
#include <chrono>
#include <unordered_map>
#include <string>

class UpdateManager {
private:
	std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_updates_;

public:
	bool should_update(const std::string& key, int interval_ms) {
		auto now = std::chrono::steady_clock::now();
		auto it = last_updates_.find(key);

		if (it == last_updates_.end()) {
			last_updates_[key] = now;
			return true;
		}

		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			now - it->second).count();

		if (elapsed >= interval_ms) {
			it->second = now;
			return true;
		}

		return false;
	}

	void force_update(const std::string& key) {
		last_updates_.erase(key);
	}
};