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
    string botName;
    char botChar;
    int posX, posY;
    int hp;
    int ammo;
    int livesLeft;
    bool isActive;
    int startHp, startAmmo;
    bool sawSomething = false;
    vector<Robot*> thingsSeen;
    int scansLeft = 3;
    vector<Robot*> botsTracked;
    bool didTracking = false;

    bool hasMoveBoost = false, hasShootBoost = false, hasSightBoost = false;
    int boostCount = 0;
    string moveBoostType, shootBoostType, sightBoostType;
    int hideCount = 0;
    bool isHiding = false;
    int jumpCount = 0;

    Robot(string name, char c, int startX, int startY, int health, int bullets, int lives)
        : botName(name), botChar(c), posX(startX), posY(startY), hp(health), ammo(bullets), livesLeft(lives), isActive(true),
          startHp(health), startAmmo(bullets) {}

    virtual void thinkStuff() = 0;
    virtual void lookAround(const vector<Robot*>& bots) = 0;
    virtual void shoot(vector<Robot*>& bots) = 0;
    virtual void moveAround(const vector<Robot*>& bots, int gridW, int gridH) = 0;

    void getHit() {
        if (!isActive) return;
        if (isHiding) {
            cout << botName << " is hiding, so it takes no damage." << endl;
            return;
        }
        hp--;
        cout << botName << " is hit! (HP=" << hp << ")\n";
        if (hp <= 0) {
            isActive = false;
            cout << botName << " is destroyed!\n";
        }
    }

    void blowUp() {
        if (!isActive) return;
        isActive = false;
        cout << botName << " self-destructs!\n";
    }

    void comeBack(int newX, int newY) {
        posX = newX; posY = newY;
        hp = startHp;
        isActive = true;
        sawSomething = false;
        isHiding = false;
        thingsSeen.clear();
        cout << botName << " comes back at (" << posX << "," << posY << ") with " << hp << " HP and " << ammo << " bullets" << endl;
    }

    int getShootRange() const {
        if (hasShootBoost && shootBoostType == "LongShotBot") {
            return 3;
        }
        return 1;
    }

    string getName() const { return botName; }
    int getX() const { return posX; }
    int getY() const { return posY; }
    bool isWorking() const { return isActive; }
};

class ThinkingRobot {
public:
    virtual void doThink(Robot* me) {
        if (me->hasMoveBoost && me->moveBoostType == "HideBot" && me->hideCount > 0) {
            cout << me->botName << " is hiding and safe this turn." << endl;
            me->isHiding = true;
            me->hideCount--;
        } else {
            cout << me->botName << " is thinking..." << endl;
        }
    }
};

class SeeingRobot {
public:
    virtual void doLook(Robot* me, const vector<Robot*>& bots) {
        me->thingsSeen.clear();

        if (me->hasSightBoost && me->sightBoostType == "TrackBot") {
            if (!me->didTracking) {
                me->botsTracked.clear();
                vector<Robot*> possible;
                for (Robot* r : bots) {
                    if (r != me && r->isActive && !r->isHiding) {
                        possible.push_back(r);
                    }
                }
                random_shuffle(possible.begin(), possible.end());
                int trackNum = min(3, (int)possible.size());
                for (int i = 0; i < trackNum; i++) {
                    me->botsTracked.push_back(possible[i]);
                }
                me->didTracking = true;
                cout << me->botName << " tracked " << trackNum << " bots." << endl;
            }

            for (Robot* t : me->botsTracked) {
                if (t->isActive && !t->isHiding) {
                    me->thingsSeen.push_back(t);
                }
            }
        }

        if (me->hasSightBoost && me->sightBoostType == "ScoutBot" &&
            me->scansLeft > 0) {
            cout << me->botName << " uses ScoutBot scan ("
                 << me->scansLeft << " left):\n";
            for (auto r : bots) {
                if (r != me && r->isActive && !r->isHiding) {
                    if (find(me->thingsSeen.begin(), me->thingsSeen.end(), r) == me->thingsSeen.end()) {
                        me->thingsSeen.push_back(r);
                    }
                }
            }
            me->scansLeft--;
        }

        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue;
                int nx = me->posX + dx, ny = me->posY + dy;
                for (auto r : bots) {
                    if (r != me && r->isActive && !r->isHiding &&
                        r->posX == nx && r->posY == ny) {
                        if (find(me->thingsSeen.begin(), me->thingsSeen.end(), r) == me->thingsSeen.end()) {
                            me->thingsSeen.push_back(r);
                        }
                    }
                }
            }
        }

        me->sawSomething = !me->thingsSeen.empty();
        if (me->sawSomething) {
            for (auto r : me->thingsSeen) {
                cout << "  " << me->botName << " sees " << r->botName << " at (" << r->posX << "," << r->posY << ")\n";
            }
        } else {
            cout << "  " << me->botName << " sees nothing.\n";
        }
    }
};

