#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) { 
    _receiver.segment_received(seg);
    _time_since_last_segment_received = 0;

    // check rst
    if(seg.header().rst) {
        _rst_received = true;
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
    }

    // check ack, send to sender
    if(seg.header().ack && _sender.syn_sent()) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }

    // reply
    if(seg.length_in_sequence_space() > 0) {
        _sender.fill_window();
        real_send();
    }
    check_finish();
 }

bool TCPConnection::active() const { return !_clean_shutdown && !_unclean_shutdown && !_rst_received; }

size_t TCPConnection::write(const string &data) {
    if(data.size() <= 0) return 0;
    size_t written_size = _sender.stream_in().write(data);
    return written_size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    _sender.tick(ms_since_last_tick);
    _time_since_last_segment_received += ms_since_last_tick;
    check_finish();
    return;
 }

void TCPConnection::end_input_stream() {}

void TCPConnection::connect() {
    _sender.fill_window();
    real_send();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::real_send() {
    while(!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        _segments_out.push(seg);
        _sender.segments_out().pop();
    }
}

void TCPConnection::check_finish() {
    // check lingering
    if(_receiver.stream_out().eof() && unassembled_bytes()==0 && _sender.stream_in().eof() && bytes_in_flight() == 0) {
        if(!_linger_after_streams_finish) {
            _clean_shutdown = true;
        } else if(_time_since_last_segment_received >= 10*_cfg.rt_timeout) {
            _unclean_shutdown = true;
        }
    }
    // set lingering
    if(_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
        _linger_after_streams_finish = false;
    }
}