/**********|**********|**********|
Program: Group64_TT4l_TT2l.cpp
Course: OOPDS
Trimester: 2510
Name: Chetan Rana, Ainul Hafiz, Samad Tamim,
ID: 242UC241RV,243UC2466D,1221304756,
Lecture Section: TC1L
Tutorial Section: TT4L,TT2L
Email: rana.chetan@student.mmu.edu.my,muhammad.ainul.hafiz@student.mmu.edu.my,1221304756@student.mmu.edu.my,
Phone: 0192647906,0175568901.01115428845,
**********|**********|**********/
#include <iostream> 
#include <fstream> 
#include <vector> 
#include <string> 
#include <cstdlib> // For random numbers
#include <ctime> 
#include <algorithm> // For sorting/searching
#include <climits> // For max/min values
#include <iomanip> 
using namespace std; 

// Base class for all robots
class Robot {
private:
    int x, y;          // Private position on the grid
    
protected:
    // Protected setter for position
    void setPosition(int newX, int newY) {
        x = newX;
        y = newY;
    }

public:
    // Robot stats and info
    string name;       // Robot's name
    char symbol;       // Letter representation on map
    int health;        // HP bool
    int shells;        // shells counter
    int lives;         //1 + lives (1 at init) =total lives
    bool alive;        //check
    int initHealth, initShells; // Starting stats for respawns
    bool sawTarget = false; // Spotted enemies flag
    vector<Robot*> seenTargets; //visible enemies
    int scansLeft = 3; //ScoutBot counter
    vector<Robot*> trackedBots; // Tracked enemies for TrackBot
    bool trackBotHasScanned = false; // TrackBot scan 
    int kills = 0;     // kill counter
    int deaths = 0;    // death counter

    // Upgrade flags and counters
    bool upgradedMoving = false, upgradedShooting = false, upgradedSeeing = false;
    int upgradeCount = 0;
    string moveUpgradeName, shootingUpgradeName, seeingUpgradeName;
    int hidesLeft = 0; //HideBot counter
    bool hidden = false; //hidebot status
    int jumpsLeft = 0; //JumpBot counter

    //Constructor - Sets up new robot
    Robot(string n, char s, int ix, int iy, int hp, int ammo, int l)
        : name(n), symbol(s), x(ix), y(iy), health(hp), shells(ammo), lives(l), alive(true),
          initHealth(hp), initShells(ammo) {}
    
    //virtual functions
    virtual void think(const vector<Robot*>& robots, int width, int height) = 0;
    virtual void look(const vector<Robot*>& robots) = 0;
    virtual void fire(vector<Robot*>& robots) = 0;
    virtual void move(const vector<Robot*>& robots, int width, int height) = 0;

    // When robot gets hit
    bool takeDamage() {
        if (!alive) return false; // check
        if (hidden) { // HideBot protection
            cout << name << " is hidden, so it takes no damage." << endl;
            return false;
        }
        health--; // 
        cout << "  "<< name << " is hit! (Health=" << health << ")\n";
        if (health <= 0) { // Check
            alive = false;
            deaths++; 
            cout << "  "<< name << " is destroyed!\n";
            return true; // Confirmed kill
        }
        return false; //alive
    }

    //Self-destruct sequence
    void destroySelf() {
        if (!alive) return; // check
        alive = false;
        deaths++; 
        cout << name << " self-destructs!\n"; 
    }

    //Come back to life
    void respawn(int newX, int newY) {
        setPosition(newX, newY); // New position
        health = initHealth; // Reset health
        alive = true;
        sawTarget = false;
        hidden = false;
        seenTargets.clear(); // Clear enemy memory
        cout << name << " respawns at (" << newX << "," << newY << ") with " << health << " health and " << shells << " shells" << endl;
    }

    //Shooting range based on upgrades
    int getFiringRange() const {
        if (upgradedShooting && shootingUpgradeName == "LongShotBot") {
            return 3; // 
        }
        return 1; // Default is adjacent only
    }

    //Getters for basic info
    string getName() const { return name; }
    int getX() const { return x; }
    int getY() const { return y; }
    bool isAlive() const { return alive; }
    
    //Calculate K/D ratio 
    double getKillDeathRatio() const {
        if (deaths == 0) return kills; //errors
        return static_cast<double>(kills) / deaths;
    }
    
