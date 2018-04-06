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
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace yacrd {
namespace utils {

// type definition
using pos_t = size_t;
using interval = std::pair<pos_t, pos_t>;
using interval_vector = std::vector<interval>;

struct mapped_read {
    mapped_read(size_t len) : _len(len) {}
    pos_t length() const { return _len; }

    bool empty() const noexcept { return _intervals.empty(); }

    const interval& first_interval() const { return _intervals.front(); }

    void add_interval(pos_t start, pos_t stop) {
        if(start > stop) {
            std::swap(start, stop);
        }
        if(start > _len) {
            return;
        }
        if(stop > _len) {
            stop = _len;
        }

        _intervals.emplace_back(start, stop);
        std::push_heap(_intervals.begin(), _intervals.end(), cmp_interval);
    }

    void pop_interval() {
        std::pop_heap(_intervals.begin(), _intervals.end(), cmp_interval);
        _intervals.pop_back();
    }

protected:
    static bool cmp_interval(const interval& x, const interval& y) { return x.first > y.first; }
    std::vector<interval> _intervals;
    pos_t _len;
};

using read2mapping_type = std::unordered_map<std::string, mapped_read>;


struct tokens_iterator {
    using value_type = std::string;
    using reference = const value_type&;
    using pointer = value_type*;

    tokens_iterator() = default;
    tokens_iterator(const std::string& str, char delimiter)
        : _stream(str), _delimiter(delimiter)
    { ++(*this); }

    //FIXME: istringstreams are not copyable. Either use a proxy or a saner tokenize algorithm.
//    tokens_iterator(const tokens_iterator&) = default;
//    tokens_iterator& operator=(const tokens_iterator&) = default;
    tokens_iterator(tokens_iterator&&) = default;
    tokens_iterator& operator=(tokens_iterator&&) = default;
    ~tokens_iterator() = default;

    tokens_iterator& operator++() {
        std::getline(_stream, _token, _delimiter);
        return *this;
    }

    reference operator*() const { return _token; }
    pointer operator->() { return &_token; }

    friend inline
    bool operator==(const tokens_iterator& x, const tokens_iterator& y)
    { return x._stream.rdstate() == y._stream.rdstate(); }

    friend inline
    bool operator!=(const tokens_iterator& x, const tokens_iterator& y)
    { return !(x == y); }

public:
    std::istringstream _stream;
    std::string _token;
    char _delimiter{};
};

template< typename T >
inline T absdiff( const T& lhs, const T& rhs ) {
  return lhs>rhs ? lhs-rhs : rhs-lhs;
}

} // namespace utils
} // namespace yacrd

#endif // UTILS_HPP
