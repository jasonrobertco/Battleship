#include "Player.h"
#include "Board.h"
#include "Game.h"
#include "globals.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

//*********************************************************************
//  AwfulPlayer
//*********************************************************************

class AwfulPlayer : public Player
{
  public:
    AwfulPlayer(string nm, const Game& g);
    virtual bool placeShips(Board& b);
    virtual Point recommendAttack();
    virtual void recordAttackResult(Point p, bool validShot, bool shotHit,
                                                bool shipDestroyed, int shipId);
    virtual void recordAttackByOpponent(Point p);
  private:
    Point m_lastCellAttacked;
};

AwfulPlayer::AwfulPlayer(string nm, const Game& g)
 : Player(nm, g), m_lastCellAttacked(0, 0)
{}

bool AwfulPlayer::placeShips(Board& b)
{
      // Clustering ships is bad strategy
    for (int k = 0; k < game().nShips(); k++)
        if ( ! b.placeShip(Point(k,0), k, HORIZONTAL))
            return false;
    return true;
}

Point AwfulPlayer::recommendAttack()
{
    if (m_lastCellAttacked.c > 0)
        m_lastCellAttacked.c--;
    else
    {
        m_lastCellAttacked.c = game().cols() - 1;
        if (m_lastCellAttacked.r > 0)
            m_lastCellAttacked.r--;
        else
            m_lastCellAttacked.r = game().rows() - 1;
    }
    return m_lastCellAttacked;
}

void AwfulPlayer::recordAttackResult(Point /* p */, bool /* validShot */,
                                     bool /* shotHit */, bool /* shipDestroyed */,
                                     int /* shipId */)
{
      // AwfulPlayer completely ignores the result of any attack
}

void AwfulPlayer::recordAttackByOpponent(Point /* p */)
{
      // AwfulPlayer completely ignores what the opponent does
}

//*********************************************************************
//  HumanPlayer
//*********************************************************************

bool getLineWithTwoIntegers(int& r, int& c)
{
    bool result(cin >> r >> c);
    if (!result)
        cin.clear();  // clear error state so can do more input operations
    cin.ignore(10000, '\n');
    return result;
}

//all decisions from user input not computer ai
class HumanPlayer : public Player
{
  public:
    HumanPlayer(string nm, const Game& g);
    virtual bool  isHuman() const;
    virtual bool  placeShips(Board& b);
    virtual Point recommendAttack();
    virtual void  recordAttackResult(Point p, bool validShot, bool shotHit, bool shipDestroyed, int shipId);
    virtual void  recordAttackByOpponent(Point p);
};

//ctor
HumanPlayer::HumanPlayer(string nm, const Game& g) : Player(nm, g) {}

//for hiding ship functions and pvp
bool HumanPlayer::isHuman() const {
    return true;
}

//placing ships
bool HumanPlayer::placeShips(Board& b)
{
    cout << "----- " << name() << " -----" << endl;
    b.clear();

    for (int k = 0; k < game().nShips(); k++)
    {
        b.display(false);
        Direction dir;

        //direction
        while (true)
        {
            cout << "Enter h or v for the direction of the " << game().shipName(k)
                 << " (length " << game().shipLength(k) << "): ";
            string line;
            getline(cin, line);
            if (!line.empty() && (line[0] == 'h' || line[0] == 'H'))
                { dir = HORIZONTAL; break; }
            else if (!line.empty() && (line[0] == 'v' || line[0] == 'V'))
                { dir = VERTICAL; break; }
            else
                cout << "Direction must be h or v." << endl;
        }

        //anchor cell 
        //hoz left
        //vert top
        while (true)
        {
            if (dir == HORIZONTAL)
                cout << "Enter the row and column of the leftmost cell (e.g., 3 5): ";
            else
                cout << "Enter the row and column of the topmost cell (e.g., 3 5): ";
            int r, c;
            if (!getLineWithTwoIntegers(r, c))
                { cout << "You must enter two integers." << endl; continue; }
            if (b.placeShip(Point(r, c), k, dir))
                break;
            cout << "The ship cannot be placed there." << endl;
        }
    }
    return true;
}

