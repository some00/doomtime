#include <gtest/gtest.h>
#include "frame_buffer.hpp"
#include <opencv2/core.hpp>

using namespace doomtime;
using namespace doomtime::client;

constexpr auto Y = frame_buffer_t::Y;
constexpr auto X = frame_buffer_t::X;

std::string fake_to_packed_rgb44(const pal_t&)
{
    return {};
}

void check_frame(const ipc::frame_t& i, const packets_t& o)
{
    cv::Mat om(cv::Size(WIDTH, HEIGHT), CV_8UC1, const_cast<uint8_t*>(o[0].data()), STEP);
    cv::Mat im(cv::Size(ipc::WIDTH, ipc::HEIGHT), CV_8UC1, const_cast<uint8_t*>(i.data()));
    for (unsigned x0 = 0; x0 < WIDTH; ++x0)
    {
        auto x1 = X + x0 * 2;
        for (unsigned y0 = 0; y0 < HEIGHT; ++y0)
        {
            auto y1 = Y + y0;
            EXPECT_EQ(im.at<uint8_t>(y1, x1), om.at<uint8_t>(x0, y0))
                << "(" << x1 << ", " << y1  << ") (" <<
                x0 << ", " << y0 << ")";
        }
    }
}

void check_pal_indices(const packets_t& o, uint8_t pal_idx)
{
    size_t i = 0;
    size_t j = 0;
    for (const auto& pkt : o)
    {
        for (int k = 0; k < COL_PER_PACKET; ++k)
            EXPECT_EQ(pkt[(k + 1) * STEP - 1], pal_idx) << "(" << i << ", " << j++ << ")";
        ++i;
    }
}

auto random_frame()
{
    ipc::frame_t f;
    std::for_each(f.begin(), f.end(), [] (auto& x) {
        x = std::rand();
    });
    return std::make_tuple(f, uint8_t(std::rand() % 14));
}

TEST(test_frame_buffer, pop_empty_timeout)
{
    frame_buffer_t fb(10, fake_to_packed_rgb44);
    ASSERT_THROW(fb.pop(), std::runtime_error);
}

TEST(test_frame_buffer, pop_one)
{
    ipc::frame_t frame;
    frame.fill(0x34);
    uint8_t pal_idx = 10;
    cv::Mat fi(cv::Size(ipc::WIDTH, ipc::HEIGHT), CV_8UC1, frame.data());
    fi.at<uint8_t>(Y, X) = 0x11;
    fi.at<uint8_t>(Y, X + (WIDTH * 2) - 2) = 0x12;
    fi.at<uint8_t>(Y + HEIGHT - 1, X) = 0x13;
    fi.at<uint8_t>(Y + HEIGHT - 1, X + (WIDTH * 2) - 2) = 0x14;

    frame_buffer_t fb(10, fake_to_packed_rgb44);
    fb.push(frame, pal_idx);
    auto [ok, pkts] = fb.pop();
    // popped one element
    ASSERT_TRUE(ok);

    // check palette
    for (const auto& pkt : pkts)
    {
        EXPECT_EQ(pkt[pkt.size() - 1], 10);
    }

    // check corners
    cv::Mat fo(cv::Size(WIDTH, HEIGHT), CV_8UC1, pkts[0].data(), 49);
    EXPECT_EQ(fo.at<uint8_t>(0, 0), 0x11);
    EXPECT_EQ(fo.at<uint8_t>(WIDTH - 1, 0), 0x12);
    EXPECT_EQ(fo.at<uint8_t>(0, HEIGHT - 1), 0x13);
    EXPECT_EQ(fo.at<uint8_t>(WIDTH - 1, HEIGHT - 1), 0x14);
    check_frame(frame, pkts);

    // check non-corners
    fo.at<uint8_t>(0, 0) = 0x34;
    fo.at<uint8_t>(WIDTH - 1, 0) = 0x34;
    fo.at<uint8_t>(0, HEIGHT - 1) = 0x34;
    fo.at<uint8_t>(WIDTH - 1, HEIGHT - 1) = 0x34;
    packets_t expected;
    for (auto& pkt : expected)
    {
        pkt.fill(0x34);
        for (int i = 0; i < COL_PER_PACKET; ++i)
            pkt[(i + 1) * STEP - 1] = 10;
    }
    for (int i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(expected[i], pkts[i]) << i;
    }
}

TEST(test_frame_buffer, pop_one_random)
{
    frame_buffer_t fb(1, fake_to_packed_rgb44);
    auto [frame, pal_idx] = random_frame();
    fb.push(frame, pal_idx);
    auto [ok, pkts] = fb.pop(1);
    ASSERT_TRUE(ok);
    check_frame(frame, pkts);
    check_pal_indices(pkts, pal_idx);
}

TEST(test_frame_buffer, pop_drop_frame)
{
    frame_buffer_t fb(10, fake_to_packed_rgb44);
    std::vector<std::tuple<ipc::frame_t, uint8_t>> frames;
    for (int i = 0; i < 2; ++i)
    {
        auto [frame, idx] = random_frame();
        frames.emplace_back(frame, idx);
        fb.push(frame, idx);
    }
    auto [ok, pkts] = fb.pop(1);
    ASSERT_TRUE(ok);
    auto [expected_frame, expected_idx] = frames.back();
    check_frame(expected_frame, pkts);
    check_pal_indices(pkts, expected_idx);
}

struct test_frame_buffer_drop_frames:
    ::testing::TestWithParam<std::tuple<size_t, size_t>>
{};

TEST_P(test_frame_buffer_drop_frames, drop_frames)
{
    frame_buffer_t fb(4, fake_to_packed_rgb44);
    auto [push, pop] = GetParam();
    std::vector<std::tuple<ipc::frame_t, uint8_t>> frames;
    for (int i = 0; i < push; ++i)
    {
        auto [frame, idx] = random_frame();
        frames.emplace_back(frame, idx);
        fb.push(frame, idx);
    }
    auto [ok, pkts] = fb.pop(pop);
    ASSERT_TRUE(ok);
    auto [ef, ei] = frames.back();
    check_frame(ef, pkts);
    check_pal_indices(pkts, ei);
}

INSTANTIATE_TEST_SUITE_P(test_frame_buffer_drop_frames, test_frame_buffer_drop_frames,
    ::testing::Values(
        std::make_tuple<size_t, size_t>(2, 1),
        std::make_tuple<size_t, size_t>(5, 1),
        std::make_tuple<size_t, size_t>(5, 2),
        std::make_tuple<size_t, size_t>(10, 9)
));


TEST(test_frame_buffer, pop_no_wait)
{
    frame_buffer_t fb(10, fake_to_packed_rgb44);
    auto [ok, pkts] = fb.pop(false);
    ASSERT_FALSE(ok);
}

TEST(test_frame_buffer, pop_no_wait_twice)
{
    frame_buffer_t fb(1, fake_to_packed_rgb44);
    auto [frame, pal_idx] = random_frame();
    fb.push(frame, pal_idx);

    {
        auto [ok, pkts] = fb.pop(false);
        ASSERT_TRUE(ok);
        check_frame(frame, pkts);
        check_pal_indices(pkts, pal_idx);
    }
    {
        auto [ok, pkts] = fb.pop(false);
        ASSERT_FALSE(ok);
    }
}