class ShootingRobot {
public:
    virtual void doShoot(Robot* me, vector<Robot*>& bots) {
        if (!me->isActive || me->ammo <= 0) return;

        if (me->shootBoostType == "SemiAutoBot" && me->sawSomething) {
            Robot* target = me->thingsSeen[rand() % me->thingsSeen.size()];
            cout << me->botName << " (SemiAutoBot) shoots 3 times at "
                 << target->botName << "! ";
            me->ammo--;
            int hits = 0;
            for (int i = 0; i < 3; i++) {
                if (rand() % 100 < 70) hits++;
            }
            if (hits > 0) {
                cout << "HIT " << hits << " times!\n";
                for (int i = 0; i < hits; i++) {
                    target->getHit();
                }
            } else {
                cout << "All miss.\n";
            }
            if (me->ammo <= 0) me->blowUp();
            return;
        }

        if (me->shootBoostType == "LongShotBot" && me->sawSomething) {
            vector<Robot*> possible;
            for (Robot* r : me->thingsSeen) {
                int dx = abs(r->posX - me->posX);
                int dy = abs(r->posY - me->posY);
                int dist = dx + dy;
                if (dist > 0 && dist <= 3) {
                    possible.push_back(r);
                }
            }
            if (!possible.empty()) {
                Robot* target = possible[rand() % possible.size()];
                cout << me->botName << " (LongShotBot) shoots at "
                     << target->botName << " (dist=" << abs(target->posX - me->posX) + abs(target->posY - me->posY) << ")... ";
                me->ammo--;
                if (rand() % 100 < 70) {
                    cout << "HIT!\n";
                    target->getHit();
                } else {
                    cout << "misses.\n";
                }
                if (me->ammo <= 0) me->blowUp();
                return;
            } else {
                cout << me->botName << " (LongShotBot) sees no target close enough." << endl;
            }
        }

        if (me->shootBoostType == "PlusShooter") {
            cout << me->botName << " shoots in + pattern!" << endl;
            bool shotFired = false;
            for (Robot* r : me->thingsSeen) {
                if ((r->posX == me->posX || r->posY == me->posY) && !(r->posX == me->posX && r->posY == me->posY)) {
                    cout << "  Aiming at " << r->botName << " at (" << r->posX << "," << r->posY << ")... ";
                    if (rand() % 100 < 70) {
                        cout << "HIT!" << endl;
                        r->getHit();
                    } else {
                        cout << "missed." << endl;
                    }
                    me->ammo--;
                    shotFired = true;
                    if (me->ammo <= 0) {
                        me->blowUp();
                        return;
                    }
                }
            }
            if (!shotFired) {
                cout << "  No targets in + pattern, regular shooting." << endl;
            } else {
                return;
            }
        }

        if (me->shootBoostType == "CrossShooter") {
            cout << me->botName << " shoots in X pattern!" << endl;
            bool shotFired = false;
            for (Robot* r : me->thingsSeen) {
                if (abs(r->posX - me->posX) == abs(r->posY - me->posY) && !(r->posX == me->posX && r->posY == me->posY)) {
                    cout << "  Aiming at " << r->botName << " at (" << r->posX << "," << r->posY << ")... ";
                    if (rand() % 100 < 70) {
                        cout << "HIT!" << endl;
                        r->getHit();
                    } else {
                        cout << "missed." << endl;
                    }
                    me->ammo--;
                    shotFired = true;
                    if (me->ammo <= 0) {
                        me->blowUp();
                        return;
                    }
                }
            }
            if (!shotFired) {
                cout << "  No targets in X pattern, regular shooting." << endl;
            } else {
                return;
            }
        }

        if (me->shootBoostType == "DoubleRowShooter") {
            cout << me->botName << " shoots across two rows!" << endl;
            bool shotFired = false;
            for (Robot* r : me->thingsSeen) {
                if (r->posY == me->posY || r->posY == me->posY + 1 || r->posY == me->posY - 1) {
                    cout << "  Aiming at " << r->botName << " at (" << r->posX << "," << r->posY << ")... ";
                    if (rand() % 100 < 70) {
                        cout << "HIT!" << endl;
                        r->getHit();
                    } else {
                        cout << "missed." << endl;
                    }
                    me->ammo--;
                    shotFired = true;
                    if (me->ammo <= 0) {
                        me->blowUp();
                        return;
                    }
                }
            }
            if (!shotFired) {
                cout << "  No targets in rows, regular shooting." << endl;
            } else {
                return;
            }
        }

        vector<Robot*> closeTargets;
        for (Robot* t : me->thingsSeen) {
            int dx = abs(t->posX - me->posX);
            int dy = abs(t->posY - me->posY);
            if (dx <= 1 && dy <= 1 && (dx != 0 || dy != 0)) {
                closeTargets.push_back(t);
            }
        }

        if (closeTargets.empty()) {
            cout << me->botName << " sees no close target to shoot." << endl;
            return;
        }

        Robot* target = closeTargets[rand() % closeTargets.size()];
        cout << me->botName << " shoots at " << target->botName << "... ";
        me->ammo--;
        if (rand() % 100 < 70) {
            cout << "HIT!" << endl;
            target->getHit();
        } else {
            cout << "misses." << endl;
        }
        if (me->ammo <= 0) me->blowUp();

        if (target && !target->isActive && me->boostCount < 3) {
            cout << me->botName << " gets a boost!" << endl;
            vector<int> options;
            if (!me->hasMoveBoost) options.push_back(1);
            if (!me->hasShootBoost) options.push_back(2);
            if (!me->hasSightBoost) options.push_back(3);

            if (!options.empty()) {
                int choice = options[rand() % options.size()];
                switch (choice) {
                    case 1: {
                        me->hasMoveBoost = true;
                        if (rand() % 2 == 0) {
                            me->moveBoostType = "JumpBot";
                            me->jumpCount = 3;
                            cout << me->botName << " upgraded to JumpBot." << endl;
                        } else {
                            me->moveBoostType = "HideBot";
                            me->hideCount = 3;
                            cout << me->botName << " upgraded to HideBot." << endl;
                        }
                        break;
                    }
                    case 2: {
                        me->hasShootBoost = true;
                        int pick = rand() % 6;
                        if (pick == 0) {
                            me->shootBoostType = "LongShotBot";
                            cout << me->botName << " upgraded to LongShotBot." << endl;
                        } else if (pick == 1) {
                            me->shootBoostType = "SemiAutoBot";
                            cout << me->botName << " upgraded to SemiAutoBot." << endl;
                        } else if (pick == 2) {
                            me->shootBoostType = "ThirtyShotBot";
                            me->ammo = 30;
                            cout << me->botName << " upgraded to ThirtyShotBot." << endl;
                        } else if (pick == 3) {
                            me->shootBoostType = "PlusShooter";
                            cout << me->botName << " upgraded to PlusShooter." << endl;
                        } else if (pick == 4) {
                            me->shootBoostType = "CrossShooter";
                            cout << me->botName << " upgraded to CrossShooter." << endl;
                        } else {
                            me->shootBoostType = "DoubleRowShooter";
                            cout << me->botName << " upgraded to DoubleRowShooter." << endl;
                        }
                        break;
                    }
                    case 3: {
                        me->hasSightBoost = true;
                        if (rand() % 2 == 0) {
                            me->sightBoostType = "ScoutBot";
                            me->scansLeft = 3;
                            cout << me->botName << " upgraded to ScoutBot." << endl;
                        } else {
                            me->sightBoostType = "TrackBot";
                            cout << me->botName << " upgraded to TrackBot." << endl;
                        }
                        break;
                    }
                }
                me->boostCount++;
            }
        }
    }
};

