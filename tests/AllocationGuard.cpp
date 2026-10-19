#include "AllocationGuard.h"
#include <atomic>
#include <cstdlib>
#include <new>
namespace {
std::atomic<bool> active{false};
std::atomic<uint64_t> count{0};
void record() noexcept {if(active.load(std::memory_order_relaxed))count.fetch_add(1,std::memory_order_relaxed);}
extern "C" void* guardedMalloc(size_t n){record();return std::malloc(n);}
extern "C" void* guardedCalloc(size_t n,size_t s){record();return std::calloc(n,s);}
extern "C" void* guardedRealloc(void* p,size_t n){record();return std::realloc(p,n);}
extern "C" void guardedFree(void* p){if(p)record();std::free(p);}
__attribute__((used)) const struct {const void* replacement;const void* original;} interposers[] __attribute__((section("__DATA,__interpose"))) = {
    {reinterpret_cast<const void*>(&guardedMalloc),reinterpret_cast<const void*>(&malloc)},
    {reinterpret_cast<const void*>(&guardedCalloc),reinterpret_cast<const void*>(&calloc)},
    {reinterpret_cast<const void*>(&guardedRealloc),reinterpret_cast<const void*>(&realloc)},
    {reinterpret_cast<const void*>(&guardedFree),reinterpret_cast<const void*>(&free)}
};
}
void startAllocationGuard() noexcept {count.store(0);active.store(true);}
uint64_t stopAllocationGuard() noexcept {active.store(false);return count.load();}
void* operator new(std::size_t n){record();if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept {if(p)record();std::free(p);}
void operator delete[](void* p) noexcept {::operator delete(p);}
void operator delete(void* p,std::size_t) noexcept {::operator delete(p);}
void operator delete[](void* p,std::size_t) noexcept {::operator delete(p);}
