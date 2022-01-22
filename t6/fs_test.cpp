#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DISC_SIZE 8192 //temp

#define BLOCK_SIZE 256

#define FILENAME_LEN 16

#define DISCNAME_LEN 16

#define MAX_FILES_PROPORTION 8

/*
Katalog - inode który w data ma listę directory elementów
plik - dodajemy do data katalogu w którym jest tworzony directory element i tworzymy inode o tym samym id
*/

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
    u_int16_t id; //not neccessary
    u_int8_t type;
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

int create_virtual_disc(char file_name[])
{
    superblock superblock_;
    strcpy((char*)superblock_.name, file_name);
    FILE* file = fopen((char*)superblock_.name, "wb+");

    superblock_.disc_size = DISC_SIZE;
    unsigned inodes = DISC_SIZE / MAX_FILES_PROPORTION / sizeof(inode);
    u_int32_t possible_datablocks_num = (superblock_.disc_size - sizeof(superblock) - inodes * sizeof(inode)) / sizeof(datablock);
    superblock_.datablocks_num = (superblock_.disc_size - sizeof(superblock) - inodes * sizeof(inode) - possible_datablocks_num * sizeof(int)) / sizeof(datablock);
    superblock_.free_datablocks_num = superblock_.datablocks_num;
    superblock_.inodes_num = inodes;
    superblock_.free_inodes_num = inodes;
    superblock_.nodes_offset = sizeof(superblock);
    superblock_.data_map_offset = superblock_.nodes_offset + superblock_.inodes_num * sizeof(inode);
    superblock_.datablocks_offset = superblock_.data_map_offset + superblock_.datablocks_num * sizeof(int);

    fwrite(&superblock_, sizeof(superblock), 1, file);

    inode root_inode;
    root_inode.type = inode_type::TYPE_DIRECTORY;
    root_inode.id = 0;
    superblock_.free_inodes_num -= 1;

    fwrite(&root_inode, sizeof(root_inode), 1, file);

    for(int i = 0; i < inodes - 1; ++i)
    {
        inode node;
        fwrite(&node, sizeof(node), 1, file);
    }

    for(int i = 0; i < superblock_.datablocks_num; i++) {
        int j = 0;
        fwrite(&j, sizeof(int), 1, file);
    }

    for(int i = 0; i < superblock_.datablocks_num; i++)
    {
        datablock db;
        fwrite(&db, sizeof(db), 1, file);
    }

    fclose(file);
    return 1;
}

int open_disc(char filename[])
{
    FILE* file = fopen(filename, "rb+");
    superblock superblock_;
    fread(&superblock_, sizeof(superblock), 1, file);
    printf("%s\n", superblock_.name);
    printf("%d\n", superblock_.datablocks_num);
    printf("%d\n", superblock_.free_datablocks_num);
    printf("%d\n", superblock_.inodes_num);
    printf("%d\n", superblock_.free_inodes_num);
    printf("%d\n", superblock_.nodes_offset);
    printf("%d\n", superblock_.data_map_offset);
    printf("%d\n", superblock_.datablocks_offset);

    inode node_tab[superblock_.inodes_num];

    fseek(file, superblock_.nodes_offset, 0);
    for(int i = 0; i < superblock_.inodes_num; i++)
    {
        inode node;
        fread(&node, sizeof(node), 1, file);
        node_tab[i] = node;
    }

    fclose(file);
    return 1;
}

