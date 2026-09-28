// A library template whose body only fails once instantiated.
template<typename T>
export T Broken(T x) {
    return Missing(x);
}
