#include <cassert>
#include <cstdint>

#include "storm/core/ResourcePool.hpp"

struct TestResourceTag;

struct Resource final {
    explicit Resource(int value) : value(value) {}
    int value{};
};

int main() {
    using Pool = storm::core::ResourcePool<Resource, TestResourceTag>;

    Pool pool;
    assert(pool.liveCount() == 0);
    assert(pool.capacity() == 0);

    const auto first = pool.emplace(42);
    assert(first.valid());
    assert(pool.liveCount() == 1);

    // A pool handle must remain usable until explicit destruction.
    assert(pool.get(first) != nullptr);
    assert(pool.get(first)->value == 42);
    assert(pool.valid(first));

    assert(pool.destroy(first));
    assert(!pool.valid(first));
    assert(pool.get(first) == nullptr);

    const auto second = pool.emplace(84);
    assert(second.valid());
    assert(second.index() == first.index());
    assert(second.generation() != first.generation());
    assert(pool.get(second)->value == 84);
    assert(!pool.valid(first));

    assert(!pool.destroy(first));
    assert(pool.destroy(second));
    assert(!pool.destroy(second));

    const auto third = pool.emplace(7);
    const auto fourth = pool.emplace(9);
    assert(pool.valid(third));
    assert(pool.valid(fourth));
    assert(pool.get(third)->value == 7);
    assert(pool.get(fourth)->value == 9);

    pool.clear();
    assert(pool.liveCount() == 0);
    assert(pool.capacity() == 0);
    assert(!pool.valid(third));

    // Clearing a pool must invalidate every pre-clear handle, even if the
    // next allocation reuses the same slot index.
    const beforeClear = pool.emplace(123);
    assert(beforeClear.valid());
    pool.clear();
    const afterClear = pool.emplace(456);
    assert(afterClear.valid());
    assert(afterClear.index() == beforeClear.index());
    assert(afterClear.generation() != beforeClear.generation());
    assert(!pool.valid(beforeClear));
    assert(pool.get(beforeClear) == nullptr);
    assert(pool.get(afterClear)->value == 456);

    return 0;
}
