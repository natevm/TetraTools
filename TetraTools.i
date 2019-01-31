%module TetraTools

%include <exception.i>

%feature("autodoc","3");

%include "std_string.i"
%include "std_vector.i"
%include "stdint.i"

namespace std {
   %template(UIntVector) vector<uint32_t>;
   %template(FloatVector) vector<float>;
};

%{
#include "./TetraTools.hxx"
%}

%include "./TetraTools.hxx"
