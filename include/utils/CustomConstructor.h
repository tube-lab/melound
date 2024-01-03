// Created by Tube Lab. Part of the meloun project.
#pragma once

namespace ml::utils
{
    class CustomConstructor
    {
    public:
        CustomConstructor(const CustomConstructor&) = delete;
        CustomConstructor(CustomConstructor&&) = delete;
        virtual ~CustomConstructor() = default;

        auto operator=(const CustomConstructor&) noexcept -> CustomConstructor = delete;

    protected:
        CustomConstructor() = default;
    };
}