#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
using namespace std;

// Base Robot class with common attributes and basic methods
class Robot {
protected:
    string name;
    char symbol;
    int x, y;            // Position on battlefield
    int health;          // Current hit points
    int shells;          // Current ammunition count
    int lives;           // Extra lives (respawns)
    bool alive;          // Alive status

    int initHealth;      // Initial HP (for respawn)
    int initShells;      // Initial ammo (for respawn)

public:
    // Constructor sets name, symbol, position, health, ammo, and extra lives
    Robot(string n, char sym, int startX, int startY, int hp, int ammo, int extraLives)
        : name(n), symbol(sym), x(startX), y(startY),
          health(hp), shells(ammo), lives(extraLives), alive(true),
          initHealth(hp), initShells(ammo) {}

    virtual ~Robot() {}

    // Getters
    string getName() const { return name; }
    char getSymbol() const { return symbol; }
    int getX() const { return x; }
    int getY() const { return y; }
    bool isAlive() const { return alive; }
    int getHealth() const { return health; }
    int getShells() const { return shells; }
    int getLives() const { return lives; }

    // Check and use extra life
    bool hasExtraLife() const { return lives > 0; }
    void useExtraLife() { if (lives > 0) lives--; }

    // Take 1 damage and handle death
    void takeDamage() {
        if (!alive) return;
        health--;
        cout << name << " is hit! (Health=" << health << ")";
        if (health <= 0) {
            alive = false;
            cout << " --> " << name << " is destroyed!";
        }
        cout << endl;
    }

    // Instant destruction (e.g. shells run out)
    void destroySelf() {
        if (!alive) return;
        alive = false;
        cout << name << " has no shells left and self-destructs!" << endl;
    }

    // Respawn with full health/ammo at a position
    void respawn(int newX, int newY) {
        alive = true;
        health = initHealth;
        shells = initShells;
        x = newX;
        y = newY;
        cout << name << " respawns at (" << x << "," << y << ") with health="
             << health << " and shells=" << shells << endl;
    }

    // Virtual actions to implement
    virtual void think() = 0;
    virtual void look(const vector<Robot*>& robots) = 0;
    virtual void fire(vector<Robot*>& robots) = 0;
    virtual void move(const vector<Robot*>& robots, int width, int height) = 0;
};

// Capability interface classes
class MovingRobot {
public:
    virtual void move(const vector<Robot*>& robots, int width, int height) = 0;
};
class ShootingRobot {
public:
    virtual void fire(vector<Robot*>& robots) = 0;
};
class SeeingRobot {
public:
    virtual void look(const vector<Robot*>& robots) = 0;
};
class ThinkingRobot {
public:
    virtual void think() = 0;
};

// GenericRobot: implements all capabilities (base behavior)
class GenericRobot : public Robot, public MovingRobot, public ShootingRobot, public SeeingRobot, public ThinkingRobot {
public:
    GenericRobot(string n, char sym, int startX, int startY, int hp, int ammo, int extraLives)
        : Robot(n, sym, startX, startY, hp, ammo, extraLives) {}

    // Think: simple action decision (just prints a message)
    void think() override {
        if (!alive) return;
        cout << name << " is thinking..." << endl;
    }

    // Look: see any robot within 1-cell (Moore neighborhood)
    void look(const vector<Robot*>& robots) override {
        if (!alive) return;
        cout << name << " looks around." << endl;
        bool found = false;
        for (Robot* other : robots) {
            if (other == this || !other->isAlive()) continue;
            int dx = abs(other->getX() - x), dy = abs(other->getY() - y);
            if (dx <= 1 && dy <= 1) {
                cout << "  " << name << " sees " << other->getName()
                     << " at (" << other->getX() << "," << other->getY() << ")" << endl;
                found = true;
            }
        }
        if (!found) {
            cout << "  " << name << " sees no one nearby." << endl;
        }
    }

