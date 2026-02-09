#ifndef UCI_H
#define UCI_H

#include <string>
#include <vector>
#include <sstream>
#include <atomic>

#include "types.h"
#include "position.h"

class TranspositionTable;

namespace UCI {

// ============================================================================
// Engine Identification
// ============================================================================

constexpr const char* ENGINE_NAME = "Boudica";
constexpr const char* ENGINE_AUTHOR = "Hive Mind Collective";
constexpr const char* ENGINE_VERSION = "1.0";

// ============================================================================
// UCI Option Types and Structures
// ============================================================================

enum OptionType {
    OPT_SPIN,
    OPT_CHECK,
    OPT_COMBO,
    OPT_BUTTON,
    OPT_STRING
};

struct Option {
    std::string name;
    OptionType type;
    std::string defaultValue;
    std::string currentValue;
    int min;
    int max;
    std::vector<std::string> comboValues;

    Option() : type(OPT_SPIN), min(0), max(0) {}

    Option(const std::string& n, int def, int minVal, int maxVal)
        : name(n), type(OPT_SPIN),
          defaultValue(std::to_string(def)),
          currentValue(std::to_string(def)),
          min(minVal), max(maxVal) {}

    Option(const std::string& n, bool def)
        : name(n), type(OPT_CHECK),
          defaultValue(def ? "true" : "false"),
          currentValue(def ? "true" : "false"),
          min(0), max(0) {}

    Option(const std::string& n, OptionType t)
        : name(n), type(t), min(0), max(0) {}

    int intValue() const { return std::stoi(currentValue); }
    bool boolValue() const { return currentValue == "true"; }
};

// ============================================================================
// Search Limits (from "go" command)
// ============================================================================

struct SearchLimits {
    int wtime;           // White time remaining (ms)
    int btime;           // Black time remaining (ms)
    int winc;            // White increment (ms)
    int binc;            // Black increment (ms)
    int movestogo;       // Moves until next time control
    int depth;           // Maximum depth to search
    uint64_t nodes;      // Maximum nodes to search
    int movetime;        // Exact time to search (ms)
    bool infinite;       // Search until "stop" command
    std::vector<Move> searchmoves;  // Restrict to these moves

    SearchLimits() { clear(); }

    void clear() {
        wtime = 0;
        btime = 0;
        winc = 0;
        binc = 0;
        movestogo = 0;
        depth = 0;
        nodes = 0;
        movetime = 0;
        infinite = false;
        searchmoves.clear();
    }
};

// ============================================================================
// Search Information (for "info" output)
// ============================================================================

struct SearchInfo {
    int depth;
    int seldepth;
    int score;           // Centipawns or mate score
    bool isMate;         // True if score is mate-in-N
    int mateIn;          // Moves to mate (+/- indicates side)
    uint64_t nodes;
    uint64_t nps;        // Nodes per second
    int hashfull;        // Hash table fill (per mille)
    int time;            // Search time (ms)
    std::vector<Move> pv;

    SearchInfo() : depth(0), seldepth(0), score(0), isMate(false),
                   mateIn(0), nodes(0), nps(0), hashfull(0), time(0) {}

    void clear() {
        depth = 0;
        seldepth = 0;
        score = 0;
        isMate = false;
        mateIn = 0;
        nodes = 0;
        nps = 0;
        hashfull = 0;
        time = 0;
        pv.clear();
    }
};

// ============================================================================
// Global Search Control
// ============================================================================

// Stop flag - search thread checks this periodically
extern std::atomic<bool> stopSearch;

// ============================================================================
// UCI Protocol Handler
// ============================================================================

class Handler {
public:
    // Constructor takes references to position and TT (owned by main)
    Handler(Boudica::Position& pos, TranspositionTable& tt);
    ~Handler();

    // Main UCI loop - reads from stdin until "quit"
    void loop();

    // Process a single UCI command string
    void processCommand(const std::string& command);

    // Output methods (static so search can call them)
    static void sendInfo(const SearchInfo& info);
    static void sendBestMove(Move bestMove, Move ponderMove = MOVE_NONE);
    static void sendString(const std::string& str);

    // Option access
    int getOptionInt(const std::string& name) const;
    bool getOptionBool(const std::string& name) const;
    std::string getOptionString(const std::string& name) const;

private:
    Boudica::Position& position;
    TranspositionTable& transpositionTable;
    std::vector<Option> options;

    // State info stack for position setup (must hold full game history)
    Boudica::StateInfo stateInfoStack[1024];
    int stateInfoIdx;

    // Command handlers
    void cmdUCI();
    void cmdIsReady();
    void cmdUCINewGame();
    void cmdPosition(std::istringstream& is);
    void cmdGo(std::istringstream& is);
    void cmdStop();
    void cmdSetOption(std::istringstream& is);
    void cmdQuit();

    // Debug commands
    void cmdDisplay();
    void cmdPerft(std::istringstream& is);
    void cmdBench();

    // Helper methods
    void initOptions();
    Move parseMove(const std::string& moveStr) const;
    void applyMoves(std::istringstream& is);
    Option* findOption(const std::string& name);
    const Option* findOption(const std::string& name) const;
};

// ============================================================================
// Utility Functions
// ============================================================================

// Convert move to UCI string format (e2e4, e7e8q)
std::string moveToUCI(Move m);

// Parse UCI move string to Move (requires position for validation)
Move uciToMove(const Boudica::Position& pos, const std::string& str);

// Convert score to UCI string (cp or mate)
std::string scoreToUCI(Value score);

// Convert PV to UCI string
std::string pvToUCI(const Move* pv, int length);
std::string pvToUCI(const std::vector<Move>& pv);

} // namespace UCI

#endif // UCI_H
