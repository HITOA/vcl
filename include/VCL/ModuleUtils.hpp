#pragma once


namespace VCL {

    struct ModuleDebugInformationSettings {
        bool generateDebugInformation = false;
        bool optimized = false;
    };

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