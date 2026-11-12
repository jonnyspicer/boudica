// Boudica Texel Tuner
//
// Tunes the linear evaluation parameters in Eval::params (eval_params.h)
// against game results. Each position is evaluated once with a trace that
// records how often each parameter was used; after that the evaluation is a
// cheap sparse dot product, so the error and its gradient can be computed
// analytically over millions of positions.
//
// Input lines are "<FEN> | <result>", result 1.0 / 0.5 / 0.0 from White's
// point of view. The error minimised is
//   E = mean((result - sigmoid(K * eval / 400))^2),  sigmoid(x) = 1 / (1 + 10^-x)
// where eval is White's point of view and K is fitted first with the initial
// parameters.
//
// Usage:
//   tuner --filter-quiet --epd raw.epd --output quiet.epd [--threads N]
//   tuner --epd quiet.epd [--epochs N] [--lr X] [--batch-size N] [--threads N]
//         [--freeze-material] [--k X] [--output tuned.txt]
//   tuner --verify --epd file.epd      check traces against Eval::evaluate()
//   tuner --print-params               print the current defaults as C++

#include "../types.h"
#include "../bitboard.h"
#include "../position.h"
#include "../evaluate.h"
#include "../search.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace Boudica;
using Eval::EvalParams;
using Eval::EvalTrace;

