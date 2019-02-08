// ┌──────────────────────────────────────────────────────────────────┐
// │  Nate VM - Jan 2019                                              │
// |                                                                  |
// |  This file contains a set of functions which can read and write  |
// |  unstructured tetrahedra data. It follows the .node and .ele     |
// |  formats described by "TetGen", but offers binary equivalents    |
// |  for faster reads                                                |
// └──────────────────────────────────────────────────────────────────┘

#include <exception>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <algorithm> 
#include <cctype>
#include <locale>

struct GridItem {
    float point [3];
    float attribute;
};

struct Ele {
    uint32_t num_tetrahedra;
    uint32_t nodes_per_tetrahedron;
    uint32_t num_attributes;

    std::vector<uint32_t> nodes;
    std::vector<float> attributes;
};

struct Node {
    uint32_t num_points;
    // Must be 3
    uint32_t dimension;
    uint32_t num_attributes;
    // Must be 0 or 1
    uint32_t num_boundary_markers;
    std::vector<float> points;
    std::vector<float> attributes;
    std::vector<float> boundary_markers;
};

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

inline void throw_if_file_does_not_exist(std::string path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        throw std::runtime_error( std::string(path + " does not exist!"));
}

/* Reads an ASCII node file */
Node read_node(std::string node_path)
{
    throw_if_file_does_not_exist(node_path);
    
    Node node;

    std::fstream file;
    file.open(node_path, std::ios::in);
    if (!file.is_open())
        throw std::runtime_error( std::string("Unable to open " + node_path));

    bool header_read = false;
    int line_number = 0;
    std::string line;
    while (getline(file, line))
    {
        line_number++;

        /* Clean up the line, and ignore any comments */
        trim(line);
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        /* Read the header */
        std::istringstream iss(line);
        if (!header_read) {
            int n;
            std::vector<int> integers;
            while (iss >> n) integers.push_back(n);

            if (integers.size() != 4)
                throw std::runtime_error( std::string("Line " + std::to_string(line_number) + " : " + node_path + " must contain 4 integers "));

            node.num_points = integers[0];
            node.dimension = integers[1];
            node.num_attributes = integers[2];
            node.num_boundary_markers = integers[3];

            if (node.num_points <= 0)
                throw std::runtime_error( std::string("Line " + std::to_string(line_number) + " : " + node_path + " number of points must be greater than 0"));

            if (!((node.dimension == 2) || (node.dimension == 3)))
                throw std::runtime_error( std::string("Line " + std::to_string(line_number) + " : " + node_path + " dimension must be 2 or 3"));
            
            if (node.num_attributes < 0)
                throw std::runtime_error( std::string("Line " + std::to_string(line_number) + " : " + node_path + " number of attributes must be greater than or equal to 0"));


            if (!((node.num_boundary_markers == 1) || (node.num_boundary_markers == 0)))
                throw std::runtime_error( std::string("Line " + std::to_string(line_number) + " : " + node_path + " number of boundary markers must be 0 or 1"));

            header_read = true;            
        }
        /* Read points and attributes */
        else {
            float n;
            std::vector<float> floats;
            while (iss >> n) floats.push_back(n);

            if (floats.size() != (1 + node.dimension + node.num_attributes + node.num_boundary_markers))
                throw std::runtime_error( std::string("Line " + std::to_string(line_number) + " : " + node_path + " must contain " + 
                    std::to_string(1 + node.dimension + node.num_attributes + node.num_boundary_markers) + " numbers "));

            int offset = 1;
            for (int i = 0; i < node.dimension; ++i, ++offset)
                node.points.push_back(floats[offset]);

            for (int i = 0; i < node.num_attributes; ++i, ++offset)
                node.attributes.push_back(floats[offset]);
            
            for (int i = 0; i < node.num_boundary_markers; ++i, ++offset)
                node.boundary_markers.push_back(floats[offset]);
        }
    }
    file.close();

    return node;
}

