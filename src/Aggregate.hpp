#pragma once

#include <VCL/Error.hpp>

#include "Type.hpp"
#include "Value.hpp"

#include <expected>
#include <vector>


namespace VCL {

    /**
     * @brief Base class for aggregate balue.s
     */
    class Aggregate : public Value {
    public:
        Aggregate() = delete;
        Aggregate(const std::vector<Handle<Value>>& values, bool isAllConst, ModuleContext* context);
        Aggregate(const Aggregate& value) = default;
        Aggregate(Aggregate&& value) noexcept = default;
        virtual ~Aggregate() = default;

        Aggregate& operator=(const Aggregate& value) = default;
        Aggregate& operator=(Aggregate&& value) noexcept = default;


        virtual std::expected<Handle<Value>, Error> Load() override;
        
        virtual std::optional<Error> Store(Handle<Value> value) override;

        virtual std::expected<Handle<Value>, Error> Splat() override;
        
        virtual std::expected<Handle<Value>, Error> Cast(Type type) override;

        /**
         * @brief Create a new aggregate value
         */
        static std::expected<Handle<Aggregate>, Error> Create(const std::vector<Handle<Value>>& values, ModuleContext* context);

    private:
        std::vector<Handle<Value>> values;
        bool isAllConst;
    };
}