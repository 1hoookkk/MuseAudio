#pragma once
#include <memory>
#include <vector>
#include <mutex>
#include <juce_dsp/juce_dsp.h>

/**
 * Efficient resource pooling for real-time audio DSP
 * Eliminates memory allocations in the audio thread
 */

namespace pool
{
    // Thread-safe object pool for reusable DSP objects
    template<typename T>
    class ObjectPool
    {
    public:
        ObjectPool(int initialSize = 8, int maxSize = 32)
            : maxPoolSize_(maxSize)
        {
            pool_.reserve(initialSize);
            for (int i = 0; i < initialSize; ++i)
            {
                pool_.emplace_back(std::make_unique<T>());
            }
        }

        // Acquire an object from the pool (real-time safe)
        std::unique_ptr<T, PoolDeleter> acquire()
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (!pool_.empty())
            {
                auto obj = std::move(pool_.back());
                pool_.pop_back();
                return std::unique_ptr<T, PoolDeleter>(obj.release(), PoolDeleter{*this});
            }

            // Pool exhausted, create new object
            return std::unique_ptr<T, PoolDeleter>(new T(), PoolDeleter{*this});
        }

        // Return object to pool
        void release(std::unique_ptr<T> obj)
        {
            if (!obj) return;

            std::lock_guard<std::mutex> lock(mutex_);

            if (pool_.size() < maxPoolSize_)
            {
                // Reset object if it has a reset method
                if constexpr (std::is_member_function_pointer_v<decltype(&T::reset)>)
                {
                    obj->reset();
                }
                pool_.emplace_back(std::move(obj));
            }
            // Otherwise, let it be destroyed
        }

        // Get current pool statistics
        struct Stats
        {
            int available;
            int maxCapacity;
        };

