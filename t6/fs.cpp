#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <algorithm>

#define BLOCK_SIZE 8192

#define FILENAME_LEN 16

#define DISCNAME_LEN 16

// 1/MAX_FILES_PROPORTION will be space for inodes
#define MAX_FILES_PROPORTION 8

#define DIR_ELEMS_PER_BLOCK (BLOCK_SIZE / sizeof(directory_element))

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
    char name[DISCNAME_LEN];
    superblock superblock_;
    u_int64_t node_tab_len;
    u_int64_t data_map_len;
    u_int64_t datablock_tab_len;
    inode *node_tab;
    bool *data_map;
    datablock *datablock_tab;
    void set_name(char file_name[]);

    void create(char file_name[], u_int64_t size);
    void open();
    void save();

    bool validate_filename(char* filename);
    u_int32_t find_free_node();
    u_int32_t find_free_datablock();
    void remove_datablock_from_inode(inode* node, uint64_t idx);
    std::vector<uint16_t> checked_inodes_ids;

    inode* find_in_dir(inode* dir, char* filename);
    directory_element* find_elem_in_dir(inode* dir, char* filename);
    inode* find_dir_inode(char* path);
    void add_elem_to_dir(inode* dir, char* name, u_int16_t node_id);
    void dealloc_datablocks(uint64_t first_db_id);

    void print_dir_content(inode* dir);
    uint64_t get_sum_files_in_dir(inode* dir);
    uint64_t get_sum_files_in_dir_recursive(inode* dir);
    uint64_t get_free_space();

    void make_dir(char* path);

    void copy_file_to_disc(char* filename, char* path);
    void copy_file_from_disc(char* path_to_dir, char* filename, char* name_on_pd);
    void link(char* path_to_dir, char* filename, char* path_to_link_dir, char* link_filename);
    void unlink(char* path_to_dir, char* filename);
    void remove_file(inode* file);

    void decrease_file_size(char* path_to_dir, char* filename, uint32_t size_amount);
    void increase_file_size(char* path_to_dir, char* filename, uint32_t size_amount);

};

void VirtualDisc::set_name(char file_name[])
{
    strncpy((char*)this->name, file_name, DISCNAME_LEN);
}

void VirtualDisc::create(char file_name[], u_int64_t size)
{
    strncpy((char*)superblock_.name, file_name, FILENAME_LEN);
    strncpy((char*)name, file_name, FILENAME_LEN);
    file = fopen((char*)superblock_.name, "wb+");
    if (!file)
    {
        std::perror("Cannot open file");
        exit(1);
    }

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

    size_t result = fwrite(&superblock_, sizeof(superblock), 1, file);
    if(result != 1)
    {
        std::cerr << "Error while saving to file\n";
        exit(1);
    }

    inode root_inode{};
    root_inode.first_data_block = -1;
    root_inode.type = inode_type::TYPE_DIRECTORY;
    root_inode.references_num = 1;

    result = fwrite(&root_inode, sizeof(root_inode), 1, file);
    if(result != 1)
    {
        std::cerr << "Error while saving to file\n";
        exit(1);
    }

    for(unsigned int i = 0; i < inodes - 1; ++i)
    {
        inode node{};
        node.first_data_block = -1;
        result = fwrite(&node, sizeof(node), 1, file);
        if(result != 1)
        {
            std::cerr << "Error while saving to file\n";
            exit(1);
        }
    }

    for(uint32_t i = 0; i < superblock_.datablocks_num; i++) {
        int j = 0;
        result = fwrite(&j, sizeof(bool), 1, file);
        if(result != 1)
        {
            std::cerr << "Error while saving to file\n";
            exit(1);
        }
    }

    for(uint32_t i = 0; i < superblock_.datablocks_num; i++)
    {
        datablock db{};
        db.next = -1;
        result = fwrite(&db, sizeof(db), 1, file);
        if(result != 1)
        {
            std::cerr << "Error while saving to file\n";
            exit(1);
        }
    }

    int res = fclose(file);
    if(res != 0)
    {
        std::cerr << "Error while saving to file\n";
        exit(1);
    }
    file = nullptr;
}

