// test_main.cpp — automated tests for Battleship
//
// Replaces main.cpp during testing. Build & run:
//
//   g++ -std=c++20 -Wall Board.cpp Game.cpp Player.cpp test_main.cpp -o tests
//   ./tests
//
// On SEASnet (catches more bugs):
//
//   g32 -o tests Board.cpp Game.cpp Player.cpp test_main.cpp
//   ./tests
//
// Exit code 0 = all passed, 1 = at least one failure.
// Each test prints pass/FAIL lines; final summary shows totals.

#include "Game.h"
#include "Board.h"
#include "Player.h"
#include "globals.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <string>

using namespace std;
using namespace std::chrono;

// ── tiny test framework ─────────────────────────────────────────────────────
static int g_pass = 0;
static int g_fail = 0;

#define SECTION(name) cout << "\n=== " << (name) << " ===\n"

#define CHECK(cond) do {                                                       \
    if (cond) { g_pass++; cout << "  pass: " << #cond << "\n"; }               \
    else      { g_fail++; cout << "  FAIL (line " << __LINE__ << "): "         \
                              << #cond << "\n"; }                              \
} while (0)

// Swallow cout for tests where we don't care about the spam (display, addShip
// error messages, full game play).
class CoutSink {
    streambuf*    m_old;
    ostringstream m_buf;
  public:
    CoutSink()  { m_old = cout.rdbuf(m_buf.rdbuf()); }
    ~CoutSink() { cout.rdbuf(m_old); }
    string str() const { return m_buf.str(); }
};

static bool addStandardShips(Game& g) {
    return g.addShip(5, 'A', "aircraft carrier") &&
           g.addShip(4, 'B', "battleship") &&
           g.addShip(3, 'D', "destroyer") &&
           g.addShip(3, 'S', "submarine") &&
           g.addShip(2, 'P', "patrol boat");
}

// ── Board: placement ────────────────────────────────────────────────────────
void test_board_placement() {
    SECTION("Board: placeShip / unplaceShip");
    CHECK(1 == 2);   // TEMPORARY - proving CI catches failures

    Game g(10, 10);
    addStandardShips(g);
    Board b(g);
    b.clear();

    // Valid horizontal place (carrier at (0,0)..(0,4))
    CHECK(b.placeShip(Point(0, 0), 0, HORIZONTAL) == true);

    // Same ship can't be placed twice without unplacing
    CHECK(b.placeShip(Point(5, 0), 0, HORIZONTAL) == false);

    // Overlap rejected (battleship at (0,2) would hit carrier)
    CHECK(b.placeShip(Point(0, 2), 1, HORIZONTAL) == false);

    // Non-overlapping placement works
    CHECK(b.placeShip(Point(1, 0), 1, HORIZONTAL) == true);

    // Off-board rejected (destroyer len 3 at col 8 → cols 8,9,10)
    CHECK(b.placeShip(Point(0, 8), 2, HORIZONTAL) == false);

    // Negative coords rejected
    CHECK(b.placeShip(Point(-1, 0), 2, HORIZONTAL) == false);

    // Invalid ship id rejected
    CHECK(b.placeShip(Point(2, 0), 99, HORIZONTAL) == false);

    // Unplace round-trips
    CHECK(b.unplaceShip(Point(0, 0), 0, HORIZONTAL) == true);
    CHECK(b.unplaceShip(Point(0, 0), 0, HORIZONTAL) == false);  // already removed
    CHECK(b.placeShip(Point(0, 0), 0, HORIZONTAL) == true);     // can re-place

    // Unplace with wrong direction/position rejected
    CHECK(b.unplaceShip(Point(0, 0), 0, VERTICAL) == false);
    CHECK(b.unplaceShip(Point(5, 5), 0, HORIZONTAL) == false);
}

// ── Board: attack semantics ────────────────────────────────────────────────
void test_board_attack() {
    SECTION("Board: attack hit / miss / sunk flags");
    Game g(10, 10);
    addStandardShips(g);
    Board b(g);
    b.clear();
    // Place only the patrol boat (id 4, length 2) at (0,0)-(0,1).
    CHECK(b.placeShip(Point(0, 0), 4, HORIZONTAL) == true);

    bool hit = false, sunk = false;
    int  id  = -1;

    // Out-of-bounds shots rejected
    CHECK(b.attack(Point(-1, 0), hit, sunk, id) == false);
    CHECK(b.attack(Point(10, 0), hit, sunk, id) == false);

    // Miss
    CHECK(b.attack(Point(5, 5), hit, sunk, id) == true);
    CHECK(hit == false);
    CHECK(sunk == false);

    // Duplicate shot rejected
    CHECK(b.attack(Point(5, 5), hit, sunk, id) == false);

    // First hit on the patrol boat: hit but not sunk
    CHECK(b.attack(Point(0, 0), hit, sunk, id) == true);
    CHECK(hit == true);
    CHECK(sunk == false);

    // Patrol boat isn't fully sunk yet
    CHECK(b.allShipsDestroyed() == false);

    // Second segment: sinks the boat, shipId must be set to 4
    CHECK(b.attack(Point(0, 1), hit, sunk, id) == true);
    CHECK(hit == true);
    CHECK(sunk == true);
    CHECK(id == 4);

    // Only ship placed is now sunk → all destroyed
    CHECK(b.allShipsDestroyed() == true);
}

// ── Board: block / unblock ─────────────────────────────────────────────────
void test_board_block() {
    SECTION("Board: block prevents placement, unblock restores");
    Game g(10, 10);
    addStandardShips(g);
    Board b(g);

    // After block(), placing the carrier at a fixed spot should fail most of
    // the time (prob of 5 specific cells all unblocked ≈ 0.028). 20/20
    // successes is virtually impossible — would mean block() isn't blocking.
    int successes = 0;
    for (int t = 0; t < 20; t++) {
        b.clear();
        b.block();
        if (b.placeShip(Point(0, 0), 0, HORIZONTAL))
            successes++;
    }
    cout << "  info: carrier placed at (0,0) succeeded " << successes
         << "/20 trials after block() (expected ~0-2)\n";
    CHECK(successes < 20);

    // unblock restores everything
    b.clear();
    b.block();
    b.unblock();
    CHECK(b.placeShip(Point(0, 0), 0, HORIZONTAL) == true);
}

// ── Board: display format ───────────────────────────────────────────────────
void test_board_display() {
    SECTION("Board: display format & shotsOnly hides ships");
    Game g(3, 4);
    g.addShip(2, 'R', "rowboat");
    Board b(g);
    b.clear();
    b.placeShip(Point(0, 0), 0, HORIZONTAL);

    // Full display: should contain header and 'RR..' row
    {
        CoutSink sink;
        b.display(false);
        string out = sink.str();
        CHECK(out.find("  0123") != string::npos);
        CHECK(out.find("RR..")   != string::npos);
    }
    // shotsOnly=true should hide the undamaged ship
    {
        CoutSink sink;
        b.display(true);
        string out = sink.str();
        CHECK(out.find("RR..") == string::npos);
        CHECK(out.find("....") != string::npos);
    }
    // After a hit, X should appear in either mode
    {
        bool hit=false, sunk=false; int id=-1;
        b.attack(Point(0, 0), hit, sunk, id);
        CoutSink sink;
        b.display(true);
        string out = sink.str();
        CHECK(out.find('X') != string::npos);
    }
}

// ── Game: addShip validation ────────────────────────────────────────────────
void test_game_addship() {
    SECTION("Game: addShip rejects invalid inputs");
    Game g(10, 10);
    CoutSink sink;  // addShip prints errors on rejection
    CHECK(g.addShip(5, 'A', "carrier") == true);
    CHECK(g.addShip(4, 'A', "dup symbol")  == false);
    CHECK(g.addShip(3, 'X', "uses X")      == false);
    CHECK(g.addShip(3, '.', "uses .")      == false);
    CHECK(g.addShip(3, 'o', "uses o")      == false);
    CHECK(g.addShip(0, 'Z', "zero length") == false);
    CHECK(g.addShip(11,'Y', "too long")    == false);  // > rows AND > cols
    CHECK(g.nShips() == 1);
}

// ── Player factory ─────────────────────────────────────────────────────────
void test_create_player() {
    SECTION("Player: createPlayer factory");
    Game g(10, 10);
    addStandardShips(g);

    Player* p;
    p = createPlayer("awful",    "a", g); CHECK(p != nullptr); CHECK(!p->isHuman()); delete p;
    p = createPlayer("human",    "h", g); CHECK(p != nullptr); CHECK(p->isHuman());  delete p;
    p = createPlayer("mediocre", "m", g); CHECK(p != nullptr); CHECK(!p->isHuman()); delete p;
    p = createPlayer("good",     "g", g); CHECK(p != nullptr); CHECK(!p->isHuman()); delete p;
    p = createPlayer("nonsense", "?", g); CHECK(p == nullptr);
}

// ── Match tests (statistical) ──────────────────────────────────────────────
static int runMatch(const char* a, const char* b, int n) {
    int aWins = 0;
    for (int k = 0; k < n; k++) {
        Game g(10, 10);
        addStandardShips(g);
        Player* p1 = createPlayer(a, "A", g);
        Player* p2 = createPlayer(b, "B", g);
        CoutSink sink;
        // Alternate who goes first to neutralize first-move advantage.
        Player* winner = (k % 2 == 0) ? g.play(p1, p2, false)
                                      : g.play(p2, p1, false);
        if (winner == p1) aWins++;
        delete p1;
        delete p2;
    }
    return aWins;
}

void test_match_mediocre_vs_awful() {
    SECTION("Match: mediocre vs awful (50 games)");
    int wins = runMatch("mediocre", "awful", 50);
    cout << "  info: mediocre won " << wins << "/50 (expect >= 40)\n";
    CHECK(wins >= 40);
}

void test_match_good_vs_mediocre() {
    SECTION("Match: good vs mediocre (50 games)");
    int wins = runMatch("good", "mediocre", 50);
    cout << "  info: good won " << wins << "/50 (expect >= 30)\n";
    CHECK(wins >= 30);
}

// ── Timing tests ───────────────────────────────────────────────────────────
void test_timing() {
    SECTION("Timing: GoodPlayer placeShips < 2s, attack cycle < 4s");
    Game g(10, 10);
    addStandardShips(g);
    Player* good = createPlayer("good", "G", g);
    Board b(g);

    auto t0 = steady_clock::now();
    bool ok = good->placeShips(b);
    auto t1 = steady_clock::now();
    long ms = duration_cast<milliseconds>(t1 - t0).count();
    cout << "  info: placeShips took " << ms << " ms\n";
    CHECK(ok);
    CHECK(ms < 2000);

    // One full attack cycle against a populated opposing board.
    Board opp(g);
    Player* victim = createPlayer("awful", "V", g);
    victim->placeShips(opp);

    long worstUs = 0;
    for (int i = 0; i < 100; i++) {
        auto a = steady_clock::now();
        Point p = good->recommendAttack();
        bool hit=false, sunk=false; int id=-1;
        opp.attack(p, hit, sunk, id);
        good->recordAttackResult(p, true, hit, sunk, id);
        auto z = steady_clock::now();
        long us = duration_cast<microseconds>(z - a).count();
        if (us > worstUs) worstUs = us;
    }
    cout << "  info: worst attack cycle over 100 shots: " << worstUs << " us\n";
    CHECK(worstUs < 4'000'000);

    delete good;
    delete victim;
}

// ── main ────────────────────────────────────────────────────────────────────
int main() {
    test_board_placement();
    test_board_attack();
    test_board_block();
    test_board_display();
    test_game_addship();
    test_create_player();
    test_match_mediocre_vs_awful();
    test_match_good_vs_mediocre();
    test_timing();

    cout << "\n========================================\n";
    cout << "Results: " << g_pass << " passed, " << g_fail << " failed\n";
    cout << "========================================\n";
    return g_fail == 0 ? 0 : 1;
}