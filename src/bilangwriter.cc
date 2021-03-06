#include "bilangwriter.hh"
#include "util.hh"
#include <cassert>

namespace warc2text{

    GzipWriter::GzipWriter() {
        dest = nullptr;
        compressed = 0;
        s.zalloc = nullptr;
        s.zfree = nullptr;
        s.opaque = nullptr;
        int ret = deflateInit2(&s, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY);
        assert(ret == Z_OK);
        buf = new unsigned char[BUFFER_SIZE];
    }

    GzipWriter::~GzipWriter() {
        if (dest) {
            this->compress("", 0, Z_FINISH);
            deflateEnd(&s);
            std::fclose(dest);
        }
        delete[] buf;
    }

    void GzipWriter::compress(const char *in, std::size_t size, int flush) {
        if (size == 0 && flush == Z_NO_FLUSH) return;
        s.avail_in = size;
        s.next_in = (Bytef *) in;
        s.avail_out = 0;
        s.next_out = buf;
        int ret = Z_OK;
        //std::size_t written;
        while (s.avail_out == 0) {
            s.avail_out = BUFFER_SIZE;
            s.next_out = buf;
            ret = deflate(&s, flush);
            assert(ret == Z_OK || ret == Z_STREAM_END); // Z_STREAM_END only happens if flush == Z_FINISH
            compressed = BUFFER_SIZE - s.avail_out;
            //written = std::fwrite(buf, 1, compressed, dest);
            std::fwrite(buf, 1, compressed, dest);
            // TODO error handling
            // if (written != compressed || std::ferror(dest)) {
            // }
        }
        assert(s.avail_in == 0);
    }

    void GzipWriter::open(const std::string& filename) {
        dest = std::fopen(filename.c_str(), "wb");
    }

    void GzipWriter::write(const char* text, std::size_t size) {
        this->compress(text, size, Z_NO_FLUSH);
    }

    void GzipWriter::writeLine(const char* text, std::size_t size) {
        this->compress(text, size, Z_NO_FLUSH);
        this->compress("\n", 1, Z_NO_FLUSH);
    }


    bool GzipWriter::is_open(){
        return dest != nullptr;
    }

    void BilangWriter::write(const Record& record) {
        const std::string* lang = &record.getLanguage();
        GzipWriter* url = &url_files[*lang];
        GzipWriter* text = &text_files[*lang];
        GzipWriter* mime = nullptr;
        GzipWriter* html = nullptr;
        if (output_files.count("mime") == 1) mime = &(mime_files[*lang]);
        if (output_files.count("html") == 1) html = &(html_files[*lang]);
        if (!url->is_open()) {
            // if one file does not exist, the rest shouldn't either
            std::string path = folder + "/" + *lang;
            util::createDirectories(path);
            url->open(path + "/url.gz");
            text->open(path + "/text.gz");
            if (mime != nullptr) mime->open(path + "/mime.gz");
            if (html != nullptr) html->open(path + "/html.gz");
        }

        url->writeLine(record.getURL().data(), record.getURL().size());
        std::string base64text;
        util::encodeBase64(record.getPlainText(), base64text);
        text->writeLine(base64text.data(), base64text.size());

        if (mime != nullptr)
            mime->writeLine(record.getHTTPcontentType().data(), record.getHTTPcontentType().size());
        if (html != nullptr) {
            std::string base64html;
            util::encodeBase64(record.getPayload(), base64html);
            html->writeLine(base64html.data(), base64html.size());
        }
    }
}