int copy_to_disc(char discname[], char filename[])
{
    FILE* file = fopen(discname, "rb+");
    superblock superblock_;
    fread(&superblock_, sizeof(superblock), 1, file);

    inode node_tab[superblock_.inodes_num];
    fseek(file, superblock_.nodes_offset, 0);
    for(int i = 0; i < superblock_.inodes_num; i++)
    {
        inode node;
        fread(&node, sizeof(node), 1, file);
        node_tab[i] = node;
    }

    int data_map[superblock_.datablocks_num];
    fseek(file, superblock_.data_map_offset, 0);
    for(int i = 0; i < superblock_.datablocks_num; i++)
    {
        int bit;
        fread(&bit, sizeof(bit), 1, file);
        data_map[i] = bit;
    }

    datablock data_tab[superblock_.datablocks_num];
    fseek(file, superblock_.datablocks_offset, 0);
    for(int i = 0; i < superblock_.datablocks_num; i++)
    {
        datablock db;
        fread(&db, sizeof(datablock), 1, file);
        data_tab[i] = db;
    }

    FILE* copy = fopen(filename, "rb");
    //check everything

    //get first free block idx
    int block_idx;
    for (int i = 0; i < superblock_.datablocks_num; i++)
    {
        if(data_map[i] == 0)
        {
            block_idx = i;
            break;
        }
    }
    //get first free node idx
    int node_idx;
    for(int i = 0; i < superblock_.inodes_num; i++)
    {
        if(node_tab[i].type == inode_type::EMPTY)
        {
            node_idx = i;
            break;
        }
    }

    //save to node
    inode node;
    node.directory = false;
    node.first_data_block = superblock_.datablocks_offset + block_idx * sizeof(datablock);
    node.id = 1; //temp
    node.used = true;
    fseek(copy, 0L, SEEK_END);
    unsigned long size = ftell(copy);
    node.size = size;
    fseek(copy, 0, 0);

    fseek(file, superblock_.nodes_offset + (node_idx * sizeof(node)), 0);
    fwrite(&node, sizeof(node), 1, file);

    datablock block;
    fread(block.data, BLOCK_SIZE, 1, copy);


    fseek(file, superblock_.datablocks_offset + (block_idx * sizeof(datablock)), 0);
    fwrite(&block, sizeof(datablock), 1, file);

    superblock_.free_datablocks_num -= 1;
    superblock_.free_inodes_num -= 1;

    printf("------------------------------------------------");
    printf("%s\n", superblock_.name);
    printf("%d\n", superblock_.datablocks_num);
    printf("%d\n", superblock_.free_datablocks_num);
    printf("%d\n", superblock_.inodes_num);
    printf("%d\n", superblock_.free_inodes_num);
    printf("%d\n", superblock_.nodes_offset);
    printf("%d\n", superblock_.data_map_offset);
    printf("%d\n", superblock_.datablocks_offset);

    fclose(file);
    fclose(copy);
    return 1;
}

int try_to_get_file(char discname[])
{
        FILE* file = fopen(discname, "rb+");
    superblock superblock_;
    fread(&superblock_, sizeof(superblock), 1, file);

    inode node_tab[superblock_.inodes_num];
    fseek(file, superblock_.nodes_offset, 0);
    for(int i = 0; i < superblock_.inodes_num; i++)
    {
        inode node;
        fread(&node, sizeof(node), 1, file);
        node_tab[i] = node;
    }

    int data_map[superblock_.datablocks_num];
    fseek(file, superblock_.data_map_offset, 0);
    for(int i = 0; i < superblock_.datablocks_num; i++)
    {
        int bit;
        fread(&bit, sizeof(bit), 1, file);
        data_map[i] = bit;
    }

    datablock data_tab[superblock_.datablocks_num];
    fseek(file, superblock_.datablocks_offset, 0);
    for(int i = 0; i < superblock_.datablocks_num; i++)
    {
        datablock db;
        fread(&db, sizeof(datablock), 1, file);
        data_tab[i] = db;
    }

    auto data = data_tab[0].data;
    printf("\n\n");
    printf((char*)data);
    printf("\n");

}


int main(int argc, char* argv[])
{
    create_virtual_disc(argv[1]);
    open_disc(argv[1]);
    copy_to_disc(argv[1], "test.txt");
    try_to_get_file(argv[1]);
    return 0;
}