import argparse

# https://en.wikipedia.org/wiki/PID_controller#Ziegler%E2%80%93Nichols_method
parser = argparse.ArgumentParser()
parser.add_argument("ku", type=float)
parser.add_argument("tu", type=float)
args = parser.parse_args()

ku = args.ku
tu = args.tu

kp = 0.6 * ku
ki = 1.2 * ku / tu
kd = 3 * ku * tu / 40


print(dict(
    kp=kp,
    ki=ki,
    kd=kd,
))
