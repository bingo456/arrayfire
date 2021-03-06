/*******************************************************
 * Copyright (c) 2018, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#pragma once
#include <common/jit/NaryNode.hpp>

namespace common {

class UnaryNode : public NaryNode {
   public:
    UnaryNode(const af::dtype type, const char *op_str, Node_ptr child, int op)
        : NaryNode(type, op_str, 1, {{child}}, op, child->getHeight() + 1) {}
};
}  // namespace common