/* Reads an ASCII ele file */
Ele read_ele(std::string ele_path)
{
    throw_if_file_does_not_exist(ele_path);

    /* Read the file */
    std::fstream file;
    file.open(ele_path, std::ios::in);
    if (!file.is_open())
        std::cout<< std::string("Unable to open " + ele_path ) << std::endl;

    Ele ele;

    bool header_read = false;
    int line_number = 0;
    std::string line;
    while (std::getline(file, line))
    {
        line_number++;

        /* Clean up the line, remove comments */
        trim(line);
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        /* Read the header */
        std::istringstream iss(line);
        if (!header_read){
            int n;
            std::vector<int> integers;
            while (iss >> n) integers.push_back(n);

            if (integers.size() != 3)
                throw std::runtime_error( std::string("Line " + std::to_string(line_number) + " : " + ele_path + " must contain 3 integers "));
            
            ele.num_tetrahedra = integers[0];
            ele.nodes_per_tetrahedron = integers[1];
            ele.num_attributes = integers[2];

            if (ele.num_tetrahedra <= 0)
                throw std::runtime_error( std::string("Line " + std::to_string(line_number) + " : " + ele_path + " number of tetrahedron must be greater than 0"));
            
            if (!((ele.nodes_per_tetrahedron == 4) || (ele.nodes_per_tetrahedron == 10)))
                throw std::runtime_error( std::string("Line " + std::to_string(line_number) + " : " + ele_path + " dimension must be 4 (corners only) or 10 (corners and edges)"));
            
            if (ele.num_attributes < 0)
                throw std::runtime_error( std::string("Line " + std::to_string(line_number) + " : " + ele_path + " number of attributes must be greater than or equal to 0"));
            
            header_read = true;            
        }
        /* Read node indices */
        else {
            float n;
            std::vector<float> floats;
            while (iss >> n) floats.push_back(n);

            if (floats.size() != (1 + ele.nodes_per_tetrahedron + ele.num_attributes))
                throw std::runtime_error( std::string("Line " + std::to_string(line_number) + " : " + ele_path + " must contain " + std::to_string(1 + ele.nodes_per_tetrahedron + ele.num_attributes) + " numbers "));
            
            int offset = 1;
            for (int i = 0; i < ele.nodes_per_tetrahedron; ++i, ++offset)
                ele.nodes.push_back(((uint32_t) floats[offset]) - 1);

            for (int i = 0; i < ele.num_attributes; ++i, ++offset)
                ele.attributes.push_back(floats[offset]);
        }
    }
    file.close();

    return ele;
}

/* Writes an ASCII node file */
void write_node(std::string node_path, Node &node)
{
    /* Create/open the file */
    std::fstream file;
    file.open(node_path, std::ios::out | std::ios::trunc );

    if (!file.is_open())
        throw std::runtime_error( std::string("Unable to create " + node_path));

    if (node.num_points <= 0)
        throw std::runtime_error( std::string("node.number of points must be greater than 0"));
    
    if (!((node.dimension == 2) || (node.dimension == 3)))
        throw std::runtime_error( std::string("node.dimension must be 2 or 3"));
    
    if (node.num_attributes < 0)
        throw std::runtime_error( std::string("node.number of attributes must be greater than or equal to 0"));
    
    if (!((node.num_boundary_markers == 1) || (node.num_boundary_markers == 0)))
        throw std::runtime_error( std::string("node.number of boundary markers must be 0 or 1"));
    
    if (node.points.size() < (node.num_points * node.dimension))
        throw std::runtime_error( std::string("node.points must equal (node.num_points * node.dimension)"));
    
    if (node.attributes.size() < (node.num_points * node.num_attributes))
        throw std::runtime_error( std::string("node.attributes must equal (node.num_points * node.num_attributes)"));
    
    /* Write the header */
    file << node.num_points << " " << node.dimension << " " << node.num_attributes << " " << node.num_boundary_markers << std::endl;
    for (uint32_t i = 0; i < node.num_points; ++i)
    {
        file << i << " ";
        // Write points
        for (uint32_t j = 0; j < node.dimension; ++j) 
            file << node.points[i * node.dimension + j] << " ";
        
        // Write attributes
        for (uint32_t j = 0; j < node.num_attributes; ++j) 
            file << node.attributes[i * node.num_attributes + j] << " ";
        
        // Write boundary markers
        for (uint32_t j = 0; j < node.num_boundary_markers; ++j) 
            file << node.boundary_markers[i * node.num_boundary_markers + j] << " ";
        
        file << std::endl;
    }

    file.close();
}

/* Writes an ASCII ele file */
void write_ele(std::string ele_path, Ele &ele)
{
    /* Create/open the file */
    std::fstream file;
    file.open(ele_path, std::ios::out | std::ios::trunc );

    if (!file.is_open()) 
        throw std::runtime_error( std::string("Unable to create " + ele_path));
    
    if (ele.num_tetrahedra <= 0)
        throw std::runtime_error( std::string("ele.num_tetrahedra must be greater than 0"));
    
    if (!((ele.nodes_per_tetrahedron == 4) || (ele.nodes_per_tetrahedron == 10)))
        throw std::runtime_error( std::string("ele.nodes_per_tetrahedron must be 4 or 10"));
    
    if (ele.num_attributes < 0)
        throw std::runtime_error( std::string("ele.number of attributes must be greater than or equal to 0"));
    
    if (ele.nodes.size() < (ele.num_tetrahedra * ele.nodes_per_tetrahedron))
        throw std::runtime_error( std::string("ele.nodes must equal (ele.num_tetrahedra * ele.nodes_per_tetrahedron)"));
    
    if (ele.attributes.size() < (ele.num_tetrahedra * ele.num_attributes))
        throw std::runtime_error( std::string("ele.attributes must equal (ele.num_tetrahedra * ele.num_attributes)"));
    
    /* Write the header */
    file << ele.num_tetrahedra << " " << ele.nodes_per_tetrahedron << " " << ele.num_attributes << std::endl;
    for (uint32_t i = 0; i < ele.num_tetrahedra; ++i)
    {
        file << i << " ";
        
        // Write nodes
        for (uint32_t j = 0; j < ele.nodes_per_tetrahedron; ++j) 
            file << ele.nodes[i * ele.nodes_per_tetrahedron + j] + 1 << " ";
        
        // Write attributes
        for (uint32_t j = 0; j < ele.num_attributes; ++j) 
            file << ele.attributes[i * ele.num_attributes + j] << " ";
        
        file << std::endl;
    }

    file.close();

}