//2 integer input
Point HumanPlayer::recommendAttack()
{
    int r, c;
    while (true)
    {
        cout << "Enter the row and column to attack (e.g., 3 5): ";
        if (getLineWithTwoIntegers(r, c))
            return Point(r, c);
        cout << "You must enter two integers." << endl;
    }
}

void HumanPlayer::recordAttackResult(Point, bool, bool, bool, int) {}
void HumanPlayer::recordAttackByOpponent(Point) {}

//*********************************************************************
//  MediocrePlayer
//*********************************************************************

/*
places ships randomly by blocking off half of board recursively
state machine
state 1 attacks randomly
state 2 attacks around cell
*/

class MediocrePlayer : public Player
{
  public:
    MediocrePlayer(string nm, const Game& g);
    virtual bool  placeShips(Board& b);
    virtual Point recommendAttack();
    virtual void  recordAttackResult(Point p, bool validShot, bool shotHit, bool shipDestroyed, int shipId);
    virtual void  recordAttackByOpponent(Point p);
  private:
    bool  m_attacked[MAXROWS][MAXCOLS];
    int   m_state; //state 1, satate 2 ^see above
    Point m_hitPoint; //state 2 around point
    bool placeShipsHelper(Board& b, int shipNum);
};

//ctor state 1 randomyl firing until something hit
MediocrePlayer::MediocrePlayer(string nm, const Game& g) : Player(nm, g), m_state(1), m_hitPoint(0, 0)
{
    for (int r = 0; r < MAXROWS; r++)
        for (int c = 0; c < MAXCOLS; c++)
            m_attacked[r][c] = false;
}

//recursively place ships until base case, retry if invalid
bool MediocrePlayer::placeShipsHelper(Board& b, int shipNum)
{
    if (shipNum == game().nShips())
        return true; //base case all ships accepted

    for (int r = 0; r < game().rows(); r++)
        for (int c = 0; c < game().cols(); c++)
            for (int d = 0; d < 2; d++)
            {
                Direction dir = (d == 0) ? HORIZONTAL : VERTICAL;
                if (b.placeShip(Point(r, c), shipNum, dir))
                {
                    if (placeShipsHelper(b, shipNum + 1))  //recurse for next ship
                        return true;
                    b.unplaceShip(Point(r, c), shipNum, dir); //backtrack
                }
            }
    return false;//no valid pos for ship
}

//block half the board
//run recursive placement
//unblock retry "up to 50 times" per spec
bool MediocrePlayer::placeShips(Board& b)
{
    for (int attempt = 0; attempt < 50; attempt++)
    {
        b.clear();
        b.block(); //block half
        if (placeShipsHelper(b, 0))
        {
            b.unblock(); //remove blocks
            return true; //return true ships placed
        }
        b.unblock(); //clear block -> retry
    }
    return false;
}

//choose next attack if state 2 find surrounding
//else state 1 randomly fire
Point MediocrePlayer::recommendAttack()
{
    if (m_state == 2)
    {
        vector<Point> cross;
        static const int dr[] = {-1, 1,  0, 0};
        static const int dc[] = { 0, 0, -1, 1};
        for (int step = 1; step <= 4; step++)
            for (int dir = 0; dir < 4; dir++)
            {
                Point p(m_hitPoint.r + dr[dir] * step,
                        m_hitPoint.c + dc[dir] * step);
                if (game().isValid(p) && !m_attacked[p.r][p.c])
                    cross.push_back(p);
            }
        if (!cross.empty())
            return cross[randInt(cross.size())];
        m_state = 1; //cross empty back to state 1
    }
    //state 1 random attack
    vector<Point> available;
    for (int r = 0; r < game().rows(); r++)
        for (int c = 0; c < game().cols(); c++)
            if (!m_attacked[r][c])
                available.push_back(Point(r, c));
    return available[randInt(available.size())];
}

//recordAttackResult
//s1 + hit no sink -> s2
//s2 + sink -> s1
void MediocrePlayer::recordAttackResult(Point p, bool validShot, bool shotHit, bool shipDestroyed, int /*shipId*/)
{
    if (!validShot) return;
    m_attacked[p.r][p.c] = true;
    switch (m_state)
    {
      case 1:
        if (shotHit && !shipDestroyed)
        {
            m_state    = 2; //hit no sink cross search
            m_hitPoint = p; //p is cross pt
        }
        break;
      case 2:
        if (shipDestroyed)
            m_state = 1; //ship sunk
        break;
    }
}

