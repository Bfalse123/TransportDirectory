#include <fstream>
#include <iostream>
#include <string_view>

#include "make_base.h"
#include "process_requests.h"

using namespace std;

string ReadFileData(const string& file_name) {
    ifstream file(file_name, ios::binary | ios::ate);
    const ifstream::pos_type end_pos = file.tellg();
    file.seekg(0, ios::beg);

    string data(end_pos, '\0');
    file.read(&data[0], end_pos);
    return data;
}

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: transport_catalog_part_o [make_base|process_requests]\n";
        return 5;
    }

    const string_view mode(argv[1]);

    if (mode == "make_base") {
        MakeBase(std::cin);

    } else if (mode == "process_requests") {
        ProcessRequests(std::cin, std::cout);
    }

    return 0;
}