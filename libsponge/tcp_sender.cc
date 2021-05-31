#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _current_rto{retx_timeout}{}

uint64_t TCPSender::bytes_in_flight() const {
     return _bytes_in_flight;
 }

void TCPSender::fill_window() {
    // not started, just send syn
    if(_next_seqno <= 0) {
        TCPSegment new_seg;
        new_seg.header().seqno = wrap(_next_seqno, _isn);
        new_seg.header().syn = true;
        send_segment(new_seg);
        return;
    }
    uint16_t window_size = _window_size;
    if (_window_size == 0) {
        window_size = 1;
    }
    // diff_size means that has sent but not _ack
    uint64_t diff_size = _next_seqno - _ack;
    if (diff_size >= static_cast<uint64_t> (window_size)){
        return;
    }
    // the real_win_size means real numebr of bytes that can send
    uint64_t real_win_size = window_size - diff_size;
    // no window to send data
    // exit loop when win_size not enough or fin_sent
    while(real_win_size > 0 && !_fin_sent && _stream.buffer_size() > 0) {
        TCPSegment new_seg;
        // no enought bytes, send them all
        if(_stream.buffer_size() < real_win_size) {

            new_seg.header().seqno = wrap(_next_seqno, _isn);
            new_seg.payload() = Buffer(std::move(_stream.read(_stream.buffer_size())));
            if(_stream.eof()) {
                new_seg.header().fin = true;
                _fin_sent = true;
            }
            send_segment(new_seg);
            real_win_size -= new_seg.length_in_sequence_space();
        } else {                
            uint64_t real_payload_size = min(real_win_size, TCPConfig::MAX_PAYLOAD_SIZE);
            new_seg.header().seqno = wrap(_next_seqno, _isn);
            new_seg.payload() = Buffer(std::move(_stream.read(real_payload_size)));
            send_segment(new_seg);
            real_win_size -= new_seg.length_in_sequence_space();
        }
        if(_timer == -1) {
            _timer = _current_rto;
        }
    }
    if(real_win_size > 0 && !_fin_sent && _stream.eof()) {
        TCPSegment new_seg;
        new_seg.header().fin = true;
        _fin_sent = true;
        new_seg.header().seqno = wrap(_next_seqno, _isn);
        send_segment(new_seg);
    }

}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    // remove outstanding seg
    uint64_t ack = unwrap(ackno, _isn, _ack);
    if(ack > _next_seqno)return;
    while(_segments_not_ack.size() > 0) {
       TCPSegment cur_seg = _segments_not_ack.front();
       uint64_t need_ack = unwrap(cur_seg.header().seqno, _isn, _ack);        
       need_ack += cur_seg.length_in_sequence_space();
       if(ack >= need_ack){
           _segments_not_ack.pop();
           _bytes_in_flight -= cur_seg.length_in_sequence_space();
       } else{
           // no more seg to pop
           break;
       }
    }
    // reset rto to init rto
    _current_rto = _initial_retransmission_timeout;
    // no outstanding seg, stop timer
    if(_segments_not_ack.size() == 0){
        _timer = -1;
    } else {
        if (ack > _ack) {
            _timer = _current_rto;
        }
    }

    _window_size = window_size;
    _ack = ack;
    _consecutive_rtm = 0;
 }

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    // timer not working, don't care
    if(_timer == -1){
        return;
    }
    if(unsigned(_timer) <= ms_since_last_tick) {
        if(_segments_not_ack.size() > 0) {
            _segments_out.push(_segments_not_ack.front());
            if(_window_size > 0) {
                _consecutive_rtm ++;
                _current_rto *= 2;
            }
            _timer = _current_rto;
        }
    } else {
        _timer -= ms_since_last_tick;
    }
    return;
 }

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_rtm; }

void TCPSender::send_empty_segment() {
    TCPSegment empty_seg;
    empty_seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(empty_seg);
    return;
}

void TCPSender::send_segment(TCPSegment new_seg) {
    _segments_out.push(new_seg);
    _segments_not_ack.push(new_seg);
    _bytes_in_flight += new_seg.length_in_sequence_space();
    _next_seqno += new_seg.length_in_sequence_space();
    if(_timer < 0) {
        _timer = _current_rto;
    }
}

bool TCPSender::syn_sent() {
    return _next_seqno != uint64_t(0);
}