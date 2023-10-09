#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include "curlHelper.h"

int main(int argc, char *argv[]) {
    const char* ENV_OPENAI_API_KEY = std::getenv("OPENAI_API_KEY");

    if (argc == 2 && std::string(argv[1]) == "--help") {
        std::cout << "Usage: ./sdrTrunkTranscriber <file_path>" << std::endl;
        return 0;
    }

    if (ENV_OPENAI_API_KEY == nullptr) {
        std::cerr << "OPENAI_API_KEY environment variable not set" << std::endl;
        return 1;
    }

    if (argc < 2) {
        std::cout << "Usage: ./sdrTrunkTranscriber <file_path>" << std::endl;
        return 1;
    }

    std::string file_path = argv[1];
    try {
        std::cout << curl_transcribe_audio(file_path) << std::endl;
    } catch (const std::runtime_error &e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
