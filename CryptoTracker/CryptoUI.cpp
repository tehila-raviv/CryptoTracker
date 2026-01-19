#include "CryptoUI.h"
#include <imgui.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>

CryptoUI::CryptoUI(std::shared_ptr<PriceManager> manager)
    : price_manager(manager), show_only_watchlist(false) {
    memset(search_buffer, 0, sizeof(search_buffer));
}

void CryptoUI::Render() {
    // Main window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGui::Begin("Crypto Portfolio Tracker", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Title
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::Text("Cryptocurrency Portfolio Tracker");
    ImGui::PopFont();
    ImGui::Separator();

    // Layout: Watchlist on left, All Coins on right
    ImGui::Columns(2, "MainColumns", true);
    ImGui::SetColumnWidth(0, 400);

    RenderWatchlist();

    ImGui::NextColumn();

    RenderAllCoins();

    ImGui::Columns(1);

    ImGui::Separator();
    RenderStatusBar();

    ImGui::End();
}

void CryptoUI::RenderWatchlist() {
    ImGui::Text("My Watchlist");
    ImGui::Separator();

    // COPY watchlist data first
    std::vector<Coin> watchlist = price_manager->GetWatchlistCoins();
    // No lock held during rendering!

    if (watchlist.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
            "No coins in watchlist.\nAdd coins from the right panel.");
        return;
    }

    // Table for watchlist
    if (ImGui::BeginTable("WatchlistTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("24h Change", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableHeadersRow();

        for (const auto& coin : watchlist) {
            ImGui::TableNextRow();

            // Symbol
            ImGui::TableNextColumn();
            ImGui::Text("%s", coin.symbol.c_str());

            // Price
            ImGui::TableNextColumn();
            ImGui::Text("%s", FormatPrice(coin.price).c_str());

            // 24h Change (color-coded)
            ImGui::TableNextColumn();
            ImVec4 color = coin.change_24h >= 0 ?
                ImVec4(0.0f, 1.0f, 0.0f, 1.0f) :  // Green
                ImVec4(1.0f, 0.0f, 0.0f, 1.0f);   // Red
            ImGui::TextColored(color, "%s", FormatChange(coin.change_24h).c_str());

            // Remove button
            ImGui::TableNextColumn();
            std::string button_label = "Remove##" + coin.id;
            if (ImGui::Button(button_label.c_str())) {
                price_manager->RemoveFromWatchlist(coin.id);
            }
        }

        ImGui::EndTable();
    }

    // Calculate total
    ImGui::Separator();
    ImGui::Text("Total Coins: %d", (int)watchlist.size());
}

void CryptoUI::RenderAllCoins() {
    ImGui::Text("All Cryptocurrencies");
    ImGui::Separator();

    // Search bar
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("Search", search_buffer, sizeof(search_buffer));
    ImGui::SameLine();

    ImGui::Checkbox("Show only watchlist", &show_only_watchlist);
    ImGui::SameLine();

    if (ImGui::Button("Refresh Now")) {
        price_manager->UpdatePrices();
    }

    ImGui::Separator();

    // COPY coin data before rendering - don't hold lock during UI interaction
    std::vector<Coin> coins_copy;
    {
        std::lock_guard<std::mutex> lock(price_manager->GetMutex());
        coins_copy = price_manager->GetCoins();
    }
    // Lock is released here!

    // Table for all coins
    if (ImGui::BeginTable("AllCoinsTable", 5,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY, ImVec2(0, 400))) {

        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_WidthFixed, 70);
        ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("24h Change", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableHeadersRow();

        std::string search_term(search_buffer);
        std::transform(search_term.begin(), search_term.end(), search_term.begin(), ::tolower);

        for (const auto& coin : coins_copy) {  // Use the copy, not the reference!
            // Apply filters
            if (show_only_watchlist && !coin.in_watchlist) {
                continue;
            }

            if (!search_term.empty()) {
                std::string name_lower = coin.name;
                std::string symbol_lower = coin.symbol;
                std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
                std::transform(symbol_lower.begin(), symbol_lower.end(), symbol_lower.begin(), ::tolower);

                if (name_lower.find(search_term) == std::string::npos &&
                    symbol_lower.find(search_term) == std::string::npos) {
                    continue;
                }
            }

            ImGui::TableNextRow();

            // Name
            ImGui::TableNextColumn();
            ImGui::Text("%s", coin.name.c_str());

            // Symbol
            ImGui::TableNextColumn();
            ImGui::Text("%s", coin.symbol.c_str());

            // Price
            ImGui::TableNextColumn();
            ImGui::Text("%s", FormatPrice(coin.price).c_str());

            // 24h Change (color-coded)
            ImGui::TableNextColumn();
            ImVec4 color = coin.change_24h >= 0 ?
                ImVec4(0.0f, 1.0f, 0.0f, 1.0f) :  // Green
                ImVec4(1.0f, 0.0f, 0.0f, 1.0f);   // Red
            ImGui::TextColored(color, "%s", FormatChange(coin.change_24h).c_str());

            // Add/Remove button - NO LOCK HELD HERE!
            ImGui::TableNextColumn();
            if (coin.in_watchlist) {
                std::string button_label = "Remove##" + coin.id;
                if (ImGui::Button(button_label.c_str())) {
                    price_manager->RemoveFromWatchlist(coin.id);
                }
            }
            else {
                std::string button_label = "Add##" + coin.id;
                if (ImGui::Button(button_label.c_str())) {
                    price_manager->AddToWatchlist(coin.id);
                }
            }
        }

        ImGui::EndTable();
    }
}

void CryptoUI::RenderStatusBar() {
    // Connection status
    bool connected = price_manager->IsConnected();
    ImVec4 status_color = connected ?
        ImVec4(0.0f, 1.0f, 0.0f, 1.0f) :  // Green
        ImVec4(1.0f, 0.0f, 0.0f, 1.0f);   // Red

    ImGui::TextColored(status_color, connected ? "Connected" : "Disconnected");
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    ImGui::Text("Last Update: %s", price_manager->GetLastUpdateTime().c_str());
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
    ImGui::Text("Auto-refresh: 30s");
}

std::string CryptoUI::FormatPrice(double price) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << "$" << price;
    return ss.str();
}

std::string CryptoUI::FormatChange(double change) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    if (change >= 0) {
        ss << "+";
    }
    ss << change << "%";
    return ss.str();
}