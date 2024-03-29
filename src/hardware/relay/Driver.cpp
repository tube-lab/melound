// Created by Tube Lab. Part of the meloun project.
#include "hardware/relay/Driver.h"

#include <memory>
using namespace ml::relay;

auto Driver::Create(const std::string& port) noexcept -> std::shared_ptr<Driver>
{
    int fd = open (port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        return nullptr;
    }

    // Do the init ( on fail all operations will be undone )
    auto r = [&]()
    {
        // Lock the port, so no other process can access the relay
        if (flock(fd, LOCK_EX | LOCK_NB) != 0)
        {
            return false;
        }

        // Init the serial port mode ( 9600 8N1 )
        termios tty {};
        if (tcgetattr (fd, &tty) != 0)
        {
            return false;
        }

        cfsetospeed (&tty, 9600);
        cfsetispeed (&tty, 9600);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;  // 8-bit chars
        tty.c_iflag &= ~IGNBRK;                      // disable break processing
        tty.c_lflag = 0;                             // no signaling chars, no echo,
        tty.c_oflag = 0;                             // no remapping, no delays
        tty.c_cc[VMIN]  = 0;                         // read doesn't block
        tty.c_cc[VTIME] = 5;                         // 0.5 seconds read timeout
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);      // shut off xon/xoff ctrl
        tty.c_cflag |= (CLOCAL | CREAD);             // ignore modem controls,
        tty.c_cflag &= ~(PARENB | PARODD);           // shut off parity
        tty.c_cflag &= ~CSTOPB;                      // use only one stop bit
        tty.c_cflag &= ~CRTSCTS;                     // disable hardware flow control

        if (tcsetattr(fd, TCSANOW, &tty) != 0)
        {
            return false;
        }

        return true;
    }();

    if (!r)
    {
        flock(fd, LOCK_UN);
        close(fd);
        return nullptr;
    }

    // Return the created driver
    auto driver = std::make_shared<Driver>();
    driver->PortFd_ = fd;
    driver->Port_ = port;
    driver->Open(); // enforce the safe state

    return driver;
}

Driver::~Driver()
{
    Close();

    close(PortFd_);
    flock(PortFd_, LOCK_UN);
}

void Driver::Close() noexcept
{
    // set dtr line high
    UpdatePort(TIOCM_DTR, 0);
    Enabled_ = true;
}

void Driver::Open() noexcept
{
    // set dtr line low
    UpdatePort(0, TIOCM_DTR);
    Enabled_ = false;
}

auto Driver::Closed() const noexcept -> bool
{
    return Enabled_;
}

auto Driver::Path() const noexcept -> std::string_view
{
    return Port_;
}

void Driver::UpdatePort(int add, int remove) noexcept
{
    std::lock_guard _ { UpdateLock_ };
    {
        // Apply the mask to the actual port status
        int st;
        if (ioctl(PortFd_, TIOCMGET, &st) == 0)
        {
            st |= add;
            st &= ~remove;

            ioctl(PortFd_, TIOCMSET, &st);
        }
    }
}

