#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <climits>
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
    vector<Robot*> trackedBots;
    bool trackBotHasScanned = false;

    bool upgradedMoving = false, upgradedShooting = false, upgradedSeeing = false;
    int upgradeCount = 0;
    string moveUpgradeName, shootingUpgradeName, seeingUpgradeName;
    int hidesLeft = 0;
    bool hidden = false;
    int jumpsLeft = 0;

    Robot(string n, char s, int ix, int iy, int hp, int ammo, int l)
        : name(n), symbol(s), x(ix), y(iy), health(hp), shells(ammo), lives(l), alive(true),
          initHealth(hp), initShells(ammo) {}

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

    // Helper function to get firing range based on upgrades
    int getFiringRange() const {
        if (upgradedShooting && shootingUpgradeName == "LongShotBot") {
            return 3;
        }
        return 1; // Default adjacent range
    }

    string getName() const { return name; }
    int getX() const { return x; }
    int getY() const { return y; }
    bool isAlive() const { return alive; }
};

class ThinkingRobot {
public:
    virtual void performThink(Robot* self) {
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
    virtual void performLook(Robot* self, const vector<Robot*>& robots) {
        self->seenTargets.clear();
        // DON'T reset hidden status here - it should persist for the entire turn

        // TrackBot logic - fixed to work properly
        if (self->upgradedSeeing && self->seeingUpgradeName == "TrackBot") {
            if (!self->trackBotHasScanned) {
                self->trackedBots.clear();
                vector<Robot*> available;
                for (Robot* r : robots) {
                    if (r != self && r->alive && !r->hidden) {
                        available.push_back(r);
                    }
                }
                random_shuffle(available.begin(), available.end());
                int toTrack = min(3, (int)available.size());
                for (int i = 0; i < toTrack; i++) {
                    self->trackedBots.push_back(available[i]);
                }
                self->trackBotHasScanned = true;
                cout << self->name << " tracked " << toTrack << " robots." << endl;
            }

            // Always add tracked bots to seenTargets if they're still alive
            for (Robot* t : self->trackedBots) {
                if (t->alive && !t->hidden) {
                    self->seenTargets.push_back(t);
                }
            }
        }

        // ScoutBot upgrade
        if (self->upgradedSeeing && self->seeingUpgradeName == "ScoutBot" &&
            self->scansLeft > 0) {
            cout << self->name << " uses ScoutBot scan ("
                 << self->scansLeft << " left):\n";
            for (auto r : robots) {
                if (r != self && r->alive && !r->hidden) {
                    // Only add if not already present
                    if (find(self->seenTargets.begin(), self->seenTargets.end(), r) == self->seenTargets.end()) {
                        self->seenTargets.push_back(r);
                    }
                }
            }
            self->scansLeft--;
        }

        // Standard vision (adjacent cells)
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue;
                int nx = self->x + dx, ny = self->y + dy;
                for (auto r : robots) {
                    if (r != self && r->alive && !r->hidden &&
                        r->x == nx && r->y == ny) {
                        // Only add if not already present
                        if (find(self->seenTargets.begin(), self->seenTargets.end(), r) == self->seenTargets.end()) {
                            self->seenTargets.push_back(r);
                        }
                    }
                }
            }
        }

        // Print what the robot sees
        self->sawTarget = !self->seenTargets.empty();
        if (self->sawTarget) {
            for (auto r : self->seenTargets) {
                cout << "  " << self->name << " sees " << r->name << " at (" << r->x << "," << r->y << ")\n";
            }
        } else {
            cout << "  " << self->name << " sees no one.\n";
        }
    }
};

