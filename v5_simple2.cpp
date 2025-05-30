// this file is used to make the robot game run
// we include a bunch of needed stuff here
#include <iostream>     // for cout and printing
#include <fstream>      // for file reading and writing
#include <vector>       // to use dynamic lists
#include <string>       // to use strings
#include <cstdlib>      // for rand and stuff
#include <ctime>        // for time functions
#include <algorithm>    // for random shuffle
#include <climits>      // for INT_MAX

using namespace std;    // so we don’t need to type std::

// this is the main robot class. other robot types will use this as base
class Robot {
public:
    string robName;         // the robot's name
    char robSymbol;         // the character used to show the robot
    int posX, posY;         // robot's position
    int hp;                 // health points
    int ammo;               // number of shells left
    int lifeLeft;           // how many lives left
    bool isAlive;           // true if robot is alive
    int startHp, startAmmo; // starting health and ammo
    bool sawEnemy = false;  // did this robot see someone last turn
    vector<Robot*> seenEnemies;  // who the robot saw
    int scanChances = 3;    // for scoutbot scans
    vector<Robot*> followedEnemies; // for trackbot
    bool hasTracked = false;    // if trackbot has scanned already

    // upgrade flags
    bool canMoveUpgrade = false;
    bool canShootUpgrade = false;
    bool canSeeUpgrade = false;
    int numUpgrades = 0;
    string moveUpgradeType;
    string shootUpgradeType;
    string seeUpgradeType;
    int hidesLeft = 0;      // for hidebot
    bool isHidden = false;  // true if hiding
    int jumpsLeft = 0;      // for jumpbot

    // constructor: sets up robot with its name, symbol and starting data
    Robot(string name, char symbol, int startX, int startY, int health, int shells, int lives)
        : robName(name), robSymbol(symbol), posX(startX), posY(startY), hp(health),
          ammo(shells), lifeLeft(lives), isAlive(true), startHp(health), startAmmo(shells) {}

    // all robots must override these
    virtual void think() = 0;
    virtual void look(const vector<Robot*>& allRobots) = 0;
    virtual void fire(vector<Robot*>& allRobots) = 0;
    virtual void move(const vector<Robot*>& allRobots, int width, int height) = 0;

    // take damage if hit
    void takeHit() {
        if (!isAlive) return; // don't hit dead robots
        if (isHidden) {
            cout << robName << " is hiding, no damage taken." << endl;
            return;
        }
        hp--; // lose 1 health
        cout << robName << " got hit! (Health now = " << hp << ")" << endl;
        if (hp <= 0) {
            isAlive = false;
            cout << robName << " is destroyed!" << endl;
        }
    }

    // robot kills itself
    void selfDestruct() {
        if (!isAlive) return;
        isAlive = false;
        cout << robName << " blew itself up!" << endl;
    }

    // bring robot back to life
    void bringBack(int newX, int newY) {
        posX = newX;
        posY = newY;
        hp = startHp;
        isAlive = true;
        sawEnemy = false;
        isHidden = false;
        seenEnemies.clear();
        cout << robName << " came back at (" << posX << "," << posY << ") with " << hp << " hp and " << ammo << " ammo." << endl;
    }

    // get shooting range if upgraded
    int getShootRange() const {
        if (canShootUpgrade && shootUpgradeType == "LongShotBot") {
            return 3;
        }
        return 1; // default range
    }

    // helper functions
    string getName() const { return robName; }
    int getX() const { return posX; }
    int getY() const { return posY; }
    bool alive() const { return isAlive; }
};

