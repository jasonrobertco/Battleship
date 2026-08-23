#include "Game.h"
#include "Board.h"
#include "Player.h"
#include "globals.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cctype>

using namespace std;

class GameImpl
{
  public:
    GameImpl(int nRows, int nCols);
    int rows() const;
    int cols() const;
    bool isValid(Point p) const;
    Point randomPoint() const;
    bool addShip(int length, char symbol, string name);
    int nShips() const;
    int shipLength(int shipId) const;
    char shipSymbol(int shipId) const;
    string shipName(int shipId) const;
    Player* play(Player* p1, Player* p2, Board& b1, Board& b2, bool shouldPause);

  private:
    //struct for ship to hold lenght, display, name
    struct ShipInfo
    {
        int    length;
        char   symbol;
        string name;
    };
    int m_rows;
    int m_cols;
    vector<ShipInfo> m_ships; //ship info
};


void waitForEnter()
{
    cout << "Press enter to continue: ";
    cin.ignore(10000, '\n'); //ignore
}

//ctor
//store board dimesnions
GameImpl::GameImpl(int nRows, int nCols) : m_rows(nRows), m_cols(nCols) //member init list
{
}

//return functions for private members
int GameImpl::rows() const {
    return m_rows; 
}
int GameImpl::cols() const {
    return m_cols; 
}

//isValid
bool GameImpl::isValid(Point p) const
{
    return p.r >= 0 && p.r < m_rows && p.c >= 0 && p.c < m_cols; //boundary ceck p
}

//random point
//return a rnaodm point
Point GameImpl::randomPoint() const
{
    return Point(randInt(m_rows), randInt(m_cols));
}


//addShip validates the arguments before calling this helper
bool GameImpl::addShip(int length, char symbol, string name)
{
    ShipInfo s;
    s.length = length;
    s.symbol = symbol;
    s.name   = name;
    m_ships.push_back(s);
    return true;
}

//nships
//return distinct ships
int GameImpl::nShips() const
{
    return static_cast<int>(m_ships.size());
}

//return ship length
int GameImpl::shipLength(int shipId) const
{
    return m_ships[shipId].length;
}

//return symbol
char GameImpl::shipSymbol(int shipId) const
{
    return m_ships[shipId].symbol;
}

//return shipname
string GameImpl::shipName(int shipId) const
{
    return m_ships[shipId].name;
}

/*
p1 vs p2
setup
place ships return nulltpr if fail

until a player has no ships left
display defending board
attack shoots -> gets checked
shot applied and result calculated
announce result
check if win
continue
announce win
pvp reveal winners board
*/
Player* GameImpl::play(Player* p1, Player* p2, Board& b1, Board& b2, bool shouldPause)
{
    //ship placement abort if bad placement
    if (!p1->placeShips(b1))
        return nullptr;
    if (!p2->placeShips(b2))
        return nullptr;

    //p1 vs p2 arrays
    Player* players[2] = { p1, p2 };
    Board*  boards[2]  = { &b1, &b2 };
    int attacker_Idx = 0; //attacker index

    while (true) //game playing
    {
        int defender_Idx = 1 - attacker_Idx;
        Player* attacker = players[attacker_Idx];
        Player* defender = players[defender_Idx];
        Board*  defBoard = boards[defender_Idx];
        //attack defender board


        //display defender board if its human hide the unhit ships, computer displays all ships
        cout << attacker->name() << "'s turn.  " << defender->name() << "'s board:" << endl;
        defBoard->display(attacker->isHuman());

        //pick a point p
        Point p = attacker->recommendAttack();

        //apply shot
        bool validShot    = false;
        bool shotHit      = false;
        bool shipDestroyed = false;
        int  shipId       = -1;

        validShot = defBoard->attack(p, shotHit, shipDestroyed, shipId);

        //update state
        //attacker shot status and stats
        attacker->recordAttackResult(p, validShot, shotHit, shipDestroyed, shipId);
        //defender track
        defender->recordAttackByOpponent(p);

        //console
        cout << attacker->name() << " attacked (" << p.r << "," << p.c << ") and ";
        if (!validShot) //invalid shot
        {
            cout << "the attack was invalid." << endl;
        }
        else if (!shotHit) //miss
        {
            cout << "missed." << endl;
        }
        else if (!shipDestroyed) //hit
        {
            cout << "hit something." << endl;
        }
        else //sink
        {
            cout << "destroyed " << defender->name() << "'s " << shipName(shipId) << "." << endl;
        }

        //pause
        if (shouldPause)
            waitForEnter();

        //end condition
        if (defBoard->allShipsDestroyed())
        {
            cout << attacker->name() << " wins the game!" << endl;
            //pvp display the winners board
            if (defender->isHuman())
            {
                cout << "Here is " << attacker->name() << "'s final board:" << endl;
                boards[attacker_Idx]->display(false);
            }
            return attacker; //ptr to winners
        }
        //swap attacker/defender
        attacker_Idx = 1 - attacker_Idx;
    }
}

