#pragma once
#include <queue>
#include <vector>
#include <span>

#include "Common.h"

namespace core {
template <typename T>
class ResourcePool {
  public:
    ResourcePool() = default;
    Handle Attach(T&& resource) {
        uint32_t index;
        if (!m_freeSlots.empty()) {
            index = m_freeSlots.front();
            m_freeSlots.pop();
            m_data[index] = std::move(resource);
            m_actives[index] = true;
            m_generations[index]++;
        } else {
            index = static_cast<uint32_t>(m_data.size());
            m_data.push_back(std::move(resource));
            m_generations.push_back(0);
            m_actives.push_back(true);
        }
        return Handle{index, m_generations[index]};
    }
    void Release(const Handle& handle) {
        if (handle.index < m_data.size() && m_actives[handle.index] &&
            m_generations[handle.index] == handle.generation) {
            m_data[handle.index] = {};
            m_actives[handle.index] = false;
            m_freeSlots.push(handle.index);
        }
    }
    T* Get(const Handle& handle) {
        if (handle.index < m_data.size() && m_actives[handle.index] &&
            m_generations[handle.index] == handle.generation) {
            return &m_data[handle.index];
        }
        return nullptr;
    }

    std::span<const T> GetDataSpan() const { return m_data; }

  private:
    std::vector<T> m_data;
    std::vector<uint32_t> m_generations;
    std::vector<uint8_t> m_actives;
    std::queue<uint32_t> m_freeSlots;
};
}  // namespace core