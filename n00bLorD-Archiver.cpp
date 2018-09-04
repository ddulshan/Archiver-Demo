#include <iostream>
#include <time.h>
#include <fstream>
#include <string>
#include <string.h>
#include <iomanip>
#include <sys/stat.h>
#include <sstream>

using namespace std;

struct archiveHeader {
    char    version[5];     //Offset 0, length 5, Archiver version
    time_t  time_created;   //Offset 5, length 8, Number of files in the archive
    char    desc[256];      //Offset 13, length 256, Archive description, Header size 296

    archiveHeader() {
        strcpy(version, "0.20");
        strcpy(desc, "THIS IS A DESCRIPTION FIELD WITH 256BYTE!");
        time(&time_created);
    }
};

struct fileEntry {
    int     id;             //Offset 0, length 4, File ID in archive
    char    file_name[128]; //Offset 4, length 128, Absolute File name
    int     file_size;      //Offset 132, length 64, File size in bytes
    int     chunk_size;     //Offset 196, length 4, Size in bytes of single chunk
    int     chunks;         //Offset 200, length 8, Number of chunks of chunk_size bytes, Header Size 208
    time_t  time_added;

    fileEntry() {
        chunk_size = 512;
        time(&time_added);
    }
};

int usage() {
    cerr << "\nUsage option, \n\n"
         << "-a,--archive\tArchive file(s)\n"
         << "-r,--restore\tRestore file(s) from archive\n"
         << "-v,--view\tView file(s) in an archive file\n"
         << "-h,--help\tShow usage information\n";

    archiveHeader head;

    return 0;
}

int archive(char *argv[], int argc) {
    char            *buffer;
    archiveHeader   archive_struct_write, archive_struct_read;
    fileEntry       file_struct_read;
    string          output = argv[argc-1]; output += ".n0b";
    string          file_name;
    int             file_size, write_counter = 1;
    bool            counter_updated = false;

    cout << "\nArchiving files into Archive > " << output << "\n\n";

    for(int i = 2; i < argc-1; i++) {
        ifstream file_read(argv[i], ios::binary);
        if(file_read.fail()) {
            cerr << argv[i] << " : File not found\n";
            continue;
        }
        else {
            struct stat check_buff;
            file_name = argv[i];
            if(stat(output.c_str(), &check_buff) == 0 && !counter_updated) {
                cout << "FILE FOUND! NOT UPDATED";
                ifstream archive_read(output.c_str(), ios::binary);
                if(archive_read.fail()) {
                    cerr << "\n" << output << " : Archive read error" << endl;
                    file_read.close();
                    break;
                }
                else {
                    archive_read.read((char*)&archive_struct_read, sizeof(archive_struct_read));
                    do{
                        archive_read.read((char*)&file_struct_read, sizeof(file_struct_read));
                        archive_read.ignore(file_struct_read.file_size);

                        write_counter = file_struct_read.id;
                    }while(!archive_read.eof());
                    archive_read.close();
                }
                counter_updated = true;
            }
            else if(stat(output.c_str(), &check_buff) == -1) {
                cout << "NO FILE! WRITE HEADER" << endl;
                ofstream archive_write(output.c_str(), ios::binary);

                archive_write.write((char*)&archive_struct_write, sizeof(archive_struct_write));
                archive_write.close();
            }
            cout << "FILE WRITE START!" << endl;
            ofstream archive_write(output.c_str(), ios::binary | ios::app);

            file_read.seekg(0, file_read.end);
            file_size = file_read.tellg();
            file_read.seekg(0, file_read.beg);

            buffer = (char*)malloc(sizeof(char)*file_size);
            file_read.read(buffer, file_size);
            file_read.close();

            const size_t last_slash = file_name.find_last_of("\\/");
            if(std::string::npos != last_slash)
                file_name.erase(0, last_slash + 1);

            fileEntry file_struct_write;

            file_struct_write.file_size = file_size;
            strcpy(file_struct_write.file_name, file_name.c_str());
            file_struct_write.chunks = 1;
            file_struct_write.id = write_counter;

            archive_write.write((char*)&file_struct_write, sizeof(file_struct_write));
            archive_write.write(buffer, file_size);

            if(archive_write.bad()) {
                cout << "\n" << output << " : Archive write error" << endl;
                archive_write.close();
                file_read.close();
                break;
            }

            write_counter++;
            counter_updated = true;
            archive_write.close();

            cout << argv[i] << " : Done\n";
        }
    }
}

int view(string archive_name) {
    char            *buffer;
    archiveHeader   archive_struct_read;
    fileEntry       file_struct_read;
    struct tm       *time_struct;
    std::stringstream    stream;

    ifstream archive_read(archive_name.c_str(), ios::binary);
    if(archive_read.fail()) {
        cerr << "\n" << archive_name << " : Archive file not found\n";
    }
    else {
        cout << "\nFiles in archive : " << archive_name << ",\n\n"
             << setw(8) << left << "#"
             << setw(40) << left << "Filename"
             << setw(10) << left << "Size"
             << setw(20) << left << "Last Modified\n--------------------------------------------------------------------------------\n";
        archive_read.read((char*)&archive_struct_read, sizeof(archive_struct_read));

        for(int c = 0; c < 3; c++) {
            archive_read.read((char*)&file_struct_read, sizeof(file_struct_read));
            buffer = (char*)malloc(sizeof(char)*file_struct_read.file_size);
            archive_read.read(buffer, file_struct_read.file_size);

            time_struct = localtime(&file_struct_read.time_added);

            stream << time_struct->tm_hour << ":"
                   << time_struct->tm_min << ":"
                   << time_struct->tm_sec << " "
                   << time_struct->tm_mday << "-"
                   << time_struct->tm_mon + 1 << "-"
                   << time_struct->tm_year + 1900;

            cout << setw(8) << left << file_struct_read.id
                 << setw(40) << left << file_struct_read.file_name
                 << setw(10) << left << file_struct_read.file_size
                 << setw(20) << left << stream.str() << endl;

            free(buffer);
            stream.str(string());
            stream.clear();
        }
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

    if(argc < 2) {
        cerr << "\nInvalid usage,\n";
        usage();
        return 0;
    }

    string archive_name = argv[argc-1];
    string option = argv[1];

    if(option == "-h" || option == "--help" || argc == 1) {
        usage();
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
        cerr << "\nInvalid usage,\n";
        usage();
    }

    return 0;
}
