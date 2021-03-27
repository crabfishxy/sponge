#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity): _stream(), _capacity(capacity), _size(0), _ended(false), _total_written(0), _total_read(0){}

size_t ByteStream::write(const string &data) {
    size_t real_size = min(remaining_capacity(), data.size());
    for(size_t i = 0; i < real_size; i ++){
        _stream.push(data[i]);
    }
    _size += real_size;
    _total_written += real_size;
    return real_size;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string res;
    size_t real_len = min(len, _size);
    auto temp_queue = _stream;
    for(size_t i = 0; i < real_len; i ++){
        res += temp_queue.front();
        temp_queue.pop();
    }
    return res;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    size_t real_len = min(len, _size);
        for(size_t i = 0; i < real_len; i ++){
        _stream.pop();
    }
    _size -= real_len;
     _total_read += real_len;
 }

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string res = peek_output(len);
    pop_output(len);
    return res;
}

void ByteStream::end_input() {_ended = true;}

bool ByteStream::input_ended() const { return _ended; }

size_t ByteStream::buffer_size() const { return _size; }

bool ByteStream::buffer_empty() const { return _size == 0; }

bool ByteStream::eof() const { return buffer_size() == 0 && input_ended(); }

size_t ByteStream::bytes_written() const { return _total_written; }

size_t ByteStream::bytes_read() const { return _total_read; }

size_t ByteStream::remaining_capacity() const { return _capacity-_size; }
