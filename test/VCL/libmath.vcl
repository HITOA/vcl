// Library imported by modules_*.vcl: templates here refer to this module's own names.

export float32 Scale = 2.0;

export float32 Twice(float32 x) {
    return x * Scale;
}

template<typename T>
export T Helper(T x) {
    return x + (T)1;
}

// Unqualified references from inside a template: a dependent template call, a non-dependent
// call and a non-dependent global. They must bind to this module, wherever it's instantiated.
template<typename T>
export T UseHelpers(T x) {
    T a = Helper<T>(x);
    float32 b = Twice((float32)2);
    return a + (T)b + (T)Scale;
}

// The workaround used by Grog's libraries: qualify calls with the module's own name.
template<typename T>
export T UseQualified(T x) {
    return libmath::Helper<T>(x);
}

// Instantiated with a struct declared by the importing module.
template<typename T>
export float32 SumFields(T s) {
    return s.a + s.b;
}