class MovingRobot {
public:
    virtual void doMove(Robot* me, const vector<Robot*>& bots, int gridW, int gridH) {
        if (!me->isActive) return;

        if (me->isHiding && !(me->hasMoveBoost && me->moveBoostType == "HideBot" && me->hideCount > 0)) {
            me->isHiding = false;
        }

        if (me->hasMoveBoost && me->moveBoostType == "JumpBot" &&
            me->jumpCount > 0 && me->sawSomething) {
            Robot* closest = nullptr;
            int minDist = INT_MAX;
            for (Robot* t : me->thingsSeen) {
                if (!t->isActive) continue;
                int dist = abs(t->posX - me->posX) + abs(t->posY - me->posY);
                if (dist < minDist) {
                    minDist = dist;
                    closest = t;
                }
            }

            if (closest) {
                vector<pair<int, int>> spots;
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = closest->posX + dx, ny = closest->posY + dy;
                        if (nx < 0 || ny < 0 || nx >= gridW || ny >= gridH) continue;
                        bool taken = false;
                        for (auto r : bots) {
                            if (r->isActive && r->posX == nx && r->posY == ny) {
                                taken = true;
                                break;
                            }
                        }
                        if (!taken) spots.push_back({nx, ny});
                    }
                }

                if (!spots.empty()) {
                    auto [x, y] = spots[rand() % spots.size()];
                    me->posX = x;
                    me->posY = y;
                    me->jumpCount--;
                    cout << me->botName << " jumps to (" << x << "," << y << ") near " << closest->botName << "\n";
                    return;
                }
            }
        }

        Robot* target = nullptr;
        int minDist = INT_MAX;

        if (me->hasSightBoost && me->sightBoostType == "TrackBot") {
            for (Robot* t : me->botsTracked) {
                if (!t->isActive || t->isHiding) continue;
                int dist = abs(t->posX - me->posX) + abs(t->posY - me->posY);
                if (dist < minDist) {
                    minDist = dist;
                    target = t;
                }
            }
        }

        if (!target && me->sawSomething) {
            for (Robot* t : me->thingsSeen) {
                if (!t->isActive) continue;
                int dist = abs(t->posX - me->posX) + abs(t->posY - me->posY);
                if (dist < minDist) {
                    minDist = dist;
                    target = t;
                }
            }
        }

        if (target) {
            int dx = (target->posX > me->posX) ? 1 :
                     (target->posX < me->posX) ? -1 : 0;
            int dy = (target->posY > me->posY) ? 1 :
                     (target->posY < me->posY) ? -1 : 0;
            int nx = me->posX + dx, ny = me->posY + dy;

            if (nx >= 0 && nx < gridW && ny >= 0 && ny < gridH) {
                bool blocked = false;
                for (auto r : bots) {
                    if (r->isActive && r->posX == nx && r->posY == ny) {
                        blocked = true;
                        break;
                    }
                }
                if (!blocked) {
                    me->posX = nx;
                    me->posY = ny;
                    cout << me->botName << " moves toward " << target->botName
                         << " to (" << nx << "," << ny << ")\n";
                    return;
                }
            }
        }

        vector<pair<int, int>> options;
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue;
                int nx = me->posX + dx, ny = me->posY + dy;
                if (nx < 0 || ny < 0 || nx >= gridW || ny >= gridH) continue;
                bool taken = false;
                for (auto r : bots) {
                    if (r->isActive && r->posX == nx && r->posY == ny) {
                        taken = true;
                        break;
                    }
                }
                if (!taken) options.push_back({nx, ny});
            }
        }
        if (!options.empty()) {
            auto [nx, ny] = options[rand() % options.size()];
            me->posX = nx;
            me->posY = ny;
            cout << me->botName << " moves to (" << nx << "," << ny << ")\n";
        }
    }
};

