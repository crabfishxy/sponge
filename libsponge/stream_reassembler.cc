#include "stream_reassembler.hh"
#include<iostream>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity),_head_index(0), _eof(false), _segments() {}

Segment::Segment(): content(), start_index(0), end_index(0) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if(data.size() <= 0 && !eof) return;
    _eof = _eof || eof;

    // update end index
    size_t remain_capacity = _capacity - _output.buffer_size();
    size_t end_index = _head_index + remain_capacity;

    // exceed cap
    if(index >= end_index){
        _eof = false;
        return;
    }
    // duplicate, already in bytestream
    if (index + data.size() <= _head_index) {
        if(_eof && empty()){
            _output.end_input();
         }
        return;
    }
    if (index + data.size() > end_index){
        _eof = false;
    }

    // only part of the data will be stored
    string real_data = data;
    size_t real_start_index = index;
    size_t real_end_index = index+data.size();
    if(index < _head_index) {
        real_data = data.substr(_head_index - index);
        real_start_index = _head_index;
    }
    if (index + data.size() >= end_index) {
        real_data = real_data.substr(0, end_index-index);
        real_end_index = end_index;
    } 
    // merge with current segs
    Segment new_seg;
    vector<Segment> new_segments;
    new_seg.end_index = real_end_index;
    new_seg.start_index = real_start_index;
    new_seg.content = data.substr(real_start_index - index, real_end_index-real_start_index);
    bool placed = false;
    for(const auto cur_seg: _segments){
        if(cur_seg.start_index > new_seg.end_index) {
            if(!placed) {
                new_segments.push_back(new_seg);
                placed = true;
            }
            new_segments.push_back(cur_seg);
        } else if(cur_seg.end_index < new_seg.start_index) {
            new_segments.push_back(cur_seg);
        } else {
            if(new_seg.end_index <= cur_seg.end_index && new_seg.start_index >= cur_seg.start_index) {
                new_seg.content = cur_seg.content;
            } else if (new_seg.end_index > cur_seg.end_index && new_seg.start_index < cur_seg.start_index){
                new_seg.content = new_seg.content;
            } else if (new_seg.start_index < cur_seg.start_index) {
                new_seg.content = new_seg.content.substr(0, cur_seg.start_index - new_seg.start_index)+cur_seg.content;
            } else if (new_seg.end_index > cur_seg.end_index) {
                new_seg.content = cur_seg.content.substr(0, new_seg.start_index - cur_seg.start_index) + new_seg.content;
            }
            new_seg.start_index = min(new_seg.start_index, cur_seg.start_index);
            new_seg.end_index = max(new_seg.end_index, cur_seg.end_index);

        }

    } 
    if (!placed) {
        new_segments.push_back(new_seg);
    }

    // add first suitable segs to bytestream
    _segments = new_segments;
    // cout << "_head_index: " << _head_index << endl;
    // for(const auto cur_seg: _segments){
    //     cout << "cur_seg content:" << cur_seg.content << " start_index: " << cur_seg.start_index << " end_index: " << cur_seg.end_index << endl;
    // } 
    if(_segments[0].start_index == _head_index) {
        _output.write(_segments[0].content);
        _head_index = _segments[0].end_index;
        _segments.erase(_segments.begin());
    }

    if(_eof && empty()){
        _output.end_input();
    }
    
}

size_t StreamReassembler::unassembled_bytes() const { 
    size_t sum = 0;
    for(const auto cur_seg: _segments){
        sum += cur_seg.content.size();
    } 
    return sum; 

}

bool StreamReassembler::empty() const { return _segments.size()==0; }
