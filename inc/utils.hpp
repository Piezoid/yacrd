/*
Copyright (c) 2018 Pierre Marijon <pierre.marijon@inria.fr>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef UTILS_HPP
#define UTILS_HPP

/* standard include */
#include <exception>
#include <algorithm>
#include <string>
#include <sstream>

#include <unordered_map>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>



namespace yacrd {
namespace utils {

// type definition
using name_len = std::pair<std::string, std::uint64_t>;
using interval = std::pair<std::uint64_t, std::uint64_t>;
using interval_vector = std::vector<interval>;

struct Read2MappingHash
{
    std::size_t operator()(const name_len& k) const
    {
	return std::hash<std::string>()(k.first);
    }
};

using read2mapping_type = std::unordered_map<name_len, std::vector<interval>, Read2MappingHash>;

struct save_errno {
    save_errno() { std::swap(saved_errno, errno); }
    ~save_errno() { std::swap(saved_errno, errno); }
protected:
    int saved_errno = 0;
};
#define _GETOPT
inline void throw_syserr(const char* what) {
    int errcode = errno;
    errno = 0;
    throw std::system_error(errcode, std::generic_category(), what);
}

#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
struct string_view {
    using element_type = const char;
    using iterator = const char*;

    string_view() : _beg(nullptr), _end(nullptr) {}
    string_view(const std::string& s) : string_view(s.cbegin().base(), s.cend().base()) {}
    string_view(const char* beg, const char* end) : _beg(beg), _end(end) {}

    const char* begin() const { return _beg; }
    const char* end() const { return _end; }

    size_t size() const { return _end >= _beg ? static_cast<std::uintptr_t>(_end - _beg) : 0; }
    bool empty() const { return _end <= _beg; }

    // Returns a copy when casted to string
    operator std::string() { return {_beg, _end}; }

    uint64_t toull() const
    {
        uint64_t res = 0;
        const char*  p = _beg;

        // Jump table entry
        auto add_digit = [&res, &p](unsigned pow) {
            uint64_t mul=1; // Power of 10, should be staticaly evalued (pow is constexpr)
            for(unsigned i = 0; i < pow ; i++) {
                mul *= 10;
            }

            uint8_t c = uint8_t(*p++) - '0';
            res += c*mul;
            return c > 9;
        };

        // Accept only up to 19 digit in order to avoid overflow checking
        switch (size()) {
            case 19: if(add_digit(18)) break;
            case 18: if(add_digit(17)) break;
            case 17: if(add_digit(16)) break;
            case 16: if(add_digit(15)) break;
            case 15: if(add_digit(14)) break;
            case 14: if(add_digit(13)) break;
            case 13: if(add_digit(12)) break;
            case 12: if(add_digit(11)) break;
            case 11: if(add_digit(10)) break;
            case 10: if(add_digit(9 )) break;
            case  9: if(add_digit(8 )) break;
            case  8: if(add_digit(7 )) break;
            case  7: if(add_digit(6 )) break;
            case  6: if(add_digit(5 )) break;
            case  5: if(add_digit(4 )) break;
            case  4: if(add_digit(3 )) break;
            case  3: if(add_digit(2 )) break;
            case  2: if(add_digit(1 )) break;
            case  1: if(add_digit(0 )) break;
                return res;
            default: break;
        }
        throw std::domain_error("yacrd::utils::string_view::toull: invalid numeral");
    }

    bool need(iterator&) const { return false; }

protected:
    iterator _beg;
    iterator _end;
};

inline std::ostream& operator<<(std::ostream& strm, const string_view& view) {
    strm.write(view.begin(), view.size());
    return strm;
}



template<typename InputBuffer> // A class providing read(char*, size_t)
struct tokenizer {
    using iterator = typename InputBuffer::iterator;

    tokenizer(InputBuffer& input, char sep='\n')
        : _input(input), _cur(_input.begin()), _sep(sep) {}

    string_view next() {
        iterator beg = _cur;

        iterator it = std::find(beg, _input.end(), _sep);
        if(it < _input.end()) {
            _cur = it + 1;
            if(done()) {
                _input.need(_cur);
            }
            //std::cout << "\"" << std::string(beg, it) << "\"" << std::endl;
            return { beg, it };
        }

        if(_input.need(_cur)) {
            return next();
        }

        _done = true;
        _cur = _input.end();
        return { beg, _cur };
    }

    bool done() const { return _done; }

private:
    InputBuffer& _input;
    iterator _cur;
    char _sep;
    bool _done = false;
};

template<typename Reader>
struct stream_buf {
    using iterator = const char*;
    stream_buf(Reader&& stream, size_t size=1UL<<14) : _buf(size, '\0'), _stream(std::move(stream)), _size(size) {
        iterator end = _buf.end().base();
        need(end);
    }

    bool need(iterator& rest) {
        // End of stream reached if the stream filled a buffer smaller than asked
        if(_buf.size() < _size) {
            return false;
        }

        // Move unprocessed part of the buffer to the begining
        iterator buf_end = _buf.end().base();
        size_t rest_len = buf_end - rest;
        std::copy(rest, buf_end, _buf.begin());

        size_t read_req_len = _size - rest_len;
        size_t read_len = _stream.read(_buf.begin().base() + rest_len, read_req_len);
        if(read_len != read_req_len) {
            // Shrink the string, eof() is set
            _buf.resize(rest_len + read_len);
        }

        rest = _buf.begin().base();
        return true;
    }

    iterator begin() const { return _buf.begin().base(); }
    iterator end() const { return _buf.end().base(); }

private:
    std::string _buf;
    Reader _stream;
    const size_t _size;
};

struct file_stream {
    file_stream() : _fd(-1) {}
    file_stream(const std::string& path) {
        save_errno saved_errno;
        _fd = ::open(path.c_str(), O_RDONLY);
        if(_fd < 0) {
            throw_syserr("open");
        }
    }

    ~file_stream() {
        if(_fd >= 0) {
            ::close(_fd);
        }
        _fd = -1;
    }

    size_t read(char* buf, size_t count) {
        save_errno saved_errno;
        size_t i = 0;
        while(i < count) {
            ssize_t got = ::read(_fd, buf, size_t(count-i));
            if(got <= 0) {
                if(got == 0) {
                    return i;
                }
                if(errno != EINTR) {
                    throw_syserr("read");
                }
                errno = 0;
                continue;
            }
            buf += size_t(got);
            i += size_t(got);
        }
        return count;
    }

    file_stream(const file_stream&) = delete;
    file_stream& operator=(const file_stream&) = delete;
    file_stream(file_stream&& from) : file_stream() { std::swap(_fd, from._fd); }
    file_stream& operator=(file_stream&& from) {
        this->~file_stream();
        std::swap(_fd, from._fd);
        return *this;
    }

protected:
    int _fd;
};


using file_buf = stream_buf<file_stream>;
using file_tokenizer = tokenizer<file_buf>;



template< typename T >
inline T absdiff( const T& lhs, const T& rhs ) {
  return lhs>rhs ? lhs-rhs : rhs-lhs;
}

} // namespace utils
} // namespace yacrd

#endif // UTILS_HPP
