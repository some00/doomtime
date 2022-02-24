import numpy as np


def rgb_to_packed_rgb444(i):
    def shift():
        for idx, x in enumerate(i.reshape(np.dot(*i.shape))):
            yield (x & 0xf0) >> (0 if idx % 2 == 0 else 4)

    def join():
        g = shift()
        while True:
            try:
                x = next(g)
            except StopIteration:
                return
            try:
                yield x | next(g)
            except StopIteration:
                yield x
                return

    assert(len(i.shape) == 2)
    assert(i.shape[1] == 3)
    return np.array(list(join()), dtype=np.uint8)