namespace {

// ============================================================================
// Parameter Layout
// ============================================================================

constexpr int NUM_SCORES = Eval::NUM_SCORE_PARAMS;
constexpr int TEMPO_INDEX = 2 * NUM_SCORES;  // Flat index of Tempo in the tuned vector
constexpr int NUM_TUNED = 2 * NUM_SCORES + 1;  // mg and eg of every Score, plus Tempo

// EvalParams is an unpadded run of Scores followed by Tempo (static_asserts
// in eval_params.h), so the Scores can be addressed by flat index
Score& score_param(EvalParams& p, int i) {
    return reinterpret_cast<Score*>(&p)[i];
}

// One entry per EvalParams member, in declaration order, for printing
struct ParamInfo {
    const char* name;
    const char* size;   // Array size as written in the source, nullptr for a scalar
    int index;
    int count;
    const char* group;  // Comment that starts a group of parameters, or nullptr
};

#define P(member, size, count, group) { #member, size, EVAL_PARAM_INDEX(member), count, group }

const ParamInfo Params[] = {
    P(PieceValue, "PIECE_TYPE_NB", PIECE_TYPE_NB, "Material"),
    P(BishopPairBonus, nullptr, 1, nullptr),
    P(DoubledPawnPenalty, nullptr, 1, "Pawn structure"),
    P(IsolatedPawnPenalty, nullptr, 1, nullptr),
    P(BackwardPawnPenalty, nullptr, 1, nullptr),
    P(ConnectedPawnBonus, nullptr, 1, nullptr),
    P(PassedPawnBonus, "RANK_NB", RANK_NB, "Passed pawns, indexed by relative rank"),
    P(BlockedPasserPenalty, "RANK_NB", RANK_NB, nullptr),
    P(ProtectedPasserBonus, nullptr, 1, nullptr),
    P(PasserEnemyKingDistance, nullptr, 1, nullptr),
    P(PasserOwnKingProximity, nullptr, 1, nullptr),
    P(RookBehindPasserBonus, nullptr, 1, nullptr),
    P(PawnShieldBonus, nullptr, 1, "King safety"),
    P(PawnShield2Bonus, nullptr, 1, nullptr),
    P(OpenFileNearKingPenalty, nullptr, 1, nullptr),
    P(SemiOpenFileNearKingPenalty, nullptr, 1, nullptr),
    P(KnightMobility, nullptr, 1, "Mobility, per square not attacked by enemy pawns"),
    P(BishopMobility, nullptr, 1, nullptr),
    P(RookMobility, nullptr, 1, nullptr),
    P(QueenMobility, nullptr, 1, nullptr),
    P(RookOnOpenFile, nullptr, 1, "Rooks"),
    P(RookOnSemiOpenFile, nullptr, 1, nullptr),
    P(RookOn7thRank, nullptr, 1, nullptr),
    P(PawnTable, "SQUARE_NB", SQUARE_NB, "Piece-square tables"),
    P(KnightTable, "SQUARE_NB", SQUARE_NB, nullptr),
    P(BishopTable, "SQUARE_NB", SQUARE_NB, nullptr),
    P(RookTable, "SQUARE_NB", SQUARE_NB, nullptr),
    P(QueenTable, "SQUARE_NB", SQUARE_NB, nullptr),
    P(KingTable, "SQUARE_NB", SQUARE_NB, nullptr),
};

#undef P

std::string score_str(Score s) {
    return "S(" + std::to_string(s.mg) + ", " + std::to_string(s.eg) + ")";
}

// Print the struct body between the BEGIN/END markers in eval_params.h,
// in exactly the format used there
std::string params_to_cpp(EvalParams& p) {
    std::ostringstream out;
    out << "    // BEGIN TUNED PARAMETERS\n";

    int expected_index = 0;
    for (size_t k = 0; k < sizeof(Params) / sizeof(Params[0]); ++k) {
        const ParamInfo& info = Params[k];
        if (info.index != expected_index) {
            std::cerr << "Parameter table out of sync with EvalParams at " << info.name << std::endl;
            std::exit(1);
        }
        expected_index += info.count;

        bool is_pst = info.count == SQUARE_NB;
        bool prev_pst = k > 0 && Params[k - 1].count == SQUARE_NB;
        if (k > 0 && (info.group || (is_pst && prev_pst)))
            out << "\n";
        if (info.group)
            out << "    // " << info.group << "\n";

        if (!info.size) {
            out << "    Score " << info.name << " = " << score_str(score_param(p, info.index)) << ";\n";
        } else if (!is_pst) {
            out << "    Score " << info.name << "[" << info.size << "] = {";
            for (int i = 0; i < info.count; ++i)
                out << (i ? ", " : " ") << score_str(score_param(p, info.index + i));
            out << " };\n";
        } else {
            out << "    Score " << info.name << "[" << info.size << "] = {\n";
            for (int r = 0; r < 8; ++r) {
                out << "        // Rank " << (r + 1) << "\n        ";
                for (int f = 0; f < 8; ++f) {
                    std::string cell = score_str(score_param(p, info.index + r * 8 + f)) + ",";
                    if (f < 7)
                        cell.resize(std::max<size_t>(cell.size(), 13), ' ');
                    out << cell << (f < 7 ? " " : "\n");
                }
            }
            out << "    };\n";
        }
    }
    if (expected_index != NUM_SCORES) {
        std::cerr << "Parameter table does not cover all of EvalParams" << std::endl;
        std::exit(1);
    }

    out << "\n    // Side to move bonus, added after tapering\n";
    out << "    int Tempo = " << p.Tempo << ";\n";
    out << "    // END TUNED PARAMETERS\n";
    return out.str();
}

// ============================================================================
// Input Parsing
// ============================================================================

// Split "<FEN> | <result>" into its parts; false for a malformed line
bool parse_line(const std::string& line, std::string& fen, double& result) {
    size_t bar = line.find('|');
    if (bar == std::string::npos)
        return false;
    fen = line.substr(0, bar);
    while (!fen.empty() && std::isspace((unsigned char)fen.back()))
        fen.pop_back();
    const char* res = line.c_str() + bar + 1;
    char* end;
    result = std::strtod(res, &end);
    return end != res && !fen.empty() && result >= 0.0 && result <= 1.0;
}

// Read up to max_lines lines; returns false at end of file with nothing read
bool read_chunk(std::istream& in, std::vector<std::string>& lines, size_t max_lines) {
    lines.clear();
    std::string line;
    while (lines.size() < max_lines && std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (!line.empty())
            lines.push_back(line);
    }
    return !lines.empty();
}

constexpr size_t CHUNK_LINES = 1 << 16;

// Run fn(thread, begin, end) over [0, n) split across threads
template<typename Fn>
void parallel_for(size_t n, int threads, Fn fn) {
    std::vector<std::thread> pool;
    size_t per = (n + threads - 1) / threads;
    for (int t = 0; t < threads; ++t) {
        size_t begin = t * per, end = std::min(n, begin + per);
        if (begin >= end)
            break;
        pool.emplace_back(fn, t, begin, end);
    }
    for (auto& th : pool)
        th.join();
}

// ============================================================================
// Quiet Position Filter
// ============================================================================

// Keep positions that are not in check and whose quiescence score equals the
// static eval, i.e. where no capture sequence changes the evaluation
int filter_quiet(const std::string& in_path, const std::string& out_path, int threads) {
    std::ifstream in(in_path);
    std::ofstream out(out_path);
    if (!in || !out) {
        std::cerr << "Cannot open " << (!in ? in_path : out_path) << std::endl;
        return 1;
    }

    init_offline_search();

    std::vector<std::string> lines;
    std::vector<char> keep;
    size_t total = 0, kept = 0, malformed = 0, in_check = 0;
    auto start = std::chrono::steady_clock::now();

    while (read_chunk(in, lines, CHUNK_LINES)) {
        keep.assign(lines.size(), 0);
        std::atomic<size_t> bad{0}, checks{0};

        parallel_for(lines.size(), threads, [&](int, size_t begin, size_t end) {
            for (size_t i = begin; i < end; ++i) {
                std::string fen;
                double result;
                if (!parse_line(lines[i], fen, result)) {
                    bad++;
                    continue;
                }
                StateInfo st;
                Boudica::Position pos;
                pos.set(fen, &st);
                if (pos.checkers()) {
                    checks++;
                    continue;
                }
                keep[i] = Eval::evaluate(pos) == qsearch_root(pos);
            }
        });

        for (size_t i = 0; i < lines.size(); ++i) {
            if (keep[i]) {
                out << lines[i] << '\n';
                kept++;
            }
        }
        total += lines.size();
        malformed += bad;
        in_check += checks;
        std::cout << "\rFiltered " << total << " positions, kept " << kept << std::flush;
    }

    double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "\nRead " << total << " lines: " << malformed << " malformed, "
              << in_check << " in check, " << (total - malformed - in_check - kept)
              << " not quiet, " << kept << " kept (" << secs << "s)" << std::endl;
    return 0;
}

// ============================================================================
// Traced Training Data
// ============================================================================

// A parameter used by a position: its Score index and White-minus-Black count
struct Coef {
    uint16_t index;
    int16_t count;
};

struct Entry {
    uint64_t start;    // First Coef in Dataset::coefs
    uint16_t length;   // Number of Coefs
    int16_t phase;     // 0..256
    int16_t nl_mg;     // Untuned nonlinear part (king attacks), White's view
    int16_t nl_eg;
    uint8_t scale;     // Out of 128
    int8_t tempo;      // +1 White to move, -1 Black
    float result;      // From White's point of view
};

struct Dataset {
    std::vector<Entry> entries;
    std::vector<Coef> coefs;
};

// Integer reconstruction of the evaluation from a trace, White's point of view
int trace_eval(const Entry& e, const Coef* c, EvalParams& p) {
    int mg = e.nl_mg, eg = e.nl_eg;
    for (int k = 0; k < e.length; ++k) {
        Score s = score_param(p, c[k].index);
        mg += c[k].count * s.mg;
        eg += c[k].count * s.eg;
    }
    int v = (mg * e.phase + eg * (Eval::PHASE_MIDGAME - e.phase)) / Eval::PHASE_MIDGAME;
    v += e.tempo * p.Tempo;
    return v * e.scale / 128;
}

// Trace a position; false if a count does not fit the compact storage
bool make_entry(const Boudica::Position& pos, double result, Entry& e, std::vector<Coef>& coefs, int& engine_eval) {
    EvalTrace t;
    Value v = Eval::evaluate_trace(pos, t);
    engine_eval = pos.side_to_move() == WHITE ? v : -v;

    e.start = coefs.size();
    e.phase = int16_t(t.phase);
    e.nl_mg = int16_t(t.nonlinear.mg);
    e.nl_eg = int16_t(t.nonlinear.eg);
    e.scale = uint8_t(t.scale);
    e.tempo = int8_t(t.tempo);
    e.result = float(result);
    if (e.nl_mg != t.nonlinear.mg || e.nl_eg != t.nonlinear.eg)
        return false;

    // A zero scale (e.g. insufficient material) makes the eval constant
    if (t.scale != 0) {
        for (int i = 0; i < NUM_SCORES; ++i) {
            if (!t.coef[i])
                continue;
            if (t.coef[i] != int16_t(t.coef[i]))
                return false;
            coefs.push_back({uint16_t(i), int16_t(t.coef[i])});
        }
    }
    e.length = uint16_t(coefs.size() - e.start);
    return true;
}

// Load and trace every position. Each trace is checked against the value the
// evaluation itself returned, so a bookkeeping error in the trace shows up here.
bool load_dataset(const std::string& path, int threads, Dataset& data) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "Cannot open " << path << std::endl;
        return false;
    }

    struct Part {
        std::vector<Entry> entries;
        std::vector<Coef> coefs;
        size_t malformed = 0, mismatched = 0;
        int max_diff = 0;
    };

    std::vector<std::string> lines;
    size_t malformed = 0, mismatched = 0;
    int max_diff = 0;
    auto start = std::chrono::steady_clock::now();

    while (read_chunk(in, lines, CHUNK_LINES)) {
        std::vector<Part> parts(threads);

        parallel_for(lines.size(), threads, [&](int t, size_t begin, size_t end) {
            Part& part = parts[t];
            for (size_t i = begin; i < end; ++i) {
                std::string fen;
                double result;
                if (!parse_line(lines[i], fen, result)) {
                    part.malformed++;
                    continue;
                }
                StateInfo st;
                Boudica::Position pos;
                pos.set(fen, &st);

                Entry e;
                int engine_eval;
                size_t mark = part.coefs.size();
                if (!make_entry(pos, result, e, part.coefs, engine_eval)) {
                    part.coefs.resize(mark);
                    part.malformed++;
                    continue;
                }
                int diff = std::abs(trace_eval(e, &part.coefs[e.start], Eval::params) - engine_eval);
                if (diff) {
                    part.mismatched++;
                    part.max_diff = std::max(part.max_diff, diff);
                }
                part.entries.push_back(e);
            }
        });

        for (Part& part : parts) {
            uint64_t offset = data.coefs.size();
            for (Entry& e : part.entries) {
                e.start += offset;
                data.entries.push_back(e);
            }
            data.coefs.insert(data.coefs.end(), part.coefs.begin(), part.coefs.end());
            malformed += part.malformed;
            mismatched += part.mismatched;
            max_diff = std::max(max_diff, part.max_diff);
        }
        std::cout << "\rLoaded " << data.entries.size() << " positions" << std::flush;
    }

    double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    double mb = (data.entries.size() * sizeof(Entry) + data.coefs.size() * sizeof(Coef)) / 1048576.0;
    std::cout << "\nLoaded " << data.entries.size() << " positions (" << malformed
              << " skipped), " << data.coefs.size() << " coefficients, "
              << mb << " MB, " << secs << "s" << std::endl;
    std::cout << "Trace check: " << mismatched << " positions differ from the evaluation"
              << " (max abs diff " << max_diff << " cp)" << std::endl;
    return !data.entries.empty();
}

