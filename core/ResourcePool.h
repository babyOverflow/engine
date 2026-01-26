#pragma once
#include <queue>
#include <vector>

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
            m_slots[index].data = std::move(resource);
            m_slots[index].isActive = true;
            m_slots[index].generation++;
        } else {
            index = static_cast<uint32_t>(m_slots.size());
            m_slots.push_back({std::move(resource), 0, true});
        }
        return Handle{index, m_slots[index].generation};
    }
    void Release(const Handle& handle) {
        if (handle.index < m_slots.size() && m_slots[handle.index].isActive &&
            m_slots[handle.index].generation == handle.generation) {
            m_slots[handle.index].isActive = false;
            m_freeSlots.push(handle.index);
        }
    }
    T* Get(const Handle& handle) {
        if (handle.index < m_slots.size() && m_slots[handle.index].isActive &&
            m_slots[handle.index].generation == handle.generation) {
            return &m_slots[handle.index].data;
        }
        return nullptr;
    }

  private:
    struct Slot {
        T data;
        uint32_t generation = -1;
        bool isActive = false;
    };
    std::vector<Slot> m_slots;
    std::queue<uint32_t> m_freeSlots;
};
}  // namespace core