class ShootingRobot {
public:
    virtual void performFire(Robot* self, vector<Robot*>& robots) {
        if (!self->alive || self->shells <= 0) return;

        // SemiAutoBot: 3 shots for 1 shell
        if (self->shootingUpgradeName == "SemiAutoBot" && self->sawTarget) {
            Robot* target = self->seenTargets[rand() % self->seenTargets.size()];
            cout << self->name << " (SemiAutoBot) fires 3 shots at "
                 << target->name << "! ";
            self->shells--;
            int hits = 0;
            for (int i = 0; i < 3; i++) {
                if (rand() % 100 < 70) hits++;
            }
            if (hits > 0) {
                cout << "HIT " << hits << " times!\n";
                for (int i = 0; i < hits; i++) {
                    target->takeDamage();
                }
            } else {
                cout << "All shots miss.\n";
            }
            if (self->shells <= 0) self->destroySelf();
            return;
        }

        // LongShotBot: fire at distance
        if (self->shootingUpgradeName == "LongShotBot" && self->sawTarget) {
            vector<Robot*> candidates;
            for (Robot* r : self->seenTargets) {
                int dx = abs(r->x - self->x);
                int dy = abs(r->y - self->y);
                int dist = dx + dy;
                if (dist > 0 && dist <= 3) {
                    candidates.push_back(r);
                }
            }
            if (!candidates.empty()) {
                Robot* target = candidates[rand() % candidates.size()];
                cout << self->name << " (LongShotBot) fires at "
                     << target->name << " (dist=" << abs(target->x - self->x) + abs(target->y - self->y) << ")... ";
                self->shells--;
                if (rand() % 100 < 70) {
                    cout << "HIT!\n";
                    target->takeDamage();
                } else {
                    cout << "misses.\n";
                }
                if (self->shells <= 0) self->destroySelf();
                return;
            } else {
                cout << self->name << " (LongShotBot) sees no target within 3-unit range." << endl;
                // Fall through to regular firing if no long-range targets
            }
        }

        // FIXED: Pattern shooters now fall back to regular firing if no pattern targets
        if (self->shootingUpgradeName == "PlusShooter") {
            cout << self->name << " fires in + pattern!" << endl;
            bool fired = false;
            for (Robot* r : self->seenTargets) {
                if ((r->x == self->x || r->y == self->y) && !(r->x == self->x && r->y == self->y)) {
                    cout << "  Targeting " << r->name << " at (" << r->x << "," << r->y << ")... ";
                    if (rand() % 100 < 70) {
                        cout << "HIT!" << endl;
                        r->takeDamage();
                    } else {
                        cout << "missed." << endl;
                    }
                    self->shells--;
                    fired = true;
                    if (self->shells <= 0) {
                        self->destroySelf();
                        return;
                    }
                }
            }
            if (!fired) {
                cout << "  No valid targets in + pattern, falling back to regular fire." << endl;
                // Fall through to regular firing
            } else {
                return;
            }
        }

        if (self->shootingUpgradeName == "CrossShooter") {
            cout << self->name << " fires in X pattern!" << endl;
            bool fired = false;
            for (Robot* r : self->seenTargets) {
                if (abs(r->x - self->x) == abs(r->y - self->y) && !(r->x == self->x && r->y == self->y)) {
                    cout << "  Targeting " << r->name << " at (" << r->x << "," << r->y << ")... ";
                    if (rand() % 100 < 70) {
                        cout << "HIT!" << endl;
                        r->takeDamage();
                    } else {
                        cout << "missed." << endl;
                    }
                    self->shells--;
                    fired = true;
                    if (self->shells <= 0) {
                        self->destroySelf();
                        return;
                    }
                }
            }
            if (!fired) {
                cout << "  No valid targets in X pattern, falling back to regular fire." << endl;
                // Fall through to regular firing
            } else {
                return;
            }
        }

        if (self->shootingUpgradeName == "DoubleRowShooter") {
            cout << self->name << " fires across two rows!" << endl;
            bool fired = false;
            for (Robot* r : self->seenTargets) {
                if (r->y == self->y || r->y == self->y + 1 || r->y == self->y - 1) {
                    cout << "  Targeting " << r->name << " at (" << r->x << "," << r->y << ")... ";
                    if (rand() % 100 < 70) {
                        cout << "HIT!" << endl;
                        r->takeDamage();
                    } else {
                        cout << "missed." << endl;
                    }
                    self->shells--;
                    fired = true;
                    if (self->shells <= 0) {
                        self->destroySelf();
                        return;
                    }
                }
            }
            if (!fired) {
                cout << "  No valid targets in row pattern, falling back to regular fire." << endl;
                // Fall through to regular firing
            } else {
                return;
            }
        }

        // Regular firing - only fire at adjacent targets
        vector<Robot*> adjacentTargets;
        for (Robot* t : self->seenTargets) {
            int dx = abs(t->x - self->x);
            int dy = abs(t->y - self->y);
            if (dx <= 1 && dy <= 1 && (dx != 0 || dy != 0)) {
                adjacentTargets.push_back(t);
            }
        }

        if (adjacentTargets.empty()) {
            cout << self->name << " sees no adjacent target to fire at." << endl;
            return;
        }

        Robot* target = adjacentTargets[rand() % adjacentTargets.size()];
        cout << self->name << " fires at " << target->name << "... ";
        self->shells--;
        if (rand() % 100 < 70) {
            cout << "HIT!" << endl;
            target->takeDamage();
        } else {
            cout << "misses." << endl;
        }
        if (self->shells <= 0) self->destroySelf();

        // Upgrade on kill
        if (target && !target->alive && self->upgradeCount < 3) {
            cout << self->name << " gets an upgrade!" << endl;
            vector<int> available;
            if (!self->upgradedMoving) available.push_back(1);
            if (!self->upgradedShooting) available.push_back(2);
            if (!self->upgradedSeeing) available.push_back(3);

            if (!available.empty()) {
                int cat = available[rand() % available.size()];
                switch (cat) {
                    case 1: {  // Movement upgrade
                        self->upgradedMoving = true;
                        if (rand() % 2 == 0) {
                            self->moveUpgradeName = "JumpBot";
                            self->jumpsLeft = 3;
                            cout << self->name << " upgraded to JumpBot." << endl;
                        } else {
                            self->moveUpgradeName = "HideBot";
                            self->hidesLeft = 3;
                            cout << self->name << " upgraded to HideBot." << endl;
                        }
                        break;
                    }
                    case 2: {  // Shooting upgrade
                        self->upgradedShooting = true;
                        int choice = rand() % 6;
                        if (choice == 0) {
                            self->shootingUpgradeName = "LongShotBot";
                            cout << self->name << " upgraded to LongShotBot." << endl;
                        } else if (choice == 1) {
                            self->shootingUpgradeName = "SemiAutoBot";
                            cout << self->name << " upgraded to SemiAutoBot." << endl;
                        } else if (choice == 2) {
                            self->shootingUpgradeName = "ThirtyShotBot";
                            self->shells = 30;  // Immediate reload
                            cout << self->name << " upgraded to ThirtyShotBot." << endl;
                        } else if (choice == 3) {
                            self->shootingUpgradeName = "PlusShooter";
                            cout << self->name << " upgraded to PlusShooter." << endl;
                        } else if (choice == 4) {
                            self->shootingUpgradeName = "CrossShooter";
                            cout << self->name << " upgraded to CrossShooter." << endl;
                        } else {
                            self->shootingUpgradeName = "DoubleRowShooter";
                            cout << self->name << " upgraded to DoubleRowShooter." << endl;
                        }
                        break;
                    }
                    case 3: {  // Seeing upgrade
                        self->upgradedSeeing = true;
                        if (rand() % 2 == 0) {
                            self->seeingUpgradeName = "ScoutBot";
                            self->scansLeft = 3;
                            cout << self->name << " upgraded to ScoutBot." << endl;
                        } else {
                            self->seeingUpgradeName = "TrackBot";
                            cout << self->name << " upgraded to TrackBot." << endl;
                        }
                        break;
                    }
                }
                self->upgradeCount++;
            }
        }
    }
};