/* Converts a node/ele file pair into a simple binary format */
void write_node_ele_as_binary(std::string node_path, std::string ele_path, uint32_t attribute_idx, std::string binary_path)
{
    Ele ele = read_ele(ele_path);
    Node node = read_node(node_path);

    /* Create/open the file */
    std::fstream file;
    file.open(binary_path, std::ios::out | std::ios::trunc | std::ios::binary );

    if ((node.num_attributes <= attribute_idx) && (attribute_idx != 0))
        throw std::runtime_error( std::string("attribute index for this node file must be less than " + std::to_string(node.num_attributes)));
    
    if (node.dimension != 3)
        throw std::runtime_error( std::string("node dimension needs to be 3"));
    
    if (ele.nodes_per_tetrahedron != 4)
        throw std::runtime_error( std::string("nodes per tetrahedron needs to be 4"));
    
    if (!file.is_open()) 
        throw std::runtime_error( std::string("Unable to create " + binary_path));
        
    /* Write out the points per primitive */
    uint32_t points_per_primitive = 4;
    file.write((char*) &points_per_primitive, sizeof(uint32_t));

    /* Write out the number of points */
    file.write((char*) &node.num_points, sizeof(uint32_t));

    /* Write out the number of indices */
    uint32_t num_indices = ele.num_tetrahedra * 4;
    file.write((char*) &num_indices, sizeof(uint32_t));

    /* Write out the point data */
    file.write((char*) node.points.data(), node.num_points * 3 * sizeof(float));

    /* Write out the scalar data */
    std::vector<float> scalars(node.num_points);
    for (uint32_t i = 0; i < node.num_points; ++i) {
        scalars[i] = (node.num_attributes > 0) ? node.attributes[i * node.num_attributes + attribute_idx] : 0.0f;
    }
    file.write((char*) scalars.data(), node.num_points * sizeof(float));
    
    /* Write indices */
    file.write((char*) ele.nodes.data(), ele.num_tetrahedra * 4 * sizeof(uint32_t));
    file.close();
}

/* Writes raw point/index data to a binary file */
void write_to_binary(std::vector<float> &points, std::vector<float> &scalars, std::vector<uint32_t> &indices, uint32_t points_per_primitive, std::string binary_path)
{
    /* Create/open the file */
    std::fstream file;
    file.open(binary_path, std::ios::out | std::ios::trunc | std::ios::binary );

    /* Write out the points per primitive */
    file.write((char*) &points_per_primitive, sizeof(uint32_t));

    /* Write out the number of points */
    uint32_t num_points = points.size() / 3;
    file.write((char*) &num_points, sizeof(uint32_t));

    /* Write out the number of indices */
    uint32_t num_indices = indices.size();
    file.write((char*) &num_indices, sizeof(uint32_t));

    /* Write out point data */
    file.write((char*) points.data(), points.size() * sizeof(float));

    /* Write out scalar data */
    file.write((char*) scalars.data(), scalars.size() * sizeof(float));

    /* Write indices */
    file.write((char*) indices.data(), indices.size() * sizeof(uint32_t));
    file.close();
}


/* Reads points and indices from a binary format */
void read_binary(std::string binary_path, uint32_t &points_per_primitive, std::vector<float> &points, std::vector<float> &scalars, std::vector<uint32_t> &indices)
{
    throw_if_file_does_not_exist(binary_path);

    std::fstream file;
    file.open(binary_path, std::ios::in | std::ios::binary );

    if (!file.is_open()) 
        throw std::runtime_error( std::string("Unable to open " + binary_path));

    file.read((char*)(&points_per_primitive), sizeof(uint32_t));
    
    uint32_t num_points;
    file.read((char*)(&num_points), sizeof(uint32_t));

    uint32_t num_indices;
    file.read((char*)(&num_indices), sizeof(uint32_t));

    points.resize(num_points * 3);
    file.read((char*)points.data(), num_points * 3 * sizeof(float));

    scalars.resize(num_points);
    file.read((char*)scalars.data(), num_points * sizeof(float));

    indices.resize(num_indices);
    file.read((char*)(indices.data()), num_indices * sizeof(uint32_t));

    file.close();
}