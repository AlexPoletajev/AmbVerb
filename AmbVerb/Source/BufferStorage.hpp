#pragma once

#include <vecLib/vDSP.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>

class FloatBuffer
{
public:
    FloatBuffer() = default;

    explicit FloatBuffer(std::size_t size)
    {
        allocate(size);
    }

    void allocate(std::size_t size)
    {
        data = std::make_unique<float[]>(size);
        bufferSize = size;
    }

    void clear() noexcept
    {
        if (data != nullptr)
            std::fill_n(data.get(), bufferSize, 0.0f);
    }

    void swapWith(FloatBuffer& other) noexcept
    {
        data.swap(other.data);
        std::swap(bufferSize, other.bufferSize);
    }

    [[nodiscard]] float* get() noexcept { return data.get(); }
    [[nodiscard]] const float* get() const noexcept { return data.get(); }
    [[nodiscard]] std::size_t size() const noexcept { return bufferSize; }

    operator float*() noexcept { return data.get(); }
    operator const float*() const noexcept { return data.get(); }
    [[nodiscard]] float* operator+(std::size_t offset) noexcept { return data.get() + offset; }
    [[nodiscard]] const float* operator+(std::size_t offset) const noexcept
    {
        return data.get() + offset;
    }
    [[nodiscard]] bool operator==(std::nullptr_t) const noexcept { return data == nullptr; }
    [[nodiscard]] bool operator!=(std::nullptr_t) const noexcept { return data != nullptr; }

    float& operator[](std::size_t index) noexcept { return data[index]; }
    const float& operator[](std::size_t index) const noexcept { return data[index]; }

private:
    std::unique_ptr<float[]> data;
    std::size_t bufferSize = 0;
};

class OwnedSplitComplex final : public DSPSplitComplex
{
public:
    OwnedSplitComplex() noexcept
    {
        realp = nullptr;
        imagp = nullptr;
    }

    void allocate(std::size_t size)
    {
        realStorage.allocate(size);
        imaginaryStorage.allocate(size);
        refreshPointers();
    }

    void swapWith(OwnedSplitComplex& other) noexcept
    {
        realStorage.swapWith(other.realStorage);
        imaginaryStorage.swapWith(other.imaginaryStorage);
        refreshPointers();
        other.refreshPointers();
    }

private:
    void refreshPointers() noexcept
    {
        realp = realStorage.get();
        imagp = imaginaryStorage.get();
    }

    FloatBuffer realStorage;
    FloatBuffer imaginaryStorage;
};