class MovingRobot {
public:
    virtual void performMove(Robot* self, const vector<Robot*>& robots, int width, int height) {
        if (!self->alive) return;

        // Reset hidden status at the end of the turn
        if (self->hidden && !(self->upgradedMoving && self->moveUpgradeName == "HideBot" && self->hidesLeft > 0)) {
            self->hidden = false;
        }

        // Jump toward enemy if possible
        if (self->upgradedMoving && self->moveUpgradeName == "JumpBot" &&
            self->jumpsLeft > 0 && self->sawTarget) {
            Robot* closest = nullptr;
            int minDist = INT_MAX;
            for (Robot* t : self->seenTargets) {
                if (!t->alive) continue;
                int dist = abs(t->x - self->x) + abs(t->y - self->y);
                if (dist < minDist) {
                    minDist = dist;
                    closest = t;
                }
            }

            if (closest) {
                // Jump to a position adjacent to the enemy
                vector<pair<int, int>> options;
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = closest->x + dx, ny = closest->y + dy;
                        if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue;
                        bool occ = false;
                        for (auto r : robots) {
                            if (r->alive && r->x == nx && r->y == ny) {
                                occ = true;
                                break;
                            }
                        }
                        if (!occ) options.push_back({nx, ny});
                    }
                }

                if (!options.empty()) {
                    auto [x, y] = options[rand() % options.size()];
                    self->x = x;
                    self->y = y;
                    self->jumpsLeft--;
                    cout << self->name << " jumps to (" << x << "," << y << ") near " << closest->name << "\n";
                    return;
                }
            }
        }

        // Move toward closest enemy (tracked bots have priority)
        Robot* target = nullptr;
        int minDist = INT_MAX;

        // Prefer tracked bots if available
        if (self->upgradedSeeing && self->seeingUpgradeName == "TrackBot") {
            for (Robot* t : self->trackedBots) {
                if (!t->alive || t->hidden) continue;
                int dist = abs(t->x - self->x) + abs(t->y - self->y);
                if (dist < minDist) {
                    minDist = dist;
                    target = t;
                }
            }
        }

        // Fallback to any seen enemy
        if (!target && self->sawTarget) {
            for (Robot* t : self->seenTargets) {
                if (!t->alive) continue;
                int dist = abs(t->x - self->x) + abs(t->y - self->y);
                if (dist < minDist) {
                    minDist = dist;
                    target = t;
                }
            }
        }

        // Move toward target if found
        if (target) {
            int dx = (target->x > self->x) ? 1 :
                     (target->x < self->x) ? -1 : 0;
            int dy = (target->y > self->y) ? 1 :
                     (target->y < self->y) ? -1 : 0;
            int nx = self->x + dx, ny = self->y + dy;

            // Check bounds and occupancy
            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                bool occupied = false;
                for (auto r : robots) {
                    if (r->alive && r->x == nx && r->y == ny) {
                        occupied = true;
                        break;
                    }
                }
                if (!occupied) {
                    self->x = nx;
                    self->y = ny;
                    cout << self->name << " moves toward " << target->name
                         << " to (" << nx << "," << ny << ")\n";
                    return;
                }
            }
        }

        // Standard movement
        vector<pair<int, int>> options;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue;
                int nx = self->x + dx, ny = self->y + dy;
                if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue;
                bool occ = false;
                for (auto r : robots) {
                    if (r->alive && r->x == nx && r->y == ny) {
                        occ = true;
                        break;
                    }
                }
                if (!occ) options.push_back({nx, ny});
            }
        }
        if (!options.empty()) {
            auto [nx, ny] = options[rand() % options.size()];
            self->x = nx;
            self->y = ny;
            cout << self->name << " moves to (" << nx << "," << ny << ")\n";
        }
    }
};

