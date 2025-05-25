#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <streambuf>
#include <cstdlib>
#include <ctime>
#include <map>
#include <algorithm>
using namespace std;

// Custom streambuf to duplicate output to console and log file
class DualBuf : public std::streambuf {
    std::streambuf *sb1, *sb2;
public:
    DualBuf(std::streambuf *buf1, std::streambuf *buf2)
        : sb1(buf1), sb2(buf2) {}
    int overflow(int c) override {
        if (c == EOF) return !EOF;
        if (sb1->sputc(c) == EOF || sb2->sputc(c) == EOF) return EOF;
        return c;
    }
    int sync() override {
        int r1 = sb1->pubsync();
        int r2 = sb2->pubsync();
        return (r1 == 0 && r2 == 0) ? 0 : -1;
    }
};

// Base Robot class with common attributes and methods
class Robot {
public:
	string name;
    char symbol;
    int x, y;            // Position on battlefield
    int health;          // Current hit points
    int shells;          // Current ammunition count
    int lives;           // Extra lives (respawns)
    bool alive;          // Alive status
    int initHealth;      // Initial HP (for respawn)
    int initShells;      // Initial ammo (for respawn)
    bool sawTarget = false;        // True if look() finds any robot
    int upgradeCount = 0;          // Number of upgrades acquired
    bool upgradedMoving = false;   // Received moving upgrade?
    bool upgradedShooting = false; // Received shooting upgrade?
    bool upgradedSeeing = false;   // Received seeing upgrade?
    bool hidden = false; //tamim -for hidebot

    // Constructor
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

    // Extra life
    bool hasExtraLife() const { return lives > 0; }
    void useExtraLife() { if (lives > 0) lives--; }

    // Take damage
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

    // Self-destruct
    void destroySelf() {
        if (!alive) return;
        alive = false;
        cout << name << " has no shells left and self-destructs!" << endl;
    }

    // Respawn
    void respawn(int newX, int newY) {
        alive = true;
        health = initHealth;
        shells = initShells;
        x = newX;
        y = newY;
        cout << name << " respawns at (" << x << "," << y << ") with health="
             << health << " and shells=" << shells << endl;
    }

    // Actions to implement
    virtual void think() = 0;
    virtual void look(const vector<Robot*>& robots) = 0;
    virtual void fire(vector<Robot*>& robots) = 0;
    virtual void move(const vector<Robot*>& robots, int width, int height) = 0;
};

// Capability interfaces (unchanged structure)
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

// GenericRobot: base behavior
class GenericRobot : public Robot, public MovingRobot, public ShootingRobot, public SeeingRobot, public ThinkingRobot {
public:
    GenericRobot(string n, char sym, int startX, int startY, int hp, int ammo, int extraLives)
        : Robot(n, sym, startX, startY, hp, ammo, extraLives) {}

    // Think action
    void think() override {
        if (!alive) return;
        cout << name << " is thinking..." << endl;
    }