void VirtualDisc::open()
{
    // Open file
    file = fopen(this->name, "rb+");
    if (!file)
    {
        std::perror("Cannot open file");
        exit(1);
    }

    //Read superblock
    size_t result = fread(&this->superblock_, sizeof(superblock), 1, file);
    if(result != 1)
    {
        std::cerr << "Error while reading file\n";
        exit(1);
    }

    this->node_tab_len = this->superblock_.inodes_num;
    this->data_map_len = this->superblock_.datablocks_num;
    this->datablock_tab_len = this->superblock_.datablocks_num;

    //Read inodes
    node_tab = new inode[superblock_.inodes_num];
    result = fread(node_tab, sizeof(inode), superblock_.inodes_num, file);
    if(result != superblock_.inodes_num)
    {
        std::cerr << "Error while reading file\n";
        exit(1);
    }

    //Read data map
    data_map = new bool[superblock_.datablocks_num];
    result = fread(data_map, sizeof(bool), superblock_.datablocks_num, file);
    if(result != superblock_.datablocks_num)
    {
        std::cerr << "Error while reading file\n";
        exit(1);
    }

    //Read datablocks
    datablock_tab = new datablock[superblock_.datablocks_num];
    result = fread(datablock_tab, sizeof(datablock), superblock_.datablocks_num, file);
    if(result != superblock_.datablocks_num)
    {
        std::cerr << "Error while reading file\n";
        exit(1);
    }

}

void VirtualDisc::save()
{
    int res = fseek(file, 0, 0);
    if(res != 0)
    {
        std::cerr << "Error while seeking in file\n";
        exit(1);
    }
    size_t result = fwrite(&this->superblock_, sizeof(superblock), 1, file);
    if(result != 1)
    {
        std::cerr << "Error while saving to file\n";
        exit(1);
    }
    result = fwrite(this->node_tab, sizeof(inode), this->node_tab_len, file);
    if(result != this->node_tab_len)
    {
        std::cerr << "Error while saving to file\n";
        exit(1);
    }
    result = fwrite(this->data_map, sizeof(bool), this->data_map_len, file);
    if(result != this->data_map_len)
    {
        std::cerr << "Error while saving to file\n";
        exit(1);
    }
    result = fwrite(this->datablock_tab, sizeof(datablock), this->datablock_tab_len, file);
    if(result != this->datablock_tab_len)
    {
        std::cerr << "Error while saving to file\n";
        exit(1);
    }
    res = fclose(file);
    if(res != 0)
    {
        std::cerr << "Error while saving to file\n";
        exit(1);
    }
    file = nullptr;
}

bool VirtualDisc::validate_filename(char* filename)
{
    return !(strlen(filename) >= FILENAME_LEN || strpbrk(filename, "/"));
}

u_int32_t VirtualDisc::find_free_node()
{
    for(uint32_t i = 0; i < superblock_.inodes_num; i++)
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
    for (uint32_t i = 0; i < superblock_.datablocks_num; i++)
    {
        if(data_map[i] == false)
        {
            return i;
        }
    }
    return -1;

}

directory_element* VirtualDisc::find_elem_in_dir(inode* dir, char* filename)
{
    if(dir->type != inode_type::TYPE_DIRECTORY)
        return nullptr;

    uint64_t datablock_idx = dir->first_data_block;
    while(datablock_idx != (uint64_t)-1)
    {
        directory_element* directory_elements = (directory_element*)datablock_tab[datablock_idx].data;
        for(long unsigned int i = 0; i < DIR_ELEMS_PER_BLOCK; i++)
        {
            directory_element curr_dir_elem = directory_elements[i];
            if ((curr_dir_elem.used == true) && strcmp((char*)curr_dir_elem.name, filename) == 0)
            {
                return &directory_elements[i];
            }
        }
        datablock_idx = datablock_tab[datablock_idx].next;
    }
    return nullptr;
}

inode* VirtualDisc::find_in_dir(inode* dir, char* filename)
{
    directory_element* elem = find_elem_in_dir(dir, filename);
    if(!elem)
        return nullptr;
    return &node_tab[elem->inode_id];
}