// ============================================================================
// Error and Gradient
// ============================================================================

constexpr double LN10 = 2.302585092994046;

double sigmoid(double K, double eval) {
    return 1.0 / (1.0 + std::pow(10.0, -K * eval / 400.0));
}

// Evaluation for the real-valued parameter vector (same formula as trace_eval
// without integer rounding)
double tuned_eval(const Entry& e, const Coef* c, const std::vector<double>& x) {
    double mg = e.nl_mg, eg = e.nl_eg;
    for (int k = 0; k < e.length; ++k) {
        mg += c[k].count * x[2 * c[k].index];
        eg += c[k].count * x[2 * c[k].index + 1];
    }
    double v = (mg * e.phase + eg * (256 - e.phase)) / 256.0 + e.tempo * x[TEMPO_INDEX];
    return v * e.scale / 128.0;
}

// Mean squared error over the given entries; also accumulates the gradient
// with respect to x when grad is non-null
double error_and_gradient(const Dataset& data, const std::vector<uint32_t>& order,
                          size_t begin, size_t end, const std::vector<double>& x, double K,
                          std::vector<double>* grad, int threads) {
    struct Part {
        double error = 0;
        std::vector<double> grad;
    };
    std::vector<Part> parts(threads);

    parallel_for(end - begin, threads, [&](int t, size_t lo, size_t hi) {
        Part& part = parts[t];
        if (grad)
            part.grad.assign(NUM_TUNED, 0.0);
        for (size_t n = begin + lo; n < begin + hi; ++n) {
            const Entry& e = data.entries[order.empty() ? n : order[n]];
            const Coef* c = &data.coefs[e.start];
            double s = sigmoid(K, tuned_eval(e, c, x));
            double diff = e.result - s;
            part.error += diff * diff;

            if (!grad || !e.scale)
                continue;
            // d(error)/d(eval), then the chain through taper and scale
            double g = -2.0 * diff * s * (1.0 - s) * LN10 * K / 400.0 * e.scale / 128.0;
            double g_mg = g * e.phase / 256.0;
            double g_eg = g * (256 - e.phase) / 256.0;
            for (int k = 0; k < e.length; ++k) {
                part.grad[2 * c[k].index] += g_mg * c[k].count;
                part.grad[2 * c[k].index + 1] += g_eg * c[k].count;
            }
            part.grad[TEMPO_INDEX] += g * e.tempo;
        }
    });

    double n = double(end - begin);
    double error = 0;
    if (grad)
        grad->assign(NUM_TUNED, 0.0);
    for (Part& part : parts) {
        error += part.error;
        if (grad && !part.grad.empty())
            for (int i = 0; i < NUM_TUNED; ++i)
                (*grad)[i] += part.grad[i] / n;
    }
    return error / n;
}