    //Friend for printing robot stats
    friend ostream& operator<<(ostream& os, const Robot& robot);
};

// Overload << operator to print robot info
ostream& operator<<(ostream& os, const Robot& robot) {
    os << robot.name << " at (" << robot.getX() << "," << robot.getY() << ") "
       << "HP=" << robot.health << " shells=" << robot.shells << " lives=" << robot.lives
       << " | Kills: " << robot.kills << " Deaths: " << robot.deaths 
       << " K/D: " << fixed << setprecision(2) << robot.getKillDeathRatio(); // Fancy K/D
    
    // Show upgrades if any
    os << " | Upgrades: ";
    if (robot.upgradedMoving) {
        os << robot.moveUpgradeName;
        if (robot.moveUpgradeName == "JumpBot") os << "(" << robot.jumpsLeft << ") "; // Show jumps left
        else if (robot.moveUpgradeName == "HideBot") os << "(" << robot.hidesLeft << ") "; // Show hides left
        else os << " ";
    }
    if (robot.upgradedShooting) os << robot.shootingUpgradeName << " ";
    if (robot.upgradedSeeing) {
        os << robot.seeingUpgradeName;
        if (robot.seeingUpgradeName == "ScoutBot") os << "(" << robot.scansLeft << ") "; // Show scans left
        else os << " ";
    }
    if (!robot.alive) os << " [DEAD]"; 
    return os;
}

// ABSTRACT CLASS - ThinkingRobot
class ThinkingRobot {
public:
    // Pure virtual function for derived classes
    virtual void performThinking(const vector<Robot*>& robots, int width, int height) = 0;
    
    // destructor 
    virtual ~ThinkingRobot() {}
};

// ABSTRACT CLASS - SeeingRobot  
class SeeingRobot {
public:
    // Pure virtual function for derived classes
    virtual void performSeeing(const vector<Robot*>& robots) = 0;
    
    //destructor
    virtual ~SeeingRobot() {}
};

// ABSTRACT CLASS - ShootingRobot
class ShootingRobot {
public:
    // Pure virtual function for derived classes
    virtual void performShooting(vector<Robot*>& robots) = 0;
    
    //destructor
    virtual ~ShootingRobot() {}
};

// ABSTRACT CLASS - MovingRobot
class MovingRobot {
public:
    // Pure virtual function for derived classes
    virtual void performMoving(const vector<Robot*>& robots, int width, int height) = 0;
    
    // Virtual destructor
    virtual ~MovingRobot() {}
};

// Generic robot inheriting from all 4 abstract classes using multiple inheritance
class GenericRobot : public Robot, public ThinkingRobot, public SeeingRobot, public ShootingRobot, public MovingRobot {
public:
    GenericRobot(string n, char s, int x, int y, int hp, int ammo, int l)
        : Robot(n, s, x, y, hp, ammo, l) {}

    // Implement Robot's pure virtual functions
    void think(const vector<Robot*>& robots, int width, int height) override { 
        performThinking(robots, width, height);  
    }
    void look(const vector<Robot*>& robots) override { 
        performSeeing(robots); 
    }
    void fire(vector<Robot*>& robots) override { 
        performShooting(robots); 
    }
    void move(const vector<Robot*>& robots, int width, int height) override {
        performMoving(robots, width, height);
    }

    // Implement ThinkingRobot's pure virtual function
    void performThinking(const vector<Robot*>& robots, int width, int height) override {
        // HideBot special handling
        if (upgradedMoving && moveUpgradeName == "HideBot" && hidesLeft > 0) {
            cout << name << " is hidden and invulnerable this turn." << endl;
            hidden = true; // Activate cloak
            hidesLeft--; // Use one hide
        } else {
            cout << name << " is thinking..." << endl; // Robot is pondering
        }
        
        // Standard thinking sequence
        performSeeing(robots); // Look around
        if (sawTarget) {
            performShooting(const_cast<vector<Robot*>&>(robots)); // 
        }
        performMoving(robots, width, height); // Change position
    }