//Spec Mediocre ignores the opponent's attack positions.
void MediocrePlayer::recordAttackByOpponent(Point) {}

//*********************************************************************
//  GoodPlayer
//*********************************************************************
/*
Place ships randomly until a desired layout is found
Attacking
Checkerboard to search efficiently
After hit, attack nearby cells until 2 hits reveal a line to attack in
the respective row/column
*/
class GoodPlayer : public Player
{
  public:
    GoodPlayer(string nm, const Game& g);
    virtual bool  placeShips(Board& b);
    virtual Point recommendAttack();
    virtual void  recordAttackResult(Point p, bool validShot, bool shotHit, bool shipDestroyed, int shipId);
    virtual void  recordAttackByOpponent(Point p);

  private:
    bool m_attacked[MAXROWS][MAXCOLS]; //true for attacked
    bool m_hit[MAXROWS][MAXCOLS]; //true for hits
    vector<Point> m_huntOrder; //checkerboard attack
    int m_huntIdx;  //next index for m_huntOrder
    bool m_targeting; //hit ship unsunk
    Point m_firstHit; //first hit point to track where the attack runs tarted
    bool m_axisKnown; //set H/V
    bool m_horizontal; //set direciton
    vector<Point> m_targetQueue; //ordered cells to try in target mode
    void buildCheckerOrder();
    void addAdjacentTargets(Point p);
    void addTargetsAlongAxis(Point anchor, bool horiz);
};

// Initialise all attack state
GoodPlayer::GoodPlayer(string nm, const Game& g) : Player(nm, g),
   m_huntIdx(0), 
   m_targeting(false), 
   m_firstHit(0, 0),
   m_axisKnown(false), 
   m_horizontal(false)
{
    for (int r = 0; r < MAXROWS; r++)
        for (int c = 0; c < MAXCOLS; c++)
            m_attacked[r][c] = m_hit[r][c] = false;
    buildCheckerOrder(); //build checker order
}

// buildCheckerOrder
//r+c even 1st
//r+c odd 2nd
void GoodPlayer::buildCheckerOrder()
{
    vector<Point> p0, p1;
    for (int r = 0; r < game().rows(); r++)
        for (int c = 0; c < game().cols(); c++)
        {
            if ((r + c) % 2 == 0){
                p0.push_back(Point(r, c));
            } 
            else{
                p1.push_back(Point(r, c));
            }               
        }
    for (int i = (int)p0.size() - 1; i > 0; i--)
        swap(p0[i], p0[randInt(i + 1)]);
    for (int i = (int)p1.size() - 1; i > 0; i--)
        swap(p1[i], p1[randInt(i + 1)]);
    m_huntOrder = p0;
    m_huntOrder.insert(m_huntOrder.end(), p1.begin(), p1.end());
}

// addAdjacentTargets to the queue to attack
void GoodPlayer::addAdjacentTargets(Point p)
{
    static const int dr[] = {-1, 1,  0, 0};
    static const int dc[] = { 0, 0, -1, 1};
    for (int i = 0; i < 4; i++)
    {
        Point n(p.r + dr[i], p.c + dc[i]);
        if (game().isValid(n) && !m_attacked[n.r][n.c])
            m_targetQueue.push_back(n);
    }
}