// Fit K by golden-section search on the error with the initial parameters
double fit_k(const Dataset& data, const std::vector<double>& x, int threads) {
    const std::vector<uint32_t> all;
    auto err = [&](double K) {
        return error_and_gradient(data, all, 0, data.entries.size(), x, K, nullptr, threads);
    };
    const double phi = (std::sqrt(5.0) - 1) / 2;
    double a = 0.05, b = 5.0;
    double c = b - phi * (b - a), d = a + phi * (b - a);
    double fc = err(c), fd = err(d);
    for (int i = 0; i < 50; ++i) {
        if (fc < fd) {
            b = d; d = c; fd = fc;
            c = b - phi * (b - a); fc = err(c);
        } else {
            a = c; c = d; fc = fd;
            d = a + phi * (b - a); fd = err(d);
        }
    }
    return (a + b) / 2;
}

// ============================================================================
// Parameter Vector Conversion
// ============================================================================

std::vector<double> params_to_vector(EvalParams& p) {
    std::vector<double> x(NUM_TUNED);
    for (int i = 0; i < NUM_SCORES; ++i) {
        x[2 * i] = score_param(p, i).mg;
        x[2 * i + 1] = score_param(p, i).eg;
    }
    x[TEMPO_INDEX] = p.Tempo;
    return x;
}