inode* VirtualDisc::find_dir_inode(char* path)
{
    // Start at root dir
    inode* curr_path = node_tab;

    for(char* curr_dir = strtok(path, "/"); curr_dir != nullptr; curr_dir = strtok(nullptr, "/"))
    {
        if(!validate_filename(curr_dir))
            std::cout <<"Invalid path" << std::endl;

        inode* next = find_in_dir(curr_path, curr_dir);

        if(next && next->type == inode_type::TYPE_DIRECTORY)
        {
            curr_path = next;
        }
        else
        {
            std::cerr << "Not a directory\n";
            return nullptr;
        }

    }
    return curr_path;
}

void VirtualDisc::add_elem_to_dir(inode* dir, char* name, u_int16_t node_id)
{
    directory_element dir_elem{0};
    dir_elem.inode_id = node_id;
    dir_elem.used = true;
    strncpy((char*)dir_elem.name, name, FILENAME_LEN);

    if(dir->first_data_block == (uint64_t)-1)
    {
        dir->first_data_block = find_free_datablock();
        if(dir->first_data_block == (uint64_t)-1)
        {
            std::cerr << "No free datablocks\n";
            return;
        }
        data_map[dir->first_data_block] = true;
    }
    uint64_t data_block_idx = dir->first_data_block;
    uint64_t last_data_block = data_block_idx;
    bool success = false;
    while(data_block_idx != (uint64_t)-1)
    {
        directory_element* directory_elements = (directory_element*)datablock_tab[data_block_idx].data;
        for(long unsigned int i = 0; i < DIR_ELEMS_PER_BLOCK; i++)
        {
            directory_element curr_dir_elem = directory_elements[i];
            if (!curr_dir_elem.used)
            {
                directory_elements[i] = dir_elem;
                success = true;
                break;
            }
        }
        if (datablock_tab[data_block_idx].next)
        {
            last_data_block = data_block_idx;
        }
        data_block_idx = datablock_tab[data_block_idx].next;
    }
    if(!success)
    {
        datablock_tab[last_data_block].next = find_free_datablock();
        if(datablock_tab[last_data_block].next == (uint64_t)-1)
        {
            std::cerr << "No free datablocks found.\n";
            return;
        }
        uint64_t new_block_id = datablock_tab[last_data_block].next;
        if (datablock_tab[last_data_block].next == (uint64_t)-1)
            return;
        *(directory_element*)(datablock_tab[new_block_id].data) = dir_elem;
        data_map[datablock_tab[last_data_block].next] = true;
    }
}

void VirtualDisc::dealloc_datablocks(uint64_t first_db_id)
{
    // Destroys datablock linked list by setting next = -1 for every datablock
    // Also changes data map to show deallocated blocks as unused
    std::vector<uint64_t> block_ids;
    uint64_t id = first_db_id;
    while(id != (uint64_t)-1)
    {
        block_ids.push_back(id);
        id = datablock_tab[id].next;
    }
    for(auto id : block_ids)
    {
        datablock_tab[id].next = -1;
        data_map[id] = false;
    }
}

void VirtualDisc::remove_datablock_from_inode(inode* node, uint64_t idx)
{
    uint64_t curr_db_idx = node->first_data_block;
    if(curr_db_idx == idx)
    {
        node->first_data_block = -1;
        return;
    }
    while(curr_db_idx != (uint64_t)-1)
    {
        if(datablock_tab[curr_db_idx].next == idx)
        {
            datablock_tab[curr_db_idx].next = -1;
            return;
        }
        curr_db_idx = datablock_tab[curr_db_idx].next;
    }
}