    // Implement SeeingRobot's pure virtual function
    void performSeeing(const vector<Robot*>& robots) override {
        seenTargets.clear(); // clear all

        // TrackBot special ability
        if (upgradedSeeing && seeingUpgradeName == "TrackBot") {
            if (!trackBotHasScanned) {
                trackedBots.clear(); // Reset tracking list
                vector<Robot*> available; // Valid targets
                for (Robot* r : robots) {
                    if (r != this && r->isAlive() && !r->hidden) {
                        available.push_back(r); // Add living targets
                    }
                }
                random_shuffle(available.begin(), available.end()); // random
                int toTrack = min(3, (int)available.size()); //3 bots
                for (int i = 0; i < toTrack; i++) {
                    trackedBots.push_back(available[i]); // Add to tracking list
                }
                trackBotHasScanned = true; 
                cout << name << " tracked " << toTrack << " robots." << endl;
            }

            // Adds tracked bots to visible list
            for (Robot* t : trackedBots) {
                if (t->isAlive() && !t->hidden) {
                    seenTargets.push_back(t);
                }
            }
        }

        // ScoutBot full map look
        if (upgradedSeeing && seeingUpgradeName == "ScoutBot" &&
            scansLeft > 0) {
            cout << name << " uses ScoutBot scan ("
                 << scansLeft << " left):\n";
            for (auto r : robots) {
                if (r != this && r->isAlive() && !r->hidden) {
                    // Add if not already in list
                    if (find(seenTargets.begin(), seenTargets.end(), r) == seenTargets.end()) {
                        seenTargets.push_back(r);
                    }
                }
            }
            scansLeft--; 
        }

        // Check adjacent squares (normal vision)
        int currentX = getX();
        int currentY = getY();
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue; // Skip self
                int nx = currentX + dx, ny = currentY + dy;
                for (auto r : robots) {
                    if (r != this && r->isAlive() && !r->hidden &&
                        r->getX() == nx && r->getY() == ny) {
                        // Add if we see them
                        if (find(seenTargets.begin(), seenTargets.end(), r) == seenTargets.end()) {
                            seenTargets.push_back(r);
                        }
                    }
                }
            }
        }

        // Update target sighting flag
        sawTarget = !seenTargets.empty();
        if (sawTarget) {
            for (auto r : seenTargets) {
                cout << "  " << name << " sees " << r->name << " at (" << r->getX() << "," << r->getY() << ")\n";
            }
        } else {
            cout << "  " << name << " sees no one.\n"; 
        }
    }

    // Implement ShootingRobot's pure virtual function
    void performShooting(vector<Robot*>& robots) override {
        if (!isAlive() || shells <= 0) return; // check

        // SemiAutoBot
        if (shootingUpgradeName == "SemiAutoBot" && sawTarget) {
            Robot* target = seenTargets[rand() % seenTargets.size()]; // Pick random target
            cout << name << " (SemiAutoBot) fires 3 shots at "
                 << target->name << "! ";
            shells--; 
            int hits = 0;
            bool destroyed = false;
            for (int i = 0; i < 3; i++) { // 
                if (rand() % 100 < 70) { // 70% hit chance
                    hits++;
                    if (target->takeDamage()) { // Check if killed
                        destroyed = true;
                        kills++; // Add to kill count
                    }
                }
            }
            if (hits > 0) {
                cout << "HIT " << hits << " times!\n";
            } else {
                cout << "All shots miss.\n"; 
            }
            if (shells <= 0) destroySelf(); 
            return;
        }

        // LongShotBot
        if (shootingUpgradeName == "LongShotBot" && sawTarget) {
            vector<Robot*> candidates;
            int currentX = getX();
            int currentY = getY();
            for (Robot* r : seenTargets) {
                int dx = abs(r->getX() - currentX);
                int dy = abs(r->getY() - currentY);
                int dist = dx + dy; // Manhattan distance
                if (dist > 0 && dist <= 3) { // Within 3 tiles
                    candidates.push_back(r);
                }
            }
            if (!candidates.empty()) {
                Robot* target = candidates[rand() % candidates.size()]; // Random valid target
                cout << name << " (LongShotBot) fires at "
                     << target->name << " (dist=" << abs(target->getX() - currentX) + abs(target->getY() - currentY) << ")... ";
                shells--;
                if (rand() % 100 < 70) { // 70% hit chance
                    cout << "HIT!\n";
                    if (target->takeDamage()) {
                        kills++; //
                    }
                } else {
                    cout << "misses.\n";
                }
                if (shells <= 0) destroySelf();
                return;
            } else {
                cout << name << " (LongShotBot) sees no target within 3-unit range." << endl;
            }
        }

        // PlusShooter: Horizontal/Vertical attack
        if (shootingUpgradeName == "PlusShooter") {
            cout << name << " fires in + pattern!" << endl;
            bool fired = false;
            int currentX = getX();
            int currentY = getY();
            for (Robot* r : seenTargets) {
                if ((r->getX() == currentX || r->getY() == currentY) && !(r->getX() == currentX && r->getY() == currentY)) {
                    cout << "  Targeting " << r->name << " at (" << r->getX() << "," << r->getY() << ")... ";
                    shells--;
                    fired = true;
                    if (rand() % 100 < 70) { // Hit check
                        cout << "HIT!" << endl;
                        if (r->takeDamage()) {
                            kills++;
                        }
                    } else {
                        cout << "missed." << endl;
                    }
                    if (shells <= 0) {
                        destroySelf(); 
                        return;
                    }
                }
            }
            if (!fired) {
                cout << "  No valid targets in + pattern, falling back to regular fire." << endl;
            } else {
                return; // Done if we fired
            }
        }

        // CrossShooter: Diagonal attack
        if (shootingUpgradeName == "CrossShooter") {
            cout << name << " fires in X pattern!" << endl;
            bool fired = false;
            int currentX = getX();
            int currentY = getY();
            for (Robot* r : seenTargets) {
                if (abs(r->getX() - currentX) == abs(r->getY() - currentY) && !(r->getX() == currentX && r->getY() == currentY)) {
                    cout << "  Targeting " << r->name << " at (" << r->getX() << "," << r->getY() << ")... ";
                    shells--;
                    fired = true;
                    if (rand() % 100 < 70) { // Hit check
                        cout << "HIT!" << endl;
                        if (r->takeDamage()) {
                            kills++;
                        }
                    } else {
                        cout << "missed." << endl;
                    }
                    if (shells <= 0) {
                        destroySelf();
                        return;
                    }
                }
            }
            if (!fired) {
                cout << "  No valid targets in X pattern, falling back to regular fire." << endl;
            } else {
                return;
            }
        }

        // DoubleRowShooter: Row-based attack
        if (shootingUpgradeName == "DoubleRowShooter") {
            cout << name << " fires across two rows!" << endl;
            bool fired = false;
            int currentY = getY();
            for (Robot* r : seenTargets) {
                if (r->getY() == currentY || r->getY() == currentY + 1 || r->getY() == currentY - 1) {
                    cout << "  Targeting " << r->name << " at (" << r->getX() << "," << r->getY() << ")... ";
                    shells--;
                    fired = true;
                    if (rand() % 100 < 70) { // Hit check
                        cout << "HIT!" << endl;
                        if (r->takeDamage()) {
                            kills++;
                        }
                    } else {
                        cout << "missed." << endl;
                    }
                    if (shells <= 0) {
                        destroySelf();
                        return;
                    }
                }
            }
            if (!fired) {
                cout << "  No valid targets in row pattern, falling back to regular fire." << endl;
            } else {
                return;
            }
        }

        // DEFAULT SHOOTING
        vector<Robot*> adjacentTargets;
        int currentX = getX();
        int currentY = getY();
        for (Robot* t : seenTargets) {
            int dx = abs(t->getX() - currentX);
            int dy = abs(t->getY() - currentY);
            if (dx <= 1 && dy <= 1 && (dx != 0 || dy != 0)) { 
                adjacentTargets.push_back(t);
            }
        }

        if (adjacentTargets.empty()) {
            cout << name << " sees no adjacent target to fire at." << endl;
            return; 
        }

        // Shoot random adjacent target
        Robot* target = adjacentTargets[rand() % adjacentTargets.size()];
        cout << name << " fires at " << target->name << "... ";
        shells--;
        if (rand() % 100 < 70) { // 70% hit chance
            cout << "HIT!" << endl;
            if (target->takeDamage()) {
                kills++; // Add to kill count
            }
        } else {
            cout << "misses." << endl; // 
        }
        if (shells <= 0) destroySelf(); // self destructs conditon


        // UPGRADE SYSTEM
        if (target && !target->isAlive() && upgradeCount < 3) {
            cout << name << " gets an upgrade!" << endl;
            vector<int> available; // Available upgrade slots
            if (!upgradedMoving) available.push_back(1); // Movement
            if (!upgradedShooting) available.push_back(2); // Shooting
            if (!upgradedSeeing) available.push_back(3); // Vision

            if (!available.empty()) {
                int cat = available[rand() % available.size()]; // Random upgrade type
                switch (cat) {
                    case 1: {  // Movement upgrade
                        upgradedMoving = true;
                        if (rand() % 2 == 0) { // 50/50 choice
                            moveUpgradeName = "JumpBot";
                            jumpsLeft = 3; // Give 3 jumps
                            cout << name << " upgraded to JumpBot." << endl;
                        } else {
                            moveUpgradeName = "HideBot";
                            hidesLeft = 3; // Give 3 hides
                            cout << name << " upgraded to HideBot." << endl;
                        }
                        break;
                    }
                    case 2: {  // Shooting upgrade
                        upgradedShooting = true;
                        int choice = rand() % 6; // 6 shooter types
                        if (choice == 0) {
                            shootingUpgradeName = "LongShotBot";
                            cout << name << " upgraded to LongShotBot." << endl;
                        } else if (choice == 1) {
                            shootingUpgradeName = "SemiAutoBot";
                            cout << name << " upgraded to SemiAutoBot." << endl;
                        } else if (choice == 2) {
                            shootingUpgradeName = "ThirtyShotBot";
                            shells = 30;  //
                            cout << name << " upgraded to ThirtyShotBot." << endl;
                        } else if (choice == 3) {
                            shootingUpgradeName = "PlusShooter";
                            cout << name << " upgraded to PlusShooter." << endl;
                        } else if (choice == 4) {
                            shootingUpgradeName = "CrossShooter";
                            cout << name << " upgraded to CrossShooter." << endl;
                        } else {
                            shootingUpgradeName = "DoubleRowShooter";
                            cout << name << " upgraded to DoubleRowShooter." << endl;
                        }
                        break;
                    }
                    case 3: {  // Vision upgrade
                        upgradedSeeing = true;
                        if (rand() % 2 == 0) { // 50/50 choice
                            seeingUpgradeName = "ScoutBot";
                            scansLeft = 3; // 3 scans
                            cout << name << " upgraded to ScoutBot." << endl;
                        } else {
                            seeingUpgradeName = "TrackBot";
                            cout << name << " upgraded to TrackBot." << endl;
                        }
                        break;
                    }
                }
                upgradeCount++; // Mark upgrade slot used
            }
        }
    }

    // Implement MovingRobot's pure virtual function
    void performMoving(const vector<Robot*>& robots, int width, int height) override {
        if (!isAlive()) return; //check

        // Resets hide status unless still hiding
        if (hidden && !(upgradedMoving && moveUpgradeName == "HideBot" && hidesLeft > 0)) {
            hidden = false; // Become visible
        }

        // JumpBot
        if (upgradedMoving && moveUpgradeName == "JumpBot" &&
            jumpsLeft > 0 && sawTarget) {
            Robot* closest = nullptr;
            int minDist = INT_MAX;
            int currentX = getX();
            int currentY = getY();
            // Find closest enemy
            for (Robot* t : seenTargets) {
                if (!t->isAlive()) continue;
                int dist = abs(t->getX() - currentX) + abs(t->getY() - currentY); 
                if (dist < minDist) {
                    minDist = dist;
                    closest = t;
                }
            }

            if (closest) {
                // Find empty spots near target
                vector<pair<int, int>> options;
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        if (dx == 0 && dy == 0) continue; // Skip target's position
                        int nx = closest->getX() + dx, ny = closest->getY() + dy;
                        if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue; // Boundary check
                        bool occ = false; //Occupied flag
                        for (auto r : robots) {
                            if (r->isAlive() && r->getX() == nx && r->getY() == ny) {
                                occ = true; // Spot occupied
                                break;
                            }
                        }
                        if (!occ) options.push_back({nx, ny}); // Valid landing spot
                    }
                }

                if (!options.empty()) {
                    auto [x_new, y_new] = options[rand() % options.size()]; // Pick random spot
                    setPosition(x_new, y_new);
                    jumpsLeft--; 
                    cout << name << " jumps to (" << x_new << "," << y_new << ") near " << closest->name << "\n";
                    return; 
                }
            }
        }

        // Find closest target to move toward
        Robot* target = nullptr;
        int minDist = INT_MAX;
        int currentX = getX();
        int currentY = getY();

        // Prefer tracked bots if available
        if (upgradedSeeing && seeingUpgradeName == "TrackBot") {
            for (Robot* t : trackedBots) {
                if (!t->isAlive() || t->hidden) continue;
                int dist = abs(t->getX() - currentX) + abs(t->getY() - currentY);
                if (dist < minDist) {
                    minDist = dist;
                    target = t;
                }
            }
        }

        // Fallback to any seen enemy
        if (!target && sawTarget) {
            for (Robot* t : seenTargets) {
                if (!t->isAlive()) continue;
                int dist = abs(t->getX() - currentX) + abs(t->getY() - currentY);
                if (dist < minDist) {
                    minDist = dist;
                    target = t;
                }
            }
        }

        // Move toward target if found
        if (target) {
            // Determine direction (1 = right/down, -1 = left/up)
            int dx = (target->getX() > currentX) ? 1 :
                     (target->getX() < currentX) ? -1 : 0;
            int dy = (target->getY() > currentY) ? 1 :
                     (target->getY() < currentY) ? -1 : 0;
            int nx = currentX + dx, ny = currentY + dy;

            // Check if move is valid
            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                bool occupied = false;
                for (auto r : robots) {
                    if (r->isAlive() && r->getX() == nx && r->getY() == ny) {
                        occupied = true; // Blocked by another bot
                        break;
                    }
                }
                if (!occupied) {
                    setPosition(nx, ny);
                    cout << name << " moves toward " << target->name
                         << " to (" << nx << "," << ny << ")\n";
                    return; 
                }
            }
        }

        // No target or blocked? Wander randomly
        vector<pair<int, int>> options;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue; // Skip staying put
                int nx = currentX + dx, ny = currentY + dy;
                if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue; // Map bounds
                bool occ = false;
                for (auto r : robots) {
                    if (r->isAlive() && r->getX() == nx && r->getY() == ny) {
                        occ = true; // Spot taken
                        break;
                    }
                }
                if (!occ) options.push_back({nx, ny}); // Valid move
            }
        }
        if (!options.empty()) {
            auto [nx, ny] = options[rand() % options.size()]; // Pick random move
            setPosition(nx, ny);
            cout << name << " moves to (" << nx << "," << ny << ")\n";
            cout << endl;
        }
    }
};

