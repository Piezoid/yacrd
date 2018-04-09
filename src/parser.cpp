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

/* standard include */
#include <fstream>

/* project include */
#include "parser.hpp"

namespace  {

inline bool insert(yacrd::parser::alignment_span& span, yacrd::utils::read2mapping_type& read2mapping) {
    if(span.beg > span.end) {
        std::swap(span.beg, span.end);
    }

    // Inserts a new vector in the map, if the read wasn't already indexed
    auto res = read2mapping.emplace(std::make_pair(span.name, span.len), yacrd::utils::interval_vector());
    auto it = res.first; // Map iterator at the preexistent or inserted entry
    it->second.push_back(std::make_pair(span.beg, span.end));

    return res.second;
}

inline bool insert(yacrd::parser::alignment& alignment, yacrd::utils::read2mapping_type& read2mapping) {
    bool ins_first = insert(alignment.first, read2mapping);
    bool ins_second = insert(alignment.second, read2mapping);
    return ins_first || ins_second;
}

} // namespace

void yacrd::parser::file(const std::string& filename, yacrd::utils::read2mapping_type& read2mapping)
{
    auto parse_line = yacrd::parser::paf_line;
    if(filename.substr(filename.find_last_of('.') + 1) == "mhap")
    {
        parse_line = yacrd::parser::mhap_line;
    }

    yacrd::utils::file_buf file(filename);
    yacrd::utils::file_tokenizer line_tokenizer(file, '\n');
    yacrd::parser::alignment alignment;
    while(!line_tokenizer.done())
    {
        auto line = line_tokenizer.next();
        if(!line.empty()) {
            (*parse_line)(line, alignment, false);
            insert(alignment, read2mapping);
        }
    }

}

void yacrd::parser::paf_line(yacrd::utils::string_view line, yacrd::parser::alignment& out, bool only_names)
{
    yacrd::utils::tokenizer<yacrd::utils::string_view> tok(line, '\t');
    out.first.name = tok.next(); // Token 0

    if(!only_names) {
        out.first.len = tok.next().toull();
        out.first.beg = tok.next().toull();
        out.first.end = tok.next().toull();

        tok.next(); // Token 4: skip
    } else {
        for(unsigned i=0 ; i < 4 ; i++) { tok.next(); }
    }

    out.second.name = tok.next(); // Token 5

    if(!only_names) {
        out.second.len = tok.next().toull();
        out.second.beg = tok.next().toull();
        out.second.end = tok.next().toull();
    }
}

void yacrd::parser::mhap_line(yacrd::utils::string_view line, yacrd::parser::alignment& out, bool only_names)
{
    yacrd::utils::tokenizer<yacrd::utils::string_view> tokenizer(line, ' ');

    out.first.name = tokenizer.next(); // Token 0

    out.second.name = tokenizer.next();

    if(!only_names) {
        for(unsigned i=0 ; i < 3 ; i++) { tokenizer.next(); }
        out.first.beg = tokenizer.next().toull(); // Token 5
        out.first.end = tokenizer.next().toull();
        out.first.len = tokenizer.next().toull();

        tokenizer.next();

        out.second.beg = tokenizer.next().toull();
        out.second.end = tokenizer.next().toull();
        out.second.len = tokenizer.next().toull();
    }
}
