#include "jitbuiltins.hpp"

#include "jittype.hpp"

#include <unordered_map>
#include <stdexcept>
#include <format>
#include <functional>


static llvm::Value* BuiltinUnaryIntrinsic(llvm::Intrinsic::ID it, std::vector<llvm::Value*>& argsv, llvm::IRBuilder<>* builder) {
    if (argsv.size() != 1)
        throw std::runtime_error{ "Builtin unary function takes only 1 argument." };
    return builder->CreateUnaryIntrinsic(it, argsv[0]);
}

static llvm::Value* BuiltinBinaryIntrinsic(llvm::Intrinsic::ID it, std::vector<llvm::Value*>& argsv, llvm::IRBuilder<>* builder) {
    if (argsv.size() != 2)
        throw std::runtime_error{ "Builtin binary function takes 2 arguments." };
    llvm::Value* argv1 = argsv[0];
    llvm::Value* argv2 = VCL::JITType::CastRHSToLHS(argv1->getType(), argsv[1], builder);
    return builder->CreateBinaryIntrinsic(it, argv1, argv2);
}

static llvm::Value* BuiltinTernaryIntrinsic(llvm::Intrinsic::ID it, std::vector<llvm::Value*>& argsv, llvm::IRBuilder<>* builder) {
    if (argsv.size() != 3)
        throw std::runtime_error{ "Builtin ternary function takes 3 arguments." };
    llvm::Value* argv1 = argsv[0];
    llvm::Value* argv2 = VCL::JITType::CastRHSToLHS(argv1->getType(), argsv[1], builder);
    llvm::Value* argv3 = VCL::JITType::CastRHSToLHS(argv1->getType(), argsv[2], builder);
    return builder->CreateIntrinsic(it, { argsv[0]->getType() }, { argv1, argv2, argv3 });
}

static llvm::Value* BuiltinFMod(std::vector<llvm::Value*>& argsv, llvm::IRBuilder<>* builder) {
    if (argsv.size() != 2)
        throw std::runtime_error{ "Builtin binary function takes 2 arguments." };
    llvm::Value* argv1 = argsv[0];
    llvm::Value* argv2 = VCL::JITType::CastRHSToLHS(argv1->getType(), argsv[1], builder);
    return builder->CreateFRem(argv1, argv2);
}

static llvm::Value* BuiltinSelect(std::vector<llvm::Value*>& argsv, llvm::IRBuilder<>* builder) {
    if (argsv.size() != 3)
        throw std::runtime_error{ "Builtin ternary function takes 3 arguments." };
    llvm::Value* argv1 = argsv[0];
    llvm::Value* argv2 = VCL::JITType::BroadcastIfNeeded(argsv[1], builder);
    llvm::Value* argv3 = VCL::JITType::BroadcastIfNeeded(argsv[2], builder);
    return builder->CreateSelect(argv1, argv2, argv3);
}

static llvm::Value* BuiltinWrite(std::vector<llvm::Value*>& argsv, llvm::IRBuilder<>* builder) {
    if (argsv.size() != 2)
        throw std::runtime_error{ "write function takes 2 arguments." };
    llvm::Value* argv1 = argsv[0];
    llvm::Value* argv2 = argsv[1];

    if (!llvm::isa<llvm::StructType>(VCL::JITType::GetBaseType(argv1)))
        throw std::runtime_error{ "write first argument should be a buffer." };

    if (argv2->getType()->isVectorTy())
        throw std::runtime_error{ "write second argument shouldn't be a vector. use vwrite instead." };

    llvm::StructType* bufferType = llvm::cast<llvm::StructType>(VCL::JITType::GetBaseType(argv1));
    
    if (!bufferType->getTypeAtIndex(0U)->isIntegerTy())
        throw std::runtime_error{ "write first argument should be a buffer." };
    if (!bufferType->getTypeAtIndex(1U)->isArrayTy())
        throw std::runtime_error{ "write first argument should be a buffer." };

    llvm::ArrayType* arrayType = llvm::cast<llvm::ArrayType>(bufferType->getTypeAtIndex(1));

    int alignment = (int)VCL::JITType::GetMaxVectorWidth() / (arrayType->getElementType()->getScalarSizeInBits() / 8);

    llvm::Value* currentIndexPtr = builder->CreateStructGEP(bufferType, argv1, 0);
    llvm::Value* currentIndex = builder->CreateLoad(bufferType->getTypeAtIndex(0U), currentIndexPtr);

    currentIndex = builder->CreateAdd(currentIndex, 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), alignment));

    llvm::Value* arrayPtr = builder->CreateStructGEP(bufferType, argv1, 1);
    llvm::Value* elementPtr = builder->CreateGEP(arrayType, arrayPtr, { 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), 0),
        currentIndex
    });


    builder->CreateStore(argv2, elementPtr);
    llvm::Value* newIndex = builder->CreateAdd(currentIndex, 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), 1));
    newIndex = builder->CreateSRem(newIndex, 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), arrayType->getNumElements()));
    builder->CreateStore(newIndex, currentIndexPtr);

    return argv2;
}

