#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <string.h>

using namespace std;

int usage(string fileName) {
    cerr << "\nUsage option, \n\n"
         << "-a,--archive\tArchive file(s)\n"
         << "-r,--restore\tRestore file(s) from archive\n"
         << "-v,--view\tView file(s) in an archive file\n"
         << "-h,--help\tShow usage information\n";

         return 0;
}

int archive(string files[], string output, int file_len) {
    FILE *read_ptr, *write_ptr;
    size_t result;
    long file_size;
    char *buffer;
    char entry_text[8] = "###=###";

    for(int c = 0; c < file_len; c++) {
        read_ptr = fopen(files[c].c_str(), "rb");
        if(read_ptr == NULL) {
            cout << files[c] << " : File not found\n";
        }
        else {
            cout << files[c] << " : File found\n";

            write_ptr = fopen(output.c_str(), "ab");

            fseek(read_ptr, 0, SEEK_END);
            file_size = ftell(read_ptr);
            rewind(read_ptr);

            buffer = (char*) malloc (sizeof(char)*file_size);
            result = fread(buffer, 1, file_size, read_ptr);

            fprintf(write_ptr, "%s %s %d ", entry_text, files[c].c_str(), file_size);
            fwrite(buffer, 1, file_size, write_ptr);

            fclose(write_ptr);
        }
    }

    return 0;
}

int view(string archive_name) {
    FILE *read_ptr;
    char *buffer, *line;
    string file_name, entry;
    float file_size;
    int offset;
    read_ptr = fopen(archive_name.c_str(), "rb");

    if(read_ptr == NULL) {
        cout << "\n" << archive_name << " : Archive file not found\n";
    }
    else {
        cout << "\nFiles in archive : " << archive_name << ",\n\n"
             << "Filename\t\tSize\n-------------------------------------------\n";

        for(int c = 0; c < 5; c++){
            fscanf(read_ptr, "%s %s %f ", &entry, &file_name, &file_size);
            buffer = (char*) malloc (sizeof(char)*file_size);
            size_t result = fread(buffer, 1, file_size, read_ptr);

            float file_size_mb = (file_size/1024)/1024;
            printf("%s\t\t%.2fMB\n",file_name, file_size_mb);

            //FILE *write_ptr = fopen("test.mp4", "wb");
            //fwrite(buffer, 1, file_sizes[0], write_ptr);
            //fclose(write_ptr);
        }
        fclose(read_ptr);
    }
}

int main(int argc, char *argv[]) {
    string fileList[128];

    for(int c = 0; c <= argc; c++) {
        string s_arg = argv[c];
        if(s_arg == "-h" || s_arg == "--help" || argc == 1) {
            usage(argv[0]);
            break;
        }
        else if(s_arg == "-a" || s_arg == "--archive") {
            if(argc >= 4) {
                cout << "\nArchiving files,\n";
                string output = argv[argc-1]; output += ".n0b";
                int x = 0;
                for(int i = c+1; i < argc-1; i++) {
                    fileList[x] = argv[i];
                    cout << "> " << fileList[x] << "\n";
                    x++;
                }
                cout << "\nInto Archive > " << output << "\n\n";

                archive(fileList, output, argc-3);
            }
            else {
                cerr << "\nOption -a,--archive requires atleast 2 arguments\n"
                     << "Usage : " << argv[0] << " <-a|--archive> <file(s)> <archive>\n";
            }

            break;
        }
        else if(s_arg == "-v" || s_arg == "--view") {
            if(argc == 3) {
                string archive_name = argv[c+1];
                view(archive_name);
            }
            else {
                cerr << "\nOption -v,--view requires 1 argument\n"
                     << "Usage : " << argv[0] << " <-v|--view> <archive>\n";
            }
            break;
        }
    }

    return 0;
}
