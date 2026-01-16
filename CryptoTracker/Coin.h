#pragma once
#include <string>

/**
 * @brief Represents a cryptocurrency with its market data
 *
 * This struct holds all relevant information about a single cryptocurrency
 * including its price, 24-hour change, and watchlist status.
 */
struct Coin {
    std::string id;           // CoinGecko ID (e.g., "bitcoin")
    std::string symbol;       // Trading symbol (e.g., "BTC")
    std::string name;         // Display name (e.g., "Bitcoin")
    double price;             // Current price in USD
    double change_24h;        // 24-hour percentage change
    bool in_watchlist;        // Is this coin in user's watchlist?

    /**
     * @brief Default constructor initializing all fields
     */
    Coin()
        : id(""), symbol(""), name(""), price(0.0), change_24h(0.0), in_watchlist(false) {
    }

    /**
     * @brief Parameterized constructor for creating a coin
     * @param id CoinGecko ID
     * @param symbol Trading symbol
     * @param name Display name
     */
    Coin(const std::string& id, const std::string& symbol, const std::string& name)
        : id(id), symbol(symbol), name(name), price(0.0), change_24h(0.0), in_watchlist(false) {
    }
};