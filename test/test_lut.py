import test_lut_module as lut
import numpy as np
import numpy.testing as npt
import pytest
from doomtime_rgb_to_packed_rgb444 import rgb_to_packed_rgb444

np.set_printoptions(formatter={'int': lambda x: f"{x:02x}"})


PAL = rgb_to_packed_rgb444(
    np.arange(256 * 3, dtype=np.uint8).reshape(256, 3) << 4)


def generate_homogen_input():
    for i in range(256):
        yield (
            rgb_to_packed_rgb444(
                np.array(
                    list(range(i * 3, i * 3 + 3)) * 2, dtype=np.uint8
                ).reshape(2, 3) << 4), i)


@pytest.mark.parametrize("pattern, input", generate_homogen_input())
def test_palette_homogen_input(pattern, input):
    x = np.array(pattern, dtype=np.uint8)
    expected = np.broadcast_to(x.T, (int(lut.shapes.output[0] / 3), 3)
                               ).reshape(lut.shapes.output[0])
    out = lut.lut(PAL, np.ones(lut.shapes.input, dtype=np.uint8) * input)
    npt.assert_equal(expected, out)


INHOMOGEN_OUTPUT = np.array(list(map(lambda x: int(x, base=16), """
01 20 12 34 53 45 67 86 78 9a b9 ab cd ec de f0 1f 01 23 42 34 56 75 67
89 a8 9a bc db cd ef 0e f0 12 31 23 45 64 56 78 97 89 ab ca bc de fd ef
01 20 12 34 53 45 67 86 78 9a b9 ab cd ec de f0 1f 01 23 42 34 56 75 67
89 a8 9a bc db cd ef 0e f0 12 31 23 45 64 56 78 97 89 ab ca bc de fd ef
01 20 12 34 53 45 67 86 78 9a b9 ab cd ec de f0 1f 01 23 42 34 56 75 67
89 a8 9a bc db cd ef 0e f0 12 31 23 45 64 56 78 97 89 ab ca bc de fd ef
""".split())), dtype=np.uint8)


def test_palette_inhomogen_input():
    i = np.arange(lut.shapes.input[0], dtype=np.uint8
                  ).reshape(lut.shapes.input)
    o = lut.lut(PAL, i)
    npt.assert_equal(o, INHOMOGEN_OUTPUT)
