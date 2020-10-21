//
// Created by lpla on 5/10/20.
//

#include "record.hh"
#include "util.hh"
#include "html.hh"
#include <boost/log/trivial.hpp>

// #include <boost/iostreams/stream.hpp>
// #include <boost/iostreams/categories.hpp>
// #include <boost/iostreams/code_converter.hpp>
#include <boost/locale.hpp>

namespace warc2text {
    std::size_t read_header(const std::string& content, std::size_t last_pos, std::unordered_map<std::string,std::string>& header) {
        std::string line;
        std::size_t header_end = content.find("\r\n\r\n", last_pos);
        std::size_t pos = 0;
        if (header_end == std::string::npos) return std::string::npos;
        pos = content.find(':', last_pos);
        while (pos < header_end){
            line = content.substr(last_pos, pos - last_pos);
            pos = content.find_first_not_of(' ', pos + 1);
            last_pos = pos;
            pos = content.find("\r\n", pos);
            // normalize header keys case
            util::toLower(line);
            header[line] = content.substr(last_pos, pos - last_pos);
            util::toLower(header[line]);
            last_pos = pos + 2;
            pos = content.find(':', last_pos);
        }
        return header_end + 4;
    }

    Record::Record(const std::string& content) {
        std::string line;
        std::size_t last_pos = 0, payload_start = 0;
        std::size_t pos = content.find("WARC/1.0\r\n");
        if (pos != 0) {
            BOOST_LOG_TRIVIAL(error) << "WARC version line not found";
            return;
        }
        last_pos = pos + 10;
        // parse WARC header
        last_pos = read_header(content, last_pos, header);

        if (last_pos == std::string::npos) {
            BOOST_LOG_TRIVIAL(error) << "Could not parse WARC header";
            return;
        }

        // get the most important stuff:
        // TODO: check for mandatory header fields
        if (header.count("warc-type") == 1)
            recordType = header["warc-type"];
        if (header.count("warc-record-id") == 1)
            uuid = header["warc-record-id"];

        if (header.count("warc-target-uri") == 1)
            url = header["warc-target-uri"];

        if (header.count("content-type") == 1)
            WARCcontentType = header["content-type"];

        payload_start = last_pos;
        if (header["warc-type"] == "response") {
            // parse HTTP header
            pos = content.find("HTTP/1.", last_pos);
            if (pos == last_pos) { // found HTTP header
                pos = content.find("\r\n", last_pos);
                payload_start = read_header(content, pos + 2, HTTPheader);
                if (payload_start == std::string::npos) {
                    // BOOST_LOG_TRIVIAL(warning) << "Response record without HTTP header";
                    payload_start = last_pos; // could not parse the header, so treat it as part of the payload
                }
            }
            // else {
            //     BOOST_LOG_TRIVIAL(warning) << "Response record without HTTP header";
            // }

            if (HTTPheader.count("content-type") == 1)
                cleanContentType(HTTPheader["content-type"]);
        }

        // convert to utf8
        // assume utf8 if unknown for now
        if (charset == "UTF-8" || charset == "") {
            payload = std::string(content, payload_start, std::string::npos);
        }
        // if not utf8, convert based on charset
        else {
            try {
                payload = boost::locale::conv::to_utf<char>(&content[payload_start], charset);
            } catch (boost::locale::conv::invalid_charset_error e) {
                BOOST_LOG_TRIVIAL(warning) << "In record " << uuid << " invalid charset " << charset;
                payload = "";
            }
        }
        util::trim(payload); //remove \r\n\r\n at the end
    }

    void Record::cleanContentType(const std::string& HTTPcontentType) {
        // we assume the format is either "A/B; charset=C" or just "A/B"
        std::size_t delim = HTTPcontentType.find(';');
        if (delim == std::string::npos)
            cleanHTTPcontentType = HTTPcontentType;
        else {
            cleanHTTPcontentType = HTTPcontentType.substr(0, delim);
            delim = HTTPcontentType.find("charset=");
            if (delim != std::string::npos) {
                // cut until next ';' or until the end otherwise
                charset = HTTPcontentType.substr(delim+8, HTTPcontentType.find(";", delim+8) - delim - 8);
                util::trim(charset);
            }
        }
        // trim just in case
        util::trim(cleanHTTPcontentType);
    }

    void Record::cleanPayload(){
        processHTML(payload, plaintext);
        unescapeEntities(plaintext, plaintext);
        util::trimLines(plaintext);
    }

    bool Record::detectLanguage(){
        return false;
    }

    const std::string& Record::getHeaderProperty(const std::string& property) const {
        std::string lc_key = property;
        util::toLower(lc_key);
        return header.at(lc_key);
    }
    bool Record::headerExists(const std::string& property) const {
        std::string lc_key = property;
        util::toLower(lc_key);
        return header.find(lc_key) != header.end();
    }

    const std::string& Record::getHTTPheaderProperty(const std::string& property) const {
        std::string lc_key = property;
        util::toLower(lc_key);
        return HTTPheader.at(lc_key);
    }

    bool Record::HTTPheaderExists(const std::string& property) const{
        std::string lc_key = property;
        util::toLower(lc_key);
        return HTTPheader.find(lc_key) != HTTPheader.end();
    }

    const std::string& Record::getPayload() const {
        return payload;
    }

    const std::string& Record::getPlainText() const {
        return plaintext;
    }

    const std::string& Record::getLanguage() const {
        return language;
    }

    const std::string& Record::getURL() const {
        return url;
    }

    const std::string& Record::getRecordType() const {
        return recordType;
    }

    const std::string& Record::getWARCcontentType() const {
        return WARCcontentType;
    }

    const std::string& Record::getHTTPcontentType() const {
        return cleanHTTPcontentType;
    }

    const std::string& Record::getCharset() const {
        return charset;
    }

} // warc2text
