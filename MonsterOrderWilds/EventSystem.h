#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include <algorithm>

template<typename... Args>
class Event
{
public:
    using Handler = std::function<void(Args...)>;
    using HandlerId = size_t;

    HandlerId AddListener(const Handler& handler) {
        HandlerId id = ++_lastId;
        _handlers.emplace_back(id, handler);
        return id;
    }

    void RemoveListener(HandlerId id) {
        _handlers.erase(
            std::remove_if(_handlers.begin(), _handlers.end(),
                [id](const auto& pair) { return pair.first == id; }),
            _handlers.end());
    }

    void Invoke(Args... args) {
        for (const auto& [id, handler] : _handlers) {
            handler(args...);
        }
    }

private:
    std::vector<std::pair<HandlerId, Handler>> _handlers;
    HandlerId _lastId = 0;
};
