#pragma once


namespace VCL {
    class CompilerInstance;
    
    class FrontendAction {
    public:
        FrontendAction() = default;
        FrontendAction(const FrontendAction& other) = delete;
        FrontendAction(FrontendAction&& other) = delete;
        virtual ~FrontendAction() = default;

        FrontendAction& operator=(const FrontendAction& other) = delete;
        FrontendAction& operator=(FrontendAction&& other) = delete;

        inline bool Prepare(CompilerInstance* instance) {
            this->instance = instance;
            return true;
        }

        virtual bool Execute() = 0;

    protected:
        CompilerInstance* instance;
    };

}