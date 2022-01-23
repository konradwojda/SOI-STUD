#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

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
public:
    FILE* file;
    char name[FILENAME_LEN];
    superblock superblock_;
    u_int64_t node_tab_len;
    u_int64_t data_map_len;
    u_int64_t datablock_tab_len;
    inode *node_tab;
    bool *data_map;
    datablock *datablock_tab;

    void create(char file_name[], u_int64_t size);
    void open();
    void save();

    bool validate_filename(char* filename);
    u_int32_t find_free_node();
    u_int32_t find_free_datablock();

    directory_element* find_dir_elem(inode* dir, char* filename);

    void make_dir(char* path);

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
    superblock_.free_inodes_num = inodes - 1; // One for root dir
    superblock_.nodes_offset = sizeof(superblock);
    superblock_.data_map_offset = superblock_.nodes_offset + superblock_.inodes_num * sizeof(inode);
    superblock_.datablocks_offset = superblock_.data_map_offset + superblock_.datablocks_num * sizeof(bool);

    fwrite(&superblock_, sizeof(superblock), 1, file);

    inode root_inode{};
    root_inode.type = inode_type::TYPE_DIRECTORY;
    root_inode.references_num = 1;

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
    file = nullptr;
}

void VirtualDisc::open()
{
    // Open file
    file = fopen(this->name, "wb+");

    //Read superblock
    fread(&this->superblock_, sizeof(superblock), 1, file);

    this->node_tab_len = this->superblock_.inodes_num;
    this->data_map_len = this->superblock_.datablocks_num;
    this->datablock_tab_len = this->superblock_.datablocks_num;

    //Read inodes
    node_tab = new inode[superblock_.inodes_num];
    // fseek(file, superblock_.nodes_offset, 0);
    fread(node_tab, sizeof(inode), superblock_.inodes_num, file);

    //Read data map
    data_map = new bool[superblock_.datablocks_num];
    // fseek(file, superblock_.data_map_offset, 0);
    fread(data_map, sizeof(bool), superblock_.datablocks_num, file);

    //Read datablocks
    datablock_tab = new datablock[superblock_.datablocks_num];
    // fseek(file, superblock_.datablocks_offset, 0);
    fread(datablock_tab, sizeof(datablock), superblock_.datablocks_num, file);

}

void VirtualDisc::save()
{
    fseek(file, 0, 0);
    fwrite(&this->superblock_, sizeof(superblock), 1, file);
    fwrite(this->node_tab, sizeof(inode), this->node_tab_len, file);
    fwrite(this->data_map, sizeof(bool), this->data_map_len, file);
    fwrite(this->datablock_tab, sizeof(datablock), this->datablock_tab_len, file);
    fclose(file);
    file = nullptr;
}

bool VirtualDisc::validate_filename(char* filename)
{
    return !(strlen(filename) >= FILENAME_LEN || strpbrk(filename, "/"));
}

u_int32_t VirtualDisc::find_free_node()
{
    for(int i = 0; i < superblock_.inodes_num; i++)
    {
        if(node_tab[i].type == inode_type::EMPTY)
        {
            return i;
        }
    }
    return -1;
}

u_int32_t VirtualDisc::find_free_datablock()
{
    for (int i = 0; i < superblock_.datablocks_num; i++)
    {
        if(data_map[i] == false)
        {
            return i;
        }
    }
    return -1;

}

void VirtualDisc::make_dir(char* path)
{
    // Start at root dir
    inode* curr_path = node_tab;

    for(char* curr_dir = strtok(path, "/"); curr_dir != nullptr; curr_dir = strtok(nullptr, "/"))
    {
        if(!validate_filename(curr_dir))
            std::cout <<"Invalid path" << std::endl;

        //TODO: search for next

        u_int32_t dir_node = find_free_node();
        if(dir_node == -1)
            return;

        node_tab[dir_node].references_num = 1;
        node_tab[dir_node].type = inode_type::TYPE_DIRECTORY;

        directory_element dir_elem{0};
        dir_elem.inode_id = dir_node;
        dir_elem.used = true;
        strncpy((char*)dir_elem.name, curr_dir, FILENAME_LEN);
        dir_elem.used = true;

        if (!curr_path->first_data_block)
        {
            curr_path->first_data_block = find_free_datablock();
            if (curr_path->first_data_block == -1)
                return;
            *(directory_element*)(datablock_tab[curr_path->first_data_block].data) = dir_elem;
            data_map[curr_path->first_data_block] = true;
        }


    }

}

int main(int argc, char* argv[])
{
    VirtualDisc vd;
    vd.create("test", 1024*1024);
    vd.open();
    vd.make_dir("katalog");
    vd.save();
    // vd.open();
    // std::cout << ((directory_element*)vd.datablock_tab[vd.node_tab[0].first_data_block].data)->name;
    // vd.save();
    return 0;
}