void VirtualDisc::copy_file_to_disc(char* filename, char* path)
{
    inode* dir = find_dir_inode(path);
    if (!dir)
    {
        std::cerr << "Not a valid path\n";
        return;
    }
    if(find_in_dir(dir, filename))
    {
        std::cerr << "File already exists\n";
        return;
    }
    u_int32_t file_inode = find_free_node();
    if(file_inode == (uint32_t)-1)
        {
            std::cerr << "No free nodes found.\n";
            return;
        }

    node_tab[file_inode].references_num = 1;
    node_tab[file_inode].type = inode_type::TYPE_FILE;
    node_tab[file_inode].size = 0;

    add_elem_to_dir(dir, filename, file_inode);

    FILE* file_to_copy = fopen(filename, "rb+");
    if (!file)
    {
        std::perror("Cannot open file");
        exit(1);
    }

    node_tab[file_inode].first_data_block = find_free_datablock();
    if(node_tab[file_inode].first_data_block == (uint64_t)-1)
    {
        std::cerr << "No free datablocks\n";
        return;
    }

    uint64_t curr_datablock_id = node_tab[file_inode].first_data_block;

    while(curr_datablock_id != (uint64_t)-1)
    {
        uint read_amount_this_block = 0;
        uint read_amount = 0;
        while(read_amount_this_block < BLOCK_SIZE)
        {
            read_amount = fread(datablock_tab[curr_datablock_id].data, 1, BLOCK_SIZE - read_amount_this_block, file_to_copy);

            if(!read_amount && feof(file_to_copy))
            {
                break;
            }
            else if (!read_amount)
            {
                std::cerr << "Error while reading file\n";
                return;
            }

            read_amount_this_block += read_amount;
            node_tab[file_inode].size += read_amount;
        }
        if(!read_amount_this_block)
        {
            remove_datablock_from_inode(&node_tab[file_inode], curr_datablock_id);
            curr_datablock_id = -1;
        }
        else if (feof(file_to_copy))
        {
            data_map[curr_datablock_id] = true;
            datablock_tab[curr_datablock_id].next = -1;
            curr_datablock_id = -1;
        }
        else
        {
            data_map[curr_datablock_id] = true;
            datablock_tab[curr_datablock_id].next = find_free_datablock();
            if(datablock_tab[curr_datablock_id].next == (uint64_t)-1)
            {
                std::cerr << "No free datablocks\n";
                return;
            }
            curr_datablock_id = datablock_tab[curr_datablock_id].next;
        }

    }
}

void VirtualDisc::copy_file_from_disc(char* path_to_dir, char* filename, char* name_on_pd)
{
    inode* dir = find_dir_inode(path_to_dir);
    if(!dir)
    {
        std::cerr << "Invalid path to directory\n";
        return;
    }

    inode* file = find_in_dir(dir, filename);
    if(!file)
    {
        std::cerr << "Invalid filename\n";
        return;
    }

    FILE* file_on_pd = fopen(name_on_pd, "wb+");
    if (!file)
    {
        std::perror("Cannot open file");
        exit(1);
    }

    uint64_t curr_datablock_index = file->first_data_block;
    uint64_t size_to_read = file->size;
    while(curr_datablock_index != (uint64_t)-1)
    {
        uint8_t* data = datablock_tab[curr_datablock_index].data;
        size_t result = fwrite(data, (size_to_read < BLOCK_SIZE ? size_to_read : BLOCK_SIZE), 1, file_on_pd);
        if(result != 1)
        {
            std::cerr << "Error while saving to file\n";
            exit(1);
        }
        size_to_read -= (size_to_read < BLOCK_SIZE ? size_to_read : BLOCK_SIZE);
        curr_datablock_index = datablock_tab[curr_datablock_index].next;
    }
}

