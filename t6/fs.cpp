#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 8192

#define FILENAME_LEN 16

#define DISCNAME_LEN 16

// 1/MAX_FILES_PROPORTION will be space for inodes
#define MAX_FILES_PROPORTION 8

enum inode_type{
    EMPTY = 0,
    TYPE_FILE = 1,
    TYPE_DIRECTORY = 2
};

struct datablock
{
    u_int64_t next;
    u_int8_t data[BLOCK_SIZE];
};

struct inode
{
    u_int64_t size;
    u_int64_t first_data_block;
    u_int8_t type;
    u_int8_t references_num;
};

struct directory_element
{
    u_int16_t inode_id;
    u_int8_t used;
    u_int8_t name[FILENAME_LEN];
};

struct superblock
{

    u_int64_t disc_size;
    u_int64_t nodes_offset;
    u_int64_t data_map_offset;
    u_int64_t datablocks_offset;

    u_int32_t datablocks_num;
    u_int32_t free_datablocks_num;
    u_int32_t inodes_num;
    u_int32_t free_inodes_num;

    u_int8_t name[DISCNAME_LEN];

};

class VirtualDisc {
private:
    FILE* file;
    char name[FILENAME_LEN];
    superblock superblock_;
    u_int64_t node_tab_len;
    u_int64_t data_map_len;
    u_int64_t datablock_tab_len;
    inode *node_tab;
    bool *data_map;
    datablock *datablock_tab;

public:
    void create(char file_name[], u_int64_t size);
    void open();
    void save();
};

void VirtualDisc::create(char file_name[], u_int64_t size)
{
    strncpy((char*)superblock_.name, file_name, FILENAME_LEN);
    strncpy((char*)name, file_name, FILENAME_LEN);
    file = fopen((char*)superblock_.name, "wb+");

    superblock_.disc_size = size;
    unsigned inodes = size / MAX_FILES_PROPORTION / sizeof(inode);
    u_int32_t possible_datablocks_num = (superblock_.disc_size - sizeof(superblock) - inodes * sizeof(inode)) / sizeof(datablock);
    superblock_.datablocks_num = (superblock_.disc_size - sizeof(superblock) - inodes * sizeof(inode) - possible_datablocks_num * sizeof(bool)) / sizeof(datablock);
    superblock_.free_datablocks_num = superblock_.datablocks_num;
    superblock_.inodes_num = inodes;
    superblock_.free_inodes_num = inodes;
    superblock_.nodes_offset = sizeof(superblock);
    superblock_.data_map_offset = superblock_.nodes_offset + superblock_.inodes_num * sizeof(inode);
    superblock_.datablocks_offset = superblock_.data_map_offset + superblock_.datablocks_num * sizeof(bool);

    fwrite(&superblock_, sizeof(superblock), 1, file);

    inode root_inode{};
    root_inode.type = inode_type::TYPE_DIRECTORY;
    root_inode.references_num = 1;
    superblock_.free_inodes_num -= 1;

    fwrite(&root_inode, sizeof(root_inode), 1, file);

    for(int i = 0; i < inodes - 1; ++i)
    {
        inode node{};
        fwrite(&node, sizeof(node), 1, file);
    }

    for(int i = 0; i < superblock_.datablocks_num; i++) {
        int j = 0;
        fwrite(&j, sizeof(bool), 1, file);
    }

    for(int i = 0; i < superblock_.datablocks_num; i++)
    {
        datablock db{};
        fwrite(&db, sizeof(db), 1, file);
    }

    fclose(file);
}

int main(int argc, char* argv[])
{
    // create_virtual_disc(argv[1], 1024*1024);
    return 0;
}