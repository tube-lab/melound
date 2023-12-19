// Created by Tube Lab. Part of the meloun project.
#pragma once

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

namespace ml
{
    // TODO: Move into separate namespace
    /**
     * @brief A high level driver for RS232 based 5V switch.
     * The switch utilizes DTR line of RS232 port.
     * Fully exception and thread safe.
     */
    class SwitchDriver
    {
        int PortFd_;
        std::string Port_;

        std::atomic<bool> Enabled_;
        std::mutex UpdateLock_;

    public:
        /** Takes ownership over the physical switch. Enforces closed state on creation. */
        static auto Create(const std::string& port) noexcept -> std::shared_ptr<SwitchDriver>;

        /** Opens the switch and releases the comport. */
        ~SwitchDriver();

        /** Immediately turns the switch on. */
        void Close() noexcept;

        /** Immediately turns the switch off. */
        void Open() noexcept;

        /** Returns the switch state. */
        auto Closed() const noexcept -> bool;

        /** Returns the part of the switch' port. */
        auto Path() const noexcept -> std::string_view;

    private:
        SwitchDriver(int fd, std::string port) noexcept;
        SwitchDriver(const SwitchDriver&) noexcept = delete;
        SwitchDriver(SwitchDriver&&) noexcept = delete;

        void UpdatePort(int add, int remove) noexcept;
    };
}