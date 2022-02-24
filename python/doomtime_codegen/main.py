import argparse
import numpy as np
import pickle
from pathlib import Path
from doomtime_rgb_to_packed_rgb444 import rgb_to_packed_rgb444
from mako.template import Template


TEMPLATE = Template("""
<%def name="makerows(data)">
{
% for idx in range(0, len(data), 12):
    ${", ".join(map(lambda x: "0x{:02x}".format(x), data[idx: idx + 12]))},
% endfor
},
</%def>
#include "display.hpp"

const pal_t display_t::palette_[14] = {
% for pal in packed_pals:
    ${makerows(pal)}
% endfor
};

const uint8_t display_t::background_[48][48 * BPP / CHAR_BIT] = {
% for col in background:
    ${makerows(col)}
% endfor
};
""")


def main(argv):
    parser = argparse.ArgumentParser(argv)
    parser.add_argument("palette", type=Path)
    parser.add_argument("frame", type=Path)
    parser.add_argument("source", type=Path)
    args = parser.parse_args()

    pals = np.fromfile(str(args.palette), dtype=np.uint8).reshape(14, 256, 3)
    with args.frame.open("rb") as f:
        frame, pal_idx = pickle.load(f)
        frame = np.frombuffer(frame, dtype=np.uint8).reshape(200, 320)
        frame = frame[10:10 + 48, 10:10 + 48]
        pal = pals[pal_idx]
    background = np.zeros((48, int(48 * 12 / 8)), dtype=np.uint8)
    for idx, col in enumerate(frame.T):
        background[idx] = rgb_to_packed_rgb444(pal[col])
    with args.source.open("w") as f:
        f.write(TEMPLATE.render(
            packed_pals=[rgb_to_packed_rgb444(pal) for pal in pals],
            background=background
        ))
