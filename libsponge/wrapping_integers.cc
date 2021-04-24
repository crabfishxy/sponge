#include "wrapping_integers.hh"
#include <cstdint>

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint64_t temp = isn.raw_value() + n % (uint64_t(uint64_t(UINT32_MAX))+1);
    uint32_t real_v = uint32_t(temp % (uint64_t(UINT32_MAX)+1));

    return WrappingInt32{real_v};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint64_t abs_seq = (n-isn) % (uint64_t(UINT32_MAX)+1);
    uint64_t abs_diff = checkpoint % (uint64_t(UINT32_MAX)+1);
    uint64_t lower_bound = abs_seq;
    if (checkpoint >= (uint64_t(UINT32_MAX)+1)) {
        lower_bound = checkpoint - abs_diff - (uint64_t(UINT32_MAX)+1) + abs_seq;
    }
    uint64_t middle_bound = checkpoint - abs_diff + abs_seq;
    uint64_t upper_bound = checkpoint - abs_diff +  (uint64_t(UINT32_MAX)+1) + abs_seq;
    uint64_t lower_diff = min(checkpoint - lower_bound, lower_bound - checkpoint);
    uint64_t middle_diff = min(checkpoint - middle_bound, middle_bound-checkpoint);
    uint64_t upper_diff = min(checkpoint - upper_bound, upper_bound-checkpoint);
    return lower_diff < upper_diff ? (lower_diff < middle_diff?lower_bound:middle_bound):(middle_diff < upper_diff?middle_bound:upper_bound);
}
