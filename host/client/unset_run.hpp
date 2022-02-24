#pragma once
#include <atomic>

namespace doomtime {
namespace client {

struct unset_run_t
{
    std::atomic_bool& run;
    explicit unset_run_t(std::atomic_bool& run)
    : run{run}
    {
    }
    ~unset_run_t()
    {
        run.store(false);
    }
};

}
}
