#pragma once

#include <vector>


namespace VCL {
    class Decl;

    class ASTConsumer {
    public:
        virtual ~ASTConsumer() = default;

        virtual void HandleTopLevelDecl(Decl* decl) {}
    };

    class MultiplexerASTConsumer : public ASTConsumer {
    public:
        void HandleTopLevelDecl(Decl* decl) override;

        inline void PushConsumer(ASTConsumer* consumer) {
            consumers.push_back(consumer);
        }

    private:
        std::vector<ASTConsumer*> consumers{};
    };

}