    // Look: reveal targets or nearby robots
    void look(const vector<Robot*>& robots) override {
        if (!alive) return;
        bool found = false;
        // If seeing upgrade active, use a scan (similar to ScoutBot)
        static const int SCAN_LIMIT = 3;
        static map<string,int> scansLeft; // track scans per robot by name
        if (upgradedSeeing && scansLeft[name] < SCAN_LIMIT) {
            cout << name << " uses a scan to reveal all robots on the map:" << endl;
            for (Robot* other : robots) {
                if (other != this && other->isAlive()) {
                    cout << "  " << other->getName() << " at ("
                         << other->getX() << "," << other->getY() << ")" << endl;
                    found = true;
                }
            }
            scansLeft[name]++;
        }
        // Normal look (1-cell vicinity)
        if (!upgradedSeeing || !found) {
            cout << name << " looks around." << endl;
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
        sawTarget = found;
    }
    void fire(vector<Robot*>& robots) override; // Declaration only

    // Move: either jump anywhere (if moving upgrade) or one-cell move
    void move(const vector<Robot*>& robots, int width, int height) override {
        if (!alive) return;
        // Jump anywhere if upgraded
        if (upgradedMoving) {
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
        // Normal adjacent move
        else {
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
    }
};

// JumpBot, SemiAutoBot, ScoutBot classes (for direct instantiation if in setup file)
class JumpBot : public GenericRobot {
public:
    JumpBot(string n, char sym, int startX, int startY, int hp, int ammo, int extraLives)
        : GenericRobot(n, sym, startX, startY, hp, ammo, extraLives) {
        upgradedMoving = true;
        upgradeCount = 1;
    }
};
class SemiAutoBot : public GenericRobot {
public:
    SemiAutoBot(string n, char sym, int startX, int startY, int hp, int ammo, int extraLives)
        : GenericRobot(n, sym, startX, startY, hp, ammo, extraLives) {
        upgradedShooting = true;
        upgradeCount = 1;
    }
};
class ScoutBot : public GenericRobot {
    int scansLeft;
public:
    ScoutBot(string n, char sym, int startX, int startY, int hp, int ammo, int extraLives)
        : GenericRobot(n, sym, startX, startY, hp, ammo, extraLives), scansLeft(3) {
        upgradedSeeing = true;
        upgradeCount = 1;
    }
    void look(const vector<Robot*>& robots) override {
        if (!isAlive()) return;
        if (scansLeft > 0) {
            cout << name << " uses a scan to reveal all robots on the map:" << endl;
            bool found = false;
            for (Robot* other : robots) {
                if (other != this && other->isAlive()) {
                    cout << "  " << other->getName() << " at ("
                         << other->getX() << "," << other->getY() << ")" << endl;
                    found = true;
                }
            }
            scansLeft--;
            sawTarget = found;
        } else {
            GenericRobot::look(robots); // fallback to normal look
        }
    }
};
// HideBot: can hide and become invulnerable
class HideBot : public GenericRobot {
    int hidesLeft;
public:
    HideBot(string n, char sym, int startX, int startY,int hp, int ammo, int extraLives)
      : GenericRobot(n, sym, startX, startY, hp, ammo, extraLives),hidesLeft(3) {}
    void think() override {
        if (!alive) return;
        if (hidesLeft > 0) {
            cout << name << " is hidden and invulnerable this turn." << endl;
            hidden = true;
            hidesLeft--;
        } else {
            GenericRobot::think();  // // Fall back to normal look
        }
    }
};

// LongShotBot: fires only at targets that are 3 or more cells away
class LongShotBot : public GenericRobot {
public:
    LongShotBot(string n, char sym, int startX, int startY, int hp, int ammo, int extraLives)
        : GenericRobot(n, sym, startX, startY, hp, ammo, extraLives) {}

    void fire(vector<Robot*>& robots) override {
        if (!alive || shells <= 0) return;

        Robot* target = nullptr;

        for (Robot* r : robots) {
            if (r != this && r->isAlive()) {
                int dx = abs(r->getX() - x);
                int dy = abs(r->getY() - y);
                int dist = dx + dy;

                if (dist >= 3) { // Fire only at long range
                    target = r;
                    break;
                }
            }
        }

        if (!target) {
            cout << name << " sees no long-range targets to fire at." << endl;
            return;
        }

        cout << name << " (LongShotBot) fires at " << target->getName() << " from a distance... ";
        shells--;

        bool hit = (rand() % 100) < 70;
        if (hit) {
            cout << "HIT!" << endl;
            target->takeDamage();
        } else {
            cout << "misses." << endl;
        }

        if (shells <= 0) {
            destroySelf();
        }
    }
};

class TrackBot : public GenericRobot {
    std::vector<GenericRobot*> tracked;  // list of robots being tracked
public:
    TrackBot(string n, char sym, int x, int y, int hp, int ammo, int lives)
        : GenericRobot(n, sym, x, y, hp, ammo, lives) {}


    virtual void act(const std::vector<GenericRobot*>& robots){
        if (!alive) return;
        std::cout << name << " uses TrackBot scanning." << std::endl;
        vector<Robot*> baseRobots(robots.begin(), robots.end());
        look(baseRobots);


        // Plant trackers on up to 3 distinct visible robots
        for (auto& r : robots) {
            if (r == this || !r->alive) continue;
            // Check if already tracking this robot
            bool already = false;
            for (auto& t : tracked) {
                if (t == r) { already = true; break; }
            }
            if (!already && tracked.size() < 3) {
                tracked.push_back(r);
                std::cout << name << " plants tracker on " << r->name << "." << std::endl;
            }
        }

        // Report locations of tracked robots
        if (!tracked.empty()) {
            std::cout << name << " reporting tracked robot locations:" << std::endl;
            for (auto& t : tracked) {
                if (t->alive) {
                    std::cout << "  - " << t->name << " is at (" << t->x << "," << t->y << ")" << std::endl;
                }
            }
        }
    }
};

class ThirtyShotBot : public GenericRobot {
public:
    ThirtyShotBot(string n, char sym, int startX, int startY, int hp, int ammo, int extraLives)
        : GenericRobot(n, sym, startX, startY, hp, 30, extraLives) {} // 30 fresh shells

    void fire(vector<Robot*>& robots) override {
        if (!alive || shells <= 0) return;

        Robot* target = nullptr;

        // Look for nearest target in 8 surrounding squares
        for (Robot* r : robots) {
            if (r != this && r->isAlive()) {
                int dx = abs(r->getX() - x);
                int dy = abs(r->getY() - y);
                if ((dx <= 1 && dy <= 1) && !(dx == 0 && dy == 0)) {
                    target = r;
                    break;
                }
            }
        }

        if (!target) {
            cout << name << " sees no nearby target to fire at." << endl;
            return;
        }

        cout << name << " (ThirtyShotBot) fires at " << target->getName() << "! ";
        shells--;

        bool hit = (rand() % 100) < 70;
        if (hit) {
            cout << "HIT!" << endl;
            target->takeDamage();
        } else {
            cout << "misses." << endl;
        }

        if (shells <= 0) {
            cout << name << " has used all 30 shells and will self-destruct!" << endl;
            destroySelf();
        }
    }
};

 // Fire: single-shot or triple-shot based on upgrades, and handle upgrades on kill
void GenericRobot::fire(vector<Robot*>& robots) {
        if (!alive || shells <= 0) return;
        if (!sawTarget) return;  // skip fire if nothing seen
        // If shooting upgrade active, do semi-auto (triple shot)
        if (upgradedShooting) {
            Robot* target = nullptr;
            for (Robot* r : robots) {
                if (r != this && r->isAlive()) { target = r; break; }
            }
            if (!target) return;
            cout << name << " (Semi-auto) fires triple burst at " << target->getName() << ": ";
            shells--;
            int hits = 0;
            for (int shot = 0; shot < 3; shot++) {
                bool hit = (rand() % 100) < 70;
                if (hit) {
                    hits++;
                    target->takeDamage();
                    if (!target->isAlive()) {
                        // Enemy destroyed, assign an upgrade
                        if (upgradeCount < 3) {
                            vector<string> options;
                            if (!upgradedMoving)   options.push_back("move");
                            if (!upgradedShooting) options.push_back("shoot");
                            if (!upgradedSeeing)   options.push_back("see");
                            if (!options.empty()) {
                                string choice = options[rand() % options.size()];
                                Robot* old = this;
                                Robot* newBot = nullptr;
                                if (choice == "move") {
                                    upgradedMoving = true; upgradeCount++;
                                    newBot = new JumpBot(name, symbol, x, y, health, shells, lives);
                                }
                                else if (choice == "shoot") {
                                    upgradedShooting = true; upgradeCount++;
                                    newBot = new SemiAutoBot(name, symbol, x, y, health, shells, lives);
                                }
                                else if (choice == "see") {
                                    upgradedSeeing = true; upgradeCount++;
                                    newBot = new ScoutBot(name, symbol, x, y, health, shells, lives);
                                }
                                if (newBot) {
                                    newBot->upgradedMoving   = upgradedMoving;
                                    newBot->upgradedShooting = upgradedShooting;
                                    newBot->upgradedSeeing   = upgradedSeeing;
                                    newBot->upgradeCount     = upgradeCount;
                                    // Replace pointer in vector
                                    for (size_t i=0; i<robots.size(); i++) {
                                        if (robots[i] == this) {
                                            robots[i] = newBot;
                                            break;
                                        }
                                    }
                                    delete old; // destroy old object
                                }
                            }
                        }
                        break; // stop further shots if target is dead
                    }
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
        // Normal single-shot fire
        else {
            Robot* target = nullptr;
            for (Robot* r : robots) {
                if (r != this && r->isAlive()) { target = r; break; }
            }
            if (!target) return;
            cout << name << " fires at " << target->getName() << "... ";
            shells--;
            bool hit = (rand() % 100) < 70;
            if (hit) {
                cout << "HIT!" << endl;
                target->takeDamage();
                if (!target->isAlive()) {
                    // Enemy destroyed, assign an upgrade
                    if (upgradeCount < 3) {
                        vector<string> options;
                        if (!upgradedMoving)   options.push_back("move");
                        if (!upgradedShooting) options.push_back("shoot");
                        if (!upgradedSeeing)   options.push_back("see");
                        if (!options.empty()) {
                            string choice = options[rand() % options.size()];
                            Robot* old = this;
                            Robot* newBot = nullptr;
                            if (choice == "move") {
                                upgradedMoving = true; upgradeCount++;
                                newBot = new JumpBot(name, symbol, x, y, health, shells, lives);
                            }
                            else if (choice == "shoot") {
                                upgradedShooting = true; upgradeCount++;
                                newBot = new SemiAutoBot(name, symbol, x, y, health, shells, lives);
                            }
                            else if (choice == "see") {
                                upgradedSeeing = true; upgradeCount++;
                                newBot = new ScoutBot(name, symbol, x, y, health, shells, lives);
                            }
                            if (newBot) {
                                newBot->upgradedMoving   = upgradedMoving;
                                newBot->upgradedShooting = upgradedShooting;
                                newBot->upgradedSeeing   = upgradedSeeing;
                                newBot->upgradeCount     = upgradeCount;
                                for (size_t i=0; i<robots.size(); i++) {
                                    if (robots[i] == this) {
                                        robots[i] = newBot;
                                        break;
                                    }
                                }
                                delete old;
                            }
                        }
                    }
                }
            } else {
                cout << "misses." << endl;
            }
            if (shells <= 0) {
                destroySelf();
            }
        }
    }

// Main simulation
int main() {
    srand((unsigned)time(0));

    // Redirect cout to also write to log file
    ofstream logFile("log.txt");
    DualBuf dualBuf(cout.rdbuf(), logFile.rdbuf());
    cout.rdbuf(&dualBuf);

    // Read setup from file
    ifstream setup("setup.txt");
    if (!setup) {
        cout << "Failed to open setup.txt" << endl;
        return 1;
    }

    int width = 0, height = 0, numTurns = 0;
    int numRobots = 0;
    string line;

    // Parse battlefield dimensions line: "M by N : width height"
    getline(setup, line);
    {
        int w,h;
        sscanf(line.c_str(), "%*s %*s %*s %*s %d %d", &w, &h);
        width = w; height = h;
    }
    // Parse steps: "steps: X"
    getline(setup, line);
    {
        sscanf(line.c_str(), "steps: %d", &numTurns);
    }
    // Parse robots count: "robots: X"
    getline(setup, line);
    {
        sscanf(line.c_str(), "robots: %d", &numRobots);
    }

    vector<Robot*> robots;
    char nextSymbol = 'A';
    const int DEFAULT_HP = 3;
    const int DEFAULT_AMMO = 5;
    const int DEFAULT_LIVES = 1;

    // Read each robot line
    for (int i = 0; i < numRobots; i++) {
        string type, name, xs, ys;
        setup >> type >> name >> xs >> ys;
        int rx, ry;
        // Determine X coordinate
        if (xs == "random") {
            rx = rand() % width;
        } else {
            rx = stoi(xs);
        }
        // Determine Y coordinate
        if (ys == "random") {
            ry = rand() % height;
        } else {
            ry = stoi(ys);
        }
        // Ensure unique random location if needed
        if (xs == "random" || ys == "random") {
			bool unique = false;
			int attempts = 0;
			while (!unique && attempts++ < 1000) {
				unique = true;
				for (Robot* r : robots) {
					if (r->isAlive() && r->getX() == rx && r->getY() == ry) {
						rx = rand() % width;
						ry = rand() % height;
						unique = false;
						break;
					}
				}
			}
			if (!unique) {
				cout << "[ERROR] Could not find unique spawn location after 1000 attempts!\n";
				return 1;
			}

					}
        // Assign a symbol (unique char)
        char sym = nextSymbol++;
        Robot* r = nullptr;
        if (type == "GenericRobot") {
            r = new GenericRobot(name, sym, rx, ry, DEFAULT_HP, DEFAULT_AMMO, DEFAULT_LIVES);
        } else if (type == "JumpBot") {
            r = new JumpBot(name, sym, rx, ry, DEFAULT_HP, DEFAULT_AMMO, DEFAULT_LIVES);
        } else if (type == "SemiAutoBot") {
            r = new SemiAutoBot(name, sym, rx, ry, DEFAULT_HP, DEFAULT_AMMO, DEFAULT_LIVES);
        } else if (type == "ScoutBot") {
            r = new ScoutBot(name, sym, rx, ry, DEFAULT_HP, DEFAULT_AMMO, DEFAULT_LIVES);
        } else {
            // Default to Generic if unknown type
            r = new GenericRobot(name, sym, rx, ry, DEFAULT_HP, DEFAULT_AMMO, DEFAULT_LIVES);
        }
        robots.push_back(r);
    }

    vector<Robot*> respawnQueue;

    // Simulation loop
    for (int turn = 1; turn <= numTurns; turn++) {
        cout << "----- Turn " << turn << " -----" << endl;

        // Respawn queued robots
        for (Robot* r : respawnQueue) {
            int nx, ny;
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
				int gx = r->getX();
				int gy = r->getY();
				if (gx >= 0 && gx < width && gy >= 0 && gy < height) {
					grid[gy][gx] = r->getSymbol();
				} else {
					cout << "[ERROR] Robot " << r->getName()
						<< " out of bounds at (" << gx << "," << gy << ")!\n";
					exit(1); // or return 1;
				}
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
                // Remove dead robot (queue if extra life)
                if (r->hasExtraLife()) {
                    r->useExtraLife();
                    respawnQueue.push_back(r);
                }
                cout << r->getName() << " is removed from battlefield." << endl;
                robots.erase(robots.begin() + i);
                i--;
                continue;
            }
            r->think();
            r->look(robots);
            r->fire(robots);
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

        // Cleanup any remaining dead robots
        for (size_t i = 0; i < robots.size(); i++) {
            Robot* r = robots[i];
            if (!r->isAlive()) {
                if (r->hasExtraLife()) {
                    r->useExtraLife();
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

    // Clean up
    for (Robot* r : robots) delete r;
    for (Robot* r : respawnQueue) delete r;
    return 0;
}