void VirtualDisc::make_dir(char* path)
{
    // Start at root dir
    inode* curr_path = node_tab;

    for(char* curr_dir = strtok(path, "/"); curr_dir != nullptr; curr_dir = strtok(nullptr, "/"))
    {
        if(!validate_filename(curr_dir))
            std::cout <<"Invalid path" << std::endl;

        inode* next = find_in_dir(curr_path, curr_dir);

        if(next && next->type == inode_type::TYPE_DIRECTORY)
        {
            curr_path = next;
        }
        else if (next)
        {
            std::cerr << "Not a directory\n";
            return;
        }
        else
        {
            u_int32_t dir_node = find_free_node();
            if(dir_node == (uint32_t)-1)
                {
                    std::cerr << "No free nodes found.\n";
                    return;
                }

            node_tab[dir_node].references_num = 1;
            node_tab[dir_node].type = inode_type::TYPE_DIRECTORY;

            directory_element dir_elem{0};
            dir_elem.inode_id = dir_node;
            dir_elem.used = true;
            strncpy((char*)dir_elem.name, curr_dir, FILENAME_LEN);
            dir_elem.used = true;

            if (curr_path->first_data_block == (uint64_t)-1)
            {
                curr_path->first_data_block = find_free_datablock();
                if (curr_path->first_data_block == (uint64_t)-1)
                    {
                        std::cerr << "No free datablocks found.\n";
                        return;
                    }
                *(directory_element*)(datablock_tab[curr_path->first_data_block].data) = dir_elem;
                data_map[curr_path->first_data_block] = true;
            }
            else
            {
                uint64_t data_block_idx = curr_path->first_data_block;
                uint64_t last_data_block = data_block_idx;
                bool success = false;
                while(data_block_idx != (uint64_t)-1)
                {
                    directory_element* directory_elements = (directory_element*)datablock_tab[data_block_idx].data;
                    for(long unsigned int i = 0; i < DIR_ELEMS_PER_BLOCK; i++)
                    {
                        directory_element curr_dir_elem = directory_elements[i];
                        if (!curr_dir_elem.used)
                        {
                            directory_elements[i] = dir_elem;
                            success = true;
                            break;
                        }
                    }
                    if (datablock_tab[data_block_idx].next)
                    {
                        last_data_block = data_block_idx;
                    }
                    data_block_idx = datablock_tab[data_block_idx].next;
                }
                if(!success)
                {
                    datablock_tab[last_data_block].next = find_free_datablock();
                    if(datablock_tab[last_data_block].next == (uint64_t)-1)
                    {
                        std::cerr << "No free datablocks found.\n";
                        return;
                    }
                    uint64_t new_block_id = datablock_tab[last_data_block].next;
                    if (datablock_tab[last_data_block].next == (uint64_t)-1)
                        return;
                    *(directory_element*)(datablock_tab[new_block_id].data) = dir_elem;
                    data_map[datablock_tab[last_data_block].next] = true;
                }
            }
            curr_path = &node_tab[dir_node];
        }

    }

}

void VirtualDisc::print_dir_content(inode* dir)
{
    std::cout << "Size of files in this directory: " << get_sum_files_in_dir(dir) << std::endl;
    std::cout << "Size of files in this direcotry and subdirectories: " << get_sum_files_in_dir_recursive(dir) << std::endl;
    std::cout << "Free space on disk: " << get_free_space() << std::endl;
    if(!(dir->type == inode_type::TYPE_DIRECTORY))
    {
        std::cerr << "Not a directory\n";
        return;
    }
    uint64_t datablock_idx = dir->first_data_block;
    while(datablock_idx != (uint64_t)-1)
    {
        directory_element* directory_elements = (directory_element*)datablock_tab[datablock_idx].data;
        for(long unsigned int i = 0; i < DIR_ELEMS_PER_BLOCK; i++)
        {
            directory_element curr_dir_elem = directory_elements[i];
            if (curr_dir_elem.used == false)
            {
                continue;
            }
            std::cout << "- " << curr_dir_elem.name << std::endl;
            std::cout << "\tinode id: " << curr_dir_elem.inode_id << std::endl;
            std::string type;
            if(node_tab[curr_dir_elem.inode_id].type == 1)
            {
                type = "file";
            }
            else if (node_tab[curr_dir_elem.inode_id].type == 2)
            {
                type = "directory";
            }
            else
            {
                type = "unknown";
            }
            std::cout << "\ttype: " << type << std::endl;

        }
        datablock_idx = datablock_tab[datablock_idx].next;
    }
    checked_inodes_ids.clear();
}

