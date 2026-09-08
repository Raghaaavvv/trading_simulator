#include "trading/Order.hpp"

#include <stdexcept>

namespace trading {

Order::Order(
    OrderId id,
    Side side,
    OrderType type,
    std::optional<Price> price,
    Quantity quantity,
    Timestamp timestamp
)
    : id_(id),
      side_(side),
      type_(type),
      price_(price),
      original_quantity_(quantity),
      remaining_quantity_(quantity),
      timestamp_(timestamp) {
    if (id_ == 0) {
        throw std::invalid_argument("order ID must be greater than zero");
    }

    if (original_quantity_ == 0) {
        throw std::invalid_argument("order quantity must be greater than zero");
    }

    if (type_ == OrderType::Limit) {
        if (!price_.has_value()) {
            throw std::invalid_argument("limit order requires a price");
        }

        if (*price_ <= 0) {
            throw std::invalid_argument("limit price must be greater than zero");
        }
    }

    if (type_ == OrderType::Market && price_.has_value()) {
        throw std::invalid_argument("market order cannot have a price");
    }

} // constructor ends

OrderId Order::id() const noexcept {
    return id_;
}

Side Order::side() const noexcept {
    return side_;
}

OrderType Order::type() const noexcept {
    return type_;
}

std::optional<Price> Order::price() const noexcept {
    return price_;
}

Quantity Order::original_quantity() const noexcept {
    return original_quantity_;
}

Quantity Order::remaining_quantity() const noexcept {
    return remaining_quantity_;
}

Timestamp Order::timestamp() const noexcept {
    return timestamp_;
}

bool Order::is_filled() const noexcept {
    return remaining_quantity_ == 0;
}

    void Order::fill(Quantity quantity) {
        if (quantity == 0) {
            throw std::invalid_argument(
                "fill quantity must be greater than zero"
            );
        
        }

        if (quantity > remaining_quantity_) {
            throw std::invalid_argument(
                "fill quantity exceeds remaining quantity"
            );
        }

        remaining_quantity_ -= quantity;
    }




} // namespace trading
