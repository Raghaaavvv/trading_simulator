#pragma once

#include <cstdint>
#include <optional>

namespace trading {

    using OrderId = std::uint64_t;
    using Price = std::int64_t;
    using Timestamp = std::uint64_t;
    using Quantity = std::uint64_t;

    enum class Side {
        Buy,
        Sell
    };

    enum class OrderType {
        Limit,
        Market

    };

    class Order {
        public:
            Order(
                OrderId id,
                Side side,
                OrderType type,
                std::optional<Price> price,
                Quantity quantity,
                Timestamp timestamp
            );
        [[nodiscard]] OrderId id() const noexcept;
        [[nodiscard]] Side side() const noexcept;
        [[nodiscard]] OrderType type() const noexcept;
        [[nodiscard]] std::optional<Price> price() const noexcept;
        [[nodiscard]] Quantity original_quantity() const noexcept;
        [[nodiscard]] Quantity remaining_quantity() const noexcept;
        [[nodiscard]] Timestamp timestamp() const noexcept;
        [[nodiscard]] bool is_filled() const noexcept;

        void fill(Quantity quantity);
        
        private:
            OrderId id_;
            Side side_;
            OrderType type_;
            std::optional<Price> price_;
            Quantity original_quantity_;
            Quantity remaining_quantity_;
            Timestamp timestamp_;


    };

} //namespace trading