//addTargetsAlongAxis rebuild queue once the target an axis set
void GoodPlayer::addTargetsAlongAxis(Point anchor, bool horiz)
{
    m_targetQueue.clear();
    const int limit = horiz ? game().cols() : game().rows();
    const int start = horiz ? anchor.c : anchor.r;
    vector<Point> pos, neg; //nearest checked first
    for (int i = start + 1; i < limit; i++)
    {
        Point q = horiz ? Point(anchor.r, i) : Point(i, anchor.c);
        if (m_attacked[q.r][q.c])
        {
            if (m_hit[q.r][q.c]) continue;
            break;
        }
        pos.push_back(q);
    }
    for (int i = start - 1; i >= 0; i--)
    {
        Point q = horiz ? Point(anchor.r, i) : Point(i, anchor.c);
        if (m_attacked[q.r][q.c])
        {
            if (m_hit[q.r][q.c]) continue;
            break;
        }
        neg.push_back(q);
    }

    //Scan backward left if hoz, up if vert
    vector<Point> order;
    for (size_t i = 0; i < pos.size() || i < neg.size(); i++)
    {
        if (i < pos.size()) order.push_back(pos[i]);
        if (i < neg.size()) order.push_back(neg[i]);
    }
    for (int i = (int)order.size() - 1; i >= 0; i--)
        m_targetQueue.push_back(order[i]);
}

// place ships 1000 VERY CONSERVATIVE 
bool GoodPlayer::placeShips(Board& b)
{
    for (int attempt = 0; attempt < 1000; attempt++)
    {
        b.clear();
        bool success = true;
        for (int k = 0; k < game().nShips(); k++)
        {
            bool placed = false;
            for (int t = 0; t < 500 && !placed; t++)
            {
                Direction dir = (randInt(2) == 0) ? HORIZONTAL : VERTICAL;
                if (b.placeShip(game().randomPoint(), k, dir))
                    placed = true;
            }
            if (!placed) { success = false; break; }  // give up on this layout
        }
        if (success) return true;
    }
    return false;
}

// recommend attack first queue targeteed then checkerboard and linearscan
/*
target use q
use checkherboard
use linear scan
random point final
*/
Point GoodPlayer::recommendAttack()
{
    //target queue mode
    while (m_targeting && !m_targetQueue.empty())
    {
        Point p = m_targetQueue.back();
        m_targetQueue.pop_back();
        if (!m_attacked[p.r][p.c])
            return p;
    }
    if (m_targeting)
    {
        m_targeting = false;
        m_axisKnown = false;
    }
    while (m_huntIdx < (int)m_huntOrder.size())
    {
        Point p = m_huntOrder[m_huntIdx++];
        if (!m_attacked[p.r][p.c])
            return p;
    }
    for (int r = 0; r < game().rows(); r++)
        for (int c = 0; c < game().cols(); c++)
            if (!m_attacked[r][c])
                return Point(r, c);
    return game().randomPoint();
}
/*
ship sunk
first hit
2nd fit
subsequent hits
*/
void GoodPlayer::recordAttackResult(Point p, bool validShot, bool shotHit, bool shipDestroyed, int /*shipId*/)
{
    if (!validShot) return;

    m_attacked[p.r][p.c] = true;
    if (shotHit) m_hit[p.r][p.c] = true;

    if (shipDestroyed)
    {
        m_targeting  = false;
        m_axisKnown  = false;
        m_targetQueue.clear();
        return;
    }
    if (shotHit)
    {
        if (!m_targeting)
        {
            m_targeting = true;
            m_firstHit  = p;
            m_axisKnown = false;
            addAdjacentTargets(p);
        }
        else if (!m_axisKnown)
        {
            m_axisKnown  = true;
            m_horizontal = (p.r == m_firstHit.r);
            addTargetsAlongAxis(m_firstHit, m_horizontal);
        }
        else
        {
            addTargetsAlongAxis(m_firstHit, m_horizontal);
        }
    }
    else if (m_targeting && m_axisKnown)
    {
        addTargetsAlongAxis(m_firstHit, m_horizontal);
    }
}
void GoodPlayer::recordAttackByOpponent(Point) {}

//*********************************************************************
//  createPlayer
//*********************************************************************

Player* createPlayer(string type, string nm, const Game& g)
{
    static string types[] = {
        "human", "awful", "mediocre", "good"
    };
    
    int pos;
    for (pos = 0; pos != sizeof(types)/sizeof(types[0])  &&
                                                     type != types[pos]; pos++)
        ;
    switch (pos)
    {
      case 0:  return new HumanPlayer(nm, g);
      case 1:  return new AwfulPlayer(nm, g);
      case 2:  return new MediocrePlayer(nm, g);
      case 3:  return new GoodPlayer(nm, g);
      default: return nullptr;
    }
}