// robot that can think before action
class ThinkingRobot {
public:
    // this robot thinks before doing stuff
    virtual void doThinking(Robot* myself) {
        if (myself->canMoveUpgrade && myself->moveUpgradeType == "HideBot" && myself->hidesLeft > 0) {
            cout << myself->robName << " is hiding and safe this round." << endl;
            myself->isHidden = true;
            myself->hidesLeft--; // one less hide
        } else {
            cout << myself->robName << " is thinking..." << endl;
        }
    }
};
// class for robots that can look around
class SeeingRobot {
public:
    // method to check surroundings
    virtual void doLooking(Robot* myself, const vector<Robot*>& allRobots) {
        myself->seenEnemies.clear();  // clear what it saw last time

        // TRACKBOT special vision
        if (myself->canSeeUpgrade && myself->seeUpgradeType == "TrackBot") {
            if (!myself->hasTracked) {
                myself->followedEnemies.clear();  // empty list of followed targets
                vector<Robot*> canBeTracked;

                // check for enemies to track
                for (Robot* r : allRobots) {
                    if (r != myself && r->alive() && !r->isHidden) {
                        canBeTracked.push_back(r);
                    }
                }

                // shuffle the list to pick random targets
                random_shuffle(canBeTracked.begin(), canBeTracked.end());

                int numToTrack = min(3, (int)canBeTracked.size()); // track max 3
                for (int i = 0; i < numToTrack; i++) {
                    myself->followedEnemies.push_back(canBeTracked[i]); // add to tracked list
                }

                myself->hasTracked = true;
                cout << myself->robName << " is now tracking " << numToTrack << " robots." << endl;
            }

            // add tracked enemies to seen list if they are alive and visible
            for (Robot* t : myself->followedEnemies) {
                if (t->alive() && !t->isHidden) {
                    myself->seenEnemies.push_back(t);
                }
            }
        }

        // SCOUTBOT wide vision
        if (myself->canSeeUpgrade && myself->seeUpgradeType == "ScoutBot" && myself->scanChances > 0) {
            cout << myself->robName << " uses ScoutBot scan (" << myself->scanChances << " left):" << endl;
            for (auto r : allRobots) {
                if (r != myself && r->alive() && !r->isHidden) {
                    if (find(myself->seenEnemies.begin(), myself->seenEnemies.end(), r) == myself->seenEnemies.end()) {
                        myself->seenEnemies.push_back(r); // only add if not seen before
                    }
                }
            }
            myself->scanChances--;
        }

        // Normal looking in 8 directions
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue; // skip own position
                int checkX = myself->posX + dx;
                int checkY = myself->posY + dy;

                for (auto r : allRobots) {
                    if (r != myself && r->alive() && !r->isHidden &&
                        r->posX == checkX && r->posY == checkY) {
                        if (find(myself->seenEnemies.begin(), myself->seenEnemies.end(), r) == myself->seenEnemies.end()) {
                            myself->seenEnemies.push_back(r); // add if not already seen
                        }
                    }
                }
            }
        }

        // print what this robot can see
        myself->sawEnemy = !myself->seenEnemies.empty(); // update flag

        if (myself->sawEnemy) {
            for (auto r : myself->seenEnemies) {
                cout << "  " << myself->robName << " sees " << r->robName
                     << " at (" << r->posX << "," << r->posY << ")" << endl;
            }
        } else {
            cout << "  " << myself->robName << " sees no one." << endl;
        }
    }
};
// class for robots that can shoot
class ShootingRobot {
public:
    // method for firing at enemies
    virtual void doShooting(Robot* myself, vector<Robot*>& allRobots) {
        // if robot is dead or has no ammo, do nothing
        if (!myself->alive() || myself->ammo <= 0) return;

        // SEMIAUTOBOT: fire 3 times with 1 shell if target seen
        if (myself->shootUpgradeType == "SemiAutoBot" && myself->sawEnemy) {
            Robot* target = myself->seenEnemies[rand() % myself->seenEnemies.size()];
            cout << myself->robName << " (SemiAutoBot) shoots 3 bullets at " << target->robName << "! ";
            myself->ammo--;  // use up 1 shell

            int totalHits = 0;
            for (int i = 0; i < 3; i++) {
                if (rand() % 100 < 70) totalHits++; // 70% hit chance
            }

            if (totalHits > 0) {
                cout << "Hit " << totalHits << " times!" << endl;
                for (int i = 0; i < totalHits; i++) {
                    target->takeHit(); // apply damage
                }
            } else {
                cout << "All missed!" << endl;
            }

            if (myself->ammo <= 0) myself->selfDestruct(); // destroy if no ammo
            return;
        }

        // LONGSHOTBOT: shoot from distance if target in range
        if (myself->shootUpgradeType == "LongShotBot" && myself->sawEnemy) {
            vector<Robot*> farTargets;

            // look for targets in range 1–3
            for (Robot* r : myself->seenEnemies) {
                int dx = abs(r->posX - myself->posX);
                int dy = abs(r->posY - myself->posY);
                int dist = dx + dy;

                if (dist > 0 && dist <= 3) {
                    farTargets.push_back(r);
                }
            }

            if (!farTargets.empty()) {
                Robot* pick = farTargets[rand() % farTargets.size()];
                cout << myself->robName << " (LongShotBot) fires at " << pick->robName
                     << " (dist=" << abs(pick->posX - myself->posX) + abs(pick->posY - myself->posY) << ")... ";

                myself->ammo--;
                if (rand() % 100 < 70) {
                    cout << "Hit!" << endl;
                    pick->takeHit();
                } else {
                    cout << "Missed." << endl;
                }

                if (myself->ammo <= 0) myself->selfDestruct();
                return;
            } else {
                cout << myself->robName << " sees no one in long range." << endl;
                // fall through to regular shooting
            }
        }

        // PLUSTYPE: shoot in + pattern (same row or same col)
        if (myself->shootUpgradeType == "PlusShooter") {
            cout << myself->robName << " shoots in a + pattern!" << endl;
            bool shot = false;

            for (Robot* r : myself->seenEnemies) {
                if ((r->posX == myself->posX || r->posY == myself->posY) &&
                    !(r->posX == myself->posX && r->posY == myself->posY)) {
                    cout << "  Targeting " << r->robName << " at (" << r->posX << "," << r->posY << ")... ";
                    if (rand() % 100 < 70) {
                        cout << "Hit!" << endl;
                        r->takeHit();
                    } else {
                        cout << "Missed." << endl;
                    }
                    myself->ammo--;
                    shot = true;
                    if (myself->ammo <= 0) {
                        myself->selfDestruct();
                        return;
                    }
                }
            }

            if (!shot) {
                cout << "  No valid + targets. Firing normal shot instead." << endl;
            } else return;
        }

        // CROSSTYPE: shoot in X pattern (diagonals)
        if (myself->shootUpgradeType == "CrossShooter") {
            cout << myself->robName << " shoots in an X pattern!" << endl;
            bool shot = false;

            for (Robot* r : myself->seenEnemies) {
                if (abs(r->posX - myself->posX) == abs(r->posY - myself->posY) &&
                    !(r->posX == myself->posX && r->posY == myself->posY)) {
                    cout << "  Targeting " << r->robName << " at (" << r->posX << "," << r->posY << ")... ";
                    if (rand() % 100 < 70) {
                        cout << "Hit!" << endl;
                        r->takeHit();
                    } else {
                        cout << "Missed." << endl;
                    }
                    myself->ammo--;
                    shot = true;
                    if (myself->ammo <= 0) {
                        myself->selfDestruct();
                        return;
                    }
                }
            }

            if (!shot) {
                cout << "  No valid X targets. Firing normal shot instead." << endl;
            } else return;
        }

        // DOUBLEROWSHOOTER: shoot across 3 rows
        if (myself->shootUpgradeType == "DoubleRowShooter") {
            cout << myself->robName << " fires across 3 rows!" << endl;
            bool shot = false;

            for (Robot* r : myself->seenEnemies) {
                if (r->posY == myself->posY || r->posY == myself->posY + 1 || r->posY == myself->posY - 1) {
                    cout << "  Targeting " << r->robName << " at (" << r->posX << "," << r->posY << ")... ";
                    if (rand() % 100 < 70) {
                        cout << "Hit!" << endl;
                        r->takeHit();
                    } else {
                        cout << "Missed." << endl;
                    }
                    myself->ammo--;
                    shot = true;
                    if (myself->ammo <= 0) {
                        myself->selfDestruct();
                        return;
                    }
                }
            }

            if (!shot) {
                cout << "  No targets in row area. Firing normal shot instead." << endl;
            } else return;
        }

        // REGULAR shooting: just nearby enemies
        vector<Robot*> nearbyTargets;
        for (Robot* r : myself->seenEnemies) {
            int dx = abs(r->posX - myself->posX);
            int dy = abs(r->posY - myself->posY);
            if (dx <= 1 && dy <= 1 && (dx != 0 || dy != 0)) {
                nearbyTargets.push_back(r);
            }
        }

        if (nearbyTargets.empty()) {
            cout << myself->robName << " has no nearby enemies to shoot." << endl;
            return;
        }

        Robot* target = nearbyTargets[rand() % nearbyTargets.size()];
        cout << myself->robName << " shoots at " << target->robName << "... ";
        myself->ammo--;
        if (rand() % 100 < 70) {
            cout << "Hit!" << endl;
            target->takeHit();
        } else {
            cout << "Missed." << endl;
        }

        if (myself->ammo <= 0) myself->selfDestruct();

        // CHECK if upgrade earned after killing
        if (target && !target->alive() && myself->numUpgrades < 3) {
            cout << myself->robName << " earned an upgrade!" << endl;

            vector<int> upgradeChoices;
            if (!myself->canMoveUpgrade) upgradeChoices.push_back(1);
            if (!myself->canShootUpgrade) upgradeChoices.push_back(2);
            if (!myself->canSeeUpgrade) upgradeChoices.push_back(3);

if (!upgradeChoices.empty()) {
    int pick = upgradeChoices[rand() % upgradeChoices.size()];
    switch (pick) {
        case 1: {  // movement upgrade
            myself->canMoveUpgrade = true;
            if (rand() % 2 == 0) {
                myself->moveUpgradeType = "JumpBot";
                myself->jumpsLeft = 3;
                cout << myself->robName << " upgraded to JumpBot!" << endl;
            } else {
                myself->moveUpgradeType = "HideBot";
                myself->hidesLeft = 3;
                cout << myself->robName << " upgraded to HideBot!" << endl;
            }
            break;
        }
        case 2: {  // shooting upgrade
            myself->canShootUpgrade = true;
            int shootChoice = rand() % 6;  // ✅ Now safe inside its own block
            if (shootChoice == 0) {
                myself->shootUpgradeType = "LongShotBot";
                cout << myself->robName << " upgraded to LongShotBot!" << endl;
            } else if (shootChoice == 1) {
                myself->shootUpgradeType = "SemiAutoBot";
                cout << myself->robName << " upgraded to SemiAutoBot!" << endl;
            } else if (shootChoice == 2) {
                myself->shootUpgradeType = "ThirtyShotBot";
                myself->ammo = 30;
                cout << myself->robName << " upgraded to ThirtyShotBot!" << endl;
            } else if (shootChoice == 3) {
                myself->shootUpgradeType = "PlusShooter";
                cout << myself->robName << " upgraded to PlusShooter!" << endl;
            } else if (shootChoice == 4) {
                myself->shootUpgradeType = "CrossShooter";
                cout << myself->robName << " upgraded to CrossShooter!" << endl;
            } else {
                myself->shootUpgradeType = "DoubleRowShooter";
                cout << myself->robName << " upgraded to DoubleRowShooter!" << endl;
            }
            break;
        }
        case 3: {  // seeing upgrade
            myself->canSeeUpgrade = true;
            if (rand() % 2 == 0) {
                myself->seeUpgradeType = "ScoutBot";
                myself->scanChances = 3;
                cout << myself->robName << " upgraded to ScoutBot!" << endl;
            } else {
                myself->seeUpgradeType = "TrackBot";
                cout << myself->robName << " upgraded to TrackBot!" << endl;
            }
            break;
        }
    }
    myself->numUpgrades++;
}

        }
    }
};
// class for robots that can move
class MovingRobot {
public:
    // move robot on grid
    virtual void doMoving(Robot* myself, const vector<Robot*>& allRobots, int width, int height) {
        if (!myself->alive()) return; // skip dead robots

        // stop hiding if hidebot turn ends
        if (myself->isHidden && !(myself->canMoveUpgrade && myself->moveUpgradeType == "HideBot" && myself->hidesLeft > 0)) {
            myself->isHidden = false;
        }

        // JUMPBOT ability: jump near seen enemy
        if (myself->canMoveUpgrade && myself->moveUpgradeType == "JumpBot" &&
            myself->jumpsLeft > 0 && myself->sawEnemy) {
            Robot* nearestEnemy = nullptr;
            int shortestDist = INT_MAX;

            for (Robot* enemy : myself->seenEnemies) {
                if (!enemy->alive()) continue;
                int distance = abs(enemy->posX - myself->posX) + abs(enemy->posY - myself->posY);
                if (distance < shortestDist) {
                    shortestDist = distance;
                    nearestEnemy = enemy;
                }
            }

            if (nearestEnemy) {
                vector<pair<int, int>> jumpOptions;
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        if (dx == 0 && dy == 0) continue;
                        int newX = nearestEnemy->posX + dx;
                        int newY = nearestEnemy->posY + dy;
                        if (newX < 0 || newY < 0 || newX >= width || newY >= height) continue;

                        bool spotTaken = false;
                        for (auto r : allRobots) {
                            if (r->alive() && r->posX == newX && r->posY == newY) {
                                spotTaken = true;
                                break;
                            }
                        }

                        if (!spotTaken) {
                            jumpOptions.push_back({newX, newY});
                        }
                    }
                }

                if (!jumpOptions.empty()) {
                    auto chosen = jumpOptions[rand() % jumpOptions.size()];
                    myself->posX = chosen.first;
                    myself->posY = chosen.second;
                    myself->jumpsLeft--;
                    cout << myself->robName << " jumps to (" << chosen.first << "," << chosen.second
                         << ") near " << nearestEnemy->robName << endl;
                    return;
                }
            }
        }

        // move toward enemy if any seen
        Robot* targetEnemy = nullptr;
        int closest = INT_MAX;

        // prefer tracked enemies if trackbot
        if (myself->canSeeUpgrade && myself->seeUpgradeType == "TrackBot") {
            for (Robot* r : myself->followedEnemies) {
                if (!r->alive() || r->isHidden) continue;
                int dist = abs(r->posX - myself->posX) + abs(r->posY - myself->posY);
                if (dist < closest) {
                    closest = dist;
                    targetEnemy = r;
                }
            }
        }

        // if no tracked enemy, use seen list
        if (!targetEnemy && myself->sawEnemy) {
            for (Robot* r : myself->seenEnemies) {
                if (!r->alive()) continue;
                int dist = abs(r->posX - myself->posX) + abs(r->posY - myself->posY);
                if (dist < closest) {
                    closest = dist;
                    targetEnemy = r;
                }
            }
        }

        // move closer to target enemy
        if (targetEnemy) {
            int dx = (targetEnemy->posX > myself->posX) ? 1 : (targetEnemy->posX < myself->posX) ? -1 : 0;
            int dy = (targetEnemy->posY > myself->posY) ? 1 : (targetEnemy->posY < myself->posY) ? -1 : 0;
            int newX = myself->posX + dx;
            int newY = myself->posY + dy;

            // check if spot is open
            if (newX >= 0 && newX < width && newY >= 0 && newY < height) {
                bool blocked = false;
                for (auto r : allRobots) {
                    if (r->alive() && r->posX == newX && r->posY == newY) {
                        blocked = true;
                        break;
                    }
                }

                if (!blocked) {
                    myself->posX = newX;
                    myself->posY = newY;
                    cout << myself->robName << " moves toward " << targetEnemy->robName
                         << " to (" << newX << "," << newY << ")" << endl;
                    return;
                }
            }
        }

        // random move if no enemy or path blocked
        vector<pair<int, int>> moveSpots;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue;
                int newX = myself->posX + dx;
                int newY = myself->posY + dy;

                if (newX < 0 || newY < 0 || newX >= width || newY >= height) continue;

                bool spotTaken = false;
                for (auto r : allRobots) {
                    if (r->alive() && r->posX == newX && r->posY == newY) {
                        spotTaken = true;
                        break;
                    }
                }

                if (!spotTaken) {
                    moveSpots.push_back({newX, newY});
                }
            }
        }

        if (!moveSpots.empty()) {
            auto choice = moveSpots[rand() % moveSpots.size()];
            myself->posX = choice.first;
            myself->posY = choice.second;
            cout << myself->robName << " moves randomly to (" << choice.first << "," << choice.second << ")" << endl;
        }
    }
};
// base robot with all 4 abilities
class GenericRobot : public Robot, public ThinkingRobot, public SeeingRobot, public ShootingRobot, public MovingRobot {
public:
    GenericRobot(string name, char sym, int x, int y, int hp, int ammo, int lives)
        : Robot(name, sym, x, y, hp, ammo, lives) {}

