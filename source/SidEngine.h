// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Parameters.h"
#include "Drums.h"
#include <array>
#include <atomic>
#include <memory>
namespace reSIDfp { class SID; }
namespace sidlab {
struct MidiEvent { uint8_t status=0, data1=0, data2=0; };
class SidEngine {
public:
    SidEngine();
    ~SidEngine();
    // Allocation, chip setup and sinc tables are confined to prepare().
    void prepare(double sampleRate);
    void setPatch(const Patch&) noexcept;
    void setTempo(double bpm) noexcept;
    void midi(MidiEvent) noexcept;
    void render(float* left, float* right, int samples) noexcept;
    void panic() noexcept;
    int soundingVoices() const noexcept;
    int heldNotes() const noexcept;
    uint8_t debugRegister(int slot, int address) const noexcept;
    double rate() const noexcept { return sampleRate; }
    std::array<std::atomic<float>,256> scope{};
    std::atomic<unsigned> scopeHead {0};
    std::atomic<unsigned> activity {0};
    std::array<std::atomic<float>,1024> analysis{};
    std::atomic<unsigned> analysisHead{0};
    std::atomic<double> visualSampleRate{48000};
private:
    struct Slot {
        std::unique_ptr<reSIDfp::SID> chip;
        std::array<uint8_t,25> regs{};
        int note=-1, channel=0;
        uint64_t age=0;
        bool gate=false, active=false, killing=false;
        float velocity=0, gain=0, tail=0, last=0;
        double released=0, releaseLimit=0.1, phase=0, arpPhase=0;
        std::array<int16_t,32> scratch{};
        Patch drumPatch{};
        DrumType drumType=DrumType::Kick;
        bool oneShot=false;
        float rootNote=60;
        double elapsed=0,autoGate=0.012;
    };
    struct Key { uint8_t count=0, velocity=0; bool sustained=false; uint64_t age=0; };
    std::array<std::array<Slot,4>,4> banks;
    std::array<std::array<Key,128>,16> keys{};
    std::array<bool,16> pedal{};
    std::array<float,16> bend{};
    Patch patch{},pendingPatch{};
    double sampleRate=48000, tempo=120;
    float cutoff=1250, master=0, transitionGain=1;
    int bank=0, mode=1, capacity=4, pendingBank=0, pendingMode=1, pendingCapacity=4;
    bool prepared=false, switching=false;
    // Preserve new hits/notes during the short architecture-change fade.
    std::array<MidiEvent,32> transitionMidi{};
    unsigned transitionMidiCount=0;
    uint64_t age=0;
    unsigned scopeDecimation=0;
    void write(Slot&,int,uint8_t) noexcept;
    void update(Slot&,int count) noexcept;
    void start(Slot&,int channel,int note,int velocity) noexcept;
    void release(Slot&) noexcept;
    void noteOn(int channel,int note,int velocity) noexcept;
    void noteOff(int channel,int note) noexcept;
    void reconcileMono() noexcept;
    void switchBank() noexcept;
    void clearChannel(int channel,bool hard) noexcept;
};
} // namespace sidlab
