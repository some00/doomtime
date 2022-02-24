import pytest
import doomtime_producer as prod
import doomtime_client as client
import socket
from concurrent import futures
from unittest.mock import MagicMock
import numpy as np
import time
import struct


ARGS = dict(
    sp=1.0,
    kp=-108.0,
    ki=-27.0,
    kd=-108.0,
    initial_fps=35.0,
    initial_stack_size=140,
    use_pid=True,
)
ZERO_FRAME = np.zeros((200, 320), dtype=np.uint8)
HIGH_PENDING = struct.pack("I", 9999)


@pytest.fixture
def socketpair():
    return socket.socketpair(socket.AF_UNIX, socket.SOCK_STREAM)


@pytest.fixture
def rsock(socketpair):
    return socketpair[0]


@pytest.fixture
def wsock(socketpair):
    return socketpair[1]


@pytest.fixture
def executor():
    with futures.ThreadPoolExecutor(max_workers=1) as pool:
        yield pool


@pytest.fixture
def mock_exit():
    return MagicMock()


def test_disconnected(wsock, executor):
    with prod.Producer(lambda x: exit(x)) as producer:
        fut = executor.submit(
            client.run, fd=wsock.fileno(), **ARGS)
        with pytest.raises(SystemExit) as e:
            producer.disconnected()
        fut.result()
        assert(e.value.code == 0)


def test_palette_changed(mock_exit):
    with prod.Producer(mock_exit) as producer:
        producer.palette_changed(1)
    mock_exit.assert_called_once_with(0)


def test_palette_changed_range_error(mock_exit):
    with pytest.raises(ValueError):
        with prod.Producer(mock_exit) as producer:
            producer.palette_changed(15)


def test_producer_frame_timeout(mock_exit):
    with prod.Producer(mock_exit) as producer:
        producer.frame(ZERO_FRAME)
    assert(list(map(lambda x: x[0][0], mock_exit.call_args_list)) == [1, 0])


def test_consumer_frame_timeout(mock_exit, wsock):
    with prod.Producer(mock_exit):
        with pytest.raises(client.FrameTimeoutError) as e:
            client.run(fd=wsock.fileno(), **ARGS)
        assert("frame buffer" in str(e))


def test_consumer_disconnected(mock_exit, wsock, executor):
    with prod.Producer(mock_exit) as producer:
        fut = executor.submit(
            client.run, fd=wsock.fileno(), **ARGS)
        producer.frame(ZERO_FRAME)
        producer.disconnected()
        fut.result()


def test_client_frame_io_error(mock_exit, wsock, rsock, executor):
    with prod.Producer(mock_exit) as producer:
        fut = executor.submit(
            client.run, fd=wsock.fileno(), **ARGS)
        rsock.close()
        producer.frame(ZERO_FRAME)
        with pytest.raises((client.PendingReadError, client.FrameWriteError)):
            fut.result()


def test_client_read_frame(mock_exit, wsock, rsock, executor):
    with prod.Producer(mock_exit) as producer:
        fut = executor.submit(
            client.run, fd=wsock.fileno(), **ARGS)
        producer.frame(ZERO_FRAME)
        assert(b"\x00" * 196 == rsock.recv(196))
        producer.disconnected()
        fut.result()


def test_client_read_frame_palette_changed(mock_exit, wsock, rsock, executor):
    with prod.Producer(mock_exit) as producer:
        fut = executor.submit(
            client.run, fd=wsock.fileno(), **ARGS)
        producer.palette_changed(2)
        producer.frame(ZERO_FRAME)
        assert(4 * (b"\x00" * 48 + b"\x02") == rsock.recv(196))
        producer.disconnected()
        fut.result()


def test_pid_direction(mock_exit, wsock, rsock, executor):
    with prod.Producer(mock_exit) as producer:
        fut = executor.submit(
            client.run, fd=wsock.fileno(), **ARGS)
        producer.frame(ZERO_FRAME)

        # verify starting fps is high
        start = time.time()
        cnt = 0
        while time.time() - start < 1:
            rsock.recv(196)
            rsock.send(HIGH_PENDING)
            cnt += 1
        assert(cnt > 30)

        high_cnt = cnt
        cnt = 0
        while time.time() - start < 1:
            rsock.recv(196)
            rsock.send(HIGH_PENDING)
            cnt += 1
        assert(cnt < high_cnt)

        producer.disconnected()
        fut.result()