    void think() override { doThinking(this); }
    void look(const vector<Robot*>& robots) override { doLooking(this, robots); }
    void fire(vector<Robot*>& robots) override { doShooting(this, robots); }
    void move(const vector<Robot*>& robots, int w, int h) override { doMoving(this, robots, w, h); }
};

// hidebot: starts with hide ability
class HideBot : public GenericRobot {
public:
    HideBot(string name, char sym, int x, int y, int hp, int ammo, int lives)
        : GenericRobot(name, sym, x, y, hp, ammo, lives) {
        canMoveUpgrade = true;
        moveUpgradeType = "HideBot";
        hidesLeft = 3;
        numUpgrades = 1;
    }

    void think() override {
        if (hidesLeft > 0) {
            cout << robName << " (HideBot) hides this turn (" << hidesLeft << " left)" << endl;
            isHidden = true;
            hidesLeft--;
        } else {
            GenericRobot::think();
        }
    }
};

// jumpbot: starts with jump ability
class JumpBot : public GenericRobot {
public:
    JumpBot(string name, char sym, int x, int y, int hp, int ammo, int lives)
        : GenericRobot(name, sym, x, y, hp, ammo, lives) {
        canMoveUpgrade = true;
        moveUpgradeType = "JumpBot";
        jumpsLeft = 3;
        numUpgrades = 1;
    }
};

