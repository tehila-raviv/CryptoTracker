#pragma once
#include "PriceManager.h"
#include <memory>

/**
 * @brief Handles the ImGui user interface for the crypto tracker
 *
 * This class manages:
 * - Main window layout and rendering
 * - Watchlist display with add/remove functionality
 * - All coins table with search and filter
 * - Color-coded price changes (green=up, red=down)
 * - Connection status indicator
 */
class CryptoUI {
public:
    /**
     * @brief Constructor
     * @param manager Shared pointer to PriceManager
     */
    CryptoUI(std::shared_ptr<PriceManager> manager);

    /**
     * @brief Render the entire UI (called every frame)
     */
    void Render();

private:
    /**
     * @brief Render the watchlist section
     */
    void RenderWatchlist();

    /**
     * @brief Render the all coins table with search
     */
    void RenderAllCoins();

    /**
     * @brief Render the status bar at the bottom
     */
    void RenderStatusBar();

    /**
     * @brief Helper to format price with proper decimals
     * @param price The price value
     * @return Formatted string
     */
    std::string FormatPrice(double price);

    /**
     * @brief Helper to format percentage change
     * @param change The change value
     * @return Formatted string with sign
     */
    std::string FormatChange(double change);

    std::shared_ptr<PriceManager> price_manager;
    char search_buffer[256];                // Buffer for search input
    bool show_only_watchlist;               // Filter flag
};