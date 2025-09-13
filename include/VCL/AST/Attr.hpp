#pragma once

#include <llvm/ADT/ilist.h>


namespace VCL {

    class Attr : public llvm::ilist_node<Attr> {
        
    };

}