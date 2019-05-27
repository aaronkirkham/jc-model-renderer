#pragma once

#include <string>
#include <vector>

namespace util
{
// https://stackoverflow.com/questions/7781898/get-an-istream-from-a-char/31597630#31597630
class byte_array_buffer : public std::streambuf
{
  public:
    byte_array_buffer(const uint8_t* begin, const size_t size)
        : begin_(begin)
        , end_(begin + size)
        , current_(begin_)
    {
        assert(std::less_equal<const uint8_t*>()(begin_, end_));
    }

  private:
    int_type underflow()
    {
        if (current_ == end_)
            return traits_type::eof();

        return traits_type::to_int_type(*current_);
    }

    int_type uflow()
    {
        if (current_ == end_)
            return traits_type::eof();

        return traits_type::to_int_type(*current_++);
    }

    int_type pbackfail(int_type ch)
    {
        if (current_ == begin_ || (ch != traits_type::eof() && ch != current_[-1]))
            return traits_type::eof();

        return traits_type::to_int_type(*--current_);
    }

    std::streamsize showmanyc()
    {
        assert(std::less_equal<const uint8_t*>()(current_, end_));
        return end_ - current_;
    }

    std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way,
                           std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
    {
        if (way == std::ios_base::beg) {
            current_ = begin_ + off;
        } else if (way == std::ios_base::cur) {
            current_ += off;
        } else if (way == std::ios_base::end) {
            current_ = end_;
        }

        if (current_ < begin_ || current_ > end_)
            return -1;

        return current_ - begin_;
    }

    std::streampos seekpos(std::streampos sp, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
    {
        current_ = begin_ + sp;

        if (current_ < begin_ || current_ > end_)
            return -1;

        return current_ - begin_;
    }

    // copy ctor and assignment not implemented;
    // copying not allowed
    byte_array_buffer(const byte_array_buffer&) = delete;
    byte_array_buffer& operator=(const byte_array_buffer&) = delete;

  private:
    const uint8_t* const begin_;
    const uint8_t* const end_;
    const uint8_t*       current_;
};

static std::vector<std::string> split(const std::string& str, const std::string& delimiter)
{
    std::vector<std::string> result;
    size_t                   prev_pos = 0;
    size_t                   pos      = 0;

    while ((pos = str.find(delimiter, pos)) != std::string::npos) {
        auto part = str.substr(prev_pos, (pos - prev_pos));
        prev_pos  = ++pos;

        result.emplace_back(part);
    }

    result.emplace_back(str.substr(prev_pos, (pos - prev_pos)));
    return result;
}

static std::string join(const std::vector<std::string>& strings, const std::string& delimiter)
{
    std::string out;
    if (auto i = strings.begin(), e = strings.end(); i != e) {
        out += *i++;
        for (; i != e; ++i) {
            out.append(delimiter).append(*i);
        }
    }

    return out;
}

static void replace(std::string& data, const char* search, const char* replace)
{
    size_t pos = data.find(search);
    while (pos != std::string::npos) {
        data.replace(pos, strlen(search), replace);
        pos = data.find(search, pos + strlen(replace));
    }
}

static int32_t stoi_s(const std::string& str)
{
    if (str.length() == 0) {
        return -1;
    }

    return std::stoi(str);
}
}; // namespace util