uint64_t VirtualDisc::get_sum_files_in_dir(inode* dir)
{
    uint64_t sum = 0;
    uint64_t curr_datablock_idx = dir->first_data_block;
    while(curr_datablock_idx != (uint64_t)-1)
    {
        directory_element* directory_elements = (directory_element*)datablock_tab[curr_datablock_idx].data;
        for(long unsigned int i = 0; i < DIR_ELEMS_PER_BLOCK; i++)
        {
            directory_element curr_dir_elem = directory_elements[i];
            if(!curr_dir_elem.used)
                continue;
            inode node = node_tab[curr_dir_elem.inode_id];
            sum += node.size;
        }
        curr_datablock_idx = datablock_tab[curr_datablock_idx].next;
    }
    return sum;
}

uint64_t VirtualDisc::get_sum_files_in_dir_recursive(inode* dir)
{
    uint64_t sum = 0;
    uint64_t curr_datablock_idx = dir->first_data_block;
    while(curr_datablock_idx != (uint64_t)-1)
    {
        directory_element* directory_elements = (directory_element*)datablock_tab[curr_datablock_idx].data;
        for(uint64_t i = 0; i < DIR_ELEMS_PER_BLOCK; i++)
        {
            directory_element curr_dir_elem = directory_elements[i];
            if(!curr_dir_elem.used)
                continue;
            if(std::find(checked_inodes_ids.begin(), checked_inodes_ids.end(), curr_dir_elem.inode_id) != checked_inodes_ids.end())
                continue;
            checked_inodes_ids.push_back(curr_dir_elem.inode_id);
            inode node = node_tab[curr_dir_elem.inode_id];
            if(node.type == inode_type::TYPE_DIRECTORY)
            {
                sum += get_sum_files_in_dir_recursive(&node);
            }
            else if(node.type == inode_type::TYPE_FILE)
            {
                sum += node.size;
            }
        }
        curr_datablock_idx = datablock_tab[curr_datablock_idx].next;
    }
    return sum;
}

uint64_t VirtualDisc::get_free_space()
{
    uint64_t number_of_free_datablocks = 0;
    for(uint64_t i = 0; i < data_map_len; i++)
    {
        if(data_map[i] == false)
        {
            number_of_free_datablocks++;
        }
    }
    return number_of_free_datablocks * BLOCK_SIZE;
}

void VirtualDisc::link(char* path_to_dir, char* filename, char* path_to_link_dir, char* link_filename)
{
    inode* dir = find_dir_inode(path_to_dir);
    if(!dir)
    {
        std::cerr << "Invalid path\n";
        return;
    }

    inode* file = find_in_dir(dir, filename);
    if(!file)
    {
        std::cerr << "Invalid filename\n";
        return;
    }

    inode* link_dir = find_dir_inode(path_to_link_dir);
    if(!link_dir)
    {
        std::cerr << "Invalid path\n";
        return;
    }

    file->references_num++;

    add_elem_to_dir(link_dir, link_filename, file-node_tab);
}

void VirtualDisc::unlink(char* path_to_dir, char* filename)
{
    inode* dir = find_dir_inode(path_to_dir);
    if(!dir)
    {
        std::cerr << "Invalid path\n";
        return;
    }

    inode* file = find_in_dir(dir, filename);
    if(!file)
    {
        std::cerr << "Invalid filename\n";
        return;
    }

    remove_file(file);

    directory_element* dir_elem = find_elem_in_dir(dir, filename);
    dir_elem->used = false;
    dir_elem->inode_id = -1;
}

void VirtualDisc::remove_file(inode* file)
{
    file->references_num--;
    if(file->references_num != 0)
        return;

    if(file->type == inode_type::TYPE_DIRECTORY)
    {
        uint64_t curr_datablock_idx = file->first_data_block;
        while(curr_datablock_idx != (uint64_t)-1)
        {
            directory_element* directory_elements = (directory_element*)datablock_tab[curr_datablock_idx].data;
            for(uint64_t i = 0; i < DIR_ELEMS_PER_BLOCK; i++)
            {
                directory_element curr_dir_elem = directory_elements[i];
                if(curr_dir_elem.used)
                {
                    remove_file(&node_tab[curr_dir_elem.inode_id]);
                }
            }
            curr_datablock_idx = datablock_tab[curr_datablock_idx].next;
        }
    }
    dealloc_datablocks(file->first_data_block);
    file->first_data_block = -1;
    file->references_num = 0;
    file->size = 0;
    file->type = inode_type::EMPTY;
}

