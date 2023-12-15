// Created by Tube Lab. Part of the meloun project.
#pragma once

#include <cmath>

// TODO: Rewrite and move code to cpp file.
template <typename Number>
class Mask
{
    Number AndMask = std::pow(256, sizeof(Number)) - 1;
    Number OrMask = 0;

public:
    Mask() noexcept = default;

    auto operator|(Number n) noexcept -> Mask<Number>&
    {
        OrMask |= n;
        return *this;
    }

    auto operator&(Number n) noexcept -> Mask<Number>&
    {
        AndMask &= n;
        return *this;
    }

    Number operator()(Number n) const noexcept
    {
        return (n | OrMask) & AndMask;
    }
};