class GenericRobot : public Robot, public ThinkingRobot, public SeeingRobot, public ShootingRobot, public MovingRobot {
public:
    GenericRobot(string n, char s, int x, int y, int hp, int ammo, int l)
        : Robot(n, s, x, y, hp, ammo, l) {}

    void thinkStuff() override { doThink(this); }
    void lookAround(const vector<Robot*>& bots) override { doLook(this, bots); }
    void shoot(vector<Robot*>& bots) override { doShoot(this, bots); }
    void moveAround(const vector<Robot*>& bots, int w, int h) override {
        doMove(this, bots, w, h);
    }
};

class HideBot : public GenericRobot {
public:
    HideBot(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        hasMoveBoost = true;
        moveBoostType = "HideBot";
        hideCount = 3;
        boostCount = 1;
    }

    void thinkStuff() override {
        if (hideCount > 0) {
            cout << botName << " (HideBot) is hidden this turn ("
                 << hideCount << " hides left)\n";
            isHiding = true;
            hideCount--;
        } else {
            GenericRobot::thinkStuff();
        }
    }
};

class JumpBot : public GenericRobot {
public:
    JumpBot(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        hasMoveBoost = true;
        moveBoostType = "JumpBot";
        jumpCount = 3;
        boostCount = 1;
    }
};