void VirtualDisc::decrease_file_size(char* path_to_dir, char* filename, uint32_t size_amount)
{
    inode* dir = find_dir_inode(path_to_dir);
    if(!dir)
    {
        std::cerr << "Invalid path\n";
        return;
    }

    inode* file = find_in_dir(dir, filename);
    if(!file || file->type != inode_type::TYPE_FILE)
    {
        std::cerr << "Invalid filename\n";
        return;
    }

    if(size_amount > file->size)
    {
        std::cerr << "Too much size to decrease\n";
        return;
    }

    file->size -= size_amount;

    uint32_t new_amount_of_blocks = file->size / BLOCK_SIZE;
    if(file->size % BLOCK_SIZE > 0)
    {
        new_amount_of_blocks++;
    }
    //Get id of first block to dealloc
    uint64_t db_id = file->first_data_block;
    uint64_t prev_db_id = -1;
    for(uint32_t i = 0; i < new_amount_of_blocks; i++)
    {
        prev_db_id = db_id;
        db_id = datablock_tab[db_id].next;
    }

    // Deallocate next blocks from given one
    dealloc_datablocks(db_id);
    if(prev_db_id == (uint64_t)-1)
    {
        file->first_data_block = -1;
    }
    else
    {
        datablock_tab[prev_db_id].next = -1;
    }
}

void VirtualDisc::increase_file_size(char* path_to_dir, char* filename, uint32_t size_amount)
{
    inode* dir = find_dir_inode(path_to_dir);
    if(!dir)
    {
        std::cerr << "Invalid path\n";
        return;
    }

    inode* file = find_in_dir(dir, filename);
    if(!file || file->type != inode_type::TYPE_FILE)
    {
        std::cerr << "Invalid filename\n";
        return;
    }

    uint64_t last_db_id = file->first_data_block;
    while(datablock_tab[last_db_id].next != (uint64_t)-1)
    {
        last_db_id = datablock_tab[last_db_id].next;
    }

    uint64_t last_db_size = file->size % BLOCK_SIZE;

    // Extend last block, if possible
    if(last_db_size > 0)
    {
        uint64_t amount_to_extend = BLOCK_SIZE - (file->size % BLOCK_SIZE);
        if(amount_to_extend > size_amount)
        {
            amount_to_extend = size_amount;
        }
        memset(datablock_tab[last_db_id].data + last_db_size * sizeof(uint8_t), 0, amount_to_extend);

        size_amount -= amount_to_extend;
        file->size += amount_to_extend;
    }
    while(size_amount > 0)
    {
        // Find free datablock and allocate
        datablock_tab[last_db_id].next = find_free_datablock();
        if (datablock_tab[last_db_id].next == (uint64_t)-1)
        {
            std::cerr << "No free datablocks found\n";
            return;
        }
        data_map[datablock_tab[last_db_id].next] = true;

        // Clear new datablock

        uint64_t size_in_new_block;
        if(size_amount > BLOCK_SIZE)
        {
            size_in_new_block = BLOCK_SIZE;
        }
        else
        {
            size_in_new_block = size_amount;
        }

        memset(datablock_tab[datablock_tab[last_db_id].next].data, 0, size_in_new_block);

        size_amount -= size_in_new_block;
        file->size += size_in_new_block;

        last_db_id = datablock_tab[last_db_id].next;

    }

}

int fs_create(VirtualDisc vd, char* disc_name, uint64_t size)
{
    vd.create(disc_name, size);
    return 0;
}

int fs_copy_to_virtual(VirtualDisc vd, char* filename, char* path)
{
    vd.open();
    vd.copy_file_to_disc(filename, path);
    vd.save();
    return 0;
}

int fs_copy_from_virtual(VirtualDisc vd, char* path, char* name_on_vd, char* name_on_pd)
{
    vd.open();
    vd.copy_file_from_disc(path, name_on_vd, name_on_pd);
    vd.save();
    return 0;
}

int fs_mkdir(VirtualDisc vd, char* path)
{
    vd.open();
    vd.make_dir(path);
    vd.save();
    return 0;
}