    // Fire: shoot one target with 70% hit chance
    void fire(vector<Robot*>& robots) override {
        if (!alive || shells <= 0) return;
        // Pick the first alive target that's not self
        Robot* target = nullptr;
        for (Robot* r : robots) {
            if (r != this && r->isAlive()) { target = r; break; }
        }
        if (!target) return;
        cout << name << " fires at " << target->getName() << "... ";
        shells--;
        // 70% hit probability using rand()
        bool hit = (rand() % 100) < 70;  // standard practice for percentage:contentReference[oaicite:6]{index=6}
        if (hit) {
            cout << "HIT!" << endl;
            target->takeDamage();
        } else {
            cout << "misses." << endl;
        }
        // If out of ammo, self-destruct
        if (shells <= 0) {
            destroySelf();
        }
    }

    // Move: pick a random free adjacent cell (8 directions)
    void move(const vector<Robot*>& robots, int width, int height) override {
        if (!alive) return;
        vector<pair<int,int>> freeCells;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue;
                int nx = x + dx, ny = y + dy;
                if (nx<0 || nx>=width || ny<0 || ny>=height) continue;
                bool occupied = false;
                for (Robot* r : robots) {
                    if (r->isAlive() && r != this && r->getX()==nx && r->getY()==ny) {
                        occupied = true; break;
                    }
                }
                if (!occupied) freeCells.push_back({nx, ny});
            }
        }
        if (!freeCells.empty()) {
            int idx = rand() % freeCells.size();
            x = freeCells[idx].first;
            y = freeCells[idx].second;
            cout << name << " moves to (" << x << "," << y << ")." << endl;
        } else {
            cout << name << " cannot find a free adjacent cell to move." << endl;
        }
    }
};

// JumpBot: can jump to any free location on the map
class JumpBot : public GenericRobot {
public:
    JumpBot(string n, char sym, int startX, int startY, int hp, int ammo, int extraLives)
        : GenericRobot(n, sym, startX, startY, hp, ammo, extraLives) {}
    void move(const vector<Robot*>& robots, int width, int height) override {
        if (!alive) return;
        vector<pair<int,int>> freeCells;
        for (int nx = 0; nx < width; nx++) {
            for (int ny = 0; ny < height; ny++) {
                bool occupied = false;
                for (Robot* r : robots) {
                    if (r->isAlive() && r != this && r->getX()==nx && r->getY()==ny) {
                        occupied = true; break;
                    }
                }
                if (!occupied) freeCells.push_back({nx, ny});
            }
        }
        if (!freeCells.empty()) {
            int idx = rand() % freeCells.size();
            x = freeCells[idx].first;
            y = freeCells[idx].second;
            cout << name << " jumps to (" << x << "," << y << ")." << endl;
        } else {
            cout << name << " cannot find a free cell to jump to." << endl;
        }
    }
};

// SemiAutoBot: fires three shots per shell
class SemiAutoBot : public GenericRobot {
public:
    SemiAutoBot(string n, char sym, int startX, int startY, int hp, int ammo, int extraLives)
        : GenericRobot(n, sym, startX, startY, hp, ammo, extraLives) {}
    void fire(vector<Robot*>& robots) override {
        if (!alive || shells <= 0) return;
        Robot* target = nullptr;
        for (Robot* r : robots) {
            if (r != this && r->isAlive()) { target = r; break; }
        }
        if (!target) return;
        cout << name << " (Semi-auto) fires triple burst at " << target->getName() << ": ";
        shells--;
        int hits = 0;
        // Up to 3 shots
        for (int shot = 0; shot < 3; shot++) {
            bool hit = (rand() % 100) < 70;
            if (hit) {
                hits++;
                target->takeDamage();
                if (!target->isAlive()) break;
            }
        }
        if (hits == 0) {
            cout << "No hits." << endl;
        } else {
            cout << hits << " hits landed." << endl;
        }
        if (shells <= 0) {
            destroySelf();
        }
    }
};