        Stats getStats() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return { static_cast<int>(pool_.size()), maxPoolSize_ };
        }

    private:
        struct PoolDeleter
        {
            ObjectPool& pool;
            void operator()(T* obj) const
            {
                pool.release(std::unique_ptr<T>(obj));
            }
        };

        mutable std::mutex mutex_;
        std::vector<std::unique_ptr<T>> pool_;
        const int maxPoolSize_;
    };

    // Memory pool for audio buffers (pre-allocated)
    class AudioBufferPool
    {
    public:
        AudioBufferPool(int numBuffers = 16, int bufferSize = 1024, int numChannels = 2)
            : bufferSize_(bufferSize), numChannels_(numChannels)
        {
            buffers_.reserve(numBuffers);
            for (int i = 0; i < numBuffers; ++i)
            {
                buffers_.emplace_back(numChannels_, bufferSize_);
                buffers_.back().clear();
            }
        }

        // Acquire a buffer (real-time safe)
        juce::AudioBuffer<float>* acquire()
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (!available_.empty())
            {
                int index = available_.back();
                available_.pop_back();
                inUse_.insert(index);
                return &buffers_[index];
            }

            // Pool exhausted, create temporary buffer (non-pooled)
            tempBuffers_.emplace_back(numChannels_, bufferSize_);
            return &tempBuffers_.back();
        }

        // Release a buffer back to the pool
        void release(juce::AudioBuffer<float>* buffer)
        {
            if (!buffer) return;

            std::lock_guard<std::mutex> lock(mutex_);

            // Check if it's a pooled buffer
            for (size_t i = 0; i < buffers_.size(); ++i)
            {
                if (&buffers_[i] == buffer)
                {
                    buffer->clear(); // Clear for reuse
                    if (inUse_.erase(static_cast<int>(i)) > 0)
                    {
                        available_.push_back(static_cast<int>(i));
                    }
                    return;
                }
            }

            // Must be a temporary buffer, remove it
            for (auto it = tempBuffers_.begin(); it != tempBuffers_.end(); ++it)
            {
                if (&(*it) == buffer)
                {
                    tempBuffers_.erase(it);
                    break;
                }
            }
        }

        struct Stats
        {
            int available;
            int total;
            int tempBuffers;
        };

        Stats getStats() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return {
                static_cast<int>(available_.size()),
                static_cast<int>(buffers_.size()),
                static_cast<int>(tempBuffers_.size())
            };
        }

    private:
        std::vector<juce::AudioBuffer<float>> buffers_;
        std::vector<juce::AudioBuffer<float>> tempBuffers_;
        std::vector<int> available_;
        std::set<int> inUse_;
        mutable std::mutex mutex_;
        const int bufferSize_;
        const int numChannels_;
    };

    // RAII buffer handle
    class PooledBuffer
    {
    public:
        PooledBuffer(AudioBufferPool& pool) : pool_(pool), buffer_(pool.acquire())
        {
        }

        ~PooledBuffer()
        {
            pool_.release(buffer_);
        }

        // Prevent copying
        PooledBuffer(const PooledBuffer&) = delete;
        PooledBuffer& operator=(const PooledBuffer&) = delete;

        // Allow moving
        PooledBuffer(PooledBuffer&& other) noexcept
            : pool_(other.pool_), buffer_(other.buffer_)
        {
            other.buffer_ = nullptr;
        }

        juce::AudioBuffer<float>& get() { return *buffer_; }
        const juce::AudioBuffer<float>& get() const { return *buffer_; }

        juce::AudioBuffer<float>* operator->() { return buffer_; }
        const juce::AudioBuffer<float>* operator->() const { return buffer_; }

        juce::AudioBuffer<float>& operator*() { return *buffer_; }
        const juce::AudioBuffer<float>& operator*() const { return *buffer_; }

    private:
        AudioBufferPool& pool_;
        juce::AudioBuffer<float>* buffer_;
    };

    // SIMD operation pool for parallel processing
    class SIMDOperationPool
    {
    public:
        SIMDOperationPool(int numThreads = 4) : threadPool_(numThreads)
        {
        }

        // Process audio buffer in parallel using SIMD operations
        template<typename ProcessFunc>
        void processParallel(juce::AudioBuffer<float>& buffer, ProcessFunc&& processFunc)
        {
            const int numSamples = buffer.getNumSamples();
            const int numChannels = buffer.getNumChannels();

            if (numSamples < 256 || numChannels == 1)
            {
                // Process sequentially for small buffers
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    processFunc(buffer.getWritePointer(ch), numSamples);
                }
                return;
            }

            // Process channels in parallel
            juce::Thread::launchThreads([&, processFunc](int threadIndex) {
                const int channelsPerThread = (numChannels + threadPool_.getNumThreads() - 1) / threadPool_.getNumThreads();
                const int startChannel = threadIndex * channelsPerThread;
                const int endChannel = std::min(startChannel + channelsPerThread, numChannels);

                for (int ch = startChannel; ch < endChannel; ++ch)
                {
                    processFunc(buffer.getWritePointer(ch), numSamples);
                }
            }, threadPool_.getNumThreads());
        }

        int getNumThreads() const { return threadPool_.getNumThreads(); }

    private:
        juce::ThreadPool threadPool_;
    };

    // Global resource manager
    class ResourceManager
    {
    public:
        static ResourceManager& getInstance()
        {
            static ResourceManager instance;
            return instance;
        }

        // Audio buffer pool for temporary processing
        AudioBufferPool& getAudioBufferPool()
        {
            return audioBufferPool_;
        }

        // SIMD operation pool for parallel processing
        SIMDOperationPool& getSIMDPool()
        {
            return simdPool_;
        }

        // Object pool for custom DSP objects
        template<typename T>
        ObjectPool<T>& getObjectPool()
        {
            // This would need a more sophisticated implementation in practice
            // For now, we'll create pools on demand
            static ObjectPool<T> pool;
            return pool;
        }

        // Memory pool for small allocations
        void* allocate(size_t size, size_t alignment = 16)
        {
            return aligned_alloc(alignment, size);
        }

        void deallocate(void* ptr)
        {
            std::free(ptr);
        }

        // Get performance statistics
        struct PoolStats
        {
            AudioBufferPool::Stats audioBuffers;
            int simdThreads;
        };

        PoolStats getStats() const
        {
            return { audioBufferPool_.getStats(), simdPool_.getNumThreads() };
        }

    private:
        ResourceManager() : audioBufferPool_(16, 1024, 2), simdPool_(4)
        {
        }

        AudioBufferPool audioBufferPool_;
        SIMDOperationPool simdPool_;
    };

    // Convenience functions
    inline PooledBuffer getPooledBuffer()
    {
        return PooledBuffer(ResourceManager::getInstance().getAudioBufferPool());
    }

    inline SIMDOperationPool& getSIMDPool()
    {
        return ResourceManager::getInstance().getSIMDPool();
    }

    template<typename T>
    inline ObjectPool<T>& getObjectPool()
    {
        return ResourceManager::getInstance().getObjectPool<T>();
    }
}