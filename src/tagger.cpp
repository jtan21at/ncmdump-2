#include "tagger.h"

#include <limits>

#include <taglib/mpegfile.h>
#include <taglib/flacfile.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/tstringlist.h>
#include <taglib/tpropertymap.h>
#include <taglib/id3v2tag.h>

bool write_tag(const std::string& album,
               const std::string& title,
               const std::vector<std::string>& artists,
               const void* img_data,
               std::size_t img_len,
               const std::string& img_mime,
               const fs::path& audio_path)
{
    if(auto audio_ext = audio_path.extension(); audio_ext == ".flac")
    {
        TagLib::FLAC::File flac_file{audio_path.generic_string().data()};
        if(!flac_file.isValid())
            return false;
        TagLib::StringList artist_list;
        for(const auto& ar : artists)
            artist_list.append({ar, TagLib::String::Type::UTF8});
        TagLib::PropertyMap map;
        map["ARTIST"] = artist_list;
        map["ALBUM"] = TagLib::String{album, TagLib::String::Type::UTF8};
        map["TITLE"] = TagLib::String{title, TagLib::String::Type::UTF8};
        flac_file.setProperties(map);

        flac_file.removePictures();
        auto front_cover = new TagLib::FLAC::Picture; // DO NOT USE SMART PTR!
        front_cover->setMimeType(img_mime);
        front_cover->setType(TagLib::FLAC::Picture::Type::FrontCover);
        if(img_len > std::numeric_limits<unsigned int>::max())
            throw std::runtime_error("[write_tag]: img_len is too big");
        front_cover->setData(TagLib::ByteVector{reinterpret_cast<char*>(const_cast<void*>(img_data)), static_cast<unsigned int>(img_len)});
        flac_file.addPicture(front_cover);
        flac_file.save();
    }
    else if(audio_ext == ".mp3")
    {
        TagLib::MPEG::File mp3_file{audio_path.generic_string().data()};
        if(!mp3_file.isValid())
            return false;
        auto tag = mp3_file.ID3v2Tag(true); // only id3v2 can have a picture frame
        tag->setAlbum(TagLib::String{album, TagLib::String::Type::UTF8});
        tag->setTitle(TagLib::String{title, TagLib::String::Type::UTF8});

        auto pic_frame = new TagLib::ID3v2::AttachedPictureFrame; // DO NOT USE SMART PTR!
        pic_frame->setMimeType(img_mime);
        pic_frame->setType(TagLib::ID3v2::AttachedPictureFrame::Type::FrontCover);
        if(img_len > std::numeric_limits<unsigned int>::max())
            throw std::runtime_error("[write_tag]: img_len is too big");
        pic_frame->setData(TagLib::ByteVector{reinterpret_cast<char*>(const_cast<void*>(img_data)), static_cast<unsigned int>(img_len)});
        tag->addFrame(pic_frame);
        mp3_file.save();
    }

    return true;
}