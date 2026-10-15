#include "trading/OrderBook.hpp"
#include <stdexcept>
#include <utility>

namespace trading {





    void OrderBook::add(Order order) {
        if (order.type() != OrderType::Limit || !order.price().has_value()) {
            throw std::invalid_argument("only limit orders can rest in the book");
        }
        if (order.is_filled()) {
            throw std::invalid_argument("filled orders cannot rest in the book");
        }
        if (order.side() != Side::Buy && order.side() != Side::Sell) {
            throw std::invalid_argument("invalid order side");
        }

        if (contains(order.id())) {
            throw std::invalid_argument("order ID already exists in the book");
        }

        const Price price = *order.price();
        auto append = [&](auto& levels) {
            auto [level, inserted] = levels.try_emplace(price);
            try {
                level->second.push_back(std::move(order));
            } catch (...) {
                // Do not leave an empty price level if allocation fails.
                if (inserted) {
                    levels.erase(level);
                }
                throw;
            }
        };

        if (order.side() == Side::Buy) {
            append(bids_);
        } else {
            append(asks_);
        }
    }

    bool OrderBook::contains(OrderId id) const noexcept {
        const auto contains_id = [id](const auto& levels) {
            for (const auto& entry : levels) {
                for (const auto& resting_order : entry.second) {
                    if (resting_order.id() == id) {
                        return true;
                    }
                }
            }
            return false;
        };
        return contains_id(bids_) || contains_id(asks_);
    }

    void OrderBook::fill_best(Side side, Quantity quantity) {
        const auto fill = [quantity](auto& levels) {
            if (levels.empty()) {
                throw std::invalid_argument("cannot fill an empty side");
            }
            auto level = levels.begin();
            auto& orders = level->second;
            orders.front().fill(quantity);
            if (orders.front().is_filled()) {
                orders.pop_front();
                if (orders.empty()) {
                    levels.erase(level);
                }
            }
        };
        if (side == Side::Buy) {
            fill(bids_);
        } else if (side == Side::Sell) {
            fill(asks_);
        } else {
            throw std::invalid_argument("invalid order side");
        }
    }

    bool OrderBook::cancel(OrderId id) {
        const auto remove_from = [id](auto& levels) {
            for (auto level = levels.begin(); level != levels.end(); ++level) {
                auto& orders = level->second;
                for (auto order = orders.begin(); order != orders.end(); ++order) {
                    if (order->id() == id) {
                        orders.erase(order);
                        if (orders.empty()) {
                            levels.erase(level);
                        }
                        return true;
                    }
                }
            }
            return false;
        };
        return remove_from(bids_) || remove_from(asks_);
    }

    std::optional<Order> OrderBook::best_bid_order() const {
        if (bids_.empty()) {
            return std::nullopt;
        }
        return bids_.begin()->second.front();
    }

    std::optional<Order> OrderBook::best_ask_order() const {
        if (asks_.empty()) {
            return std::nullopt;
        }
        return asks_.begin()->second.front();
    }

    std::optional<Price> OrderBook::best_bid() const noexcept {
        if (bids_.empty()) {
            return std::nullopt;
        }
        return bids_.begin()->first;
    }
    std::optional<Price> OrderBook::best_ask() const noexcept {
        if (asks_.empty()) {
            return std::nullopt;
        }
        return asks_.begin()->first;
    }
} //namespace trading