// longshotbot: can fire long range
class LongShotBot : public GenericRobot {
public:
    LongShotBot(string name, char sym, int x, int y, int hp, int ammo, int lives)
        : GenericRobot(name, sym, x, y, hp, ammo, lives) {
        canShootUpgrade = true;
        shootUpgradeType = "LongShotBot";
        numUpgrades = 1;
    }
};

// semiautobot: 3-shot per shell
class SemiAutoBot : public GenericRobot {
public:
    SemiAutoBot(string name, char sym, int x, int y, int hp, int ammo, int lives)
        : GenericRobot(name, sym, x, y, hp, ammo, lives) {
        canShootUpgrade = true;
        shootUpgradeType = "SemiAutoBot";
        numUpgrades = 1;
    }
};

// 30 shot bot: comes with full ammo
class ThirtyShotBot : public GenericRobot {
public:
    ThirtyShotBot(string name, char sym, int x, int y, int hp, int ammo, int lives)
        : GenericRobot(name, sym, x, y, hp, 30, lives) {
        canShootUpgrade = true;
        shootUpgradeType = "ThirtyShotBot";
        numUpgrades = 1;
    }
};

// scoutbot: see everything 3 times
class ScoutBot : public GenericRobot {
public:
    ScoutBot(string name, char sym, int x, int y, int hp, int ammo, int lives)
        : GenericRobot(name, sym, x, y, hp, ammo, lives) {
        canSeeUpgrade = true;
        seeUpgradeType = "ScoutBot";
        scanChances = 3;
        numUpgrades = 1;
    }
};

