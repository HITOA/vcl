#pragma once


namespace VCL {

    class Decl;

    class DeclContext {
    public:
        class Iterator {
        public:
            Iterator() = delete;
            Iterator(Decl* decl) : decl{ decl } {}
            Iterator(const Iterator& other) = default;
            Iterator(Iterator&& other) = default;
            ~Iterator() = default;
            
            Iterator& operator=(const Iterator& other) = default;
            Iterator& operator=(Iterator&& other) = default;

            Iterator& operator++();
            Iterator operator++(int);

            inline bool operator==(const Iterator& rhs) const { return decl == rhs.decl; }
            inline bool operator!=(const Iterator& rhs) const { return !(*this == rhs); }
            
            inline Decl* operator->() { return decl; }
            inline Decl& operator*() { return *decl; }
            
            inline Decl* Get() { return decl; }

        private:
            Decl* decl = nullptr;
        }; 

    public:
        virtual ~DeclContext() {};    

        void InsertFront(Decl* decl);
        void InsertBack(Decl* decl);

        inline Iterator Begin() { return Iterator{ first }; }
        inline Iterator End() { return Iterator{ nullptr }; } 

    private:
        Decl* first = nullptr;
        Decl* last = nullptr;
    };

}