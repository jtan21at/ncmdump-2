#include "decrypt.h"
#include "tagger.h"

#include <cstring>
#include <string>
#include <iostream>

void print_help(char const* const argv0)
{
    std::cout << "Usage: " << fs::path{argv0}.stem() << " file_1 [file_2 ... file_n]" << std::endl;
}

std::string_view detect_mime(char* data, std::size_t len)
{
    using raw_data_type = std::vector<std::uint8_t>;
    using mime_type = std::string;
    static std::vector<std::pair<raw_data_type, mime_type>> mime_trait_table{
        {{0xff, 0xd8, 0xff}, "image/jpeg"},
        {{0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a}, "image/png"}
    };

    for(const auto& mt : mime_trait_table)
    {
        if(len < mt.first.size())
            continue;
        if(!std::memcmp(data, mt.first.data(),  mt.first.size()))
            return mt.second;
    }

    return {};
}

void extract_meta(const Json::Value& meta,
                  std::string& album,
                  std::string& title,
                  std::vector<std::string>& artists)
{
    album = meta["album"].asString();
    title = meta["musicName"].asString();
    for(const auto& ar : meta["artist"])
        artists.emplace_back(ar[0].asString());
}

int main(int argc, char const* const argv[])
{
    if(argc <= 1)
    {
        print_help(argv[0]);
        return 255;
    }

    int success_count{0}, failure_count{0};

    for(auto i = 1; i < argc; ++i) // skip argv[0]
    {
        auto msg_prefix{(std::string{"[file: "} += argv[i]) += "] "};
        fs::path in_path{argv[i]};
        if(!fs::exists(in_path))
        {
            std::cerr << msg_prefix << "not exists" << std::endl;
            ++failure_count;
            continue;
        }

        std::ifstream in{in_path.generic_string(), std::ios::binary};
        if(!in.good())
        {
            std::cerr << msg_prefix << "cannot open" << std::endl;
            ++failure_count;
            continue;
        }

        ncmdump cracker{std::move(in)};
        switch(cracker.failure())
        {
        case ncmdump::failure_t::invalid_ncm_format:
            std::cerr << msg_prefix << "invalid ncm format" << std::endl;
            ++failure_count;
            continue;
            break;

        case ncmdump::failure_t::invalid_metadata:
            std::cerr << msg_prefix << "invalid metadata" << std::endl;
            ++failure_count;
            continue;
            break;

        case ncmdump::failure_t::no_error:
            break;
        }

        fs::path out_path{in_path};
        auto out_ext{std::string{"."} += cracker.metadata()["format"].asString()};
        out_path.replace_extension(out_ext);
        if(fs::exists(out_path))
        {
            std::cerr << msg_prefix << "dumpfile already exists" << std::endl;
            ++failure_count;
            continue;
        }
        std::ofstream out(out_path.generic_string(), std::ios::binary);
        if(!out.good())
        {
            std::cerr << msg_prefix << "cannot create dumpfile" << std::endl;
            ++failure_count;
            continue;
        }

        cracker.dump(out);
        out.close();

        std::string album, title;
        std::vector<std::string> artists;
        extract_meta(cracker.metadata(), album, title, artists);
        auto cover_mime = detect_mime(cracker.extra_info().img_data.get(), cracker.extra_info().img_len);
        if(!write_tag(album, title, artists,
                      cracker.extra_info().img_data.get(), cracker.extra_info().img_len, cover_mime,
                      out_path))
        {
            std::cerr << msg_prefix << "cannot write metadata" << std::endl;
            if(!fs::remove(out_path))
                std::cerr << msg_prefix << "cannot clean temp" << std::endl;
            ++failure_count;
            continue;
        }

        std::cout << "Success: " << in_path << std::endl;
        ++success_count;
    }

    std::cout << "\nTotal: " << argc - 1 << "\tSuccess: " << success_count << "\tFailure: " << failure_count << std::endl;

    if(success_count == argc - 1)
        return 0;
    else if(failure_count == argc - 1)
    {
        std::cerr << "All failed" << std::endl;
        return 2;
    }
    else
        return 1;
}
