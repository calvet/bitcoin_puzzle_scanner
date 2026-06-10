#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

#include "config.h"
#include "puzzles.h"
#include "types.h"
#include "scanner_engine.h"
#include <iomanip>

void set_bit(Types::UInt256& num, int bit) {
    if (bit < 64) num.q0 |= (1ULL << bit);
    else if (bit < 128) num.q1 |= (1ULL << (bit - 64));
    else if (bit < 192) num.q2 |= (1ULL << (bit - 128));
    else if (bit < 256) num.q3 |= (1ULL << (bit - 192));
}

Types::UInt256 get_lower_bound(int N) {
    Types::UInt256 res;
    if (N > 0) {
        set_bit(res, N - 1);
    }
    return res;
}

Types::UInt256 get_upper_bound(int N) {
    Types::UInt256 res;
    for (int i = 0; i < N; ++i) {
        set_bit(res, i);
    }
    return res;
}

int main() {
    std::cout << "Bem-vindo ao Bitcoin Puzzle Scanner!\n";
    std::cout << "Escolha o numero do Puzzle (1 a 160)\n";
    std::cout << "[Nota: O Puzzle #71 e o mais facil atualmente disponivel com saldo!]\n";
    std::cout << "Puzzle [Default 71]: ";
    std::string input;
    std::getline(std::cin, input);
    
    int puzzle_num = 71;
    if (!input.empty()) {
        try { puzzle_num = std::stoi(input); } catch (...) {}
    }
    if (puzzle_num < 1) puzzle_num = 1;
    if (puzzle_num > 160) puzzle_num = 160;

    const auto& puzzles = Config::GetPuzzles();
    if (puzzles.find(puzzle_num) == puzzles.end()) {
        std::cerr << "Erro: Informacoes do Puzzle " << puzzle_num << " nao encontradas.\n";
        return 1;
    }
    
    auto puzzle_info = puzzles.at(puzzle_num);
    std::string target_hash_hex = puzzle_info.hash160;
    std::string target_address = puzzle_info.address;

    if (puzzle_info.solved) {
        std::cout << "[AVISO] O Puzzle #" << puzzle_num << " ja consta como RESOLVIDO no diretorio!\n";
    }

    int max_threads = std::thread::hardware_concurrency();
    if (max_threads == 0) max_threads = 4;
    int num_threads = max_threads;
    std::cout << "Quantos threads deseja utilizar? (Max: " << max_threads << ") [Default: " << max_threads << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) {
        try { num_threads = std::stoi(input); } catch (...) {}
    }
    if (num_threads < 1) num_threads = 1;
    if (num_threads > max_threads) num_threads = max_threads;

    Types::UInt256 lower_bound = get_lower_bound(puzzle_num);
    Types::UInt256 upper_bound = get_upper_bound(puzzle_num);

    std::cout << "\nIniciando Scanner...\n";
    std::cout << "Puzzle: #" << puzzle_num << " (Status: " << (puzzle_info.solved ? "RESOLVIDO" : "NAO RESOLVIDO") << ")\n";
    std::cout << "Target Address: " << target_address << "\n";
    std::cout << "Target HASH160: " << target_hash_hex << "\n";
    std::cout << "Threads: " << num_threads << "\n";
    std::cout << "Search Range: 0x" << lower_bound.to_hex() << " to 0x" << upper_bound.to_hex() << "\n\n";

    Types::Hash160 target_hash160_bytes;
    // Convert hex string to bytes for target_hash160_bytes
    for (size_t i = 0; i < target_hash_hex.length() && i < 40; i += 2) {
        std::string byteString = target_hash_hex.substr(i, 2);
        target_hash160_bytes[i / 2] = static_cast<uint8_t>(std::stoul(byteString, nullptr, 16));
    }

    try {
        Scanner::ScannerEngine engine(
            lower_bound,
            upper_bound,
            target_hash160_bytes,
            num_threads,
            puzzle_num
        );
        engine.start();

        // After scan finishes, display final stats if no match was found
        // Or display match details if found (handled by progress manager)

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Scanner finished.\n";

    return 0;
}