//******************** Game functions *******************************

// These functions for the most part simply delegate to GameImpl's functions.
// You probably don't want to change any of the code from this point down.

Game::Game(int nRows, int nCols)
{
    if (nRows < 1  ||  nRows > MAXROWS)
    {
        cout << "Number of rows must be >= 1 and <= " << MAXROWS << endl;
        exit(1);
    }
    if (nCols < 1  ||  nCols > MAXCOLS)
    {
        cout << "Number of columns must be >= 1 and <= " << MAXCOLS << endl;
        exit(1);
    }
    m_impl = new GameImpl(nRows, nCols);
}

Game::~Game()
{
    delete m_impl;
}

int Game::rows() const
{
    return m_impl->rows();
}

int Game::cols() const
{
    return m_impl->cols();
}

bool Game::isValid(Point p) const
{
    return m_impl->isValid(p);
}

Point Game::randomPoint() const
{
    return m_impl->randomPoint();
}

bool Game::addShip(int length, char symbol, string name)
{
    if (length < 1)
    {
        cout << "Bad ship length " << length << "; it must be >= 1" << endl;
        return false;
    }
    if (length > rows()  &&  length > cols())
    {
        cout << "Bad ship length " << length << "; it won't fit on the board"
             << endl;
        return false;
    }
    if (!isascii(symbol)  ||  !isprint(symbol))
    {
        cout << "Unprintable character with decimal value " << symbol
             << " must not be used as a ship symbol" << endl;
        return false;
    }
    if (symbol == 'X'  ||  symbol == '.'  ||  symbol == 'o')
    {
        cout << "Character " << symbol << " must not be used as a ship symbol"
             << endl;
        return false;
    }
    int totalOfShipLengths = 0;
    for (int s = 0; s < nShips(); s++)
    {
        totalOfShipLengths += shipLength(s);
        if (shipSymbol(s) == symbol)
        {
            cout << "Ship symbol " << symbol
                 << " must not be used for more than one ship" << endl;
            return false;
        }
    }
    if (totalOfShipLengths + length > rows() * cols())
    {
        cout << "Board is too small to fit all ships" << endl;
        return false;
    }
    return m_impl->addShip(length, symbol, name);
}

int Game::nShips() const
{
    return m_impl->nShips();
}

int Game::shipLength(int shipId) const
{
    assert(shipId >= 0  &&  shipId < nShips());
    return m_impl->shipLength(shipId);
}

char Game::shipSymbol(int shipId) const
{
    assert(shipId >= 0  &&  shipId < nShips());
    return m_impl->shipSymbol(shipId);
}

string Game::shipName(int shipId) const
{
    assert(shipId >= 0  &&  shipId < nShips());
    return m_impl->shipName(shipId);
}

Player* Game::play(Player* p1, Player* p2, bool shouldPause)
{
    if (p1 == nullptr  ||  p2 == nullptr  ||  nShips() == 0)
        return nullptr;
    Board b1(*this);
    Board b2(*this);
    return m_impl->play(p1, p2, b1, b2, shouldPause);
}

