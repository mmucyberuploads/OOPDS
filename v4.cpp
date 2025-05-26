// FINAL FIXED AND PORTABLE VERSION WITH FULL UPGRADE SUPPORT
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
using namespace std;

class Robot {
public:
    string name;
    char symbol;
    int x, y;
    int health;
    int shells;
    int lives;
    bool alive;
    int initHealth, initShells;
    bool sawTarget = false;
    vector<Robot*> seenTargets;
    int scansLeft = 3;

    bool upgradedMoving = false, upgradedShooting = false, upgradedSeeing = false;
    int upgradeCount = 0;
    string moveUpgradeName, shootingUpgradeName, seeingUpgradeName;
    int hidesLeft = 0;
    bool hidden = false;

    Robot(string n, char s, int ix, int iy, int hp, int ammo, int l)
        : name(n), symbol(s), x(ix), y(iy), health(hp), shells(ammo), lives(l), alive(true), initHealth(hp), initShells(ammo) {}

    virtual void think() = 0;
    virtual void look(const vector<Robot*>& robots) = 0;
    virtual void fire(vector<Robot*>& robots) = 0;
    virtual void move(const vector<Robot*>& robots, int width, int height) = 0;

    void takeDamage() {
        if (!alive) return;
        if (hidden) {
            cout << name << " is hidden, so it takes no damage." << endl;
            return;
        }
        health--;
        cout << name << " is hit! (Health=" << health << ")\n";
        if (health <= 0) {
            alive = false;
            cout << name << " is destroyed!\n";
        }
    }

    void destroySelf() {
        if (!alive) return;
        alive = false;
        cout << name << " self-destructs!\n";
    }

    void respawn(int newX, int newY) {
        x = newX; y = newY;
        health = initHealth;
        alive = true;
        sawTarget = false;
        hidden = false;
        seenTargets.clear();
        cout << name << " respawns at (" << x << "," << y << ") with " << health << " health and " << shells << " shells" << endl;
    }

    string getName() const { return name; }
    int getX() const { return x; }
    int getY() const { return y; }
    bool isAlive() const { return alive; }
};

class ThinkingRobot {
public:
    void performThink(Robot* self) {
        if (self->upgradedMoving && self->moveUpgradeName == "HideBot" && self->hidesLeft > 0) {
            cout << self->name << " is hidden and invulnerable this turn." << endl;
            self->hidden = true;
            self->hidesLeft--;
        } else {
            cout << self->name << " is thinking..." << endl;
        }
    }
};

class SeeingRobot {
public:
    void performLook(Robot* self, const vector<Robot*>& robots) {
        self->seenTargets.clear();
        if (self->upgradedSeeing && self->scansLeft > 0) {
            cout << self->name << " uses a scan:" << endl;
            for (auto r : robots) {
                if (r != self && r->alive) {
                    cout << "  " << r->name << " at (" << r->x << "," << r->y << ")" << endl;
                    self->seenTargets.push_back(r);
                }
            }
            self->scansLeft--;
        } else {
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = self->x + dx, ny = self->y + dy;
                    for (auto r : robots) {
                        if (r != self && r->alive && r->x == nx && r->y == ny) {
                            cout << "  " << self->name << " sees " << r->name << " at (" << nx << "," << ny << ")" << endl;
                            self->seenTargets.push_back(r);
                        }
                    }
                }
            }
        }
        self->sawTarget = !self->seenTargets.empty();
        if (!self->sawTarget) cout << "  " << self->name << " sees no one." << endl;
    }
};

