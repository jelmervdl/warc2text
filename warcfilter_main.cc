#include <algorithm>
#include <string>
#include <fstream>
#include <thread>
#include <unordered_set>
#include <vector>
#include "util/pcqueue.hh"
#include "src/record.hh"
#include "src/warcreader.hh"

using namespace warc2text;

void ReadURLS(std::istream &in, std::unordered_set<std::string> &urls) {
	std::string url;
	while (std::getline(in, url))
		urls.emplace(url);
}

std::string StripProtocol(const std::string &url) {
	auto protocol_separator_pos = url.find("://");

	if (protocol_separator_pos == std::string::npos)
		return url;
	else
		return url.substr(protocol_separator_pos + 3);
}

int main(int argc, char *argv[]) {
	std::unordered_set<std::string> urls;

	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " URLFILE [ WARC [ WARC ... ] ]" << std::endl;
		return 127;
	}

	std::ifstream fin(argv[1]);
	ReadURLS(fin, urls);
	fin.close();

	// No files? No problem! Must be a Friday!
	if (argc < 3)
		return 0;

	int thread_count = std::min((int) std::thread::hardware_concurrency(), argc - 2);

	util::PCQueue<std::string> file_queue(thread_count * 8);
	std::mutex out_mutex;

	std::vector<std::thread> threads;
	threads.reserve(thread_count);

	while(thread_count--) {
		threads.emplace_back([&file_queue, &out_mutex, &urls]() {
			std::string filename;
			while (!file_queue.Consume(filename).empty()) {
				WARCReader reader(filename);

				std::string content;
				while (reader.getRecord(content)) {
					Record record(content);

					if (urls.find(record.getURL()) == urls.end()
						&& urls.find(StripProtocol(record.getURL())) == urls.end())
						continue;

					std::lock_guard<std::mutex> out_lock(out_mutex);
					std::cout << content;
				}
			}
		});
	}

	// Pass in the files to comb through
	for (int i = 2; i < argc; ++i)
		file_queue.Produce(argv[i]);

	// Queue up the end of work marker
	for (int i = threads.size(); i > 0; --i)
		file_queue.Produce(std::string());

	// And wait for all the workers to finish.
	for (auto &thread : threads)
		thread.join();

	// And at the end of the day, everyone can go home.
	return 0;
}