class LongShotBot : public GenericRobot {
public:
    LongShotBot(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        hasShootBoost = true;
        shootBoostType = "LongShotBot";
        boostCount = 1;
    }
};

class SemiAutoBot : public GenericRobot {
public:
    SemiAutoBot(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        hasShootBoost = true;
        shootBoostType = "SemiAutoBot";
        boostCount = 1;
    }
};

class ThirtyShotBot : public GenericRobot {
public:
    ThirtyShotBot(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, 30, l) {
        hasShootBoost = true;
        shootBoostType = "ThirtyShotBot";
        boostCount = 1;
    }
};

class ScoutBot : public GenericRobot {
public:
    ScoutBot(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        hasSightBoost = true;
        sightBoostType = "ScoutBot";
        scansLeft = 3;
        boostCount = 1;
    }
};

class TrackBot : public GenericRobot {
public:
    TrackBot(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        hasSightBoost = true;
        sightBoostType = "TrackBot";
        boostCount = 1;
    }
};

class PlusShooter : public GenericRobot {
public:
    PlusShooter(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        hasShootBoost = true;
        shootBoostType = "PlusShooter";
        boostCount = 1;
    }
};

class CrossShooter : public GenericRobot {
public:
    CrossShooter(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        hasShootBoost = true;
        shootBoostType = "CrossShooter";
        boostCount = 1;
    }
};

class DoubleRowShooter : public GenericRobot {
public:
    DoubleRowShooter(string n, char s, int x, int y, int hp, int ammo, int l)
        : GenericRobot(n, s, x, y, hp, ammo, l) {
        hasShootBoost = true;
        shootBoostType = "DoubleRowShooter";
        boostCount = 1;
    }
};