// Specialized bot: HideBot (starts with Hide ability)
class HideBot : public GenericRobot {
public:
    HideBot(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        upgradedMoving = true;
        moveUpgradeName = "HideBot";
        hidesLeft = 3; // Start with 3 hides
        upgradeCount = 1;
    }

    // Override think for HideBot logic
    void think(const vector<Robot*>& robots, int width, int height) override {
        if (hidesLeft > 0) {
            cout << name << " (HideBot) is hidden this turn ("
                 << hidesLeft << " hides left)\n";
            hidden = true; // Activate cloak
            hidesLeft--; // Use one hide
            
            // Still looks and shoot while hidden
            look(robots);
            if (sawTarget) {
                fire(const_cast<vector<Robot*>&>(robots));
            }
            move(robots, width, height);
        } else {
            // No hides left, Act like normal bot
            performThinking(robots, width, height);
        }
    }
};

//upgrade classes

class JumpBot : public GenericRobot {
public:
    JumpBot(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        upgradedMoving = true;
        moveUpgradeName = "JumpBot";
        jumpsLeft = 3; // 3 jumps,
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
        shootingUpgradeName = "SemiAutoBot"; // 
        upgradeCount = 1;
    }
};

class ThirtyShotBot : public GenericRobot {
public:
    ThirtyShotBot(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, 30, l) {  // Start with 30 ammo
        upgradedShooting = true;
        shootingUpgradeName = "ThirtyShotBot"; // 
        upgradeCount = 1;
    }
};

