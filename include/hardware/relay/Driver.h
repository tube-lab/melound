// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "utils/CustomConstructor.h"

#include <memory>
#include <optional>
#include <atomic>

#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <climits>
#include <sys/file.h>
#include <cerrno>

namespace ml::relay
{
    /**
     * @brief A high level driver for RS232 based 5V relay.
     * The relay utilizes DTR line of RS232 port.
     * Fully exception and thread safe.
     */
    class Driver : public utils::CustomConstructor
    {
        int PortFd_;
        std::string Port_;

        std::atomic<bool> Enabled_;
        std::mutex UpdateLock_;

    public:
        /** Takes ownership over the physical relay. Enforces closed state on creation. */
        static auto Create(const std::string& port) noexcept -> std::shared_ptr<Driver>;

        /** Opens the relay and releases the comport. */
        ~Driver();

        /** Immediately turns the relay on. */
        void Close() noexcept;

        /** Immediately turns the relay off. */
        void Open() noexcept;

        /** Returns the relay state. */
        auto Closed() const noexcept -> bool;

        /** Returns the part of the relay' port. */
        auto Path() const noexcept -> std::string_view;

    private:
        void UpdatePort(int add, int remove) noexcept;
    };
}