class ShootingRobot {
public:
    void performFire(Robot* self, vector<Robot*>& robots) {
        if (!self->alive || self->shells <= 0) return;
        Robot* target = nullptr;
        bool killed = false;

        if (self->shootingUpgradeName == "LongShotBot") {
            for (Robot* r : robots) {
                if (r != self && r->isAlive()) {
                    int dist = abs(r->getX() - self->x) + abs(r->getY() - self->y);
                    if (dist >= 3) { target = r; break; }
                }
            }
            if (!target) {
                cout << self->name << " (LongShotBot) sees no long-range targets to fire at." << endl;
                return;
            }
        } else {
            if (!self->sawTarget || self->seenTargets.empty()) return;
            target = self->seenTargets[rand() % self->seenTargets.size()];
        }

        cout << self->name << " fires at " << target->name << "... ";
        self->shells--;
        if (rand() % 100 < 70) {
            cout << "HIT!" << endl;
            target->takeDamage();
            if (!target->isAlive()) killed = true;
        } else {
            cout << "misses." << endl;
        }

        if (killed && self->upgradeCount < 3) {
            cout << self->name << " gets an upgrade!" << endl;
            vector<int> available;
            if (!self->upgradedMoving) available.push_back(1);
            if (!self->upgradedShooting) available.push_back(2);
            if (!self->upgradedSeeing) available.push_back(3);
            if (!available.empty()) {
                int cat = available[rand() % available.size()];
                switch (cat) {
                    case 1:
                        self->upgradedMoving = true;
                        if (rand() % 2 == 0) {
                            self->moveUpgradeName = "JumpBot";
                            cout << self->name << " upgraded to JumpBot." << endl;
                        } else {
                            self->moveUpgradeName = "HideBot";
                            self->hidesLeft = 3;
                            cout << self->name << " upgraded to HideBot." << endl;
                        }
                        break;
                    case 2:
                        self->upgradedShooting = true;
                        switch (rand() % 3) {
                            case 0: self->shootingUpgradeName = "LongShotBot"; cout << self->name << " upgraded to LongShotBot." << endl; break;
                            case 1: self->shootingUpgradeName = "SemiAutoBot"; cout << self->name << " upgraded to SemiAutoBot." << endl; break;
                            case 2: self->shootingUpgradeName = "ThirtyShotBot"; self->shells = 30; cout << self->name << " upgraded to ThirtyShotBot." << endl; break;
                        }
                        break;
                    case 3:
                        self->upgradedSeeing = true;
                        if (rand() % 2 == 0) {
                            self->seeingUpgradeName = "ScoutBot";
                            cout << self->name << " upgraded to ScoutBot." << endl;
                        } else {
                            self->seeingUpgradeName = "TrackBot";
                            cout << self->name << " upgraded to TrackBot." << endl;
                        }
                        break;
                }
                self->upgradeCount++;
            }
        }

        if (self->shells <= 0) {
            cout << self->name << " has no shells left and self-destructs!" << endl;
            self->destroySelf();
        }
    }
};

class MovingRobot {
public:
    void performMove(Robot* self, const vector<Robot*>& robots, int width, int height) {
        if (!self->alive) return;
        vector<pair<int, int>> options;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue;
                int nx = self->x + dx, ny = self->y + dy;
                if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue;
                bool occ = false;
                for (auto r : robots) {
                    if (r != self && r->alive && r->x == nx && r->y == ny) { occ = true; break; }
                }
                if (!occ) options.push_back({nx, ny});
            }
        }
        if (self->upgradedMoving && self->moveUpgradeName == "JumpBot") {
            for (int dx = -2; dx <= 2; dx++) {
                for (int dy = -2; dy <= 2; dy++) {
                    if (abs(dx) < 2 && abs(dy) < 2) continue;
                    int nx = self->x + dx, ny = self->y + dy;
                    if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue;
                    bool occ = false;
                    for (auto r : robots) {
                        if (r != self && r->alive && r->x == nx && r->y == ny) { occ = true; break; }
                    }
                    if (!occ) options.push_back({nx, ny});
                }
            }
        }
        if (!options.empty()) {
            auto [nx, ny] = options[rand() % options.size()];
            self->x = nx; self->y = ny;
            cout << self->name << " moves to (" << nx << "," << ny << ")" << endl;
        }
    }
};

class GenericRobot : public Robot, public ThinkingRobot, public SeeingRobot, public ShootingRobot, public MovingRobot {
public:
    GenericRobot(string n, char s, int x, int y, int hp, int ammo, int l)
        : Robot(n, s, x, y, hp, ammo, l) {}

