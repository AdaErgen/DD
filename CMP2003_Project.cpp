#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>
#include <algorithm>
#include <queue>
#include <cstdlib> // For system-specific functions

#ifdef _WIN32
#include <ShlObj.h> // For SHGetFolderPathA on Windows
#pragma comment(lib, "Shell32.lib")
#endif

// Node structure for each element in the hash table
struct HashNode {
    std::string key;
    int visits;
    HashNode* next; // For handling collisions
};

// Hash table class
class HashTable {
private:
    std::vector<HashNode*> table; // Vector to store pointers to HashNode, represents the hash table
    size_t size; // Size of the hash table

    // Improved hash function to compute index for a key
    size_t hashFunction(const std::string& key) {
        size_t hash = 5381;
        for (char ch : key) {
            hash = ((hash << 5) + hash) + ch; // Bitwise operation to compute hash value
        }
        return hash % size; // Ensure the hash value fits within the table size
    }

public:
    // Constructor to initialize the hash table with a given size
    HashTable(size_t initialSize) : size(initialSize) {
        table.resize(size, nullptr); // Allocate memory for the hash table
    }

    // Destructor to properly deallocate memory, avoiding memory leaks
    ~HashTable() {
        for (auto& head : table) {
            HashNode* current = head;
            while (current != nullptr) {
                HashNode* temp = current;
                current = current->next; // Traverse the linked list at each index
                delete temp; // Free memory for each node
            }
        }
    }

    // Insert a key into the hash table
    void insert(const std::string& key) {
        size_t index = hashFunction(key); // Compute the hash index
        HashNode* current = table[index]; // Access the chain (linked list) at the hash index
        // Search for the key in the chain
        while (current != nullptr) {
            if (current->key == key) {
                current->visits++; // Increment visit count if key is found
                return;
            }
            current = current->next; // Move to the next node in the chain    
        }
        // If key not found, create a new node and add it to the front of the chain
        HashNode* newNode = new HashNode{ key, 1, table[index] };
        table[index] = newNode;
    }

    // Find and return the top 10 most visited pages
    std::vector<std::pair<std::string, int>> getTop10() const {
        std::priority_queue<std::pair<int, std::string>> maxHeap; // Max heap to sort pages by visit count
        // Iterate through all buckets in the hash table
        for (const auto& bucket : table) {
            HashNode* current = bucket;
            // Push each node's data into the max heap
            while (current != nullptr) {
                maxHeap.push({ current->visits, current->key });
                current = current->next;
            }
        }
        // Extract the top 10 entries from the max heap
        std::vector<std::pair<std::string, int>> topPages;
        for (int i = 0; i < 10 && !maxHeap.empty(); ++i) {
            topPages.emplace_back(maxHeap.top().second, maxHeap.top().first);
            maxHeap.pop();
        }
        return topPages; // Return the sorted top pages
    }
};

// Extracts the filename from a log line. It looks for the pattern "GET <filename> HTTP/1.0".
std::string extractFilename(const std::string& logLine) {
    size_t startPos = logLine.find("GET ") + 4; // Identifies the start of the filename.
    size_t endPos = logLine.find(" HTTP/1.0");  // Identifies the end of the filename.
    // Extracts and returns the filename if both start and end patterns are found.
    if (startPos != std::string::npos && endPos != std::string::npos) {
        return logLine.substr(startPos, endPos - startPos);
    }
    return ""; // Returns an empty string if the pattern is not found.
}

// Processes a log file, updating both a custom hash table and an unordered_map with visit counts.
void processLogFile(const std::string& filePath, HashTable& hashTable, std::unordered_map<std::string, int>& umap) {
    std::ifstream logFile(filePath); // Opens the log file.
    std::string line;
    // Reads each line of the log file.
    while (getline(logFile, line)) {
        // Extracts the filename directly from the log line.
        std::string filename = extractFilename(line);
        if (!filename.empty()) {
            hashTable.insert(filename); // Updates the custom hash table.
            umap[filename]++;           // Updates the unordered_map.
        }
    }

    // Print the top 10 pages and their visit counts from the custom hash table
    auto topPagesCustom = hashTable.getTop10();
    std::cout << "Top 10 Most Visited Pages from Custom Hash Table:" << std::endl;
    for (const auto& page : topPagesCustom) {
        std::cout << page.first << " - " << page.second << " visits" << std::endl;
    }
}

// Gets the top 10 most visited pages from an unordered_map.
std::vector<std::pair<std::string, int>> getTop10FromUnorderedMap(const std::unordered_map<std::string, int>& umap) {
    std::priority_queue<std::pair<int, std::string>> maxHeap; // Max heap to sort pages by visit count.
    // Pushes all entries into the max heap.
    for (const auto& entry : umap) {
        maxHeap.push({ entry.second, entry.first });
    }
    // Extracts the top 10 entries.
    std::vector<std::pair<std::string, int>> topPages;
    for (int i = 0; i < 10 && !maxHeap.empty(); ++i) {
        topPages.emplace_back(maxHeap.top().second, maxHeap.top().first);
        maxHeap.pop();
    }
    return topPages; // Returns the sorted top pages.
}

// Function to get the desktop path in a platform-independent way
std::string getDesktopPath() {
#ifdef _WIN32
    // Windows
    char desktopPath[MAX_PATH];
    if (SHGetFolderPathA(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0, desktopPath) == S_OK) {
        return std::string(desktopPath) + "\\";
    }
#else
    // Assume Unix-like systems
    const char* desktopPath = getenv("HOME");
    if (desktopPath != nullptr) {
        return std::string(desktopPath) + "/Desktop/";
    }
#endif
    // Return an empty string if the desktop path cannot be determined
    return "";
}

int main() {
    HashTable customHashTable(10000);  
    std::unordered_map<std::string, int> fileVisits;  

    // Get desktop path
    std::string desktopPath = getDesktopPath();

    if (desktopPath.empty()) {
        std::cerr << "Error: Could not determine desktop path." << std::endl;
        return 1;
    }

    // Constructing the full file path based on your information
    std::string accessLogPath = desktopPath + "access_log";

    // Prompt user for access_log file or use default if it exists
    std::cout << "Enter the access_log file name or press Enter to use the default ('access_log'): ";
    std::string userInput;
    std::getline(std::cin, userInput);

    if (!userInput.empty()) {
        accessLogPath = desktopPath + userInput;
    }

    // Start timing for custom hash table
    auto startCustom = std::chrono::high_resolution_clock::now();
    processLogFile(accessLogPath, customHashTable, fileVisits);
    auto endCustom = std::chrono::high_resolution_clock::now();

    // Start timing for unordered_map
    auto startUnorderedMap = std::chrono::high_resolution_clock::now();
    auto topPagesUnorderedMap = getTop10FromUnorderedMap(fileVisits);
    auto endUnorderedMap = std::chrono::high_resolution_clock::now();

    // Print top 10 pages from unordered_map
    std::cout << "\nTop 10 Most Visited Pages from std::unordered_map:" << std::endl;
    for (const auto& page : topPagesUnorderedMap) {
        std::cout << page.first << " - " << page.second << " visits" << std::endl;
    }

    // Output timing results
    auto durationCustom = std::chrono::duration_cast<std::chrono::milliseconds>(endCustom - startCustom).count();
    auto durationUnorderedMap = std::chrono::duration_cast<std::chrono::milliseconds>(endUnorderedMap - startUnorderedMap).count();
    std::cout << "\nCustom Hash Table processing time: " << durationCustom << "ms\n";
    std::cout << "std::unordered_map processing time: " << durationUnorderedMap << "ms\n";

    return 0;
}