EvalParams vector_to_params(const std::vector<double>& x) {
    EvalParams p;
    for (int i = 0; i < NUM_SCORES; ++i)
        score_param(p, i) = S(int(std::lround(x[2 * i])), int(std::lround(x[2 * i + 1])));
    p.Tempo = int(std::lround(x[TEMPO_INDEX]));
    return p;
}

void write_params(const std::vector<double>& x, const std::string& path) {
    EvalParams p = vector_to_params(x);
    std::string cpp = params_to_cpp(p);
    if (path.empty()) {
        std::cout << cpp;
        return;
    }
    std::ofstream out(path);
    out << cpp;
}

// ============================================================================
// Tuning
// ============================================================================

struct Options {
    std::string epd, output;
    int epochs = 100;
    double lr = 1.0;
    size_t batch_size = 0;  // 0 = full batch
    int threads = std::max(1u, std::thread::hardware_concurrency());
    bool freeze_material = false;
    double k = 0;           // 0 = fit
};

int tune(const Options& opt) {
    Dataset data;
    if (!load_dataset(opt.epd, opt.threads, data))
        return 1;

    std::vector<double> x = params_to_vector(Eval::params);

    // Parameters left untouched by the optimiser
    std::vector<char> frozen(NUM_TUNED, 0);
    if (opt.freeze_material)
        for (int pt = 0; pt < PIECE_TYPE_NB; ++pt)
            frozen[2 * (EVAL_PARAM_INDEX(PieceValue) + pt)] =
            frozen[2 * (EVAL_PARAM_INDEX(PieceValue) + pt) + 1] = 1;

    double K = opt.k > 0 ? opt.k : fit_k(data, x, opt.threads);
    const std::vector<uint32_t> all;
    double initial = error_and_gradient(data, all, 0, data.entries.size(), x, K, nullptr, opt.threads);
    std::printf("K = %.6f, initial error %.8f\n", K, initial);

    size_t n = data.entries.size();
    size_t batch = opt.batch_size ? std::min(opt.batch_size, n) : n;
    std::vector<uint32_t> order;
    if (batch < n) {
        order.resize(n);
        std::iota(order.begin(), order.end(), 0);
    }
    std::mt19937 rng(12345);

    // Adam
    const double beta1 = 0.9, beta2 = 0.999, eps = 1e-8;
    std::vector<double> m(NUM_TUNED, 0.0), v(NUM_TUNED, 0.0), grad;
    long step = 0;
    auto start = std::chrono::steady_clock::now();

    for (int epoch = 1; epoch <= opt.epochs; ++epoch) {
        if (!order.empty())
            std::shuffle(order.begin(), order.end(), rng);

        double epoch_error = 0;
        for (size_t b = 0; b < n; b += batch) {
            size_t end = std::min(n, b + batch);
            double err = error_and_gradient(data, order, b, end, x, K, &grad, opt.threads);
            epoch_error += err * (end - b);

            ++step;
            double c1 = 1 - std::pow(beta1, step), c2 = 1 - std::pow(beta2, step);
            for (int i = 0; i < NUM_TUNED; ++i) {
                if (frozen[i])
                    continue;
                m[i] = beta1 * m[i] + (1 - beta1) * grad[i];
                v[i] = beta2 * v[i] + (1 - beta2) * grad[i] * grad[i];
                x[i] -= opt.lr * (m[i] / c1) / (std::sqrt(v[i] / c2) + eps);
            }
        }

        double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::printf("epoch %4d  error %.8f  (%.1fs)\n", epoch, epoch_error / n, secs);
        std::fflush(stdout);

        if (!opt.output.empty() && epoch % 25 == 0)
            write_params(x, opt.output);
    }

    double final_error = error_and_gradient(data, all, 0, n, x, K, nullptr, opt.threads);
    EvalParams rounded = vector_to_params(x);
    std::vector<double> xr = params_to_vector(rounded);
    double rounded_error = error_and_gradient(data, all, 0, n, xr, K, nullptr, opt.threads);
    std::printf("Final error %.8f (%.8f with rounded parameters, initial %.8f)\n",
                final_error, rounded_error, initial);

    write_params(x, opt.output);
    if (!opt.output.empty())
        std::cout << "Wrote parameters to " << opt.output << std::endl;
    return 0;
}