// ScoutBot: can reveal entire map up to 3 times
class ScoutBot : public GenericRobot {
    int scansLeft;
public:
    ScoutBot(string n, char sym, int startX, int startY, int hp, int ammo, int extraLives)
        : GenericRobot(n, sym, startX, startY, hp, ammo, extraLives), scansLeft(3) {}
    void look(const vector<Robot*>& robots) override {
        if (!alive) return;
        if (scansLeft > 0) {
            cout << name << " uses a scan to reveal all robots on the map:" << endl;
            for (Robot* other : robots) {
                if (other != this && other->isAlive()) {
                    cout << "  " << other->getName() << " at (" 
                         << other->getX() << "," << other->getY() << ")" << endl;
                }
            }
            scansLeft--;
        } else {
            // Fall back to normal look
            GenericRobot::look(robots);
        }
    }
};

// Main simulation
int main() {
    srand((unsigned)time(0));  // Seed RNG

    const int width = 10, height = 5, numTurns = 10;
    vector<Robot*> robots;
    // Initial robots: name, symbol, x, y, health, shells, extra lives
    robots.push_back(new GenericRobot("GenericBot", 'G', 1, 1, 3, 5, 1));
    robots.push_back(new JumpBot("JumpBot", 'J', 1, 3, 3, 4, 1));
    robots.push_back(new SemiAutoBot("SemiAutoBot", 'M', 8, 1, 3, 3, 1));
    robots.push_back(new ScoutBot("ScoutBot", 'S', 8, 3, 3, 4, 1));

    vector<Robot*> respawnQueue;
    for (int turn = 1; turn <= numTurns; turn++) {
        cout << "----- Turn " << turn << " -----" << endl;

        // Respawn robots waiting in queue
        for (Robot* r : respawnQueue) {
            int nx, ny;
            // Find a free location
            while (true) {
                nx = rand() % width; ny = rand() % height;
                bool occ = false;
                for (Robot* o : robots) {
                    if (o->isAlive() && o->getX()==nx && o->getY()==ny) { occ = true; break; }
                }
                if (!occ) break;
            }
            r->respawn(nx, ny);
            robots.push_back(r);
        }
        respawnQueue.clear();

        // Draw battlefield
        vector<string> grid(height, string(width, '.'));
        for (Robot* r : robots) {
            if (r->isAlive()) {
                grid[r->getY()][r->getX()] = r->getSymbol();
            }
        }
        cout << "Battlefield:" << endl;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                cout << grid[y][x] << ' ';
            }
            cout << endl;
        }

        // Each robot takes a turn
        for (size_t i = 0; i < robots.size(); i++) {
            Robot* r = robots[i];
            if (!r->isAlive()) {
                // Remove dead robot (queue if it has extra life)
                if (r->hasExtraLife()) {
                    r->useExtraLife();
                    respawnQueue.push_back(r);
                }
                cout << r->getName() << " is removed from battlefield." << endl;
                robots.erase(robots.begin() + i);
                i--; 
                continue;
            }
            // Actions: think, look, fire, move
            r->think();
            r->look(robots);
            r->fire(robots);

            // If it died after firing (e.g. self-destruct or got hit)
            if (!r->isAlive()) {
                if (r->hasExtraLife()) {
                    r->useExtraLife();
                    respawnQueue.push_back(r);
                }
                cout << r->getName() << " is removed from battlefield." << endl;
                robots.erase(robots.begin() + i);
                i--;
                continue;
            }
            r->move(robots, width, height);
        }

        // Cleanup: remove any remaining dead robots
        for (size_t i = 0; i < robots.size(); i++) {
            Robot* r = robots[i];
            if (!r->isAlive()) {
                if (r->hasExtraLife()) {
                    r-b>useExtraLife();
                    respawnQueue.push_back(r);
                }
                cout << r->getName() << " is removed from battlefield." << endl;
                robots.erase(robots.begin() + i);
                i--;
            }
        }

        // Status after turn
        cout << "--- Status after Turn " << turn << " ---" << endl;
        for (Robot* r : robots) {
            cout << r->getName() << " at (" << r->getX() << "," << r->getY() << ") ";
            cout << "HP=" << r->getHealth() 
                 << " shells=" << r->getShells() 
                 << " lives=" << r->getLives() << endl;
        }
        cout << endl;
    }

    // Clean up memory
    for (Robot* r : robots) delete r;
    for (Robot* r : respawnQueue) delete r;
    return 0;
}