    void think() override { performThink(this); }
    void look(const vector<Robot*>& robots) override { performLook(this, robots); }
    void fire(vector<Robot*>& robots) override { performFire(this, robots); }
    void move(const vector<Robot*>& robots, int width, int height) override { performMove(this, robots, width, height); }
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


int main() {
    ofstream logfile("log.txt");
    streambuf* originalCout = cout.rdbuf();
    // Mirror output to both console and log
    streambuf* logbuf = logfile.rdbuf();
    // Removed duplicate declaration of originalCout
    class DualBuf : public streambuf {
        streambuf *buf1, *buf2;
    public:
        DualBuf(streambuf* b1, streambuf* b2) : buf1(b1), buf2(b2) {}
        int overflow(int c) override {
            if (c == EOF) return !EOF;
            if (buf1) buf1->sputc(c);
            if (buf2) buf2->sputc(c);
            return c;
        }
    } dualbuf(originalCout, logbuf);
    cout.rdbuf(&dualbuf);
    ifstream setup("setup.txt");
    if (!setup) {
        cerr << "Failed to open setup.txt" << endl;
        return 1;
    }

    int width = 10, height = 10, numTurns = 100, numRobots = 0;
    string line;
    while (getline(setup, line)) {
        if (line.find("M by N") != string::npos) {
            sscanf(line.c_str(), "M by N : %d %d", &width, &height);
        } else if (line.find("steps") != string::npos) {
            sscanf(line.c_str(), "steps: %d", &numTurns);
        } else if (line.find("robots") != string::npos) {
            sscanf(line.c_str(), "robots: %d", &numRobots);
            break; // Next lines will be robot entries
        }
    }

    vector<Robot*> robots, respawnQueue;
    char nextSymbol = 'A';
    for (int i = 0; i < numRobots; ++i) {
        string type, name;
        string xs, ys;
        setup >> type >> name >> xs >> ys;
        int x = (xs == "random") ? rand() % width : stoi(xs);
        int y = (ys == "random") ? rand() % height : stoi(ys);

        if (type == "GenericRobot") {
            robots.push_back(new GenericRobot(name, nextSymbol++, x, y, 1, 10, 3));
        } // Future: handle other types
    }
    setup.close();


    srand((unsigned)time(0));

    // Removed duplicate declarations below as they already exist earlier.

    int turn = 1;
    while (turn <= numTurns) {
        int aliveCount = count_if(robots.begin(), robots.end(), [](Robot* r) { return r->alive; });
        if (aliveCount <= 1 && respawnQueue.empty()) break;

        cout << "----- Turn " << turn << " -----" << endl;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                char cell = '.';
                for (auto* r : robots) {
                    if (r->alive && r->x == x && r->y == y) {
                        cell = r->symbol;
                        break;
                    }
                }
                cout << cell << " ";
            }
            cout << endl;
        }

        if (!respawnQueue.empty()) {
            Robot* r = respawnQueue.front();
            respawnQueue.erase(respawnQueue.begin());
            int nx, ny;
            do {
                nx = rand() % width;
                ny = rand() % height;
                bool occ = false;
                for (auto* other : robots) {
                    if (other->alive && other->x == nx && other->y == ny) {
                        occ = true; break;
                    }
                }
                if (!occ) break;
            } while (true);
            r->respawn(nx, ny);
        }

        for (Robot* r : robots) {
            if (!r->alive) continue;
            r->think();
            r->look(robots);
            if (r->sawTarget) {
                r->think();
                r->fire(robots);
            } else {
                r->move(robots, width, height);
            }
        }

        for (Robot* r : robots) {
            if (!r->alive && r->lives > 0 && find(respawnQueue.begin(), respawnQueue.end(), r) == respawnQueue.end()) {
                r->lives--;
                respawnQueue.push_back(r);
            }
        }

        cout << "--- Status after Turn " << turn << " ---" << endl;
        for (Robot* r : robots) {
            cout << r->name << " at (" << r->x << "," << r->y << ") shells=" << r->shells << " lives=" << r->lives;
            cout << " | Upgrades: ";
            if (r->upgradedMoving) cout << r->moveUpgradeName << " ";
            if (r->upgradedShooting) cout << r->shootingUpgradeName << " ";
            if (r->upgradedSeeing) cout << r->seeingUpgradeName << " ";
            if (!r->alive) cout << " [DEAD]";
            cout << endl;
        }

        cout << endl;
        turn++;
    }

    for (Robot* r : robots) delete r;
    for (Robot* r : respawnQueue) delete r;
        cout.rdbuf(originalCout);  // Restore original cout
    logfile.close();
    return 0;
}