static llvm::Value* BuiltinRead(std::vector<llvm::Value*>& argsv, llvm::IRBuilder<>* builder) {
    if (argsv.size() != 2)
        throw std::runtime_error{ "read function takes 2 arguments." };
    llvm::Value* argv1 = argsv[0];
    llvm::Value* argv2 = argsv[1];

    if (!llvm::isa<llvm::StructType>(VCL::JITType::GetBaseType(argv1)))
        throw std::runtime_error{ "read first argument should be a buffer." };

    if (argv2->getType()->isVectorTy())
        throw std::runtime_error{ "read second argument shouldn't be a vector." };

    if (argv2->getType()->isFloatTy())
        argv2 = builder->CreateFPToSI(argv2, llvm::Type::getInt32Ty(builder->getContext()));

    llvm::StructType* bufferType = llvm::cast<llvm::StructType>(VCL::JITType::GetBaseType(argv1));
    
    if (!bufferType->getTypeAtIndex(0U)->isIntegerTy())
        throw std::runtime_error{ "read first argument should be a buffer." };
    if (!bufferType->getTypeAtIndex(1U)->isArrayTy())
        throw std::runtime_error{ "read first argument should be a buffer." };

    llvm::ArrayType* arrayType = llvm::cast<llvm::ArrayType>(bufferType->getTypeAtIndex(1));

    int alignment = (int)VCL::JITType::GetMaxVectorWidth() / (arrayType->getElementType()->getScalarSizeInBits() / 8);

    llvm::Value* currentIndexPtr = builder->CreateStructGEP(bufferType, argv1, 0);
    llvm::Value* currentIndex = builder->CreateLoad(bufferType->getTypeAtIndex(0U), currentIndexPtr);

    currentIndex = builder->CreateSRem(
        builder->CreateAdd(
            currentIndex,
            builder->CreateSub(
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), arrayType->getNumElements()),
                argv2
            )
        ),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), arrayType->getNumElements())
    );

    currentIndex = builder->CreateAdd(currentIndex, 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), alignment));

    llvm::Value* arrayPtr = builder->CreateStructGEP(bufferType, argv1, 1);
    llvm::Value* elementPtr = builder->CreateGEP(arrayType, arrayPtr, { 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), 0),
        currentIndex
    });

    return builder->CreateLoad(arrayType->getElementType(), elementPtr);
}

