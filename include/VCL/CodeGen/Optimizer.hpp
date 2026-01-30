#pragma once


namespace VCL {
    class CodeGenModule;

    class Optimizer {
    public:
        Optimizer() = default;
        Optimizer(const Optimizer& other) = default;
        Optimizer(Optimizer&& other) = default;
        ~Optimizer() = default;

        Optimizer& operator=(const Optimizer& other) = default;
        Optimizer& operator=(Optimizer&& other) = default;

        bool Optimize(CodeGenModule& cgm);
    };

}