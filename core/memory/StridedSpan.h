#pragma once

#include <cassert>
#include <concepts>
#include <cstdint>
#include <iterator>

namespace core::memory {

template <typename T>
class StridedSpan {
  public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using size_type = std::size_t;
    using InternalPtr =
        std::conditional_t<std::is_const_v<T>, const unsigned char*, unsigned char*>;

    // 반복자를 템플릿으로 만들어 T와 const T 모두 대응하게 합니다.
    template <typename U>
    class IteratorImpl {
      public:
        template <typename>
        friend class IteratorImpl;

        using iterator_category = std::random_access_iterator_tag;
        using iterator_concept = std::random_access_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::remove_cv_t<U>;
        using pointer = U*;
        using reference = U&;

        using IterInternalPtr =
            std::conditional_t<std::is_const_v<U>, const unsigned char*, unsigned char*>;

        IteratorImpl() : m_ptr(nullptr), m_stride(0), m_index(0) {}
        IteratorImpl(unsigned char* ptr, size_type index, uint32_t stride)
            : m_ptr(ptr), m_stride(stride), m_index(index) {}

        template <typename OtherU>
            requires std::convertible_to<OtherU*, U*>
        IteratorImpl(const IteratorImpl<OtherU>& other)
            : m_ptr(other.m_ptr), m_stride(other.m_stride), m_index(other.m_index) {}

        // 참조 및 포인터 연산
        reference operator*() const { return *reinterpret_cast<U*>(m_ptr); }
        pointer operator->() const { return reinterpret_cast<U*>(m_ptr); }
        reference operator[](difference_type n) const { return *(*this + n); }

        // 증감 연산
        IteratorImpl& operator++() {
            m_ptr += m_stride;
            ++m_index;
            return *this;
        }
        IteratorImpl operator++(int) {
            IteratorImpl tmp = *this;
            m_ptr += m_stride;
            ++m_index;
            return tmp;
        }
        IteratorImpl& operator--() {
            m_ptr -= m_stride;
            --m_index;
            return *this;
        }
        IteratorImpl operator--(int) {
            IteratorImpl tmp = *this;
            m_ptr -= m_stride;
            --m_index;
            return tmp;
        }

        // 산술 연산
        IteratorImpl& operator+=(difference_type n) {
            m_ptr += (n * m_stride);
            m_index += n;
            return *this;
        }
        IteratorImpl& operator-=(difference_type n) {
            m_ptr -= (n * m_stride);
            m_index -= n;
            return *this;
        }

        friend IteratorImpl operator+(IteratorImpl it, difference_type n) { return it += n; }
        friend IteratorImpl operator+(difference_type n, IteratorImpl it) { return it += n; }
        friend IteratorImpl operator-(IteratorImpl it, difference_type n) { return it -= n; }

        // 거리 계산 (중요: Stride로 나누기)
        difference_type operator-(const IteratorImpl& other) const {
            return (static_cast<difference_type>(m_index - other.m_index));
        }

        // 비교 연산 (Spaceship operator <=> 를 쓰면 간결하지만, 구형 컴파일러 고려 시 전체 구현)
        bool operator==(const IteratorImpl& other) const { return m_index == other.m_index; }
        auto operator<=>(const IteratorImpl&) const = default;

      private:
        IterInternalPtr m_ptr;
        uint32_t m_stride;
        size_type m_index;
    };

    using iterator = IteratorImpl<T>;
    using const_iterator = IteratorImpl<const T>;

    StridedSpan() : m_data(nullptr), m_stride(0), m_count(0) {}
    StridedSpan(InternalPtr data, uint32_t stride, size_type count)
        : m_data(data), m_stride(stride), m_count(count) {}

    static StridedSpan MakeRepeated(element_type& value, size_type count) {
        return StridedSpan(reinterpret_cast<InternalPtr>(&value), 0, count);
    }
    static StridedSpan MakeRepeated(const element_type&& value, size_type count) = delete;

    static StridedSpan MakeZero(size_type count) {
        static_assert(std::is_const_v<T>,
                      "MakeZero() can only be used with const T types (Read-Only).");
        static const auto kZero = T{};
        return MakeRepeated(kZero, count);
    }

    // 인덱스 접근
    T& operator[](size_type index) const {
        assert(index < m_count);
        return *reinterpret_cast<T*>(m_data + (index * m_stride));
    }

    iterator begin() { return iterator(m_data, 0, m_stride); }
    iterator end() { return iterator(m_data + (m_count * m_stride), m_count, m_stride); }

    const_iterator begin() const { return const_iterator(m_data, 0, m_stride); }
    const_iterator end() const {
        return const_iterator(m_data + (m_count * m_stride), m_count, m_stride);
    }

    size_type size() const { return m_count; }
    uint32_t stride() const { return m_stride; }
    bool empty() const { return m_count == 0; }

    T& front() { return *begin(); }
    T& back() { return *(begin() + (m_count - 1)); }

  private:
    InternalPtr m_data;
    uint32_t m_stride;
    size_type m_count;
};


}  // namespace core::memory