// Base robot class
class GenericRobot : public Robot, public ThinkingRobot, public SeeingRobot, public ShootingRobot, public MovingRobot {
public:
    GenericRobot(string n, char s, int x, int y, int hp, int ammo, int l)
        : Robot(n, s, x, y, hp, ammo, l) {}

    void think() override { performThink(this); }
    void look(const vector<Robot*>& robots) override { performLook(this, robots); }
    void fire(vector<Robot*>& robots) override { performFire(this, robots); }
    void move(const vector<Robot*>& robots, int width, int height) override {
        performMove(this, robots, width, height);
    }
};

// Upgrade classes
class HideBot : public GenericRobot {
public:
    HideBot(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        upgradedMoving = true;
        moveUpgradeName = "HideBot";
        hidesLeft = 3;
        upgradeCount = 1;
    }

    void think() override {
        if (hidesLeft > 0) {
            cout << name << " (HideBot) is hidden this turn ("
                 << hidesLeft << " hides left)\n";
            hidden = true;
            hidesLeft--;
        } else {
            GenericRobot::think();
        }
    }
};

class JumpBot : public GenericRobot {
public:
    JumpBot(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        upgradedMoving = true;
        moveUpgradeName = "JumpBot";
        jumpsLeft = 3;
        upgradeCount = 1;
    }
};

class LongShotBot : public GenericRobot {
public:
    LongShotBot(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        upgradedShooting = true;
        shootingUpgradeName = "LongShotBot";
        upgradeCount = 1;
    }
};

class SemiAutoBot : public GenericRobot {
public:
    SemiAutoBot(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        upgradedShooting = true;
        shootingUpgradeName = "SemiAutoBot";
        upgradeCount = 1;
    }
};

class ThirtyShotBot : public GenericRobot {
public:
    ThirtyShotBot(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, 30, l) {  // Set shells to 30
        upgradedShooting = true;
        shootingUpgradeName = "ThirtyShotBot";
        upgradeCount = 1;
    }
};

class ScoutBot : public GenericRobot {
public:
    ScoutBot(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        upgradedSeeing = true;
        seeingUpgradeName = "ScoutBot";
        scansLeft = 3;
        upgradeCount = 1;
    }
};

class TrackBot : public GenericRobot {
public:
    TrackBot(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        upgradedSeeing = true;
        seeingUpgradeName = "TrackBot";
        upgradeCount = 1;
    }
};

class PlusShooter : public GenericRobot {
public:
    PlusShooter(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        upgradedShooting = true;
        shootingUpgradeName = "PlusShooter";
        upgradeCount = 1;
    }
};

class CrossShooter : public GenericRobot {
public:
    CrossShooter(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        upgradedShooting = true;
        shootingUpgradeName = "CrossShooter";
        upgradeCount = 1;
    }
};

