// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#include <doctest/doctest.h>

#include "fabric/name.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

    moq::fabric::NameSpace Ns(std::initializer_list<std::string_view> tuples)
    {
        std::vector<std::vector<uint8_t>> entries;
        entries.reserve(tuples.size());
        for (const auto tuple : tuples) {
            entries.emplace_back(tuple.begin(), tuple.end());
        }
        return moq::fabric::NameSpace(entries);
    }

    uint64_t TupleHash(std::string_view tuple)
    {
        const auto bytes = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(tuple.data()), tuple.size());
        return moq::fabric::hash(bytes);
    }

    uint64_t HashPreviousAndTuple(uint64_t previous, std::string_view tuple)
    {
        std::vector<uint8_t> material(sizeof(previous) + tuple.size());
        for (std::size_t i = 0; i < sizeof(previous); ++i) {
            material[i] = static_cast<uint8_t>(previous >> (56 - (8 * i)));
        }
        for (std::size_t i = 0; i < tuple.size(); ++i) {
            material[sizeof(previous) + i] = static_cast<uint8_t>(tuple[i]);
        }
        return moq::fabric::hash(std::span<const uint8_t>(material.data(), material.size()));
    }

}

TEST_CASE("Namespace hash chains each tuple")
{
    using moq::fabric::hash;
    using moq::fabric::NameSpace;

    const NameSpace empty;
    CHECK_EQ(empty.Hash(), hash(std::span<const uint8_t>{}));

    const auto only_a = Ns({ "a" });
    CHECK_EQ(only_a.Hash(), TupleHash("a"));

    const auto split = Ns({ "a", "b", "c" });
    const auto joined = Ns({ "a", "bc" });
    CHECK(split.Hash() != joined.Hash());

    const auto ab = Ns({ "a", "b" });
    const auto ba = Ns({ "b", "a" });
    CHECK(ab.Hash() != ba.Hash());

    uint64_t expected = TupleHash("a");
    expected = HashPreviousAndTuple(expected, "b");
    expected = HashPreviousAndTuple(expected, "c");
    CHECK_EQ(split.Hash(), expected);

    CHECK_EQ(split.Hash(), HashPreviousAndTuple(ab.Hash(), "c"));
}
