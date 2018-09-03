#include <iostream>
#include <time.h>
#include <fstream>
#include <string>
#include <string.h>
#include <iomanip>
#include <stdlib.h>

using namespace std;

struct header {
    const float version = 1.00;
    unsigned int files_no;
};

struct entry {
    unsigned int id;
    char *file_name;
    unsigned int file_size;
};

int usage(string fileName) {
    cerr << "\nUsage option, \n\n"
         << "-a,--archive\tArchive file(s)\n"
         << "-r,--restore\tRestore file(s) from archive\n"
         << "-v,--view\tView file(s) in an archive file\n"
         << "-h,--help\tShow usage information\n";

    return 0;
}

int archive(char *argv[], int argc) {
    FILE *read_ptr, *write_ptr;
    size_t result;
    int file_size;
    char *buffer;
    char entry_text[8] = "###=###";

    string output = argv[argc-1]; output += ".n0b";

    cout << "\nArchiving files into Archive : " << output << endl;

    for(int i = 2; i < argc-1; i++) {
        read_ptr = fopen(argv[i], "rb");
        if(read_ptr == NULL) {
            cout << argv[i] << " : File not found\n";
        }
        else {
            write_ptr = fopen(output.c_str(), "ab");
            string file_name = argv[2];
            const size_t last_slash = file_name.find_last_of("\\/");
            if(std::string::npos != last_slash)
                file_name.erase(0, last_slash + 1);

            fseek(read_ptr, 0, SEEK_END);
            file_size = ftell(read_ptr);
            rewind(read_ptr);

            buffer = (char*) malloc (sizeof(char)*file_size);
            if((result = fread(buffer, 1, file_size, read_ptr)) != file_size) {
                cout << argv[i] << " : File read error\n";
                continue;
            }

            fprintf(write_ptr, "%s %s %d ", entry_text, file_name.c_str(), file_size);
            fwrite(buffer, 1, file_size, write_ptr);

            fclose(write_ptr);
            cout << argv[i] << " : Done\n";
        }
    }
}

int view(string archive_name) {
    FILE *read_ptr;
    char *buffer, *line;
    char *file_name, *entry;
    float file_size, file_size_readable;
    int offset;

    read_ptr = fopen(archive_name.c_str(), "rb");

    if(read_ptr == NULL) {
        cout << "\n" << archive_name << " : Archive file not found\n";
    }
    else {
        cout << "\nFiles in archive : " << archive_name << ",\n\n"
             << "Filename\t\tSize\n-------------------------------------------\n";

        do{
            file_name = (char*) malloc(sizeof(char)*128);
            entry = (char*) malloc(sizeof(char)*10);

            fscanf(read_ptr, "%s %s %f ", entry, file_name, &file_size);

            if(file_size > 1024*1024) {
                file_size_readable = (file_size/1024)/1024;
                cout << file_name << "\t\t" << fixed << setprecision(2) << file_size_readable << "MB\n";
            }
            else if(file_size > 1024) {
                file_size_readable = file_size/1024;
                cout << file_name << "\t\t" << fixed << setprecision(2) << file_size_readable << "KB\n";
            }
            else {
                file_size_readable = file_size;
                cout << file_name << "\t\t" << fixed << setprecision(2) << file_size_readable << "B\n";
            }

            buffer = (char*) malloc(sizeof(char)*file_size);
            size_t result = fread(buffer, 1, file_size, read_ptr);

            free(buffer);
            free(file_name);
            free(entry);
        }while(fgetc(read_ptr) != EOF);
        fclose(read_ptr);
    }

    return 0;
}

int restore(string archive_name) {
    FILE *read_ptr, *write_ptr;
    char *buffer, *file_name, *entry;
    float file_size;
    size_t result;

    file_name = (char*) malloc(sizeof(char)*128);
    entry = (char*) malloc(sizeof(char)*10);

    read_ptr = fopen(archive_name.c_str(), "rb");
    if(read_ptr == NULL) {
        cout << "\n" << archive_name << " : Archive file not found\n";
    }
    else {
        cout << "\nExtracting Archive : " << archive_name << "\n" << endl;
        do {
            fscanf(read_ptr, "%s %s %f ", entry, file_name, &file_size);

            buffer = (char*) malloc(sizeof(char)*file_size);
            result = fread(buffer, 1, file_size, read_ptr);

            write_ptr = fopen(file_name, "wb");
            fwrite(buffer, 1, file_size, write_ptr);
            fclose(write_ptr);

            cout << file_name << " : Done\n";
        }while(fgetc(read_ptr) != EOF);
        fclose(read_ptr);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    string archive_name = argv[argc-1];
    string option = argv[1];

    if(option == "-h" || option == "--help" || argc == 1) {
        usage(argv[0]);
    }
    else if(option == "-a" || option == "--archive") {
        if(argc >= 4) {
            archive(argv, argc);
        }
        else {
            cerr << "\nOption -a,--archive requires atleast 2 arguments\n"
                 << "Usage : " << argv[0] << " <-a|--archive> <file(s)> <archive>\n";
        }
    }
    else if(option == "-v" || option == "--view") {
        if(argc == 3) {
            view(archive_name);
        }
        else {
            cerr << "\nOption -v,--view requires 1 argument\n"
                 << "Usage : " << argv[0] << " <-v|--view> <archive>\n";
        }
    }
    else if(option == "-r" || option == "--restore") {
        if(argc == 3) {
            restore(archive_name);
        }
        else {
            cerr << "\nOption -r,--restore requires 1 argument\n"
                 << "Usage : " << argv[0] << " <-r|--restore> <archive>\n";
        }
    }
    else {
        cout << "\nInvalid usage,\n";
        usage(argv[0]);
    }

    return 0;
}
