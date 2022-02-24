#pragma once
#include <numeric>
// source
// http://brettbeauregard.com/blog/2011/04/improving-the-beginner%e2%80%99s-pid-reset-windup/

namespace doomtime {
namespace client {

struct pid_controller_t
{
    using fp_t = float;
    pid_controller_t(
        fp_t sp,
        fp_t kp, fp_t ki, fp_t kd,
        fp_t out_min, fp_t out_max
    )
    : sp_{sp}
    , kp_{kp}
    , ki_{ki}
    , kd_{kd}
    , out_min_{out_min}
    , out_max_{out_max}
    {
    }
    fp_t compute(fp_t input)
    {
        const fp_t error = sp_ - input;
        iterm_ += ki_ * error;
        iterm_ = std::clamp(iterm_, out_min_, out_max_);
        const fp_t dinput = input - last_input_;

        fp_t output = kp_ * error + iterm_ - kd_ * dinput;
        output = std::clamp(output, out_min_, out_max_);

        last_input_ = input;
        return output;
    }
private:
    fp_t sp_;
    fp_t iterm_{0}, last_input_{0};
    fp_t kp_, ki_, kd_;
    fp_t out_min_, out_max_;
};

}
}
