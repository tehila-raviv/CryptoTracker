#include "PriceManager.h"
#include <json.hpp>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

using json = nlohmann::json;
namespace fs = std::filesystem;

// Helper function to make HTTP GET request using WinHTTP
std::string HttpGet(const std::wstring& server, const std::wstring& path) {
    std::string response;

    HINTERNET hSession = WinHttpOpen(L"CryptoTracker/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    if (!hSession) return "";

    HINTERNET hConnect = WinHttpConnect(hSession, server.c_str(),
        INTERNET_DEFAULT_HTTP_PORT, 0);

    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return "";
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0);

    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    BOOL bResults = WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0,
        0, 0);

    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, NULL);
    }

    if (bResults) {
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;

        do {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                break;
            }

            char* pszOutBuffer = new char[dwSize + 1];
            ZeroMemory(pszOutBuffer, dwSize + 1);

            if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer,
                dwSize, &dwDownloaded)) {
                delete[] pszOutBuffer;
                break;
            }

            response.append(pszOutBuffer, dwDownloaded);
            delete[] pszOutBuffer;

        } while (dwSize > 0);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return response;
}

PriceManager::PriceManager() : should_stop(false), is_connected(false) {
    InitializeCoins();
    LoadWatchlist();

    // Start background thread for periodic updates
    update_thread = std::thread(&PriceManager::UpdateThreadFunc, this);
}

PriceManager::~PriceManager() {
    // Signal thread to stop
    should_stop.store(true);

    // Wait for thread to finish
    if (update_thread.joinable()) {
        update_thread.join();
    }

    // Save watchlist before exit
    {
        std::lock_guard<std::mutex> lock(data_mutex);
        SaveWatchlist();
    }
}

void PriceManager::InitializeCoins() {
    // Initialize with 20 popular cryptocurrencies
    coins = {
        Coin("bitcoin", "BTC", "Bitcoin"),
        Coin("ethereum", "ETH", "Ethereum"),
        Coin("tether", "USDT", "Tether"),
        Coin("binancecoin", "BNB", "BNB"),
        Coin("solana", "SOL", "Solana"),
        Coin("ripple", "XRP", "XRP"),
        Coin("usd-coin", "USDC", "USD Coin"),
        Coin("cardano", "ADA", "Cardano"),
        Coin("dogecoin", "DOGE", "Dogecoin"),
        Coin("tron", "TRX", "TRON"),
        Coin("avalanche-2", "AVAX", "Avalanche"),
        Coin("polkadot", "DOT", "Polkadot"),
        Coin("chainlink", "LINK", "Chainlink"),
        Coin("shiba-inu", "SHIB", "Shiba Inu"),
        Coin("bitcoin-cash", "BCH", "Bitcoin Cash"),
        Coin("litecoin", "LTC", "Litecoin"),
        Coin("polygon", "MATIC", "Polygon"),
        Coin("uniswap", "UNI", "Uniswap"),
        Coin("stellar", "XLM", "Stellar"),
        Coin("monero", "XMR", "Monero")
    };
}

std::vector<Coin>& PriceManager::GetCoins() {
    return coins;
}

std::vector<Coin> PriceManager::GetWatchlistCoins() {
    std::lock_guard<std::mutex> lock(data_mutex);
    std::vector<Coin> watchlist;

    for (const auto& coin : coins) {
        if (coin.in_watchlist) {
            watchlist.push_back(coin);
        }
    }

    return watchlist;
}

void PriceManager::AddToWatchlist(const std::string& coinId) {
    std::lock_guard<std::mutex> lock(data_mutex);

    for (auto& coin : coins) {
        if (coin.id == coinId) {
            coin.in_watchlist = true;
            break;
        }
    }
    // Don't save here - will save on app close
}

void PriceManager::RemoveFromWatchlist(const std::string& coinId) {
    std::lock_guard<std::mutex> lock(data_mutex);

    for (auto& coin : coins) {
        if (coin.id == coinId) {
            coin.in_watchlist = false;
            break;
        }
    }
    // Don't save here - will save on app close
}

