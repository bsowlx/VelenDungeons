#include "undo.h"
#include "inventory.h"
#include "leaderboard.h"
#include "pathfind.h"
#include "dungeon.h"
#include "enemy.h"
#include <cstdio>
#include <string>

static int g_total  = 0;
static int g_failed = 0;

#define EXPECT(cond) do {                                                   \
    ++g_total;                                                              \
    if (!(cond)) {                                                          \
        ++g_failed;                                                         \
        std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);\
    }                                                                       \
} while (0)

static void run_undo_tests() {
    UndoStack s;
    EXPECT(s.empty());
    EXPECT(s.size() == 0);
    EXPECT(!s.pop().has_value());

    s.push({{1.0f, 2.0f}, 0, 3});
    s.push({{4.0f, 5.0f}, 1, 2});
    EXPECT(s.size() == 2);

    auto p = s.pop();
    EXPECT(p.has_value());
    EXPECT(p->activeRoom == 1);
    EXPECT(p->hp == 2);

    p = s.pop();
    EXPECT(p.has_value());
    EXPECT(p->activeRoom == 0);

    EXPECT(s.empty());
    EXPECT(!s.pop().has_value());
}

static void run_inventory_tests() {
    Inventory inv;
    EXPECT(inv.empty());
    EXPECT(inv.size() == 0);

    inv.add({"A", 0.1f, 1, 100.0f, 1, 0.0f});
    inv.add({"B", 0.2f, 2, 200.0f, 1, 0.0f});
    inv.add({"C", 0.3f, 3, 300.0f, 1, 0.0f});
    EXPECT(inv.size() == 3);
    EXPECT(inv.current().name == "A");

    inv.cycleNext();
    EXPECT(inv.current().name == "B");
    inv.cycleNext();
    EXPECT(inv.current().name == "C");
    inv.cycleNext();
    EXPECT(inv.current().name == "A");

    inv.cyclePrev();
    EXPECT(inv.current().name == "C");

    inv.remove();
    EXPECT(inv.size() == 2);
    EXPECT(inv.current().name == "A");
}

static void run_leaderboard_tests() {
    Leaderboard lb;
    EXPECT(lb.empty());
    EXPECT(lb.top(5).empty());

    lb.submit(100);
    EXPECT(!lb.empty());
    EXPECT(lb.size() == 1);
    auto t = lb.top(5);
    EXPECT(t.size() == 1);
    EXPECT(t[0] == 100);

    lb.submit(300);
    lb.submit(200);
    lb.submit(500);
    lb.submit(400);

    t = lb.top(3);
    EXPECT(t.size() == 3);
    EXPECT(t[0] == 500);
    EXPECT(t[1] == 400);
    EXPECT(t[2] == 300);

    auto all = lb.top(100);
    EXPECT(all.size() == 5);
}

static void run_pathfind_tests() {
    // 5x5 grid. Column x=2 is a wall (rows 0..4).
    auto walkable = [](int x, int y) -> bool {
        if (x < 0 || x >= 5 || y < 0 || y >= 5) return false;
        if (x == 2) return false;
        return true;
    };

    auto same = findPath({1, 1}, {1, 1}, walkable, 5, 5);
    EXPECT(same.size() == 1);

    auto blocked = findPath({0, 0}, {4, 4}, walkable, 5, 5);
    EXPECT(blocked.empty());

    auto open = findPath({0, 0}, {1, 4}, walkable, 5, 5);
    EXPECT(!open.empty());
    EXPECT(open.front().x == 0 && open.front().y == 0);
    EXPECT(open.back().x == 1 && open.back().y == 4);
}

static void run_dungeon_tests() {
    Dungeon d;
    int a = d.addRoom("A", {2, 2, 4, 4});
    int b = d.addRoom("B", {6, 2, 8, 4});
    d.addEdge(a, b, {6, 3}, {4, 3});

    EXPECT(d.roomCount() == 2);
    EXPECT(d.roomAt(3, 3) == a);
    EXPECT(d.roomAt(7, 3) == b);
    EXPECT(d.roomAt(5, 3) == -1);
    EXPECT(d.roomAt(0, 0) == -1);
    EXPECT(d.roomIdByName("A") == a);
    EXPECT(d.roomIdByName("B") == b);
    EXPECT(d.roomIdByName("Z") == -1);
}

static void run_combat_tests() {
    int kills = 0;
    Enemy e{ {0.0f, 0.0f}, 0, 2, true, 0.0f };

    EXPECT(!applyDamage(e, 1, kills));
    EXPECT(e.alive);
    EXPECT(e.hp == 1);
    EXPECT(kills == 0);

    EXPECT(applyDamage(e, 1, kills));
    EXPECT(!e.alive);
    EXPECT(kills == 1);

    // Double-hit on an already-dead enemy must not double-count.
    EXPECT(!applyDamage(e, 5, kills));
    EXPECT(kills == 1);

    // Overkill in one hit.
    Enemy e2{ {0.0f, 0.0f}, 0, 1, true, 0.0f };
    EXPECT(applyDamage(e2, 99, kills));
    EXPECT(!e2.alive);
    EXPECT(kills == 2);
}

int main() {
    run_undo_tests();
    run_inventory_tests();
    run_leaderboard_tests();
    run_pathfind_tests();
    run_dungeon_tests();
    run_combat_tests();

    std::fprintf(stderr, "%d/%d passed\n", g_total - g_failed, g_total);
    return g_failed == 0 ? 0 : 1;
}