// trackbot: remembers who it sees
class TrackBot : public GenericRobot {
public:
    TrackBot(string name, char sym, int x, int y, int hp, int ammo, int lives)
        : GenericRobot(name, sym, x, y, hp, ammo, lives) {
        canSeeUpgrade = true;
        seeUpgradeType = "TrackBot";
        numUpgrades = 1;
    }
};

// other shooting bots
class PlusShooter : public GenericRobot {
public:
    PlusShooter(string name, char sym, int x, int y, int hp, int ammo, int lives)
        : GenericRobot(name, sym, x, y, hp, ammo, lives) {
        canShootUpgrade = true;
        shootUpgradeType = "PlusShooter";
        numUpgrades = 1;
    }
};

class CrossShooter : public GenericRobot {
public:
    CrossShooter(string name, char sym, int x, int y, int hp, int ammo, int lives)
        : GenericRobot(name, sym, x, y, hp, ammo, lives) {
        canShootUpgrade = true;
        shootUpgradeType = "CrossShooter";
        numUpgrades = 1;
    }
};

class DoubleRowShooter : public GenericRobot {
public:
    DoubleRowShooter(string name, char sym, int x, int y, int hp, int ammo, int lives)
        : GenericRobot(name, sym, x, y, hp, ammo, lives) {
        canShootUpgrade = true;
        shootUpgradeType = "DoubleRowShooter";
        numUpgrades = 1;
    }
};