int main() {
    ofstream logFile("log.txt");
    streambuf* originalOut = cout.rdbuf();
    streambuf* logBuffer = logFile.rdbuf();
    class DoubleBuffer : public streambuf {
        streambuf *buf1, *buf2;
    public:
        DoubleBuffer(streambuf* b1, streambuf* b2) : buf1(b1), buf2(b2) {}
        int overflow(int c) override {
            if (c == EOF) return !EOF;
            if (buf1) buf1->sputc(c);
            if (buf2) buf2->sputc(c);
            return c;
        }
    } doubleBuffer(originalOut, logBuffer);
    cout.rdbuf(&doubleBuffer);
    ifstream setupFile("setup.txt");
    if (!setupFile) {
        cerr << "Can't open setup.txt" << endl;
        return 1;
    }

    int gridW = 10, gridH = 10, totalTurns = 100, botCount = 0;
    string line;
    while (getline(setupFile, line)) {
        if (line.find("M by N") != string::npos) {
            sscanf(line.c_str(), "M by N : %d %d", &gridW, &gridH);
        } else if (line.find("steps") != string::npos) {
            sscanf(line.c_str(), "steps: %d", &totalTurns);
        } else if (line.find("robots") != string::npos) {
            sscanf(line.c_str(), "robots: %d", &botCount);
            break;
        }
    }

    vector<Robot*> botList, respawnList;
    char nextChar = 'A';
    for (int i = 0; i < botCount; ++i) {
        string type, name;
        string xs, ys;
        setupFile >> type >> name >> xs >> ys;
        int x = (xs == "random") ? rand() % gridW : stoi(xs);
        int y = (ys == "random") ? rand() % gridH : stoi(ys);

        int lives = 2;

        if (type == "GenericRobot") {
            botList.push_back(new GenericRobot(name, nextChar++, x, y, 1, 10, lives));
        }
        else if (type == "HideBot") {
            botList.push_back(new HideBot(name, nextChar++, x, y, 1, 10, lives));
        }
        else if (type == "JumpBot") {
            botList.push_back(new JumpBot(name, nextChar++, x, y, 1, 10, lives));
        }
        else if (type == "LongShotBot") {
            botList.push_back(new LongShotBot(name, nextChar++, x, y, 1, 10, lives));
        }
        else if (type == "SemiAutoBot") {
            botList.push_back(new SemiAutoBot(name, nextChar++, x, y, 1, 10, lives));
        }
        else if (type == "ThirtyShotBot") {
            botList.push_back(new ThirtyShotBot(name, nextChar++, x, y, 1, 10, lives));
        }
        else if (type == "ScoutBot") {
            botList.push_back(new ScoutBot(name, nextChar++, x, y, 1, 10, lives));
        }
        else if (type == "TrackBot") {
            botList.push_back(new TrackBot(name, nextChar++, x, y, 1, 10, lives));
        }
        else if (type == "PlusShooter") {
            botList.push_back(new PlusShooter(name, nextChar++, x, y, 1, 10, lives));
        }
        else if (type == "CrossShooter") {
            botList.push_back(new CrossShooter(name, nextChar++, x, y, 1, 10, lives));
        }
        else if (type == "DoubleRowShooter") {
            botList.push_back(new DoubleRowShooter(name, nextChar++, x, y, 1, 10, lives));
        }
    }
    setupFile.close();

    srand((unsigned)time(0));

    int currentTurn = 1;
    while (currentTurn <= totalTurns) {
        int activeBots = count_if(botList.begin(), botList.end(), [](Robot* r) { return r->isWorking(); });
        if (activeBots <= 1 && respawnList.empty()) break;

        cout << "----- Turn " << currentTurn << " -----" << endl;

        for (int y = 0; y < gridH; ++y) {
            for (int x = 0; x < gridW; ++x) {
                char cell = '.';
                for (auto* r : botList) {
                    if (r->isWorking() && !r->isHiding && r->getX() == x && r->getY() == y) {
                        cell = r->botChar;
                        break;
                    }
                }
                cout << cell << " ";
            }
            cout << endl;
        }

        if (!respawnList.empty()) {
            Robot* r = respawnList.front();
            respawnList.erase(respawnList.begin());
            int nx, ny;
            do {
                nx = rand() % gridW;
                ny = rand() % gridH;
                bool taken = false;
                for (auto* other : botList) {
                    if (other->isWorking() && other->getX() == nx && other->getY() == ny) {
                        taken = true; break;
                    }
                }
                if (!taken) break;
            } while (true);
            r->comeBack(nx, ny);
        }

        for (Robot* r : botList) {
            if (!r->isWorking()) continue;
            r->thinkStuff();
            r->lookAround(botList);
            if (r->sawSomething) {
                r->shoot(botList);
            }
            r->moveAround(botList, gridW, gridH);
        }

        for (Robot* r : botList) {
            if (!r->isWorking() && r->livesLeft > 0 &&
                find(respawnList.begin(), respawnList.end(), r) == respawnList.end()) {
                r->livesLeft--;
                respawnList.push_back(r);
            }
        }

        cout << "--- Status after Turn " << currentTurn << " ---" << endl;
        for (Robot* r : botList) {
            cout << r->getName() << " at (" << r->getX() << "," << r->getY() << ") ammo=" << r->ammo << " lives=" << r->livesLeft;
            cout << " | Boosts: ";
            if (r->hasMoveBoost) {
                cout << r->moveBoostType;
                if (r->moveBoostType == "JumpBot") cout << "(" << r->jumpCount << ") ";
                else if (r->moveBoostType == "HideBot") cout << "(" << r->hideCount << ") ";
                else cout << " ";
            }
            if (r->hasShootBoost) cout << r->shootBoostType << " ";
            if (r->hasSightBoost) {
                cout << r->sightBoostType;
                if (r->sightBoostType == "ScoutBot") cout << "(" << r->scansLeft << ") ";
                else cout << " ";
            }
            if (!r->isWorking()) cout << " [DEAD]";
            cout << endl;
        }
        cout << endl;
        currentTurn++;
    }

    for (Robot* r : botList) delete r;
    for (Robot* r : respawnList) delete r;
    cout.rdbuf(originalOut);
    logFile.close();
    return 0;
}