#include <iostream>
#include <time.h>
#include <fstream>
#include <string>
#include <string.h>
#include <iomanip>
#include <sys/stat.h>
#include <sstream>
#include <dirent.h>

using namespace std;

#define CHUNK_SIZE 67108864

struct archiveHeader {
    char    version[5];     //Offset 0, length 5, Archiver version
    time_t  time_created;   //Offset 5, length 8, Number of files in the archive
    char    desc[256];      //Offset 13, length 256, Archive description, Header size 296?

    archiveHeader() {
        strcpy(version, "0.25");
        strcpy(desc, "THIS IS A DESCRIPTION FIELD WITH 256BYTE!");
        time(&time_created);
    }
};

struct fileEntry {
    int     id;             //Offset 0, length 4, File ID in archive
    char    file_name[128]; //Offset 4, length 128, Absolute File name
    int     file_size;      //Offset 132, length 64, File size in bytes
    int     chunk_size;     //Offset 196, length 4, Size in bytes of single chunk
    int     chunks;         //Offset 200, length 8, Number of chunks of chunk_size bytes
    int     chunk_rem;
    time_t  time_added;     //Offset 208, length 4, Time object created, Header Size 212?

    fileEntry() {
        chunk_size = CHUNK_SIZE;
        time(&time_added);
    }
};

int usage() {
    cerr << "\nUsage option, \n\n"
         << "-a,--archive\tArchive file(s)\n"
         << "-r,--restore\tRestore file(s) from archive\n"
         << "-v,--view\tView file(s) in an archive file\n"
         << "-h,--help\tShow usage information\n";

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


    cout << "\nArchiving file(s) into Archive > " << output << "\n\n";

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
                ifstream archive_read(output.c_str(), ios::binary);
                if(archive_read.fail()) {
                    cerr << "\n" << output << " : Archive read error" << endl;
                    file_read.close();
                    break;
                }
                else {
                    archive_read.read((char*)&archive_struct_read, sizeof(archive_struct_read));
                    while(archive_read.read((char*)&file_struct_read, sizeof(file_struct_read))){
                        archive_read.seekg(archive_read.tellg() + file_struct_read.file_size);

                        write_counter = file_struct_read.id + 1;
                    }
                    archive_read.close();
                }
                counter_updated = true;
            }
            else if(stat(output.c_str(), &check_buff) == -1) {
                ofstream archive_write(output.c_str(), ios::binary);

                archive_write.write((char*)&archive_struct_write, sizeof(archive_struct_write));
                archive_write.close();
            }

            ofstream archive_write(output.c_str(), ios::binary | ios::app);

            file_read.seekg(0, file_read.end);
            file_size = file_read.tellg();
            file_read.seekg(0, file_read.beg);

            int chunk_num = file_size/CHUNK_SIZE;
            int chunk_rem = file_size%CHUNK_SIZE;
            int size_written = 0;

            const size_t last_slash = file_name.find_last_of("\\/");
            if(std::string::npos != last_slash)
                file_name.erase(0, last_slash + 1);

            fileEntry file_struct_write;

            file_struct_write.file_size = file_size;
            strcpy(file_struct_write.file_name, file_name.c_str());
            file_struct_write.id = write_counter;
            file_struct_write.chunks = chunk_num;
            file_struct_write.chunk_rem = chunk_rem;

            archive_write.write((char*)&file_struct_write, sizeof(file_struct_write));

            if(chunk_num == 0 && chunk_rem != 0) {
                buffer = (char*)malloc(sizeof(char)*chunk_rem);
                file_read.read(buffer, chunk_rem);
                archive_write.write(buffer, chunk_rem);
                size_written += chunk_rem;
                free(buffer);
            }
            else {
                for(int c = 1; c <= chunk_num; c++) {
                    buffer = (char*)malloc(sizeof(char)*CHUNK_SIZE);
                    file_read.read(buffer, CHUNK_SIZE);
                    archive_write.write(buffer, CHUNK_SIZE);
                    size_written += CHUNK_SIZE;
                    if(c == chunk_num) {
                        file_read.read(buffer, chunk_rem);
                        archive_write.write(buffer, chunk_rem);
                        size_written += chunk_rem;
                    }
                    cout << setprecision(4) << (float)((size_written/(float)file_size)*(float)100) << "%" << endl;
                    free(buffer);
                }
                file_read.close();
            }

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
    stringstream    stream;

    ifstream archive_read(archive_name.c_str(), ios::binary);
    if(archive_read.fail()) {
        cerr << "\n" << archive_name << " : Archive file not found\n";
    }
    else {
        archive_read.read((char*)&archive_struct_read, sizeof(archive_struct_read));
        if(strcmp(archive_struct_read.desc, "THIS IS A DESCRIPTION FIELD WITH 256BYTE!") != 0){
            cout << "\n" << archive_name << " : Corrupted or unknown file type\n";
            return -1;
        }

        cout << "\nFiles in archive : " << archive_name << ",\n\n"
             << setw(8) << left << "#"
             << setw(30) << left << "Filename"
             << setw(15) << right << "Size"
             << setw(25) << right << "Last Modified" << endl
             << "--------------------------------------------------------------------------------\n";

        while(archive_read.read((char*)&file_struct_read, sizeof(file_struct_read))) {

            archive_read.seekg(archive_read.tellg()+file_struct_read.file_size);

            time_struct = localtime(&file_struct_read.time_added);

            stream << time_struct->tm_hour << ":"
                   << time_struct->tm_min << ":"
                   << time_struct->tm_sec << " "
                   << time_struct->tm_mday << "-"
                   << time_struct->tm_mon + 1 << "-"
                   << time_struct->tm_year + 1900;

            float file_size_readable = (float)((file_struct_read.file_size/1024)/1024)/1024;

            cout << setw(8) << left << file_struct_read.id
                 << setw(30) << left << file_struct_read.file_name
                 << setw(15) << right << file_struct_read.file_size
                 << setw(25) << right << stream.str() << endl;

            stream.str(string());
            stream.clear();
        }
    }

    return 0;
}

