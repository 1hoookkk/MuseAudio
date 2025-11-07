#pragma once
// PsychoBus + Lock-free SPSC + SharedResource wrapper
// C++17+ header-only. Requires JUCE core module for SharedResourcePointer.

#include <atomic>
#include <array>
#include <cstdint>
#include <type_traits>
#include <limits>

#include <juce_core/juce_core.h>

namespace dsp_core {

constexpr size_t kCacheLine = 64;
constexpr size_t kMaxSubscribers = 32;
constexpr size_t kFeatureQueueSize = 64;
constexpr size_t kIntentQueueSize  = 64;
constexpr size_t kBarkBands        = 24;

using PluginId = uint32_t;

constexpr uint32_t fnv1a32(const char* s) noexcept
{
    uint32_t h = 2166136261u;
    while (*s != '\0')
    {
        h ^= static_cast<uint8_t>(*s++);
        h *= 16777619u;
    }
    return h;
}

struct FeatureFrame
{
    uint64_t audioFrameIndex = 0;  // running sample index at start of hop
    uint32_t hopSize         = 0;  // samples contributing to this frame
    float    fs              = 48000.0f;
    std::array<float, kBarkBands> barkEnergy{}; // linear energy per Bark band
    float spectralFlux = 0.0f;
    float crestFactor  = 0.0f;
    float loudnessLUFS = 0.0f;
    uint64_t seq       = 0;        // monotonic frame counter
};
static_assert(std::is_trivially_copyable_v<FeatureFrame>, "FeatureFrame must be trivial");

enum class IntentType : uint8_t
{
    None = 0,
    LimitHeadroomDb,
    EaseMorph,
    TogglePostSaturation
};

struct Intent
{
    PluginId   plugin  = 0;              // 0 => broadcast to all subscribers
    IntentType type    = IntentType::None;
    float      value   = 0.0f;           // meaning depends on type
    uint64_t   seqFrom = 0;              // originating FeatureFrame seq
};
static_assert(std::is_trivially_copyable_v<Intent>, "Intent must be trivial");

template <typename T, size_t CapacityPow2>
class LockFreeSPSC
{
    static_assert((CapacityPow2 & (CapacityPow2 - 1)) == 0, "Capacity must be power-of-two");
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

public:
    bool push(const T& v) noexcept
    {
        const size_t head = head_.load(std::memory_order_relaxed);
        const size_t tail = tail_.load(std::memory_order_acquire);
        if ((head - tail) == CapacityPow2)
            return false; // full
        buffer_[head & mask_] = v;
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& out) noexcept
    {
        const size_t tail = tail_.load(std::memory_order_relaxed);
        const size_t head = head_.load(std::memory_order_acquire);
        if (tail == head)
            return false; // empty
        out = buffer_[tail & mask_];
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    void reset() noexcept
    {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

    size_t size() const noexcept
    {
        const size_t tail = tail_.load(std::memory_order_acquire);
        const size_t head = head_.load(std::memory_order_acquire);
        return head - tail;
    }

private:
    static constexpr size_t mask_ = CapacityPow2 - 1;

    alignas(kCacheLine) std::array<T, CapacityPow2> buffer_{};
    alignas(kCacheLine) std::atomic<size_t> head_{ 0 };
    alignas(kCacheLine) std::atomic<size_t> tail_{ 0 };
};

class PsychoBus
{
public:
    using FeatureQueue = LockFreeSPSC<FeatureFrame, kFeatureQueueSize>;
    using IntentQueue  = LockFreeSPSC<Intent,       kIntentQueueSize>;

    struct Subscriber
    {
        std::atomic<bool> inUse{ false };
        PluginId pluginId{ 0 };
        FeatureQueue featureQ{};
        IntentQueue  intentQ{};
    };

    bool tryBecomeLeader(PluginId id) noexcept
    {
        bool expected = false;
        if (leaderClaimed_.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        {
            leaderId_.store(id, std::memory_order_release);
            return true;
        }
        return false;
    }

    void resignLeader(PluginId id) noexcept
    {
        if (leaderId_.load(std::memory_order_acquire) == id)
        {
            leaderId_.store(0, std::memory_order_release);
            leaderClaimed_.store(false, std::memory_order_release);
        }
    }

    bool hasLeader() const noexcept { return leaderClaimed_.load(std::memory_order_acquire); }

    int subscribe(PluginId id) noexcept
    {
        for (size_t i = 0; i < kMaxSubscribers; ++i)
        {
            bool expected = false;
            if (subscribers_[i].inUse.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
            {
                subscribers_[i].pluginId = id;
                subscribers_[i].featureQ.reset();
                subscribers_[i].intentQ.reset();
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    void unsubscribe(int slot) noexcept
    {
        if (slot < 0 || slot >= static_cast<int>(kMaxSubscribers))
            return;
        subscribers_[slot].inUse.store(false, std::memory_order_release);
        subscribers_[slot].pluginId = 0;
        subscribers_[slot].featureQ.reset();
        subscribers_[slot].intentQ.reset();
    }

    size_t publishFeatureFrame(const FeatureFrame& frame) noexcept
    {
        lastSeq_.store(frame.seq, std::memory_order_release);
        size_t pushed = 0;
        for (auto& sub : subscribers_)
            if (sub.inUse.load(std::memory_order_acquire))
                pushed += sub.featureQ.push(frame) ? 1u : 0u;
        return pushed;
    }

    size_t publishIntent(const Intent& intent) noexcept
    {
        size_t pushed = 0;
        for (auto& sub : subscribers_)
        {
            if (!sub.inUse.load(std::memory_order_acquire))
                continue;
            if (intent.plugin != 0 && intent.plugin != sub.pluginId)
                continue;
            pushed += sub.intentQ.push(intent) ? 1u : 0u;
        }
        return pushed;
    }

    bool tryPopFeature(int slot, FeatureFrame& out) noexcept
    {
        if (slot < 0 || slot >= static_cast<int>(kMaxSubscribers))
            return false;
        return subscribers_[slot].featureQ.pop(out);
    }

    bool tryPopIntent(int slot, Intent& out) noexcept
    {
        if (slot < 0 || slot >= static_cast<int>(kMaxSubscribers))
            return false;
        return subscribers_[slot].intentQ.pop(out);
    }

    bool framesFresh(uint64_t latestSeq, uint64_t maxStaleness) const noexcept
    {
        const uint64_t last = lastSeq_.load(std::memory_order_acquire);
        return latestSeq <= last && (last - latestSeq) <= maxStaleness;
    }

private:
    std::atomic<bool>     leaderClaimed_{ false };
    std::atomic<PluginId> leaderId_{ 0 };
    std::atomic<uint64_t> lastSeq_{ 0 };
    std::array<Subscriber, kMaxSubscribers> subscribers_{};
};

struct PsychoBusResource
{
    PsychoBus bus;
    JUCE_LEAK_DETECTOR(PsychoBusResource)
};

using SharedPsychoBus = juce::SharedResourcePointer<PsychoBusResource>;

class PsychoBusClient
{
public:
    ~PsychoBusClient()
    {
        if (slot_ >= 0)
            resource_->bus.unsubscribe(slot_);
    }

    bool registerClient(PluginId id)
    {
        if (slot_ >= 0)
            return true;
        slot_ = resource_->bus.subscribe(id);
        id_   = id;
        return slot_ >= 0;
    }

    int slot() const noexcept { return slot_; }
    PluginId id() const noexcept { return id_; }
    PsychoBus& bus() noexcept { return resource_->bus; }

private:
    SharedPsychoBus resource_{};
    int      slot_{ -1 };
    PluginId id_{};
};

constexpr PluginId kMorphEngineId    = fnv1a32("morphEngine");
constexpr PluginId kPitchEngineId    = fnv1a32("pitchEngine");
constexpr PluginId kSpectralEngineId = fnv1a32("spectralEngine");

} // namespace dsp_core
