#pragma once

#include "emu_core/clock.hpp"
#include "emu_core/memory.hpp"
#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <fmt/format.h>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace emu::memory {

template <std::unsigned_integral _Address_t>
struct MemoryMapper : public MemoryInterface<_Address_t> {
    using Address_t = _Address_t;
    using Iface = MemoryInterface<_Address_t>;

    using RangePair = std::pair<Address_t, Address_t>;
    using AreaInterface = MemoryInterface<Address_t> *;
    using Area = std::pair<RangePair, AreaInterface>;

    struct AreaComp {
        bool operator()(const Area &a, const Area &b) const {
            auto [min_a, max_a] = a.first;
            auto [min_b, max_b] = b.first;
            if ((max_a < min_b && min_b < max_a) || (max_b < min_a && min_a < max_b)) {
                throw std::runtime_error(fmt::format(
                    "AreaComp: overlapping ranges {:04x}:{:04x} <-> {:04x}:{:04x}", min_a,
                    max_a, min_b, max_b));
            }
            return min_a < min_b;
        }
    };

    using VectorType = std::vector<uint8_t>;
    using AreaSet = std::set<Area, AreaComp>;

    Clock *const clock;
    const bool strict_access;
    std::ostream *const verbose_stream;

    MemoryMapper(Clock *clock, const AreaSet &area = {}, bool strict_access = false,
                 std::ostream *verbose_stream = nullptr)
        : clock(clock), strict_access(strict_access), verbose_stream(verbose_stream),
          areas() {
        for (auto [range, ptr] : area) {
            MapArea(range, ptr);
        }
    }
    MemoryMapper(Clock *clock, bool strict_access = false,
                 std::ostream *verbose_stream = nullptr)
        : MemoryMapper(clock, {}, strict_access, verbose_stream) {}

    void MapArea(Address_t offset, Address_t size, AreaInterface mem_iface) {
        auto end_addr = static_cast<Address_t>(offset + size - 1);
        MapArea({offset, end_addr}, mem_iface);
    }

    void MapArea(RangePair range, AreaInterface mem_iface) {
        if (mem_iface == nullptr) {
            //TODO
        }
        if (range.first > range.second) {
            //TODO
        }
        //TODO: verify overlapping ranges
        areas.emplace(range, std::move(mem_iface));
    }

    uint8_t Load(Address_t address) const override {
        WaitForNextCycle();
        auto area = LookupAddress(address);
        if (area.has_value()) {
            auto [min, max] = area->first;
            Address_t relative = address - min;
            auto v = area->second->Load(relative);
            AccessLog(address, v, false, false);
            return v;
        }

        AccessLog(address, 0, false, true);
        throw std::runtime_error(fmt::format(
            "MemoryMapper: Attempt to read unmapped address {:04x}", address));
    }

    void Store(Address_t address, uint8_t value) override {
        WaitForNextCycle();
        auto area = LookupAddress(address);
        if (area.has_value()) {
            AccessLog(address, value, true, false);
            auto [min, max] = area->first;
            Address_t relative = address - min;
            return area->second->Store(relative, value);
        }
        AccessLog(address, value, true, true);
        throw std::runtime_error(fmt::format(
            "MemoryMapper: Attempt to write unmapped address {:04x}", address));
    }

    [[nodiscard]] std::optional<uint8_t> DebugRead(Address_t address) const override {
        auto area = LookupAddress(address);
        if (!area.has_value()) {
            return std::nullopt;
        }

        auto [min, max] = area->first;
        Address_t relative = address - min;
        return area->second->DebugRead(relative);
    }

private:
    AreaSet areas;

    std::optional<Area> LookupAddress(Address_t addr) const {
        auto area_it = std::find_if(areas.begin(), areas.end(), [addr](const auto &item) {
            auto [min, max] = item.first;
            return min <= addr && addr <= max;
        });

        if (area_it == areas.end()) {
            return std::nullopt;
        }

        return *area_it;
    }

    void AccessLog(Address_t address, uint8_t value, bool write,
                   bool not_mapped = false) const {
        if (verbose_stream != nullptr) {
            Iface::WriteAccessLog(*verbose_stream, "MAPPER", {}, write, address, value,
                                  (not_mapped ? "NOT MAPPED" : ""));
        }
    }

    void WaitForNextCycle() const {
        if (clock != nullptr) {
            clock->WaitForNextCycle();
        }
    }
};

using MemoryMapper16 = MemoryMapper<uint16_t>;

} // namespace emu::memory
