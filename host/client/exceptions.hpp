#pragma once
#include <stdexcept>

namespace doomtime {
namespace client {

struct pending_read_error : std::runtime_error
{
    using std::runtime_error::runtime_error;
};
struct pending_timer_error : std::runtime_error
{
    using std::runtime_error::runtime_error;
};
struct frame_write_error : std::runtime_error
{
    using std::runtime_error::runtime_error;
};
struct frame_timer_error : std::runtime_error
{
    using std::runtime_error::runtime_error;
};
struct frame_timeout_error : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

}
}