void PriceManager::UpdatePrices() {
    FetchPricesFromAPI();
}

std::string PriceManager::GetLastUpdateTime() const {
    return last_update_time;
}

void PriceManager::UpdateThreadFunc() {
    // Perform initial update
    FetchPricesFromAPI();

    // Loop until stop signal
    while (!should_stop.load()) {
        // Sleep for UPDATE_INTERVAL_SEC seconds
        for (int i = 0; i < UPDATE_INTERVAL_SEC && !should_stop.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        if (!should_stop.load()) {
            FetchPricesFromAPI();
        }
    }
}

bool PriceManager::FetchPricesFromAPI() {
    try {
        // Build comma-separated list of coin IDs
        std::string ids;
        {
            std::lock_guard<std::mutex> lock(data_mutex);
            for (size_t i = 0; i < coins.size(); ++i) {
                ids += coins[i].id;
                if (i < coins.size() - 1) ids += ",";
            }
        }

        // Build request path
        std::string pathStr = "/api/v3/simple/price?ids=" + ids +
            "&vs_currencies=usd&include_24hr_change=true";
        std::wstring path(pathStr.begin(), pathStr.end());

        // Make HTTP request using WinHTTP
        std::string responseBody = HttpGet(L"api.coingecko.com", path);

        if (responseBody.empty()) {
            std::cerr << "HTTP request failed!" << std::endl;
            is_connected.store(false);
            return false;
        }

        // Parse JSON response
        json data = json::parse(responseBody);

        // Update coin prices
        {
            std::lock_guard<std::mutex> lock(data_mutex);

            for (auto& coin : coins) {
                if (data.contains(coin.id)) {
                    auto coin_data = data[coin.id];

                    if (coin_data.contains("usd")) {
                        coin.price = coin_data["usd"].get<double>();
                    }

                    if (coin_data.contains("usd_24h_change")) {
                        coin.change_24h = coin_data["usd_24h_change"].get<double>();
                    }
                }
            }

            // Update timestamp
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time), "%H:%M:%S");
            last_update_time = ss.str();
        }

        is_connected.store(true);
        std::cout << "Prices updated successfully at " << last_update_time << std::endl;
        return true;

    }
    catch (const std::exception& e) {
        std::cerr << "Error fetching prices: " << e.what() << std::endl;
        is_connected.store(false);
        return false;
    }
}

void PriceManager::SaveWatchlist() {
    try {
        if (!fs::exists("data")) {
            fs::create_directory("data");
        }

        json watchlist_json = json::array();

        std::lock_guard<std::mutex> lock(data_mutex);

        for (const auto& coin : coins) {
            if (coin.in_watchlist) {
                watchlist_json.push_back(coin.id);
            }
        }

        std::ofstream file("data/watchlist.json");
        if (file.is_open()) {
            file << watchlist_json.dump(4);
            file.close();
            std::cout << "Watchlist saved successfully" << std::endl;
        }

    }
    catch (const std::exception& e) {
        std::cerr << "Error saving watchlist: " << e.what() << std::endl;
    }
}

void PriceManager::LoadWatchlist() {
    try {
        if (!fs::exists("data/watchlist.json")) {
            std::cout << "No watchlist file found, starting fresh" << std::endl;
            return;
        }

        std::ifstream file("data/watchlist.json");
        if (!file.is_open()) {
            return;
        }

        json watchlist_json;
        file >> watchlist_json;
        file.close();

        std::lock_guard<std::mutex> lock(data_mutex);

        for (const auto& coin_id : watchlist_json) {
            std::string id = coin_id.get<std::string>();

            for (auto& coin : coins) {
                if (coin.id == id) {
                    coin.in_watchlist = true;
                    break;
                }
            }
        }

        std::cout << "Watchlist loaded successfully" << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "Error loading watchlist: " << e.what() << std::endl;
    }
}