// ============================================================================
// Trace Verification
// ============================================================================

// Compare Eval::evaluate(), the traced evaluation and the reconstruction from
// the trace for every position (single-threaded, so the pawn hash is exact).
// Returns the number of reconstructions that differ.
size_t verify_pass(const std::vector<std::string>& lines, const char* label) {
    size_t total = 0, differ = 0, trace_differ = 0;
    int max_diff = 0;
    for (const std::string& line : lines) {
        std::string fen;
        double result;
        if (!parse_line(line, fen, result))
            continue;
        StateInfo st;
        Boudica::Position pos;
        pos.set(fen, &st);

        Value v = Eval::evaluate(pos);
        int engine = pos.side_to_move() == WHITE ? v : -v;
        std::vector<Coef> coefs;
        Entry e;
        int traced;
        if (!make_entry(pos, result, e, coefs, traced)) {
            std::cerr << "Trace overflow: " << fen << std::endl;
            continue;
        }
        int rebuilt = trace_eval(e, coefs.data(), Eval::params);
        int diff = std::abs(rebuilt - engine);
        total++;
        differ += diff != 0;
        trace_differ += traced != engine;
        max_diff = std::max(max_diff, diff);
        if (diff && differ <= 10)
            std::cout << "Mismatch: eval " << engine << " trace " << rebuilt << "  " << fen << std::endl;
    }
    std::cout << label << ": " << total << " positions, " << differ
              << " reconstructions differ from Eval::evaluate (max abs diff " << max_diff
              << " cp), " << trace_differ << " traced evals differ" << std::endl;
    return differ;
}

