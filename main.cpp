#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <sstream>
#include <iomanip>

#include <sodium.h>

// ==================== Configuration ====================
static constexpr size_t SEED_SIZE = 32;
static constexpr size_t HASH_SIZE = 64;
static constexpr size_t PUBLIC_KEY_SIZE = 32;
static constexpr size_t PRIVATE_KEY_SIZE = 64;          // MeshCore format: 32 (clamped) + 32 (hash tail)
static constexpr size_t HEX_PUBLIC_KEY_SIZE = 64;       // 32 bytes -> 64 hex chars
static constexpr size_t HEX_PRIVATE_KEY_SIZE = 128;     // 64 bytes -> 128 hex chars

// ==================== Global data ====================
static std::atomic<bool> done{ false };
static std::atomic<uint64_t> total_attempts{ 0 };
static std::mutex output_mutex;
static std::condition_variable found_cv;
static std::mutex found_mutex;

// Result storage (protected by found_mutex)
static bool key_found = false;
static std::string found_public_hex;
static std::string found_private_hex;
static uint64_t found_attempts = 0;
static double found_time_sec = 0.0;

// ==================== Utility functions ====================

// Convert binary data to lowercase hex string
std::string bytes_to_hex(const unsigned char* data, size_t len) {
    static const char* hex_chars = "0123456789abcdef";
    std::string result;
    result.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        result.push_back(hex_chars[data[i] >> 4]);
        result.push_back(hex_chars[data[i] & 0x0F]);
    }
    return result;
}

// Check if a prefix (hex string) matches the beginning of a public key hex string
bool prefix_matches(const std::string& pub_hex, const std::string& prefix) {
    if (prefix.length() > pub_hex.length()) return false;
    return std::equal(prefix.begin(), prefix.end(), pub_hex.begin(),
        [](char a, char b) { return std::tolower(a) == std::tolower(b); });
}

// Clamp the first 32 bytes of a hash according to Ed25519 RFC 8032
void clamp_scalar(unsigned char* scalar) {
    scalar[0] &= 248;
    scalar[31] &= 63;
    scalar[31] |= 64;
}

// ==================== Worker thread function ====================
void worker_thread(const std::string& target_prefix, int thread_id) {
    // Local attempt counter to reduce atomic updates
    uint64_t local_attempts = 0;
    const uint64_t update_interval = 1000;   // update global counter every 1000 attempts

    unsigned char seed[SEED_SIZE];
    unsigned char hash[HASH_SIZE];
    unsigned char scalar[32];                 // clamped scalar (first 32 bytes of hash)
    unsigned char public_key[PUBLIC_KEY_SIZE];
    unsigned char private_key[PRIVATE_KEY_SIZE];

    while (!done.load(std::memory_order_relaxed)) {
        // 1. Generate random seed
        randombytes_buf(seed, SEED_SIZE);

        // 2. SHA512(seed)
        crypto_hash_sha512(hash, seed, SEED_SIZE);

        // 3. Extract first 32 bytes and clamp
        memcpy(scalar, hash, 32);
        clamp_scalar(scalar);

        // 4. Compute public key: scalar * G (no clamping applied)
        if (crypto_scalarmult_ed25519_base_noclamp(public_key, scalar) != 0) {
            // Should never happen with a valid scalar
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cerr << "Thread " << thread_id << ": crypto_scalarmult_ed25519_base_noclamp failed" << std::endl;
            continue;
        }

        // 5. Build private key (MeshCore format)
        memcpy(private_key, scalar, 32);
        memcpy(private_key + 32, hash + 32, 32);

        // 6. Convert public key to hex
        std::string pub_hex = bytes_to_hex(public_key, PUBLIC_KEY_SIZE);

        // 7. Check prefix
        if (prefix_matches(pub_hex, target_prefix)) {
            // Found a match
            {
                std::lock_guard<std::mutex> lock(found_mutex);
                if (!key_found) {
                    key_found = true;
                    found_public_hex = pub_hex;
                    found_private_hex = bytes_to_hex(private_key, PRIVATE_KEY_SIZE);
                    found_attempts = total_attempts.load() + local_attempts + 1; // approximate
                    // time will be set by main thread
                }
            }
            done.store(true, std::memory_order_release);
            found_cv.notify_one();
            break;
        }

        // 8. Update counters
        ++local_attempts;
        if (local_attempts % update_interval == 0) {
            total_attempts.fetch_add(update_interval, std::memory_order_relaxed);
        }
    }

    // Add remaining attempts to global counter
    if (local_attempts % update_interval != 0) {
        total_attempts.fetch_add(local_attempts % update_interval, std::memory_order_relaxed);
    }
}

