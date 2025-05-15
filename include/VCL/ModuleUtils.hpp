#pragma once


namespace VCL {

    struct ModuleOptimizerSettings {
        bool enableLoopUnrolling = true;
        bool enableInliner = true;
        bool enableVectorizer = true;
    };

    struct ModuleVerifierSettings {
        bool brokenDebugInformationAsError = false;
        bool enableSelectRecursionCheck = true;
        bool selectRecursionAsError = false;
    };

}