class DoubleRowShooter : public GenericRobot {
public:
    DoubleRowShooter(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        upgradedShooting = true;
        shootingUpgradeName = "DoubleRowShooter";
        upgradeCount = 1;
    }
};

int main() {
    ofstream logfile("log.txt");
    streambuf* originalCout = cout.rdbuf();
    // Mirror output to both console and log
    streambuf* logbuf = logfile.rdbuf();
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
            break;
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

        // FIXED: Initialize lives to 2 instead of 3
        int lives = 2;

        if (type == "GenericRobot") {
            robots.push_back(new GenericRobot(name, nextSymbol++, x, y, 1, 10, lives));
        }
        else if (type == "HideBot") {
            robots.push_back(new HideBot(name, nextSymbol++, x, y, 1, 10, lives));
        }
        else if (type == "JumpBot") {
            robots.push_back(new JumpBot(name, nextSymbol++, x, y, 1, 10, lives));
        }
        else if (type == "LongShotBot") {
            robots.push_back(new LongShotBot(name, nextSymbol++, x, y, 1, 10, lives));
        }
        else if (type == "SemiAutoBot") {
            robots.push_back(new SemiAutoBot(name, nextSymbol++, x, y, 1, 10, lives));
        }
        else if (type == "ThirtyShotBot") {
            robots.push_back(new ThirtyShotBot(name, nextSymbol++, x, y, 1, 10, lives));
        }
        else if (type == "ScoutBot") {
            robots.push_back(new ScoutBot(name, nextSymbol++, x, y, 1, 10, lives));
        }
        else if (type == "TrackBot") {
            robots.push_back(new TrackBot(name, nextSymbol++, x, y, 1, 10, lives));
        }
        else if (type == "PlusShooter") {
            robots.push_back(new PlusShooter(name, nextSymbol++, x, y, 1, 10, lives));
        }
        else if (type == "CrossShooter") {
            robots.push_back(new CrossShooter(name, nextSymbol++, x, y, 1, 10, lives));
        }
        else if (type == "DoubleRowShooter") {
            robots.push_back(new DoubleRowShooter(name, nextSymbol++, x, y, 1, 10, lives));
        }
    }
    setup.close();

    srand((unsigned)time(0));

    int turn = 1;
    while (turn <= numTurns) {
        int aliveCount = count_if(robots.begin(), robots.end(), [](Robot* r) { return r->alive; });
        if (aliveCount <= 1 && respawnQueue.empty()) break;

        cout << "----- Turn " << turn << " -----" << endl;

        // Display battlefield
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                char cell = '.';
                for (auto* r : robots) {
                    if (r->alive && !r->hidden && r->x == x && r->y == y) {
                        cell = r->symbol;
                        break;
                    }
                }
                cout << cell << " ";
            }
            cout << endl;
        }

        // Respawn robots
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

        // Process each robot's turn
        for (Robot* r : robots) {
            if (!r->alive) continue;
            r->think();
            r->look(robots);
            if (r->sawTarget) {
                r->fire(robots);
            }
            r->move(robots, width, height);
        }

        // Handle respawn queue
        for (Robot* r : robots) {
            if (!r->alive && r->lives > 0 &&
                find(respawnQueue.begin(), respawnQueue.end(), r) == respawnQueue.end()) {
                r->lives--;
                respawnQueue.push_back(r);
            }
        }

        // Display status
        cout << "--- Status after Turn " << turn << " ---" << endl;
        for (Robot* r : robots) {
            cout << r->name << " at (" << r->x << "," << r->y << ") shells=" << r->shells << " lives=" << r->lives;
            cout << " | Upgrades: ";
            if (r->upgradedMoving) {
                cout << r->moveUpgradeName;
                if (r->moveUpgradeName == "JumpBot") cout << "(" << r->jumpsLeft << ") ";
                else if (r->moveUpgradeName == "HideBot") cout << "(" << r->hidesLeft << ") ";
                else cout << " ";
            }
            if (r->upgradedShooting) cout << r->shootingUpgradeName << " ";
            if (r->upgradedSeeing) {
                cout << r->seeingUpgradeName;
                if (r->seeingUpgradeName == "ScoutBot") cout << "(" << r->scansLeft << ") ";
                else cout << " ";
            }
            if (!r->alive) cout << " [DEAD]";
            cout << endl;
        }
        cout << endl;
        turn++;
    }

    // Cleanup
    for (Robot* r : robots) delete r;
    for (Robot* r : respawnQueue) delete r;
    cout.rdbuf(originalCout);
    logfile.close();
    return 0;
}