// ==================== Main ====================
int main(int argc, char* argv[]) {
    // -------------------- Argument parsing --------------------
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <prefix>" << std::endl;
        std::cerr << "  <prefix> : hex string of 1-8 characters" << std::endl;
        return 1;
    }

    std::string target_prefix = argv[1];
    // Basic validation
    if (target_prefix.empty() || target_prefix.length() > 8) {
        std::cerr << "Error: Prefix must be 1-8 characters long." << std::endl;
        return 1;
    }
    for (char c : target_prefix) {
        if (!isxdigit(static_cast<unsigned char>(c))) {
            std::cerr << "Error: Prefix must contain only hexadecimal characters." << std::endl;
            return 1;
        }
    }
    // Convert to lowercase for consistency
    for (char& c : target_prefix) c = std::tolower(static_cast<unsigned char>(c));

    // -------------------- Initialize libsodium --------------------
    if (sodium_init() < 0) {
        std::cerr << "Error: Failed to initialize libsodium." << std::endl;
        return 1;
    }

    // -------------------- Open log file --------------------
    std::ofstream log_file("keygen.log", std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "Warning: Could not open keygen.log for writing." << std::endl;
    }

    // -------------------- Start worker threads --------------------
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;   // fallback
    std::cout << "Starting " << num_threads << " threads, target prefix: " << target_prefix << std::endl;
    if (log_file.is_open()) {
        log_file << "Starting " << num_threads << " threads, target prefix: " << target_prefix << std::endl;
    }

    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (unsigned int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker_thread, target_prefix, i);
    }

    // -------------------- Statistics and monitoring --------------------
    auto start_time = std::chrono::steady_clock::now();
    auto last_output_time = start_time;
    uint64_t last_attempts = 0;

    // Wait for key found or timeout loop
    {
        std::unique_lock<std::mutex> lock(found_mutex);
        while (!key_found) {
            // Wait for 1 second or until notified
            found_cv.wait_for(lock, std::chrono::seconds(1), [] { return key_found; });

            auto now = std::chrono::steady_clock::now();
            auto elapsed_sec = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
            auto elapsed_since_last = std::chrono::duration_cast<std::chrono::seconds>(now - last_output_time).count();

            // Output statistics every minute (or at least 60 seconds)
            if (elapsed_since_last >= 60) {
                uint64_t attempts = total_attempts.load(std::memory_order_relaxed);
                double rate = attempts / static_cast<double>(elapsed_sec);

                std::stringstream ss;
                ss << "[" << elapsed_sec << "s] Attempts: " << attempts
                    << " | Speed: " << std::fixed << std::setprecision(1) << rate << " keys/s";

                {
                    std::lock_guard<std::mutex> out_lock(output_mutex);
                    std::cout << ss.str() << std::endl;
                    if (log_file.is_open()) {
                        log_file << ss.str() << std::endl;
                    }
                }

                last_output_time = now;
                last_attempts = attempts;
            }

            // If key found, break out
            if (key_found) break;
        }
    }

    // -------------------- Key found, finalize --------------------
    auto end_time = std::chrono::steady_clock::now();
    double total_time_sec = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() / 1000.0;

    // Wait for all threads to finish (they will exit because done == true)
    for (auto& t : threads) {
        t.join();
    }

    // Get final attempt count (if not set precisely in found block)
    uint64_t final_attempts = total_attempts.load(std::memory_order_relaxed);
    if (key_found) {
        // The found_attempts is approximate; we can refine it
        // (but it's okay for display)
    }

    // -------------------- Output result --------------------
    if (key_found) {
        std::stringstream result_ss;
        result_ss << "\n=== Key Found ===\n";
        result_ss << "Public key  (hex): " << found_public_hex << "\n";
        result_ss << "Private key (hex): " << found_private_hex << "\n";
        result_ss << "Attempts: " << final_attempts << "\n";
        result_ss << "Time: " << std::fixed << std::setprecision(2) << total_time_sec << " seconds\n";
        result_ss << "Average speed: " << std::fixed << std::setprecision(1)
            << (final_attempts / total_time_sec) << " keys/s\n";

        // Console output
        {
            std::lock_guard<std::mutex> out_lock(output_mutex);
            std::cout << result_ss.str() << std::endl;
        }

        // File output
        if (log_file.is_open()) {
            log_file << result_ss.str() << std::endl;
        }
    }
    else {
        std::cerr << "Key generation stopped unexpectedly." << std::endl;
    }

    return 0;
}