#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <limits>
#include <filesystem>
#include <algorithm>
#include <functional>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────
//  Simple hash (FNV-1a 64-bit) — no OpenSSL needed
// ─────────────────────────────────────────────
std::string hashPassword(const std::string& password) {
    uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : password) {
        hash ^= c;
        hash *= 1099511628211ULL;
    }
    // Convert to hex string
    std::ostringstream oss;
    oss << std::hex << hash;
    return oss.str();
}

// ─────────────────────────────────────────────
//  Input helpers
// ─────────────────────────────────────────────
void clearInput() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string getHiddenInput(const std::string& prompt) {
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

// ─────────────────────────────────────────────
//  Validation
// ─────────────────────────────────────────────
bool isValidUsername(const std::string& username) {
    if (username.length() < 3 || username.length() > 20) return false;
    for (char c : username)
        if (!std::isalnum(c) && c != '_') return false;
    return true;
}

bool isValidPassword(const std::string& password) {
    if (password.length() < 6) return false;
    bool hasUpper = false, hasDigit = false;
    for (char c : password) {
        if (std::isupper(c)) hasUpper = true;
        if (std::isdigit(c)) hasDigit = true;
    }
    return hasUpper && hasDigit;
}

// ─────────────────────────────────────────────
//  File helpers
// ─────────────────────────────────────────────
const std::string DB_DIR = "users/";

void ensureDirectory() {
    if (!fs::exists(DB_DIR))
        fs::create_directory(DB_DIR);
}

std::string userFilePath(const std::string& username) {
    return DB_DIR + username + ".dat";
}

bool userExists(const std::string& username) {
    return fs::exists(userFilePath(username));
}

bool saveUser(const std::string& username, const std::string& passwordHash) {
    ensureDirectory();
    std::ofstream file(userFilePath(username));
    if (!file.is_open()) return false;
    file << "username:" << username << "\n";
    file << "password:" << passwordHash << "\n";
    return true;
}

bool loadUser(const std::string& username, std::string& storedHash) {
    std::ifstream file(userFilePath(username));
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("password:", 0) == 0)
            storedHash = line.substr(9);
    }
    return !storedHash.empty();
}

// ─────────────────────────────────────────────
//  Banner
// ─────────────────────────────────────────────
void printBanner() {
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════╗\n";
    std::cout << "  ║     USER AUTHENTICATION SYSTEM       ║\n";
    std::cout << "  ╚══════════════════════════════════════╝\n\n";
}

// ─────────────────────────────────────────────
//  Register
// ─────────────────────────────────────────────
void registerUser() {
    std::cout << "\n  ── REGISTER ──────────────────────────\n\n";

    std::string username;
    std::cout << "  Username (3-20 chars, alphanumeric/_): ";
    std::getline(std::cin, username);

    // Trim whitespace
    username.erase(0, username.find_first_not_of(" \t"));
    username.erase(username.find_last_not_of(" \t") + 1);

    if (!isValidUsername(username)) {
        std::cout << "\n  [ERROR] Invalid username.\n"
                  << "         - Must be 3-20 characters\n"
                  << "         - Only letters, digits, and underscores allowed\n\n";
        return;
    }

    if (userExists(username)) {
        std::cout << "\n  [ERROR] Username '" << username << "' is already taken.\n\n";
        return;
    }

    std::string password = getHiddenInput("  Password (min 6 chars, 1 uppercase, 1 digit): ");
    if (!isValidPassword(password)) {
        std::cout << "\n  [ERROR] Password too weak.\n"
                  << "         - Minimum 6 characters\n"
                  << "         - At least one uppercase letter\n"
                  << "         - At least one digit\n\n";
        return;
    }

    std::string confirm = getHiddenInput("  Confirm password: ");
    if (password != confirm) {
        std::cout << "\n  [ERROR] Passwords do not match.\n\n";
        return;
    }

    std::string hash = hashPassword(password);
    if (saveUser(username, hash)) {
        std::cout << "\n  [SUCCESS] Account created for '" << username << "'!\n"
                  << "            You can now log in.\n\n";
    } else {
        std::cout << "\n  [ERROR] Failed to save account. Check file permissions.\n\n";
    }
}

// ─────────────────────────────────────────────
//  Login
// ─────────────────────────────────────────────
void loginUser() {
    std::cout << "\n  ── LOGIN ─────────────────────────────\n\n";

    std::string username;
    std::cout << "  Username: ";
    std::getline(std::cin, username);

    username.erase(0, username.find_first_not_of(" \t"));
    username.erase(username.find_last_not_of(" \t") + 1);

    if (!userExists(username)) {
        std::cout << "\n  [ERROR] No account found for '" << username << "'.\n\n";
        return;
    }

    std::string password = getHiddenInput("  Password: ");
    std::string enteredHash = hashPassword(password);

    std::string storedHash;
    if (!loadUser(username, storedHash)) {
        std::cout << "\n  [ERROR] Could not read account data.\n\n";
        return;
    }

    if (enteredHash == storedHash) {
        std::cout << "\n  ╔══════════════════════════════════════╗\n";
        std::cout << "  ║  LOGIN SUCCESSFUL! Welcome, " << username;
        // Padding
        int padding = 12 - (int)username.length();
        for (int i = 0; i < padding; i++) std::cout << ' ';
        std::cout << "║\n";
        std::cout << "  ╚══════════════════════════════════════╝\n\n";
    } else {
        std::cout << "\n  [ERROR] Incorrect password.\n\n";
    }
}

// ─────────────────────────────────────────────
//  Main menu
// ─────────────────────────────────────────────
int main() {
    printBanner();

    while (true) {
        std::cout << "  Select an option:\n";
        std::cout << "    [1] Register\n";
        std::cout << "    [2] Login\n";
        std::cout << "    [3] Exit\n\n";
        std::cout << "  > ";

        std::string choice;
        std::getline(std::cin, choice);

        if (choice == "1") {
            registerUser();
        } else if (choice == "2") {
            loginUser();
        } else if (choice == "3") {
            std::cout << "\n  Goodbye!\n\n";
            break;
        } else {
            std::cout << "\n  [ERROR] Invalid option. Please enter 1, 2, or 3.\n\n";
        }
    }

    return 0;
}