static llvm::Value* BuiltinVWrite(std::vector<llvm::Value*>& argsv, llvm::IRBuilder<>* builder) {
    if (argsv.size() != 2)
        throw std::runtime_error{ "vwrite function takes 2 arguments." };
    llvm::Value* argv1 = argsv[0];
    llvm::Value* argv2 = argsv[1];

    if (!llvm::isa<llvm::StructType>(VCL::JITType::GetBaseType(argv1)))
        throw std::runtime_error{ "vwrite first argument should be a buffer." };

    if (!argv2->getType()->isVectorTy())
        throw std::runtime_error{ "vwrite second argument should be a vector." };

    llvm::StructType* bufferType = llvm::cast<llvm::StructType>(VCL::JITType::GetBaseType(argv1));
    
    if (!bufferType->getTypeAtIndex(0U)->isIntegerTy())
        throw std::runtime_error{ "vwrite first argument should be a buffer." };
    if (!bufferType->getTypeAtIndex(1U)->isArrayTy())
        throw std::runtime_error{ "vwrite first argument should be a buffer." };

    llvm::ArrayType* arrayType = llvm::cast<llvm::ArrayType>(bufferType->getTypeAtIndex(1));

    int alignment = (int)VCL::JITType::GetMaxVectorWidth() / (arrayType->getElementType()->getScalarSizeInBits() / 8);

    llvm::Value* currentIndexPtr = builder->CreateStructGEP(bufferType, argv1, 0);
    llvm::Value* currentIndex = builder->CreateLoad(bufferType->getTypeAtIndex(0U), currentIndexPtr);
    llvm::Value* currentIndexOffseted = builder->CreateAdd(currentIndex, 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), alignment));

    llvm::Value* arrayPtr = builder->CreateStructGEP(bufferType, argv1, 1);
    llvm::Value* elementPtr = builder->CreateGEP(arrayType, arrayPtr, { 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), 0),
        currentIndexOffseted
    });

    llvm::StoreInst* s = builder->CreateAlignedStore(argv2, elementPtr, llvm::Align(1));
    //if (arrayType->getNumElements() > 16384) s->setMetadata(llvm::LLVMContext::MD_nontemporal, llvm::MDNode::get(builder->getContext(), {}));
    llvm::Value* newIndex = builder->CreateAdd(currentIndex, 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), alignment));
    newIndex = builder->CreateSRem(newIndex, 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), arrayType->getNumElements()));
    
    llvm::Value* isWrapping = builder->CreateICmpSLT(
        newIndex,
        currentIndex
    );
    
    llvm::Function* expectFunction = llvm::Intrinsic::getDeclaration(
        builder->GetInsertBlock()->getModule(), llvm::Intrinsic::expect, llvm::Type::getInt1Ty(builder->getContext())
    );

    llvm::Value* expectedCond = builder->CreateCall(expectFunction, 
        { isWrapping, llvm::ConstantInt::get(llvm::Type::getInt1Ty(builder->getContext()), 1) });
    
    llvm::Function* function = builder->GetInsertBlock()->getParent();

    llvm::BasicBlock* wrappingBB = llvm::BasicBlock::Create(builder->getContext(), "vwrite_wrapping", function);
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(builder->getContext(), "vwrite_end", function);

    builder->CreateCondBr(expectedCond, wrappingBB, endBB);

    builder->SetInsertPoint(wrappingBB);
    elementPtr = builder->CreateGEP(arrayType, arrayPtr, { 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), 0),
        newIndex
    });
    s = builder->CreateAlignedStore(argv2, elementPtr, llvm::Align(1));
    //if (arrayType->getNumElements() > 16384) s->setMetadata(llvm::LLVMContext::MD_nontemporal, llvm::MDNode::get(builder->getContext(), {}));
    builder->CreateBr(endBB);

    builder->SetInsertPoint(endBB); 
    builder->CreateStore(newIndex, currentIndexPtr);

    return argv2;
}


static llvm::Value* BuiltinVRead(std::vector<llvm::Value*>& argsv, llvm::IRBuilder<>* builder) {
    if (argsv.size() != 2)
        throw std::runtime_error{ "vread function takes 2 arguments." };
    llvm::Value* argv1 = argsv[0];
    llvm::Value* argv2 = argsv[1];

    if (!llvm::isa<llvm::StructType>(VCL::JITType::GetBaseType(argv1)))
        throw std::runtime_error{ "vread first argument should be a buffer." };

    if (argv2->getType()->isVectorTy())
        throw std::runtime_error{ "vread second argument shouldn't be a vector." };

    if (argv2->getType()->isFloatTy())
        argv2 = builder->CreateFPToSI(argv2, llvm::Type::getInt32Ty(builder->getContext()));

    llvm::StructType* bufferType = llvm::cast<llvm::StructType>(VCL::JITType::GetBaseType(argv1));
    
    if (!bufferType->getTypeAtIndex(0U)->isIntegerTy())
        throw std::runtime_error{ "read first argument should be a buffer." };
    if (!bufferType->getTypeAtIndex(1U)->isArrayTy())
        throw std::runtime_error{ "read first argument should be a buffer." };

    llvm::ArrayType* arrayType = llvm::cast<llvm::ArrayType>(bufferType->getTypeAtIndex(1));

    int alignment = (int)VCL::JITType::GetMaxVectorWidth() / (arrayType->getElementType()->getScalarSizeInBits() / 8);

    llvm::Value* currentIndexPtr = builder->CreateStructGEP(bufferType, argv1, 0);
    llvm::Value* currentIndex = builder->CreateLoad(bufferType->getTypeAtIndex(0U), currentIndexPtr);

    currentIndex = builder->CreateSRem(
        builder->CreateAdd(
            currentIndex,
            builder->CreateSub(
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), arrayType->getNumElements()),
                argv2
            )
        ),
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), arrayType->getNumElements())
    );

    llvm::Value* currentIndexOffseted = builder->CreateAdd(currentIndex, 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), alignment));

    llvm::Value* arrayPtr = builder->CreateStructGEP(bufferType, argv1, 1);
    llvm::Value* elementPtr = builder->CreateGEP(arrayType, arrayPtr, { 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), 0),
        currentIndexOffseted
    });

    llvm::Type* loadType = llvm::FixedVectorType::get(arrayType->getElementType(), 
        VCL::JITType::GetMaxVectorWidth() / (arrayType->getElementType()->getScalarSizeInBits() / 8));

    llvm::Value* tp = builder->CreateAlignedLoad(loadType, elementPtr, llvm::Align(1));

    llvm::Value* newIndex = builder->CreateAdd(currentIndex, 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), alignment));
    newIndex = builder->CreateSRem(newIndex, 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), arrayType->getNumElements()));

    elementPtr = builder->CreateGEP(arrayType, arrayPtr, { 
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), 0),
        newIndex
    });
    
    llvm::Value* bp = builder->CreateAlignedLoad(loadType, elementPtr, llvm::Align(1));

    std::vector<llvm::Constant*> indices{};
    for (int i = 0; i < alignment; ++i)
        indices.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), alignment - i - 1));

    llvm::Constant* sequence = llvm::ConstantVector::get(indices);

    llvm::Value* c = builder->CreateICmpULT(newIndex, llvm::ConstantInt::get(llvm::Type::getInt32Ty(builder->getContext()), 8));
    llvm::Value* mask = builder->CreateICmpULT(sequence, VCL::JITType::BroadcastIfNeeded(newIndex, builder));
    mask = builder->CreateAnd(mask, builder->CreateVectorSplat(alignment, c));

    return builder->CreateSelect(mask, bp, tp);
}