int fs_rm(VirtualDisc vd, char* path, char* filename)
{
    // Removes directories recursively
    vd.open();
    vd.unlink(path, filename);
    vd.save();
    return 0;
}

int fs_unlink(VirtualDisc vd, char* path, char* filename)
{
    // Removes file if number of references is zero
    vd.open();
    vd.unlink(path, filename);
    vd.save();
    return 0;
}

int fs_link(VirtualDisc vd, char* path_to_dir, char* filename, char* path_to_link_dir, char* link_filename)
{
    vd.open();
    vd.link(path_to_dir, filename, path_to_link_dir, link_filename);
    vd.save();
    return 0;
}

int fs_ls(VirtualDisc vd, char* path)
{
    vd.open();
    vd.print_dir_content(vd.find_dir_inode(path));
    vd.save();
    return 0;
}

int fs_increase_filesize(VirtualDisc vd, char* path, char* filename, uint32_t size)
{
    vd.open();
    vd.increase_file_size(path, filename, size);
    vd.save();
    return 0;
}

int fs_decrease_filesize(VirtualDisc vd, char* path, char* filename, uint32_t size)
{
    vd.open();
    vd.decrease_file_size(path, filename, size);
    vd.save();
    return 0;
}

void print_help()
{
    std::cout << "Usage: ./fs <disc_name> <command> <arguments>\n";
    std::cout << "- create <name> <size> - creates new virtual disc\n";
    std::cout << "- cpto <name_of_file> <path_on_vd> - copies file to virtual disc\n";
    std::cout << "- cpfrom <path_to_dir> <name_on_virtual> <name> - copies file from virtual disc\n";
    std::cout << "- mkdir <path> - makes directories (nested possible)\n";
    std::cout << "- rm <path> <filename> - removes file (or directory) of given name from given directory\n";
    std::cout << "- link <path_to_dir> <filename> <path_to_link_dir> <linked_filename> - creates hard link\n";
    std::cout << "- unlink <path_to_dir> <filename> - removes link\n";
    std::cout << "- ls <path> - shows content of given path\n";
    std::cout << "- inc <path_to_dir> <filename> <size> - increases size of given file by <size> bytes\n";
    std::cout << "- dec <path_to_dir> <filename> <size> - decreases size of file by <size> bytes\n";
    std::cout << "Warning! Root directory stands for \"\"\n";
}

int main(int argc, char* argv[])
{
    VirtualDisc vd;
    if(argc < 3)
    {
        print_help();
        return 1;
    }
    if(strcmp(argv[1], "create") == 0)
    {
        return fs_create(vd, argv[2], std::stoul(argv[3]));
    }
    else {
        vd.set_name(argv[1]);
        if(strcmp(argv[2], "cpto") == 0)
        {
            return fs_copy_to_virtual(vd, argv[3], argv[4]);
        }
        else if(strcmp(argv[2], "cpfrom") == 0)
        {
            return fs_copy_from_virtual(vd, argv[3], argv[4], argv[5]);
        }
        else if(strcmp(argv[2], "mkdir") == 0)
        {
            return fs_mkdir(vd, argv[3]);
        }
        else if(strcmp(argv[2], "rm") == 0)
        {
            return fs_rm(vd, argv[3], argv[4]);
        }
        else if(strcmp(argv[2], "link") == 0)
        {
            return fs_link(vd, argv[3], argv[4], argv[5], argv[6]);
        }
        else if(strcmp(argv[2], "unlink") == 0)
        {
            return fs_unlink(vd, argv[3], argv[4]);
        }
        else if(strcmp(argv[2], "ls") == 0)
        {
            return fs_ls(vd, argv[3]);
        }
        else if(strcmp(argv[2], "inc") == 0)
        {
            return fs_increase_filesize(vd, argv[3], argv[4], atoi(argv[5]));
        }
        else if(strcmp(argv[2], "dec") == 0)
        {
            return fs_decrease_filesize(vd, argv[3], argv[4], atoi(argv[5]));
        }
        else
        {
            print_help();
        }

    }
}