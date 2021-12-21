#pragma once

#include <cstdint>
#include <cassert>
#include <memory>
#include <fstream>

#include <json/json.h>

struct extra_info_t
{
    std::unique_ptr<char[]> img_data;
    std::uint32_t img_len;
};

class ncmdump
{
public:
    enum class failure_t : std::uint8_t { no_error = 0, invalid_ncm_format, invalid_metadata };

private:
    failure_t d_failbit;
    std::unique_ptr<std::uint8_t[]> dp_box;
    std::ifstream d_ifs;
    Json::Value d_meta;
    extra_info_t d_extra;

private:
    void prepare();

public:
    ncmdump(std::ifstream&& is) :
        d_failbit{0},
        d_ifs{std::move(is)}
    {
        assert(d_ifs.good());
        prepare();
    }

    auto failure()& noexcept { return d_failbit; }

    const auto& metadata()& noexcept { return d_meta; }

    const auto& extra_info()& noexcept { return d_extra; };

    void dump(std::ostream& os)&;
};
