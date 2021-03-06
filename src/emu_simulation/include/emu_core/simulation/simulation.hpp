#pragma once

#include "emu_6502/cpu/cpu.hpp"
#include "emu_6502/cpu/debugger.hpp"
#include "emu_core/clock.hpp"
#include "emu_core/device_factory.hpp"
#include "emu_core/memory/memory_mapper.hpp"
#include "emu_core/memory_configuration_file.hpp"
#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace emu {

struct EmuSimulation {
    const std::unique_ptr<Clock> clock;
    const std::unique_ptr<memory::MemoryMapper16> memory;
    const std::unique_ptr<emu6502::cpu::Cpu> cpu;
    const std::unique_ptr<emu6502::cpu::Debugger> debugger;
    const std::vector<std::shared_ptr<Device>> devices;
    const std::vector<std::shared_ptr<Memory16>> mapped_devices;

    EmuSimulation(std::unique_ptr<Clock> _clock,
                  std::unique_ptr<memory::MemoryMapper16> _memory,
                  std::unique_ptr<emu6502::cpu::Cpu> _cpu,
                  std::unique_ptr<emu6502::cpu::Debugger> _debugger,
                  std::vector<std::shared_ptr<Device>> _devices,
                  std::vector<std::shared_ptr<Memory16>> _mapped_devices)
        : clock(std::move(_clock)), memory(std::move(_memory)), cpu(std::move(_cpu)),
          debugger(std::move(_debugger)), devices(std::move(_devices)),
          mapped_devices(std::move(_mapped_devices)) {}

    struct Result {
        double duration;
        uint64_t cpu_cycles;
        std::optional<uint8_t> halt_code;
    };

    class SimulationFailedException : public std::runtime_error {
    public:
        SimulationFailedException(const std::string &message, std::exception_ptr e,
                                  Result r)
            : runtime_error(message), exception(e), result(r) {}

        const Result &GetResult() const { return result; }
        void Retrow() const { std::rethrow_exception(exception); }
        std::exception_ptr Get() const { return exception; }

    private:
        std::exception_ptr exception;
        Result result;
    };

    Result Run(std::chrono::nanoseconds timeout = {});
};

} // namespace emu
