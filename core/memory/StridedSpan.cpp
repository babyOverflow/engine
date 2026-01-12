#include "StridedSpan.h"
namespace core::memory {

static_assert(std::random_access_iterator<StridedSpan<int>::iterator>);
static_assert(std::random_access_iterator<StridedSpan<int>::const_iterator>);
}  // namespace core::memory