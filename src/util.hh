#ifndef WARC2TEXT_UTIL_HH
#define WARC2TEXT_UTIL_HH

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>

namespace util {
    void toLower(std::string& s);

    // trim consecutive spaces from left and right
    void trim(std::string& s);

    // trim consecutive spaces but respect newlines:
    void trimLines(std::string& text);
    void trimLinesCopy(const std::string& original, std::string& result);

    // detect charset using uchardet
    bool detectCharset(const std::string& text, std::string& charset);

    typedef boost::archive::iterators::base64_from_binary<
        boost::archive::iterators::transform_width<
            std::string::const_iterator,
            6,
            8
        >
    > base64_text;

    void encodeBase64(const std::string& original, std::string& base64);

    enum ErrorCode : int {
        SUCCESS = 0,
        HTML_PARSING_ERROR = 1,
        FILTERED_DOCUMENT_ERROR = 2,
        UNKNOWN_ENCODING_ERROR = 3,
        UTF8_CONVERSION_ERROR = 4
    };


    typedef std::unordered_map<std::string, std::unordered_set<std::string>> umap_attr_filters;
    typedef std::unordered_map<std::string, umap_attr_filters> umap_tag_filters;

    void readTagFilters(const std::string& filename, umap_tag_filters& filters);
}

#endif
