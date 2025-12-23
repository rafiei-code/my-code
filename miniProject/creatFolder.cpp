#include <iostream>
#include <filesystem>
#include <string>
#include <cstdlib> 
#include <thread>
#include <chrono>

namespace fs = std::filesystem;
using namespace std;

void createFolders(const fs::path& basePath) {


    int i = 0;
    while(true) {
        i++;
        fs::path newFolder = basePath / ("Folder_" + to_string(i) + "_" + to_string(i));

        try {
            fs::create_directory(newFolder);

            string command = "xdg-open \"" + newFolder.string() + "\"";
            system(command.c_str());

            this_thread::sleep_for(chrono::milliseconds(30));

        } catch (const fs::filesystem_error& e) {
            cout << "Error: " << e.what() << endl;
            continue;
        }
        createFolders(newFolder);
    }
}
int main() {
    fs::path basePath("/home/rafiei/Desktop/test");
    string open = "xdg-open \"" + basePath.string() + "\"";
    system(open.c_str());
    createFolders(basePath);

    return 0;
}