using namespace std::placeholders;

static std::unordered_map<std::string_view, std::function<llvm::Value*(std::vector<llvm::Value*>&,llvm::IRBuilder<>*)>> builtins{
    { "sqrt",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::sqrt, _1, _2) },
    { "sin",        std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::sin, _1, _2) },
    { "cos",        std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::cos, _1, _2) },
    { "tan",        std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::tan, _1, _2) },
    { "asin",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::asin, _1, _2) },
    { "acos",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::acos, _1, _2) },
    { "atan",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::atan, _1, _2) },
    { "sinh",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::sinh, _1, _2) },
    { "cosh",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::cosh, _1, _2) },
    { "tanh",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::tanh, _1, _2) },
    { "log",        std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::log, _1, _2) },
    { "log10",      std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::log10, _1, _2) },
    { "log2",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::log2, _1, _2) },
    { "exp",        std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::exp, _1, _2) },
    { "exp10",      std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::exp10, _1, _2) },
    { "exp2",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::exp2, _1, _2) },
    { "fabs",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::fabs, _1, _2) },
    { "ceil",       std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::ceil, _1, _2) },
    { "floor",      std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::floor, _1, _2) },
    { "round",      std::bind(BuiltinUnaryIntrinsic, llvm::Intrinsic::round, _1, _2) },

    { "pow",        std::bind(BuiltinBinaryIntrinsic, llvm::Intrinsic::pow, _1, _2) },
    { "min",        std::bind(BuiltinBinaryIntrinsic, llvm::Intrinsic::minnum, _1, _2) },
    { "max",        std::bind(BuiltinBinaryIntrinsic, llvm::Intrinsic::maxnum, _1, _2) },
    { "fmod",       BuiltinFMod },

    // Ring buffer access function
    { "write",      BuiltinWrite },
    { "read",       BuiltinRead },
    { "vwrite",     BuiltinVWrite },
    { "vread",      BuiltinVRead },

    { "fma",        std::bind(BuiltinTernaryIntrinsic, llvm::Intrinsic::fma, _1, _2) },
    { "brev",       std::bind(BuiltinTernaryIntrinsic, llvm::Intrinsic::bitreverse, _1, _2) },
    { "select",     BuiltinSelect }
};


bool VCL::JITBuiltins::IsBuiltinFunction(std::string_view name) {
    return builtins.count(name);
}

llvm::Value* VCL::JITBuiltins::CallBuiltinFunction(std::string_view name, std::vector<llvm::Value*>& args, llvm::IRBuilder<>* builder) {
    if (!builtins.count(name))
        throw std::runtime_error{ std::format("Tried to call the builtin function \"{}\" wich does not exist.", name) };

    return builtins[name](args, builder);
}