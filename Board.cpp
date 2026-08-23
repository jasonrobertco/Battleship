#include "Board.h"
#include "Game.h"
#include "globals.h"
#include <iostream>
#include <vector>

using namespace std;

class BoardImpl
{
public:
    BoardImpl(const Game &g);
    void clear();
    void block();
    void unblock();
    bool placeShip(Point topOrLeft, int shipId, Direction dir);
    bool unplaceShip(Point topOrLeft, int shipId, Direction dir);
    void display(bool shotsOnly) const;
    bool attack(Point p, bool &shotHit, bool &shipDestroyed, int &shipId);
    bool allShipsDestroyed() const;

private:
    const Game &m_game; // given
    //
    int m_grid[MAXROWS][MAXCOLS];
    /*
        Game grid
        (m_grid[r][c])
        -1 = empty water
        -2 = blocked
        N = ship ID
    */
    bool m_attacked[MAXROWS][MAXCOLS]; // Attacked grid ie what has already been attacked true/false
    vector<bool> m_placed; //placing ships
    vector<int> m_hits; //store ship segment hits
};


//ctor
//init board to empty state
BoardImpl::BoardImpl(const Game &g) : m_game(g), m_placed(g.nShips(), false), m_hits(g.nShips(), 0)
{
    clear();
}

//clear all and reset gameboard to water and attackboard to false
void BoardImpl::clear()
{
    for (int r = 0; r < m_game.rows(); r++)
        for (int c = 0; c < m_game.cols(); c++)
        {
            m_grid[r][c] = -1; //water
            m_attacked[r][c] = false; //not attacked
        }
    //ship tracking
    m_placed.assign(m_game.nShips(), false);
    m_hits.assign(m_game.nShips(), 0);
}

//block
void BoardImpl::block()
{
    int total = m_game.rows() * m_game.cols();
    int toBlock = total / 2; //toblock half of cells on board

    vector<Point> positions; //create empty list positions
    positions.reserve(total);

    for (int r = 0; r < m_game.rows(); r++)
        for (int c = 0; c < m_game.cols(); c++)
            positions.push_back(Point(r, c)); //add to pos

    //select randomly form the list to block, swap as to not block 2x
    for (int i = 0; i < toBlock; i++)
    {
        int j = i + randInt(total - i); //random index
        swap(positions[i], positions[j]);
        m_grid[positions[i].r][positions[i].c] = -2;//block
    }
}

//unblock set to wter
void BoardImpl::unblock()
{
    for (int r = 0; r < m_game.rows(); r++)
        for (int c = 0; c < m_game.cols(); c++)
            if (m_grid[r][c] == -2) //if blocked unblock
                m_grid[r][c] = -1;
}

//placeship
//check if ship is valid
bool BoardImpl::placeShip(Point topOrLeft, int shipId, Direction dir)
{
    //checkship ID
    if (shipId < 0 || shipId >= m_game.nShips())
        return false;

    //alr placed
    if (m_placed[shipId])
        return false;

    //check if board can suppor tthis chip
    int len = m_game.shipLength(shipId);
    for (int k = 0; k < len; k++)
    {
        int r = topOrLeft.r + (dir == VERTICAL ? k : 0);
        int c = topOrLeft.c + (dir == HORIZONTAL ? k : 0);
        if (!m_game.isValid(Point(r, c))) //inside board
            return false;
        if (m_grid[r][c] != -1) //plain water not ship /blocked
            return false;
    }
    //place ship per unit length
    for (int k = 0; k < len; k++)
    {
        int r = topOrLeft.r + (dir == VERTICAL ? k : 0);
        int c = topOrLeft.c + (dir == HORIZONTAL ? k : 0);
        m_grid[r][c] = shipId; //set ID
    }
    m_placed[shipId] = true;
    return true;
}

