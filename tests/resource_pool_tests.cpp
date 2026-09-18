#include <cassert>
#include <cstdint>

#include "storm/core/ResourcePool.hpp"

struct TestResourceTag;

struct Resource final {
    explicit Resource(int value) : value(value) {}
    int value{};
};

struct ThrowingResource final {
    static bool shouldThrow;
    explicit ThrowingResource(int value) {
        if (shouldThrow) throw value;
        this->value = value;
    }
    int value{};
};
bool ThrowingResource::shouldThrow = false;

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

    using ThrowingPool = storm::core::ResourcePool<ThrowingResource, struct ThrowingTag>;
    ThrowingPool throwingPool;
    const auto live = throwingPool.emplace(11);
    assert(throwingPool.destroy(live));
    ThrowingResource::shouldThrow = true;
    bool threw = false;
    try {
        (void)throwingPool.emplace(22);
    } catch (int value) {
        threw = (value == 22);
    }
    ThrowingResource::shouldThrow = false;
    assert(threw);
    const auto recovered = throwingPool.emplace(33);
    assert(recovered.valid());
    assert(throwingPool.get(recovered)->value == 33);

    return 0;
}
