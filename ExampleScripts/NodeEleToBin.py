node_path = "./cow.node"
ele_path = "./cow.ele"
bin_path = "./cow.bin"

import TetraTools

# This function is just a python binding to TetraTools.hxx
TetraTools.write_node_ele_as_binary(node_path, ele_path, 0, bin_path)

# To read in the bin, we'd use this, but in c++ instead of python
points = TetraTools.FloatVector()
indices = TetraTools.UIntVector()

TetraTools.read_binary(bin_path, points, indices)

# Can use points here...