int restore(string archive_name) {
    archiveHeader   archive_struct_read;
    fileEntry       file_struct_read, file_struct_write;
    char            *buffer;
    int             size_written = 0;

    ifstream archive_read(archive_name.c_str(), ios::binary);

    archive_read.read((char*)&archive_struct_read, sizeof(archive_struct_read));

    while(archive_read.read((char*)&file_struct_read, sizeof(file_struct_read))) {

        ofstream file_write(file_struct_read.file_name, ios::binary);

        for(int c = 0; c <= file_struct_read.chunks; c++) {

            if(file_struct_read.chunks == 0 && size_written <= file_struct_read.chunk_rem) {
                buffer = new char[file_struct_read.chunk_rem];
                archive_read.read(buffer, file_struct_read.chunk_rem);
                file_write.write(buffer, file_struct_read.chunk_rem);
                size_written += file_struct_read.chunk_rem;
            }
            else if(file_struct_read.chunks == c+1 && size_written <= file_struct_read.chunk_rem) {
                buffer = new char[file_struct_read.chunk_rem];
                archive_read.read(buffer, file_struct_read.chunk_rem);
                file_write.write(buffer, file_struct_read.chunk_rem);
                size_written += file_struct_read.chunk_rem;
                c++;
            }
            else {
                buffer = new char[CHUNK_SIZE];
                archive_read.read(buffer, CHUNK_SIZE);
                file_write.write(buffer, CHUNK_SIZE);
                size_written += CHUNK_SIZE;
            }

            free(buffer);
            cout << (float)((float)size_written/(float)file_struct_read.file_size)*100 << "%" <<endl;
        }
        file_write.close();
    }
    archive_read.close();

    return 0;
}

int main(int argc, char *argv[]) {

    if(argc < 2) {
        cerr << "\nInvalid usage\n";
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
