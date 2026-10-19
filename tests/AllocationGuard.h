#pragma once
#include <cstdint>
void startAllocationGuard() noexcept;
uint64_t stopAllocationGuard() noexcept;
