#!/usr/bin/python3
import os
from pathlib import Path
import numpy as np
from collections import namedtuple
import sys
import tempfile


DATA_PATH = Path(__file__).parent / "background.npz"
Data = namedtuple("Data", ["frame", "palettes", "pal_idx"])
sys.path.append(Path(__file__).parent)
HEADER = """#include "display.hpp"
const uint8_t display_t::background_[48][48 * BPP / CHAR_BIT] = {"""


def main():
    from rgb_to_packed_rgb444 import rgb_to_packed_rgb444
    with np.load(DATA_PATH, allow_pickle=False) as f:
        data = Data(**f)
    frame = data.frame[10:10 + 48, 10:10 + 48]
    background = np.zeros((48, int(48 * 12 / 8)), dtype=np.uint8)
    pal = data.palettes[data.pal_idx]
    for idx, col in enumerate(frame.T):
        background[idx] = rgb_to_packed_rgb444(pal[col])

    output = Path(os.environ.get("MYNEWT_USER_SRC_DIR",
                                 tempfile.gettempdir())) / "background.cpp"
    with output.open("w") as f:
        print(HEADER, file=f)
        for col in background:
            for idx in range(0, len(col), 12):
                print(
                    ", ".join(map(lambda x: "0x{:02x}".format(x),
                                  col[idx: idx + 12])),
                    file=f,
                    end=",\n"
                )
        print('};', file=f)


if __name__ == "__main__":
    main()