class ScoutBot : public GenericRobot {
public:
    ScoutBot(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        upgradedSeeing = true;
        seeingUpgradeName = "ScoutBot"; 
        scansLeft = 3; // 3 scans
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

// Main game function
int main() {
    ofstream logfile("log.txt"); // Create log file
    streambuf* originalCout = cout.rdbuf(); // Save original cout

    // Dual output (console + log file)
    streambuf* logbuf = logfile.rdbuf();
    class DualBuf : public streambuf {
        streambuf *buf1, *buf2; // Two output buffers
    public:
        DualBuf(streambuf* b1, streambuf* b2) : buf1(b1), buf2(b2) {}
        int overflow(int c) override {
            if (c == EOF) return !EOF;
            if (buf1) buf1->sputc(c); // Send to first buffer (console)
            if (buf2) buf2->sputc(c); // Send to second buffer (log file)
            return c;
        }
    } dualbuf(originalCout, logbuf);
    
    cout.rdbuf(&dualbuf); // Redirect cout to dual output
    
    // Read game setup
    ifstream setup("setup.txt");
    if (!setup) {
        cerr << "Failed to open setup.txt" << endl;
        return 1;
    }

    // Default game settings
    int width = 10, height = 10, numTurns = 100, numRobots = 0;
    string line;
    while (getline(setup, line)) {
        if (line.find("M by N") != string::npos) {
            sscanf(line.c_str(), "M by N : %d %d", &width, &height); // Read map size
        } else if (line.find("steps") != string::npos) {
            sscanf(line.c_str(), "steps: %d", &numTurns); // Read max turns
        } else if (line.find("robots") != string::npos) {
            sscanf(line.c_str(), "robots: %d", &numRobots); // Read robot count
            break;
        }
    }

    // Create robots
    vector<Robot*> robots, respawnQueue;
    char nextSymbol = 'A'; // Starting map symbol
    for (int i = 0; i < numRobots; ++i) {
        string type, name;
        string xs, ys;
        setup >> type >> name >> xs >> ys; // Read robot config
        
        // Handling random positions
        int x = (xs == "random") ? rand() % width : stoi(xs);
        int y = (ys == "random") ? rand() % height : stoi(ys);

        int lives = 2; // Default lives, 3?

        // Create robot based on type
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

    srand((unsigned)time(0)); // Seed random generator

    // Main game loop
    int turn = 1;
    while (turn <= numTurns) {
        int aliveCount = count_if(robots.begin(), robots.end(), [](Robot* r) { return r->isAlive(); });
        if (aliveCount <= 1 && respawnQueue.empty()) break; // Stop if only 1 bot left

        cout << "----- Turn " << turn << " -----" << endl;

        // Draw battle map
        cout << string(width * 2 + 2, '*') << endl; // Top border
        
        for (int y = 0; y < height; ++y) {
            cout << '*'; // Left border
            for (int x = 0; x < width; ++x) {
                char cell = '.'; // Empty space
                // Check for robots at this position
                for (auto* r : robots) {
                    if (r->isAlive() && !r->hidden && r->getX() == x && r->getY() == y) {
                        cell = r->symbol; // Robot letter
                        break;
                    }
                }
                cout << cell << " "; // Print cell
            }
            cout << '*' << endl; // Right border
        }
        
        // Bottom border
        cout << string(width * 2 + 2, '*') << endl;

        // Respawn dead robots
        if (!respawnQueue.empty()) {
            Robot* r = respawnQueue.front();
            respawnQueue.erase(respawnQueue.begin());
            int nx, ny;
            // Find empty spot
            do {
                nx = rand() % width;
                ny = rand() % height;
                bool occ = false;
                for (auto* other : robots) {
                    if (other->isAlive() && other->getX() == nx && other->getY() == ny) {
                        occ = true; break;
                    }
                }
                if (!occ) break;
            } while (true);
            r->respawn(nx, ny); 
        }

        // Process each robot's turn
        for (Robot* r : robots) {
            if (!r->isAlive()) continue; // Skip dead bots
            r->think(robots, width, height); // AI thinking
        }

        // Queue dead robots for respawn
        for (Robot* r : robots) {
            if (!r->isAlive() && r->lives > 0 &&
                find(respawnQueue.begin(), respawnQueue.end(), r) == respawnQueue.end()) {
                r->lives--; // Use one life
                respawnQueue.push_back(r); // Add to respawn line
            }
        }

        // Print status report
        cout << "--- Status after Turn " << turn << " ---" << endl;
        for (Robot* r : robots) {
            cout << *r << endl; // Use overloaded << operator
        }
        cout << endl;
        turn++;
    }

    // Cleanup
    for (Robot* r : robots) delete r; // Delete active robots
    for (Robot* r : respawnQueue) delete r; // Delete respawn queue
    cout.rdbuf(originalCout); // Restore cout
    logfile.close(); // Close log
    return 0;
}
