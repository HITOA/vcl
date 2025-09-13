#include <VCL/AST/DeclContext.hpp>

#include <VCL/AST/Decl.hpp>



VCL::DeclContext::Iterator& VCL::DeclContext::Iterator::operator++() {
    decl = decl->GetNext();
    return *this;
}

VCL::DeclContext::Iterator VCL::DeclContext::Iterator::operator++(int) {
    Iterator oldIt{ *this };
    decl = decl->GetNext();
    return oldIt;
}

void VCL::DeclContext::InsertFront(Decl* decl) {
    if (!last) {
        first = decl;
        last = decl;
    } else {
        decl->SetNext(first);
        first = decl;
    }
}

void VCL::DeclContext::InsertBack(Decl* decl) {
    if (!first) {
        first = decl;
        last = decl;
    } else {
        last->SetNext(decl);
        last = decl;
    }
}