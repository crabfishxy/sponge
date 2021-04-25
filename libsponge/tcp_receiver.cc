#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader &seg_header = seg.header();
    // first seg with syn flag
    if(!_started && seg_header.syn) {
        _isn = seg_header.seqno;
    }

    uint64_t seq_start = unwrap(seg_header.seqno, _isn, _checkpoint);
    uint64_t seq_end = seq_start + seg.length_in_sequence_space();
    uint64_t payload_start = seq_start;
    uint64_t payload_end = seq_end;
    if (!seg_header.syn)payload_start --;
    if(seg_header.fin)payload_end --;

    uint64_t window_start = 0;
    if(ackno().has_value()){
        window_start = unwrap(_ackno, _isn, _checkpoint);
    }
    uint64_t window_end = window_start + window_size();

    if(!_started && seg_header.syn) {
        _started = true;
    }
    bool is_accepted = (seq_start >= window_start && seq_start < window_end) || (payload_end >= window_start && payload_end < window_end);
    if(is_accepted) {
        string real_data = seg.payload().copy();
        _reassembler.push_substring(real_data, payload_start, seg_header.fin);
        _checkpoint = _reassembler.first_unassembled_byte();
        
    }
    uint64_t fin_number = 0;
    if(seg_header.fin && _started) _finished = true;
    if (_finished && _reassembler.unassembled_bytes() == 0) {
        fin_number = 1;
    }
    _ackno = wrap(_reassembler.first_unassembled_byte()+1+fin_number, _isn);
    // no data
    if (seg.length_in_sequence_space() == 2 && seg_header.syn && seg_header.fin){
        stream_out().end_input();
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (_started) {
        return _ackno;
    }
    return nullopt;
 }

size_t TCPReceiver::window_size() const { return stream_out().remaining_capacity(); }
