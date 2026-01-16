#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>
#include <atomic>
#include <thread>
#include "Coin.h"

/**
 * @brief Manages cryptocurrency price data and API interactions
 *
 * This class handles:
 * - Fetching live price data from CoinGecko API using httplib
 * - Managing the list of available coins
 * - Background thread for periodic price updates
 * - Thread-safe access to shared price data using mutex
 * - Saving/loading user's watchlist to/from file using fstream
 */
class PriceManager {
public:
    /**
     * @brief Constructor initializes coins and starts background thread
     */
    PriceManager();

    /**
     * @brief Destructor stops background thread and cleans up
     */
    ~PriceManager();

    /**
     * @brief Get reference to all coins (thread-safe)
     * @return Vector of all available coins
     */
    std::vector<Coin>& GetCoins();

    /**
     * @brief Get coins that are in the watchlist
     * @return Vector of watchlist coins
     */
    std::vector<Coin> GetWatchlistCoins();

    /**
     * @brief Add a coin to the watchlist
     * @param coinId CoinGecko ID of the coin
     */
    void AddToWatchlist(const std::string& coinId);

    /**
     * @brief Remove a coin from the watchlist
     * @param coinId CoinGecko ID of the coin
     */
    void RemoveFromWatchlist(const std::string& coinId);

    /**
     * @brief Manually trigger a price update
     */
    void UpdatePrices();

    /**
     * @brief Check if connection to API is healthy
     * @return true if last update was successful
     */
    bool IsConnected() const { return is_connected.load(); }

    /**
     * @brief Get the last update timestamp
     * @return String with last update time
     */
    std::string GetLastUpdateTime() const;

    /**
     * @brief Save watchlist to file
     */
    void SaveWatchlist();

    /**
     * @brief Load watchlist from file
     */
    void LoadWatchlist();

    /**
     * @brief Lock for accessing coin data from UI thread
     */
    std::mutex& GetMutex() { return data_mutex; }

private:
    /**
     * @brief Initialize the list of popular cryptocurrencies
     */
    void InitializeCoins();

    /**
     * @brief Background thread function for periodic updates
     */
    void UpdateThreadFunc();

    /**
     * @brief Fetch prices from CoinGecko API
     * @return true if successful
     */
    bool FetchPricesFromAPI();

    std::vector<Coin> coins;                    // List of all available coins
    std::mutex data_mutex;                      // Protects shared data access
    std::atomic<bool> should_stop;              // Signal to stop background thread
    std::atomic<bool> is_connected;             // Connection status
    std::thread update_thread;                  // Background update thread
    std::string last_update_time;               // Timestamp of last update
    static constexpr int UPDATE_INTERVAL_SEC = 30; // Update every 30 seconds
};