int main() {
    ofstream logFile("log.txt");            // make a log file
    streambuf* normalOut = cout.rdbuf();    // save normal console output
    streambuf* logStream = logFile.rdbuf(); // get log file output stream

    // class to print to both console and log
    class DualOutput : public streambuf {
        streambuf *toConsole, *toLog;
    public:
        DualOutput(streambuf* a, streambuf* b) : toConsole(a), toLog(b) {}
        int overflow(int ch) override {
            if (ch == EOF) return !EOF;
            if (toConsole) toConsole->sputc(ch);
            if (toLog) toLog->sputc(ch);
            return ch;
        }
    } bothOutput(normalOut, logStream);

    cout.rdbuf(&bothOutput); // from now, print to both

    ifstream setupFile("setup.txt");       // open setup file
    if (!setupFile) {
        cerr << "Can't open setup.txt file!" << endl;
        return 1;
    }

    int gridWidth = 10, gridHeight = 10, turnLimit = 100, totalRobots = 0;
    string line;

    // read battlefield settings
    while (getline(setupFile, line)) {
        if (line.find("M by N") != string::npos) {
            sscanf(line.c_str(), "M by N : %d %d", &gridWidth, &gridHeight);
        } else if (line.find("steps") != string::npos) {
            sscanf(line.c_str(), "steps: %d", &turnLimit);
        } else if (line.find("robots") != string::npos) {
            sscanf(line.c_str(), "robots: %d", &totalRobots);
            break;
        }
    }

    vector<Robot*> robotArmy, waitingRespawn;
    char robotSymbol = 'A'; // starting symbol

    // create all robots from setup file
    for (int i = 0; i < totalRobots; i++) {
        string botType, botName;
        string posXStr, posYStr;
        setupFile >> botType >> botName >> posXStr >> posYStr;

        int x = (posXStr == "random") ? rand() % gridWidth : stoi(posXStr);
        int y = (posYStr == "random") ? rand() % gridHeight : stoi(posYStr);
        int lives = 2; // as per fix in code

        // make correct bot type
        if (botType == "GenericRobot") robotArmy.push_back(new GenericRobot(botName, robotSymbol++, x, y, 1, 10, lives));
        else if (botType == "HideBot") robotArmy.push_back(new HideBot(botName, robotSymbol++, x, y, 1, 10, lives));
        else if (botType == "JumpBot") robotArmy.push_back(new JumpBot(botName, robotSymbol++, x, y, 1, 10, lives));
        else if (botType == "LongShotBot") robotArmy.push_back(new LongShotBot(botName, robotSymbol++, x, y, 1, 10, lives));
        else if (botType == "SemiAutoBot") robotArmy.push_back(new SemiAutoBot(botName, robotSymbol++, x, y, 1, 10, lives));
        else if (botType == "ThirtyShotBot") robotArmy.push_back(new ThirtyShotBot(botName, robotSymbol++, x, y, 1, 10, lives));
        else if (botType == "ScoutBot") robotArmy.push_back(new ScoutBot(botName, robotSymbol++, x, y, 1, 10, lives));
        else if (botType == "TrackBot") robotArmy.push_back(new TrackBot(botName, robotSymbol++, x, y, 1, 10, lives));
        else if (botType == "PlusShooter") robotArmy.push_back(new PlusShooter(botName, robotSymbol++, x, y, 1, 10, lives));
        else if (botType == "CrossShooter") robotArmy.push_back(new CrossShooter(botName, robotSymbol++, x, y, 1, 10, lives));
        else if (botType == "DoubleRowShooter") robotArmy.push_back(new DoubleRowShooter(botName, robotSymbol++, x, y, 1, 10, lives));
    }

    setupFile.close(); // done reading

    srand((unsigned)time(0)); // set random seed

    int roundNum = 1;

    while (roundNum <= turnLimit) {
        int stillAlive = count_if(robotArmy.begin(), robotArmy.end(),
                                  [](Robot* r) { return r->alive(); });

        if (stillAlive <= 1 && waitingRespawn.empty()) break;

        cout << "----- Turn " << roundNum << " -----" << endl;

        // draw the field
        for (int y = 0; y < gridHeight; ++y) {
            for (int x = 0; x < gridWidth; ++x) {
                char cellChar = '.';
                for (auto* r : robotArmy) {
                    if (r->alive() && !r->isHidden && r->posX == x && r->posY == y) {
                        cellChar = r->robSymbol;
                        break;
                    }
                }
                cout << cellChar << " ";
            }
            cout << endl;
        }

        // bring back robots from respawn queue
        if (!waitingRespawn.empty()) {
            Robot* r = waitingRespawn.front();
            waitingRespawn.erase(waitingRespawn.begin());

            int newX, newY;
            while (true) {
                newX = rand() % gridWidth;
                newY = rand() % gridHeight;
                bool taken = false;
                for (auto* other : robotArmy) {
                    if (other->alive() && other->posX == newX && other->posY == newY) {
                        taken = true;
                        break;
                    }
                }
                if (!taken) break;
            }
            r->bringBack(newX, newY); // place robot again
        }

        // run robot actions
        for (Robot* r : robotArmy) {
            if (!r->alive()) continue;
            r->think();              // think what to do
            r->look(robotArmy);     // look around
            if (r->sawEnemy) {
                r->fire(robotArmy); // fire if enemy seen
            }
            r->move(robotArmy, gridWidth, gridHeight); // move somewhere
        }

        // check who needs to go to respawn
        for (Robot* r : robotArmy) {
            if (!r->alive() && r->lifeLeft > 0 &&
                find(waitingRespawn.begin(), waitingRespawn.end(), r) == waitingRespawn.end()) {
                r->lifeLeft--;
                waitingRespawn.push_back(r);
            }
        }

        // show robot status
        cout << "--- Status after Turn " << roundNum << " ---" << endl;
        for (Robot* r : robotArmy) {
            cout << r->robName << " at (" << r->posX << "," << r->posY
                 << ") shells=" << r->ammo << " lives=" << r->lifeLeft
                 << " | Upgrades: ";

            if (r->canMoveUpgrade) {
                cout << r->moveUpgradeType;
                if (r->moveUpgradeType == "JumpBot") cout << "(" << r->jumpsLeft << ") ";
                else if (r->moveUpgradeType == "HideBot") cout << "(" << r->hidesLeft << ") ";
                else cout << " ";
            }

            if (r->canShootUpgrade) cout << r->shootUpgradeType << " ";
            if (r->canSeeUpgrade) {
                cout << r->seeUpgradeType;
                if (r->seeUpgradeType == "ScoutBot") cout << "(" << r->scanChances << ") ";
                else cout << " ";
            }

            if (!r->alive()) cout << " [DEAD]";
            cout << endl;
        }

        cout << endl;
        roundNum++;
    }

    // clean up memory
    for (Robot* r : robotArmy) delete r;
    for (Robot* r : waitingRespawn) delete r;

    cout.rdbuf(normalOut); // restore regular output
    logFile.close();       // close log

    return 0;
}