//unplaceship
bool BoardImpl::unplaceShip(Point topOrLeft, int shipId, Direction dir)
{
    //checkship ID
    if (shipId < 0 || shipId >= m_game.nShips())
        return false;

    int len = m_game.shipLength(shipId);

    //check if board HAS this ship
    for (int k = 0; k < len; k++)
    {
        int r = topOrLeft.r + (dir == VERTICAL ? k : 0);
        int c = topOrLeft.c + (dir == HORIZONTAL ? k : 0);
        if (!m_game.isValid(Point(r, c))) //invalid removal do not try to remove
            return false;

        if (m_grid[r][c] != shipId) //wrong ship
            return false;
    }

    //remove ship
    for (int k = 0; k < len; k++)
    {
        int r = topOrLeft.r + (dir == VERTICAL ? k : 0);
        int c = topOrLeft.c + (dir == HORIZONTAL ? k : 0);
        m_grid[r][c] = -1; //to water
    }
    m_placed[shipId] = false;
    return true;
}

//display
/*
X attacked ship
o miss
. not attacked
*/
void BoardImpl::display(bool shotsOnly) const
{
    //Header title
    cout << "  ";
    for (int c = 0; c < m_game.cols(); c++)
        cout << c;
    cout << endl;

    //Row title
    for (int r = 0; r < m_game.rows(); r++)
    {
        cout << r << " ";
        for (int c = 0; c < m_game.cols(); c++)
        {
            if (m_attacked[r][c]) //ATTACK
            {
                if (m_grid[r][c] >= 0)
                    cout << 'X'; //HIT
                else
                    cout << 'o'; //WATER
            }
            else
            {
                if (!shotsOnly && m_grid[r][c] >= 0)
                    cout << m_game.shipSymbol(m_grid[r][c]); //ship seg
                else
                    cout << '.';
            }
        }
        cout << endl;
    }
}

//attack
bool BoardImpl::attack(Point p, bool &shotHit, bool &shipDestroyed, int &shipId)
{
    //bounds error
    if (!m_game.isValid(p))
        return false;

    //cannot attack same spot
    if (m_attacked[p.r][p.c])
        return false;

    //mark
    m_attacked[p.r][p.c] = true;

    //store id of attack target
    int id = m_grid[p.r][p.c];
    if (id >= 0)
    {
        //ship segment
        shotHit = true;
        m_hits[id]++;
        //full ship destroyed
        if (m_hits[id] == m_game.shipLength(id))
        {
            shipDestroyed = true;
            shipId = id;
        }
        else
        {
            shipDestroyed = false;
            //shipId not modified
        }
    }
    else
    {
        //Miss
        shotHit = false;
        shipDestroyed = false;
        //shipId not modified
    }

    return true;
}

// allShipsDestroyed
bool BoardImpl::allShipsDestroyed() const
{
    for (int id = 0; id < m_game.nShips(); id++)
    {
        //check ships on board and if there are any undamaged segments then game is still ongoing
        if (m_placed[id] && m_hits[id] < m_game.shipLength(id))
            return false;
    }
    return true;
}

//******************** Board functions ********************************

// These functions simply delegate to BoardImpl's functions.
// You probably don't want to change any of this code.

Board::Board(const Game& g)
{
    m_impl = new BoardImpl(g);
}

Board::~Board()
{
    delete m_impl;
}

void Board::clear()
{
    m_impl->clear();
}

void Board::block()
{
    return m_impl->block();
}

void Board::unblock()
{
    return m_impl->unblock();
}

bool Board::placeShip(Point topOrLeft, int shipId, Direction dir)
{
    return m_impl->placeShip(topOrLeft, shipId, dir);
}

bool Board::unplaceShip(Point topOrLeft, int shipId, Direction dir)
{
    return m_impl->unplaceShip(topOrLeft, shipId, dir);
}

void Board::display(bool shotsOnly) const
{
    m_impl->display(shotsOnly);
}

bool Board::attack(Point p, bool& shotHit, bool& shipDestroyed, int& shipId)
{
    return m_impl->attack(p, shotHit, shipDestroyed, shipId);
}

bool Board::allShipsDestroyed() const
{
    return m_impl->allShipsDestroyed();
}
