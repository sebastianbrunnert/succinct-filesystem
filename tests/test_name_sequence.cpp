#include <gtest/gtest.h>
#include "../src/name_sequence/name_sequence.hpp"
#include <memory>

class NameSequenceTest : public ::testing::TestWithParam<std::function<NameSequence*()>> {
protected:
    NameSequence* create_name_sequence() {
        return GetParam()();
    }
};

TEST_P(NameSequenceTest, InsertAndAccess) {
    auto name_sequence = this->create_name_sequence();
    for (size_t i = 0; i < 10; i++) {
        name_sequence->insert(i, "name" + std::to_string(i));
        EXPECT_EQ(name_sequence->access(i), "name" + std::to_string(i));
    }
    for (size_t i = 0; i < 10; i++) {
        EXPECT_EQ(name_sequence->access(i), "name" + std::to_string(i));
    }
    EXPECT_EQ(name_sequence->size(), 10);
    EXPECT_THROW(name_sequence->access(10), std::out_of_range);
    delete name_sequence;
}

TEST_P(NameSequenceTest, Set) {
    auto name_sequence = this->create_name_sequence();
    for (size_t i = 0; i < 10; i++) {
        name_sequence->insert(i, "name" + std::to_string(i));
    }
    for (size_t i = 0; i < 10; i++) {
        name_sequence->set(i, "new_name" + std::to_string(i));
    }
    for (size_t i = 0; i < 10; i++) {
        EXPECT_EQ(name_sequence->access(i), "new_name" + std::to_string(i));
    }
    delete name_sequence;
}

TEST_P(NameSequenceTest, Remove) {
    auto name_sequence = this->create_name_sequence();
    for (size_t i = 0; i < 10; i++) {
        name_sequence->insert(i, "name" + std::to_string(i));
    }
    for (size_t i = 0; i < 10; i++) {
        name_sequence->remove(0);
        for (size_t j = 0; j < 9 - i; j++) {
            EXPECT_EQ(name_sequence->access(j), "name" + std::to_string(j + i + 1));
        }
    }
    EXPECT_EQ(name_sequence->size(), 0);
    delete name_sequence;
}

INSTANTIATE_TEST_SUITE_P(
    NameSequenceStrategies,
    NameSequenceTest,
    ::testing::Values(
        std::function<NameSequence*()>([]() { return create_name_sequence<ArrayNameSequenceStrategy>(); }),
        std::function<NameSequence*()>([]() { return create_name_sequence<ConcatenatedNameSequenceStrategy>(); }),
        std::function<NameSequence*()>([]() { return create_name_sequence<ImmerNameSequenceStrategy>(); }),
        std::function<NameSequence*()>([]() { return create_name_sequence<MapNameSequenceStrategy>(); })
    )
);