// Check the trace with the default parameters, then with random ones so that
// every coefficient is exercised (a term missing from the trace would only
// cancel out by accident)
int verify(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "Cannot open " << path << std::endl;
        return 1;
    }
    std::vector<std::string> lines;
    read_chunk(in, lines, SIZE_MAX);

    size_t differ = verify_pass(lines, "Default parameters");

    std::mt19937 rng(1);
    std::uniform_int_distribution<int> dist(-200, 200);
    for (int i = 0; i < NUM_SCORES; ++i)
        score_param(Eval::params, i) = S(dist(rng), dist(rng));
    Eval::params.Tempo = dist(rng);
    Eval::init();  // Pawn hash holds scores for the old parameters
    differ += verify_pass(lines, "Random parameters");

    Eval::params = EvalParams();
    Eval::init();
    return differ ? 1 : 0;
}

void usage() {
    std::cout <<
        "Usage:\n"
        "  tuner --filter-quiet --epd raw.epd --output quiet.epd [--threads N]\n"
        "  tuner --epd quiet.epd [options]\n"
        "  tuner --verify --epd file.epd\n"
        "  tuner --print-params\n"
        "\n"
        "Input lines are \"<FEN> | <result>\" with result 1.0, 0.5 or 0.0 for White.\n"
        "\n"
        "Tuning options:\n"
        "  --epochs N          Passes over the data (default 100)\n"
        "  --lr X              Adam learning rate in centipawns (default 1.0)\n"
        "  --batch-size N      Mini-batch size (default: full batch)\n"
        "  --threads N         Worker threads (default: all cores)\n"
        "  --freeze-material   Keep PieceValue fixed\n"
        "  --k X               Sigmoid scale instead of fitting it\n"
        "  --output FILE       Write the tuned EvalParams body here (default stdout)\n";
}

} // namespace

int main(int argc, char* argv[]) {
    init_bitboards();
    Zobrist::init();
    init_lmr_table();
    Eval::init();

    Options opt;
    bool filter = false, verify_only = false, print_only = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for " << arg << std::endl;
                std::exit(1);
            }
            return argv[++i];
        };
        if (arg == "--epd") opt.epd = value();
        else if (arg == "--output") opt.output = value();
        else if (arg == "--epochs") opt.epochs = std::stoi(value());
        else if (arg == "--lr") opt.lr = std::stod(value());
        else if (arg == "--batch-size") opt.batch_size = std::stoul(value());
        else if (arg == "--threads") opt.threads = std::max(1, std::stoi(value()));
        else if (arg == "--k") opt.k = std::stod(value());
        else if (arg == "--freeze-material") opt.freeze_material = true;
        else if (arg == "--filter-quiet") filter = true;
        else if (arg == "--verify") verify_only = true;
        else if (arg == "--print-params") print_only = true;
        else if (arg == "--help" || arg == "-h") { usage(); return 0; }
        else {
            std::cerr << "Unknown option " << arg << std::endl;
            usage();
            return 1;
        }
    }

    if (print_only) {
        std::cout << params_to_cpp(Eval::params);
        return 0;
    }
    if (opt.epd.empty()) {
        usage();
        return 1;
    }
    if (verify_only)
        return verify(opt.epd);
    if (filter) {
        if (opt.output.empty()) {
            std::cerr << "--filter-quiet needs --output" << std::endl;
            return 1;
        }
        return filter_quiet(opt.epd, opt.output, opt.threads);
    }
    return tune(opt);
}
