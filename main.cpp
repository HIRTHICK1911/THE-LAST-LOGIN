#include "raylib.h"
#include <cmath>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>

using namespace std;

// ============================================================
// THE LAST LOGIN
// ============================================================

const int SCREEN_W = 1100;
const int SCREEN_H = 700;

const float WORLD_W = 4200.0f;
const float WORLD_H = 3200.0f;

const float PI_F = 3.14159265359f;

// ============================================================
// GAME
// ============================================================

enum GameState
{
    MENU,
    PLAYING
};

enum StoryStage
{
    FIND_HICHU,
    HICHU_TALK,
    GO_CLASSROOM_101,
    SEARCH_101,
    CLUE_FOUND,
    CAMPUS_AFTER_CLUE,
    HICHU_SIGHTING,
    OLD_BLOCK_UNLOCKED
};

enum Facing
{
    FACE_DOWN,
    FACE_UP,
    FACE_LEFT,
    FACE_RIGHT
};

// ============================================================
// STRUCTS
// ============================================================

struct Wall
{
    Rectangle rect;
};

struct NPC
{
    Vector2 pos;
    Vector2 target;

    Color shirt;
    Color hair;

    float speed;
    float walkCycle;
    float wait;

    bool walking;
    bool teacher;
    bool sitting;

    Facing facing;
};

struct StoryNPC
{
    Vector2 pos;

    string name;

    bool active;

    Facing facing;
};

// ============================================================
// GLOBALS
// ============================================================

GameState gameState = MENU;
StoryStage storyStage = FIND_HICHU;

vector<Wall> walls;
vector<NPC> npcs;

StoryNPC hichu;
StoryNPC karthi;

Camera2D camera = {};

Vector2 playerPos = {
    2000.0f,
    2300.0f
};

Facing playerFacing = FACE_DOWN;

float playerWalkCycle = 0.0f;

bool playerWalking = false;
bool playerRunning = false;

float gameTime = 0.0f;

bool dialogueOpen = false;
int dialogueLine = 0;

bool clueOpen = false;

bool oldBlockDoorOpen = false;

float sightingTimer = 0.0f;

bool objectiveFlash = false;
float objectiveFlashTimer = 0.0f;

// ============================================================
// HELPERS
// ============================================================

float Distance(Vector2 a, Vector2 b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;

    return sqrtf(dx * dx + dy * dy);
}

bool Near(Vector2 a, Vector2 b, float distance)
{
    return Distance(a, b) < distance;
}

float LengthV(Vector2 v)
{
    return sqrtf(v.x * v.x + v.y * v.y);
}

Vector2 NormalizeV(Vector2 v)
{
    float len = LengthV(v);

    if (len <= 0.0001f)
        return { 0.0f, 0.0f };

    return {
        v.x / len,
        v.y / len
    };
}

float ClampFloat(float value, float minValue, float maxValue)
{
    if (value < minValue)
        return minValue;

    if (value > maxValue)
        return maxValue;

    return value;
}

void AddWall(float x, float y, float w, float h)
{
    walls.push_back({
        { x, y, w, h }
    });
}

// ============================================================
// COLLISION
// ============================================================

bool Collides(Vector2 position)
{
    Rectangle playerRect = {
        position.x - 13.0f,
        position.y - 19.0f,
        26.0f,
        38.0f
    };

    for (const Wall& wall : walls)
    {
        if (CheckCollisionRecs(playerRect, wall.rect))
            return true;
    }

    return false;
}

// ============================================================
// FACING
// ============================================================

Facing DirectionFromMovement(Vector2 movement)
{
    if (fabsf(movement.x) > fabsf(movement.y))
    {
        if (movement.x > 0)
            return FACE_RIGHT;

        return FACE_LEFT;
    }

    if (movement.y > 0)
        return FACE_DOWN;

    return FACE_UP;
}

// ============================================================
// PIXEL HELPERS
// ============================================================

void PixelRect(
    int x,
    int y,
    int w,
    int h,
    Color color)
{
    DrawRectangle(x, y, w, h, color);
}

void PixelShadow(
    Vector2 pos,
    float width)
{
    DrawEllipse(
        (int)pos.x,
        (int)(pos.y + 17),
        (int)width,
        5,
        Fade(BLACK, 0.28f)
    );
}

// ============================================================
// PIXEL CHARACTER
//
// This is intentionally drawn like a small pixel/cel-shaded
// sprite instead of using realistic vector limbs.
//
// IMPORTANT:
// The legs are animated by actual alternating foot placement.
// The torso remains stable.
// ============================================================

void DrawVihzSprite(
    Vector2 pos,
    Facing facing,
    bool walking,
    float walkCycle,
    bool running,
    bool player)
{
    // --------------------------------------------------------
    // Pixel scale
    // --------------------------------------------------------

    const int S = player ? 3 : 3;

    float step = 0.0f;

    if (walking)
        step = sinf(walkCycle);

    float stepOpposite = -step;

    // Much smaller bob than before.
    float bob = 0.0f;

    if (walking)
        bob = fabsf(sinf(walkCycle)) * 1.2f;
    else
        bob = sinf(gameTime * 2.0f) * 0.35f;

    Vector2 p = {
        pos.x,
        pos.y + bob
    };

    PixelShadow(
        pos,
        player ? 17.0f : 15.0f
    );

    // --------------------------------------------------------
    // COLORS
    // --------------------------------------------------------

    Color skin = {
        214,
        168,
        135,
        255
    };

    Color skinShadow = {
        177,
        128,
        100,
        255
    };

    Color hair = {
        27,
        24,
        27,
        255
    };

    Color hairLight = {
        47,
        42,
        45,
        255
    };

    Color shirt = player
        ? Color{ 47, 64, 98, 255 }
        : Color{ 82, 75, 101, 255 };

    Color shirtLight = {
        (unsigned char)min(255, shirt.r + 25),
        (unsigned char)min(255, shirt.g + 25),
        (unsigned char)min(255, shirt.b + 25),
        255
    };

    Color pants = {
        34,
        35,
        42,
        255
    };

    Color pantsLight = {
        50,
        51,
        60,
        255
    };

    Color shoes = {
        18,
        18,
        22,
        255
    };

    // --------------------------------------------------------
    // PIXEL BODY DIMENSIONS
    // --------------------------------------------------------

    const int headW = player ? 21 : 19;
    const int headH = player ? 22 : 20;

    const int torsoW = player ? 25 : 22;
    const int torsoH = player ? 27 : 24;

    // --------------------------------------------------------
    // BACKPACK
    // --------------------------------------------------------

    if (player)
    {
        if (facing == FACE_DOWN ||
            facing == FACE_LEFT ||
            facing == FACE_RIGHT)
        {
            PixelRect(
                (int)p.x - 15,
                (int)p.y - 10,
                7,
                22,
                Color{ 25, 29, 39, 255 }
            );

            PixelRect(
                (int)p.x - 14,
                (int)p.y - 8,
                5,
                15,
                Color{ 45, 49, 62, 255 }
            );
        }
    }

    // ========================================================
    // DOWN / FRONT
    // ========================================================

    if (facing == FACE_DOWN)
    {
        // ----------------------------------------------------
        // LEGS
        // ----------------------------------------------------

        float stride =
            walking
            ? step * (running ? 7.0f : 5.0f)
            : 0.0f;

        int leftX =
            (int)(p.x - 7 + stride);

        int rightX =
            (int)(p.x + 1 - stride);

        int legY =
            (int)(p.y + 10);

        // Back/left leg shadow.
        PixelRect(
            leftX,
            legY,
            7,
            15,
            pants
        );

        PixelRect(
            leftX + 1,
            legY + 2,
            5,
            9,
            pantsLight
        );

        // Right leg.
        PixelRect(
            rightX,
            legY,
            7,
            15,
            pants
        );

        PixelRect(
            rightX + 1,
            legY + 2,
            5,
            9,
            pantsLight
        );

        // Shoes.
        PixelRect(
            leftX - 2,
            legY + 13,
            10,
            5,
            shoes
        );

        PixelRect(
            rightX - 1,
            legY + 13,
            10,
            5,
            shoes
        );

        // ----------------------------------------------------
        // TORSO
        // ----------------------------------------------------

        PixelRect(
            (int)p.x - torsoW / 2,
            (int)p.y - 15,
            torsoW,
            torsoH,
            shirt
        );

        // Shirt light side.
        PixelRect(
            (int)p.x - torsoW / 2 + 3,
            (int)p.y - 13,
            4,
            20,
            shirtLight
        );

        // Collar.
        PixelRect(
            (int)p.x - 5,
            (int)p.y - 14,
            10,
            4,
            Color{ 35, 42, 63, 255 }
        );

        // ----------------------------------------------------
        // ARMS
        // Small natural swing.
        // ----------------------------------------------------

        float arm =
            walking
            ? step * (running ? 3.5f : 2.5f)
            : 0.0f;

        // Left arm.
        PixelRect(
            (int)p.x - torsoW / 2 - 5,
            (int)(p.y - 11 + arm),
            5,
            18,
            shirt
        );

        PixelRect(
            (int)p.x - torsoW / 2 - 5,
            (int)(p.y + 5 + arm),
            5,
            7,
            skin
        );

        // Right arm.
        PixelRect(
            (int)p.x + torsoW / 2,
            (int)(p.y - 11 - arm),
            5,
            18,
            shirt
        );

        PixelRect(
            (int)p.x + torsoW / 2,
            (int)(p.y + 5 - arm),
            5,
            7,
            skin
        );

        // ----------------------------------------------------
        // NECK
        // ----------------------------------------------------

        PixelRect(
            (int)p.x - 4,
            (int)p.y - 20,
            8,
            7,
            skin
        );

        // ----------------------------------------------------
        // HEAD
        // ----------------------------------------------------

        PixelRect(
            (int)p.x - headW / 2,
            (int)p.y - 34,
            headW,
            headH,
            skin
        );

        // Ears.
        PixelRect(
            (int)p.x - headW / 2 - 2,
            (int)p.y - 28,
            3,
            7,
            skin
        );

        PixelRect(
            (int)p.x + headW / 2 - 1,
            (int)p.y - 28,
            3,
            7,
            skin
        );

        // ----------------------------------------------------
        // HAIR
        // ----------------------------------------------------

        PixelRect(
            (int)p.x - headW / 2,
            (int)p.y - 36,
            headW,
            8,
            hair
        );

        PixelRect(
            (int)p.x - headW / 2 + 3,
            (int)p.y - 40,
            headW - 7,
            5,
            hair
        );

        // Hair highlights.
        PixelRect(
            (int)p.x - 7,
            (int)p.y - 37,
            4,
            3,
            hairLight
        );

        PixelRect(
            (int)p.x + 2,
            (int)p.y - 36,
            4,
            3,
            hairLight
        );

        // ----------------------------------------------------
        // FACE
        // ----------------------------------------------------

        PixelRect(
            (int)p.x - 6,
            (int)p.y - 27,
            3,
            3,
            Color{ 25, 25, 28, 255 }
        );

        PixelRect(
            (int)p.x + 3,
            (int)p.y - 27,
            3,
            3,
            Color{ 25, 25, 28, 255 }
        );
    }

    // ========================================================
    // UP / BACK
    // ========================================================

    else if (facing == FACE_UP)
    {
        float stride =
            walking
            ? step * (running ? 7.0f : 5.0f)
            : 0.0f;

        int leftX =
            (int)(p.x - 7 + stride);

        int rightX =
            (int)(p.x + 1 - stride);

        int legY =
            (int)(p.y + 10);

        // Legs.
        PixelRect(
            leftX,
            legY,
            7,
            15,
            pants
        );

        PixelRect(
            rightX,
            legY,
            7,
            15,
            pants
        );

        // Shoes.
        PixelRect(
            leftX - 2,
            legY + 13,
            10,
            5,
            shoes
        );

        PixelRect(
            rightX - 1,
            legY + 13,
            10,
            5,
            shoes
        );

        // Backpack visible from behind.
        if (player)
        {
            PixelRect(
                (int)p.x - 12,
                (int)p.y - 15,
                24,
                23,
                Color{ 27, 31, 41, 255 }
            );

            PixelRect(
                (int)p.x - 8,
                (int)p.y - 11,
                16,
                3,
                Color{ 54, 58, 69, 255 }
            );
        }

        // Torso.
        PixelRect(
            (int)p.x - torsoW / 2,
            (int)p.y - 15,
            torsoW,
            torsoH,
            shirt
        );

        // Shoulders.
        PixelRect(
            (int)p.x - torsoW / 2 - 4,
            (int)p.y - 11,
            5,
            19,
            shirt
        );

        PixelRect(
            (int)p.x + torsoW / 2 - 1,
            (int)p.y - 11,
            5,
            19,
            shirt
        );

        // Neck.
        PixelRect(
            (int)p.x - 4,
            (int)p.y - 20,
            8,
            7,
            skin
        );

        // Back of head.
        PixelRect(
            (int)p.x - headW / 2,
            (int)p.y - 34,
            headW,
            headH,
            hair
        );

        // Hair layers.
        PixelRect(
            (int)p.x - headW / 2 - 2,
            (int)p.y - 31,
            4,
            12,
            hair
        );

        PixelRect(
            (int)p.x + headW / 2 - 2,
            (int)p.y - 31,
            4,
            12,
            hair
        );

        PixelRect(
            (int)p.x - headW / 2 + 3,
            (int)p.y - 39,
            headW - 6,
            8,
            hairLight
        );

        PixelRect(
            (int)p.x - 6,
            (int)p.y - 38,
            12,
            5,
            hair
        );
    }

    // ========================================================
    // LEFT
    // ========================================================

    else if (facing == FACE_LEFT)
    {
        float stride =
            walking
            ? step * (running ? 7.0f : 5.0f)
            : 0.0f;

        int backLeg =
            (int)(p.y + 10 + stride);

        int frontLeg =
            (int)(p.y + 10 - stride);

        // Back leg.
        PixelRect(
            (int)p.x - 6,
            backLeg,
            7,
            15,
            pants
        );

        // Front leg.
        PixelRect(
            (int)p.x + 1,
            frontLeg,
            7,
            15,
            pants
        );

        // Shoes point left.
        PixelRect(
            (int)p.x - 12,
            backLeg + 13,
            11,
            5,
            shoes
        );

        PixelRect(
            (int)p.x - 11,
            frontLeg + 13,
            11,
            5,
            shoes
        );

        // Torso.
        PixelRect(
            (int)p.x - torsoW / 2,
            (int)p.y - 15,
            torsoW,
            torsoH,
            shirt
        );

        PixelRect(
            (int)p.x - torsoW / 2,
            (int)p.y - 13,
            5,
            20,
            shirtLight
        );

        // One visible arm.
        float arm =
            walking
            ? step * (running ? 3.5f : 2.5f)
            : 0.0f;

        PixelRect(
            (int)p.x - 15,
            (int)(p.y - 9 + arm),
            6,
            19,
            shirt
        );

        PixelRect(
            (int)p.x - 15,
            (int)(p.y + 7 + arm),
            6,
            7,
            skin
        );

        // Neck.
        PixelRect(
            (int)p.x - 7,
            (int)p.y - 20,
            8,
            7,
            skin
        );

        // Head.
        PixelRect(
            (int)p.x - 14,
            (int)p.y - 34,
            22,
            22,
            skin
        );

        // Hair.
        PixelRect(
            (int)p.x - 15,
            (int)p.y - 37,
            23,
            9,
            hair
        );

        PixelRect(
            (int)p.x - 17,
            (int)p.y - 32,
            6,
            12,
            hair
        );

        // Nose.
        PixelRect(
            (int)p.x - 17,
            (int)p.y - 26,
            4,
            4,
            skin
        );

        // Eye.
        PixelRect(
            (int)p.x - 12,
            (int)p.y - 27,
            3,
            3,
            Color{ 20, 20, 22, 255 }
        );
    }

    // ========================================================
    // RIGHT
    // ========================================================

    else if (facing == FACE_RIGHT)
    {
        float stride =
            walking
            ? step * (running ? 7.0f : 5.0f)
            : 0.0f;

        int backLeg =
            (int)(p.y + 10 + stride);

        int frontLeg =
            (int)(p.y + 10 - stride);

        PixelRect(
            (int)p.x - 8,
            backLeg,
            7,
            15,
            pants
        );

        PixelRect(
            (int)p.x - 1,
            frontLeg,
            7,
            15,
            pants
        );

        // Shoes point right.
        PixelRect(
            (int)p.x + 1,
            backLeg + 13,
            11,
            5,
            shoes
        );

        PixelRect(
            (int)p.x,
            frontLeg + 13,
            11,
            5,
            shoes
        );

        // Torso.
        PixelRect(
            (int)p.x - torsoW / 2,
            (int)p.y - 15,
            torsoW,
            torsoH,
            shirt
        );

        PixelRect(
            (int)p.x + torsoW / 2 - 5,
            (int)p.y - 13,
            5,
            20,
            shirtLight
        );

        float arm =
            walking
            ? step * (running ? 3.5f : 2.5f)
            : 0.0f;

        PixelRect(
            (int)p.x + 9,
            (int)(p.y - 9 + arm),
            6,
            19,
            shirt
        );

        PixelRect(
            (int)p.x + 9,
            (int)(p.y + 7 + arm),
            6,
            7,
            skin
        );

        // Neck.
        PixelRect(
            (int)p.x - 1,
            (int)p.y - 20,
            8,
            7,
            skin
        );

        // Head.
        PixelRect(
            (int)p.x - 8,
            (int)p.y - 34,
            22,
            22,
            skin
        );

        // Hair.
        PixelRect(
            (int)p.x - 8,
            (int)p.y - 37,
            23,
            9,
            hair
        );

        PixelRect(
            (int)p.x + 9,
            (int)p.y - 32,
            6,
            12,
            hair
        );

        // Nose.
        PixelRect(
            (int)p.x + 14,
            (int)p.y - 26,
            4,
            4,
            skin
        );

        // Eye.
        PixelRect(
            (int)p.x + 9,
            (int)p.y - 27,
            3,
            3,
            Color{ 20, 20, 22, 255 }
        );
    }

    // ========================================================
    // PLAYER LITTLE MARKER
    // ========================================================

    if (player)
    {
        DrawRectangle(
            (int)p.x - 2,
            (int)p.y - 45,
            4,
            3,
            Color{ 220, 185, 85, 255 }
        );
    }
}

// ============================================================
// BUILDINGS
// ============================================================

void DrawBuilding(
    Rectangle r,
    Color wallColor,
    Color roofColor,
    const char* title)
{
    DrawRectangleRec(r, wallColor);

    DrawRectangle(
        (int)r.x,
        (int)r.y,
        (int)r.width,
        12,
        roofColor
    );

    DrawRectangleLinesEx(
        r,
        3,
        Color{ 30, 30, 35, 255 }
    );

    DrawText(
        title,
        (int)r.x + 18,
        (int)r.y + 18,
        20,
        Color{ 35, 35, 40, 255 }
    );
}

void DrawClassroom(
    Rectangle r,
    const char* room,
    bool secondFloor = false)
{
    Color buildingColor =
        secondFloor
        ? Color{ 105, 110, 125, 255 }
        : Color{ 125, 130, 140, 255 };

    DrawRectangleRec(
        r,
        buildingColor
    );

    DrawRectangleLinesEx(
        r,
        2,
        Color{ 35, 37, 43, 255 }
    );

    // Door.
    DrawRectangle(
        (int)(r.x + r.width / 2 - 18),
        (int)(r.y + r.height - 38),
        36,
        38,
        Color{ 65, 58, 55, 255 }
    );

    // Windows.
    DrawRectangle(
        (int)r.x + 12,
        (int)r.y + 35,
        35,
        28,
        Color{ 165, 195, 205, 255 }
    );

    DrawRectangle(
        (int)(r.x + r.width - 47),
        (int)r.y + 35,
        35,
        28,
        Color{ 165, 195, 205, 255 }
    );

    DrawText(
        room,
        (int)(r.x + r.width / 2 - 12),
        (int)r.y + 12,
        18,
        WHITE
    );
}

// ============================================================
// WALLS
// ============================================================

void BuildWalls()
{
    walls.clear();

    // Academic.
    AddWall(900, 500, 900, 35);
    AddWall(900, 1100, 900, 35);

    AddWall(900, 500, 35, 600);
    AddWall(1765, 500, 35, 600);

    AddWall(1150, 500, 25, 210);
    AddWall(1150, 830, 25, 270);

    AddWall(1450, 500, 25, 210);
    AddWall(1450, 830, 25, 270);

    // Library.
    AddWall(600, 1250, 500, 30);
    AddWall(600, 1800, 500, 30);
    AddWall(600, 1250, 30, 580);
    AddWall(1070, 1250, 30, 580);

    // Canteen.
    AddWall(1200, 1300, 500, 30);
    AddWall(1200, 1700, 500, 30);
    AddWall(1200, 1300, 30, 400);
    AddWall(1670, 1300, 30, 400);

    // Admin.
    AddWall(1900, 700, 500, 30);
    AddWall(1900, 1200, 500, 30);
    AddWall(1900, 700, 30, 530);
    AddWall(2370, 700, 30, 530);

    // Old block.
    AddWall(2550, 2300, 650, 30);
    AddWall(2550, 2900, 650, 30);
    AddWall(2550, 2300, 30, 600);
    AddWall(3170, 2300, 30, 600);

    // World boundary.
    AddWall(-50, -50, WORLD_W + 100, 50);
    AddWall(-50, WORLD_H, WORLD_W + 100, 50);
    AddWall(-50, -50, 50, WORLD_H + 100);
    AddWall(WORLD_W, -50, 50, WORLD_H + 100);
}

// ============================================================
// PATH
// ============================================================

void DrawPath(Rectangle r)
{
    DrawRectangleRec(
        r,
        Color{ 180, 174, 158, 255 }
    );

    DrawRectangleLinesEx(
        r,
        1,
        Color{ 155, 150, 137, 255 }
    );
}

// ============================================================
// TREE
// ============================================================

void DrawTree(Vector2 p, float scale)
{
    DrawEllipse(
        (int)p.x,
        (int)(p.y + 25 * scale),
        (int)(30 * scale),
        (int)(9 * scale),
        Fade(BLACK, 0.20f)
    );

    DrawRectangle(
        (int)(p.x - 7 * scale),
        (int)(p.y - 5 * scale),
        (int)(14 * scale),
        (int)(45 * scale),
        Color{ 90, 62, 42, 255 }
    );

    DrawCircleV(
        { p.x, p.y - 28 * scale },
        28 * scale,
        Color{ 42, 77, 54, 255 }
    );

    DrawCircleV(
        { p.x - 20 * scale, p.y - 18 * scale },
        21 * scale,
        Color{ 48, 87, 59, 255 }
    );

    DrawCircleV(
        { p.x + 20 * scale, p.y - 18 * scale },
        21 * scale,
        Color{ 48, 87, 59, 255 }
    );

    DrawCircleV(
        { p.x, p.y - 48 * scale },
        20 * scale,
        Color{ 55, 94, 64, 255 }
    );
}

// ============================================================
// BENCH
// ============================================================

void DrawBench(Vector2 p)
{
    DrawRectangle(
        (int)p.x - 35,
        (int)p.y - 4,
        70,
        8,
        Color{ 105, 72, 47, 255 }
    );

    DrawRectangle(
        (int)p.x - 30,
        (int)p.y + 13,
        8,
        20,
        Color{ 65, 55, 48, 255 }
    );

    DrawRectangle(
        (int)p.x + 22,
        (int)p.y + 13,
        8,
        20,
        Color{ 65, 55, 48, 255 }
    );
}

// ============================================================
// MAP
// ============================================================

void DrawMap()
{
    // Grass.
    DrawRectangle(
        0,
        0,
        (int)WORLD_W,
        (int)WORLD_H,
        Color{ 74, 103, 77, 255 }
    );

    // Paths.
    DrawPath({
        0,
        2050,
        WORLD_W,
        160
    });

    DrawPath({
        1850,
        0,
        160,
        WORLD_H
    });

    DrawPath({
        760,
        1000,
        1600,
        120
    });

    // Main quad.
    DrawRectangle(
        1350,
        1500,
        900,
        500,
        Color{ 89, 117, 86, 255 }
    );

    DrawRectangleLinesEx(
        { 1350, 1500, 900, 500 },
        3,
        Color{ 105, 134, 96, 255 }
    );

    DrawText(
        "MAIN QUAD",
        1650,
        1515,
        28,
        Fade(WHITE, 0.35f)
    );

    // Academic.
    DrawBuilding(
        { 900, 500, 900, 600 },
        Color{ 145, 145, 150, 255 },
        Color{ 65, 67, 75, 255 },
        "ACADEMIC BLOCK"
    );

    DrawClassroom(
        { 950, 550, 190, 240 },
        "101"
    );

    DrawClassroom(
        { 1200, 550, 190, 240 },
        "102"
    );

    DrawClassroom(
        { 1450, 550, 280, 240 },
        "LAB"
    );

    DrawClassroom(
        { 950, 830, 190, 240 },
        "104"
    );

    DrawClassroom(
        { 1200, 830, 190, 240 },
        "105"
    );

    DrawClassroom(
        { 1450, 830, 280, 240 },
        "106"
    );

    // Second floor.
    DrawRectangle(
        900,
        250,
        900,
        180,
        Color{ 105, 110, 122, 255 }
    );

    DrawRectangleLinesEx(
        { 900, 250, 900, 180 },
        3,
        Color{ 45, 48, 55, 255 }
    );

    DrawText(
        "SECOND FLOOR",
        1190,
        270,
        25,
        WHITE
    );

    for (int i = 0; i < 10; i++)
    {
        float x = 925 + (i % 5) * 170;
        float y = 315 + (i / 5) * 70;

        DrawRectangle(
            (int)x,
            (int)y,
            145,
            55,
            Color{ 130, 135, 145, 255 }
        );

        string room =
            "20" + to_string(i + 1);

        DrawText(
            room.c_str(),
            (int)x + 55,
            (int)y + 17,
            17,
            WHITE
        );
    }

    // Library.
    DrawBuilding(
        { 600, 1250, 500, 580 },
        Color{ 135, 125, 115, 255 },
        Color{ 70, 62, 58, 255 },
        "LIBRARY"
    );

    DrawText(
        "ARCHIVES",
        735,
        1320,
        24,
        Color{ 50, 45, 45, 255 }
    );

    // Canteen.
    DrawBuilding(
        { 1200, 1300, 500, 400 },
        Color{ 180, 145, 105, 255 },
        Color{ 80, 62, 45, 255 },
        "CANTEEN"
    );

    DrawText(
        "FOOD • CHAT • MEMORIES",
        1280,
        1450,
        18,
        Color{ 65, 50, 40, 255 }
    );

    // Administration.
    DrawBuilding(
        { 1900, 700, 500, 530 },
        Color{ 150, 150, 155, 255 },
        Color{ 68, 70, 78, 255 },
        "ADMINISTRATION"
    );

    // Old block.
    Color oldColor =
        oldBlockDoorOpen
        ? Color{ 87, 85, 83, 255 }
        : Color{ 63, 61, 60, 255 };

    DrawBuilding(
        { 2550, 2300, 650, 600 },
        oldColor,
        Color{ 39, 39, 40, 255 },
        "OLD BLOCK"
    );

    DrawText(
        oldBlockDoorOpen
            ? "ACCESS OPEN"
            : "LOCKED",
        2790,
        2370,
        22,
        oldBlockDoorOpen
            ? Color{ 150, 190, 155, 255 }
            : Color{ 190, 115, 100, 255 }
    );

    // Broken windows.
    for (int i = 0; i < 5; i++)
    {
        float x = 2610 + i * 115;

        DrawRectangle(
            (int)x,
            2480,
            70,
            55,
            Color{ 40, 43, 45, 255 }
        );

        DrawLine(
            (int)x,
            2480,
            (int)x + 70,
            2535,
            Color{ 90, 90, 90, 255 }
        );

        DrawLine(
            (int)x + 70,
            2480,
            (int)x,
            2535,
            Color{ 90, 90, 90, 255 }
        );
    }

    // Fountain.
    DrawCircleV(
        { 1800, 1770 },
        90,
        Color{ 105, 110, 115, 255 }
    );

    DrawCircleV(
        { 1800, 1770 },
        70,
        Color{ 95, 145, 165, 255 }
    );

    DrawCircleV(
        { 1800, 1770 },
        25,
        Color{ 120, 170, 185, 255 }
    );

    // Benches.
    DrawBench({ 1450, 1900 });
    DrawBench({ 1700, 1900 });
    DrawBench({ 1950, 1900 });
    DrawBench({ 2200, 1900 });

    // Trees.
    DrawTree({ 350, 500 }, 1.2f);
    DrawTree({ 450, 850 }, 1.0f);
    DrawTree({ 350, 1500 }, 1.2f);
    DrawTree({ 450, 2200 }, 1.1f);
    DrawTree({ 700, 2300 }, 1.0f);

    DrawTree({ 3400, 450 }, 1.3f);
    DrawTree({ 3650, 800 }, 1.1f);
    DrawTree({ 3500, 1300 }, 1.2f);

    DrawTree({ 3500, 2100 }, 1.4f);
    DrawTree({ 3750, 2450 }, 1.2f);
    DrawTree({ 3400, 2750 }, 1.3f);

    // Back campus.
    DrawText(
        "BACK CAMPUS",
        3000,
        1800,
        28,
        Fade(WHITE, 0.45f)
    );

    DrawRectangle(
        2800,
        1750,
        700,
        350,
        Fade(Color{ 65, 90, 70, 255 }, 0.5f)
    );

    DrawRectangleLinesEx(
        { 2800, 1750, 700, 350 },
        3,
        Fade(WHITE, 0.12f)
    );

    // Playground.
    DrawRectangleLinesEx(
        { 2950, 1850, 400, 220 },
        5,
        Color{ 155, 135, 105, 255 }
    );

    DrawCircleLines(
        3150,
        1960,
        45,
        Color{ 155, 135, 105, 255 }
    );

    // Gate.
    DrawRectangle(
        1600,
        3100,
        1000,
        100,
        Color{ 50, 53, 60, 255 }
    );

    DrawRectangle(
        1780,
        3070,
        640,
        40,
        Color{ 72, 75, 83, 255 }
    );

    DrawText(
        "SKCET",
        2050,
        3080,
        30,
        WHITE
    );
}

// ============================================================
// NPC UPDATE
// ============================================================

void UpdateNPCs(float dt)
{
    for (NPC& npc : npcs)
    {
        if (npc.wait > 0)
        {
            npc.wait -= dt;
            npc.walking = false;

            npc.walkCycle += dt * 1.5f;

            continue;
        }

        Vector2 dir = {
            npc.target.x - npc.pos.x,
            npc.target.y - npc.pos.y
        };

        float dist = LengthV(dir);

        if (dist < 12.0f)
        {
            npc.walking = false;

            npc.wait =
                1.0f +
                (float)(rand() % 30) / 10.0f;

            npc.target = {
                500.0f + (float)(rand() % 3200),
                400.0f + (float)(rand() % 2400)
            };

            continue;
        }

        dir = NormalizeV(dir);

        npc.facing =
            DirectionFromMovement(dir);

        npc.pos.x +=
            dir.x * npc.speed * dt;

        npc.pos.y +=
            dir.y * npc.speed * dt;

        if (Collides(npc.pos))
        {
            npc.target = {
                500.0f + (float)(rand() % 3200),
                400.0f + (float)(rand() % 2400)
            };
        }

        npc.walking = true;

        npc.walkCycle +=
            dt * (npc.speed > 65 ? 8.0f : 6.0f);
    }
}

// ============================================================
// STORY DIALOGUE
// ============================================================

vector<string> hichuLines =
{
    "You came late, Vihz.",
    "Did you check Classroom 101?",
    "There's something I need to show you.",
    "Go there. I'll meet you inside."
};

vector<string> karthiLines =
{
    "Hichu? Yeah... I saw him this morning.",
    "He looked like he hadn't slept.",
    "Actually... he kept looking toward the Old Block.",
    "I thought that place was locked."
};

// ============================================================
// DIALOGUE
// ============================================================

void StartDialogue()
{
    dialogueOpen = true;
    dialogueLine = 0;
}

void AdvanceDialogue()
{
    dialogueLine++;

    if (storyStage == HICHU_TALK)
    {
        if (dialogueLine >= (int)hichuLines.size())
        {
            dialogueOpen = false;

            hichu.active = false;

            storyStage = GO_CLASSROOM_101;

            objectiveFlash = true;
            objectiveFlashTimer = 2.0f;
        }
    }

    else if (storyStage == CAMPUS_AFTER_CLUE)
    {
        if (dialogueLine >= (int)karthiLines.size())
        {
            dialogueOpen = false;

            storyStage = HICHU_SIGHTING;

            sightingTimer = 0.0f;
        }
    }
}

// ============================================================
// STORY
// ============================================================

void UpdateStory(float dt)
{
    if (dialogueOpen)
        return;

    // --------------------------------------------------------
    // FIND HICHU
    // --------------------------------------------------------

    if (storyStage == FIND_HICHU)
    {
        if (hichu.active &&
            Near(playerPos, hichu.pos, 85))
        {
            if (IsKeyPressed(KEY_E) ||
                IsKeyPressed(KEY_SPACE))
            {
                StartDialogue();

                storyStage = HICHU_TALK;
            }
        }
    }

    // --------------------------------------------------------
    // HICHU TALK
    // --------------------------------------------------------

    else if (storyStage == HICHU_TALK)
    {
        if (IsKeyPressed(KEY_E) ||
            IsKeyPressed(KEY_SPACE))
        {
            AdvanceDialogue();
        }
    }

    // --------------------------------------------------------
    // CLASSROOM 101
    // --------------------------------------------------------

    else if (storyStage == GO_CLASSROOM_101)
    {
        Vector2 door = {
            1200,
            625
        };

        if (Near(playerPos, door, 90))
        {
            if (IsKeyPressed(KEY_E) ||
                IsKeyPressed(KEY_SPACE))
            {
                storyStage = SEARCH_101;
            }
        }
    }

    // --------------------------------------------------------
    // SEARCH 101
    // --------------------------------------------------------

    else if (storyStage == SEARCH_101)
    {
        Vector2 clue = {
            1280,
            830
        };

        if (!clueOpen &&
            Near(playerPos, clue, 110))
        {
            if (IsKeyPressed(KEY_E) ||
                IsKeyPressed(KEY_SPACE))
            {
                clueOpen = true;
            }
        }

        if (clueOpen)
        {
            if (IsKeyPressed(KEY_E) ||
                IsKeyPressed(KEY_SPACE))
            {
                clueOpen = false;

                storyStage = CLUE_FOUND;

                objectiveFlash = true;
                objectiveFlashTimer = 2.0f;
            }
        }
    }

    // --------------------------------------------------------
    // CLUE FOUND
    // --------------------------------------------------------

    else if (storyStage == CLUE_FOUND)
    {
        Vector2 door = {
            1200,
            625
        };

        if (Near(playerPos, door, 100))
        {
            storyStage = CAMPUS_AFTER_CLUE;
        }
    }

    // --------------------------------------------------------
    // Karthi
    // --------------------------------------------------------

    else if (storyStage == CAMPUS_AFTER_CLUE)
    {
        if (Near(playerPos, karthi.pos, 90))
        {
            if (IsKeyPressed(KEY_E) ||
                IsKeyPressed(KEY_SPACE))
            {
                StartDialogue();
            }
        }
    }

    // --------------------------------------------------------
    // HICHU SIGHTING
    // --------------------------------------------------------

    else if (storyStage == HICHU_SIGHTING)
    {
        sightingTimer += dt;

        if (sightingTimer > 1.0f)
        {
            hichu.active = true;

            hichu.pos = {
                3000,
                2350
            };

            hichu.facing = FACE_LEFT;
        }

        if (sightingTimer > 5.0f)
        {
            hichu.active = false;

            storyStage = OLD_BLOCK_UNLOCKED;

            oldBlockDoorOpen = true;

            objectiveFlash = true;
            objectiveFlashTimer = 2.0f;
        }
    }

    // --------------------------------------------------------
    // OLD BLOCK
    // --------------------------------------------------------

    else if (storyStage == OLD_BLOCK_UNLOCKED)
    {
        Vector2 door = {
            2750,
            2550
        };

        if (Near(playerPos, door, 120))
        {
            if (IsKeyPressed(KEY_E) ||
                IsKeyPressed(KEY_SPACE))
            {
                oldBlockDoorOpen = true;
            }
        }
    }

    if (objectiveFlash)
    {
        objectiveFlashTimer -= dt;

        if (objectiveFlashTimer <= 0)
            objectiveFlash = false;
    }
}

// ============================================================
// PLAYER
// ============================================================

void UpdatePlayer(float dt)
{
    if (dialogueOpen || clueOpen)
    {
        playerWalking = false;
        playerRunning = false;
        return;
    }

    Vector2 movement = {
        0,
        0
    };

    if (IsKeyDown(KEY_W))
        movement.y -= 1;

    if (IsKeyDown(KEY_S))
        movement.y += 1;

    if (IsKeyDown(KEY_A))
        movement.x -= 1;

    if (IsKeyDown(KEY_D))
        movement.x += 1;

    float len = LengthV(movement);

    playerWalking =
        len > 0.01f;

    playerRunning =
        playerWalking &&
        (IsKeyDown(KEY_LEFT_SHIFT) ||
         IsKeyDown(KEY_RIGHT_SHIFT));

    if (playerWalking)
    {
        movement =
            NormalizeV(movement);

        playerFacing =
            DirectionFromMovement(movement);

        float speed =
            playerRunning
            ? 390.0f
            : 260.0f;

        Vector2 next = playerPos;

        next.x +=
            movement.x * speed * dt;

        if (!Collides(next))
            playerPos.x = next.x;

        next = playerPos;

        next.y +=
            movement.y * speed * dt;

        if (!Collides(next))
            playerPos.y = next.y;

        // IMPORTANT:
        // This is a clean human-like walk cycle.
        playerWalkCycle +=
            dt * (playerRunning ? 12.0f : 8.0f);
    }
    else
    {
        playerWalkCycle +=
            dt * 1.5f;
    }

    playerPos.x =
        ClampFloat(
            playerPos.x,
            40,
            WORLD_W - 40
        );

    playerPos.y =
        ClampFloat(
            playerPos.y,
            40,
            WORLD_H - 40
        );
}

// ============================================================
// OBJECTIVE
// ============================================================

string GetObjective()
{
    switch (storyStage)
    {
        case FIND_HICHU:
        case HICHU_TALK:
            return "Find Hichu around the campus.";

        case GO_CLASSROOM_101:
            return "Go to Classroom 101.";

        case SEARCH_101:
            return "Search Classroom 101.";

        case CLUE_FOUND:
            return "Leave Classroom 101.";

        case CAMPUS_AFTER_CLUE:
            return "Ask someone about Hichu.";

        case HICHU_SIGHTING:
            return "Something moved near the Old Block.";

        case OLD_BLOCK_UNLOCKED:
            return "Investigate the Old Block.";

        default:
            return "Explore the campus.";
    }
}

// ============================================================
// PROMPT
// ============================================================

string GetPrompt()
{
    if (storyStage == FIND_HICHU &&
        hichu.active &&
        Near(playerPos, hichu.pos, 85))
    {
        return "[E] Talk to Hichu";
    }

    if (storyStage == GO_CLASSROOM_101 &&
        Near(playerPos, { 1200, 625 }, 90))
    {
        return "[E] Enter Classroom 101";
    }

    if (storyStage == SEARCH_101 &&
        Near(playerPos, { 1280, 830 }, 110))
    {
        return "[E] Search";
    }

    if (storyStage == CAMPUS_AFTER_CLUE &&
        Near(playerPos, karthi.pos, 90))
    {
        return "[E] Talk";
    }

    if (storyStage == OLD_BLOCK_UNLOCKED &&
        Near(playerPos, { 2750, 2550 }, 120))
    {
        return "[E] Enter Old Block";
    }

    return "";
}

// ============================================================
// HUD
// ============================================================

void DrawObjective(const string& text)
{
    DrawRectangle(
        20,
        20,
        470,
        58,
        Fade(Color{ 15, 17, 21, 255 }, 0.92f)
    );

    DrawRectangleLinesEx(
        { 20, 20, 470, 58 },
        2,
        Fade(WHITE, 0.15f)
    );

    DrawText(
        "OBJECTIVE",
        38,
        30,
        14,
        Color{ 180, 180, 190, 255 }
    );

    DrawText(
        text.c_str(),
        38,
        48,
        18,
        WHITE
    );
}

void DrawPrompt(const string& text)
{
    int width =
        MeasureText(
            text.c_str(),
            20
        );

    DrawRectangle(
        SCREEN_W / 2 - width / 2 - 20,
        SCREEN_H - 80,
        width + 40,
        45,
        Fade(BLACK, 0.82f)
    );

    DrawText(
        text.c_str(),
        SCREEN_W / 2 - width / 2,
        SCREEN_H - 68,
        20,
        WHITE
    );
}

void DrawDialogue(
    const string& speaker,
    const string& text)
{
    DrawRectangle(
        45,
        SCREEN_H - 190,
        SCREEN_W - 90,
        135,
        Fade(Color{ 12, 14, 18, 255 }, 0.96f)
    );

    DrawRectangleLinesEx(
        {
            45,
            SCREEN_H - 190,
            SCREEN_W - 90,
            135
        },
        2,
        Fade(WHITE, 0.20f)
    );

    DrawText(
        speaker.c_str(),
        70,
        SCREEN_H - 165,
        22,
        Color{ 210, 180, 100, 255 }
    );

    DrawText(
        text.c_str(),
        70,
        SCREEN_H - 128,
        20,
        WHITE
    );

    DrawText(
        "E / SPACE",
        SCREEN_W - 165,
        SCREEN_H - 75,
        15,
        Color{ 150, 150, 160, 255 }
    );
}

// ============================================================
// CLUE
// ============================================================

void DrawClue()
{
    DrawRectangle(
        150,
        120,
        SCREEN_W - 300,
        SCREEN_H - 240,
        Fade(Color{ 8, 9, 12, 255 }, 0.97f)
    );

    DrawRectangleLinesEx(
        {
            150,
            120,
            SCREEN_W - 300,
            SCREEN_H - 240
        },
        3,
        Color{ 95, 95, 100, 255 }
    );

    DrawText(
        "FOUND",
        190,
        160,
        18,
        Color{ 160, 160, 170, 255 }
    );

    DrawText(
        "A TORN NOTE",
        190,
        195,
        34,
        WHITE
    );

    DrawText(
        "A torn note lies beneath a desk.",
        190,
        275,
        21,
        Color{ 190, 190, 195, 255 }
    );

    DrawText(
        "\"Don't trust the last login.\"",
        190,
        335,
        28,
        Color{ 210, 185, 125, 255 }
    );

    DrawText(
        "E / SPACE TO CLOSE",
        190,
        460,
        17,
        Color{ 140, 140, 150, 255 }
    );
}

// ============================================================
// ATMOSPHERE
// ============================================================

void DrawAtmosphere()
{
    if (gameTime > 30)
    {
        float darkness =
            ClampFloat(
                (gameTime - 30) / 100,
                0,
                0.35f
            );

        DrawRectangle(
            0,
            0,
            SCREEN_W,
            SCREEN_H,
            Fade(
                Color{ 10, 14, 25, 255 },
                darkness
            )
        );
    }

    if (storyStage == HICHU_SIGHTING ||
        storyStage == OLD_BLOCK_UNLOCKED)
    {
        DrawRectangle(
            0,
            0,
            SCREEN_W,
            SCREEN_H,
            Fade(
                Color{ 15, 18, 25, 255 },
                0.08f
            )
        );
    }
}

// ============================================================
// MENU
// ============================================================

void DrawMenu()
{
    ClearBackground(
        Color{ 8, 10, 13, 255 }
    );

    DrawCircleV(
        {
            SCREEN_W / 2.0f,
            300
        },
        260,
        Fade(
            Color{ 45, 50, 65, 255 },
            0.07f
        )
    );

    DrawText(
        "THE LAST LOGIN",
        285,
        210,
        62,
        WHITE
    );

    DrawText(
        "Some memories never stay buried.",
        350,
        285,
        20,
        Color{ 145, 145, 155, 255 }
    );

    DrawRectangle(
        430,
        380,
        240,
        60,
        Color{ 25, 27, 33, 255 }
    );

    DrawRectangleLinesEx(
        { 430, 380, 240, 60 },
        2,
        Color{ 100, 100, 110, 255 }
    );

    DrawText(
        "ENTER CAMPUS",
        465,
        400,
        18,
        WHITE
    );

    DrawText(
        "WASD  Move",
        470,
        500,
        16,
        Color{ 125, 125, 135, 255 }
    );

    DrawText(
        "SHIFT  Run",
        470,
        525,
        16,
        Color{ 125, 125, 135, 255 }
    );

    DrawText(
        "E / SPACE  Interact",
        430,
        550,
        16,
        Color{ 125, 125, 135, 255 }
    );
}

// ============================================================
// NPC CREATION
// ============================================================

void CreateNPCs()
{
    npcs.clear();

    Color shirts[] =
    {
        { 55, 90, 130, 255 },
        { 120, 65, 75, 255 },
        { 80, 105, 75, 255 },
        { 135, 105, 60, 255 },
        { 80, 75, 115, 255 },
        { 105, 80, 65, 255 }
    };

    Color hairs[] =
    {
        { 30, 25, 22, 255 },
        { 45, 30, 24, 255 },
        { 25, 27, 30, 255 },
        { 55, 40, 30, 255 }
    };

    for (int i = 0; i < 12; i++)
    {
        NPC npc;

        npc.pos = {
            500.0f +
                (float)(rand() % 3200),

            400.0f +
                (float)(rand() % 2400)
        };

        npc.target = {
            500.0f +
                (float)(rand() % 3200),

            400.0f +
                (float)(rand() % 2400)
        };

        npc.shirt =
            shirts[i % 6];

        npc.hair =
            hairs[i % 4];

        npc.speed =
            45.0f +
            (float)(rand() % 35);

        npc.walkCycle =
            (float)(rand() % 100);

        npc.wait =
            (float)(rand() % 30) / 10.0f;

        npc.walking = false;
        npc.teacher = false;
        npc.sitting = false;

        npc.facing = FACE_DOWN;

        npcs.push_back(npc);
    }
}

// ============================================================
// MAIN
// ============================================================

int main()
{
    srand((unsigned int)time(nullptr));

    InitWindow(
        SCREEN_W,
        SCREEN_H,
        "The Last Login"
    );

    SetTargetFPS(60);

    BuildWalls();
    CreateNPCs();

    // --------------------------------------------------------
    // HICHU
    // --------------------------------------------------------

    hichu.pos = {
        2050,
        2150
    };

    hichu.name = "Hichu";
    hichu.active = true;
    hichu.facing = FACE_DOWN;

    // --------------------------------------------------------
    // KARTHI
    // --------------------------------------------------------

    karthi.pos = {
        1600,
        1740
    };

    karthi.name = "Karthi";
    karthi.active = true;
    karthi.facing = FACE_DOWN;

    // --------------------------------------------------------
    // CAMERA
    // --------------------------------------------------------

    camera.target =
        playerPos;

    camera.offset = {
        SCREEN_W / 2.0f,
        SCREEN_H / 2.0f
    };

    camera.rotation = 0;
    camera.zoom = 1.0f;

    // ========================================================
    // GAME LOOP
    // ========================================================

    while (!WindowShouldClose())
    {
        float dt =
            GetFrameTime();

        // ====================================================
        // MENU
        // ====================================================

        if (gameState == MENU)
        {
            if (IsKeyPressed(KEY_ENTER) ||
                IsKeyPressed(KEY_SPACE))
            {
                gameState = PLAYING;
            }

            BeginDrawing();

            DrawMenu();

            EndDrawing();

            continue;
        }

        // ====================================================
        // UPDATE
        // ====================================================

        gameTime += dt;

        UpdatePlayer(dt);
        UpdateNPCs(dt);
        UpdateStory(dt);

        // ====================================================
        // CAMERA
        // ====================================================

        Vector2 cameraTarget =
            playerPos;

        if (storyStage == HICHU_SIGHTING &&
            sightingTimer > 1.0f &&
            sightingTimer < 5.0f)
        {
            cameraTarget =
                hichu.pos;
        }

        camera.target.x +=
            (cameraTarget.x - camera.target.x) * 0.08f;

        camera.target.y +=
            (cameraTarget.y - camera.target.y) * 0.08f;

        if (storyStage == HICHU_SIGHTING &&
            sightingTimer > 1.0f &&
            sightingTimer < 5.0f)
        {
            camera.zoom +=
                (1.15f - camera.zoom) * 0.04f;
        }
        else
        {
            camera.zoom +=
                (1.0f - camera.zoom) * 0.04f;
        }

        // ====================================================
        // DRAW
        // ====================================================

        BeginDrawing();

        ClearBackground(
            Color{ 74, 103, 77, 255 }
        );

        BeginMode2D(camera);

        // ----------------------------------------------------
        // MAP
        // ----------------------------------------------------

        DrawMap();

        // ----------------------------------------------------
        // NPCS
        // ----------------------------------------------------

        for (const NPC& npc : npcs)
        {
            DrawVihzSprite(
                npc.pos,
                npc.facing,
                npc.walking,
                npc.walkCycle,
                false,
                false
            );
        }

        // ----------------------------------------------------
        // KARTHI
        // ----------------------------------------------------

        if (karthi.active)
        {
            DrawVihzSprite(
                karthi.pos,
                karthi.facing,
                false,
                gameTime,
                false,
                false
            );
        }

        // ----------------------------------------------------
        // HICHU
        // ----------------------------------------------------

        if (hichu.active)
        {
            bool hichuWalking =
                storyStage == HICHU_SIGHTING;

            DrawVihzSprite(
                hichu.pos,
                hichu.facing,
                hichuWalking,
                gameTime * 8.0f,
                false,
                false
            );
        }

        // ----------------------------------------------------
        // VIHZ
        // ----------------------------------------------------

        DrawVihzSprite(
            playerPos,
            playerFacing,
            playerWalking,
            playerWalkCycle,
            playerRunning,
            true
        );

        EndMode2D();

        // ====================================================
        // ATMOSPHERE
        // ====================================================

        DrawAtmosphere();

        // ====================================================
        // HUD
        // ====================================================

        DrawObjective(
            GetObjective()
        );

        string prompt =
            GetPrompt();

        if (!prompt.empty() &&
            !dialogueOpen &&
            !clueOpen)
        {
            DrawPrompt(prompt);
        }

        // ====================================================
        // DIALOGUE
        // ====================================================

        if (dialogueOpen)
        {
            if (storyStage == HICHU_TALK)
            {
                int index =
                    min(
                        dialogueLine,
                        (int)hichuLines.size() - 1
                    );

                DrawDialogue(
                    "HICHU",
                    hichuLines[index]
                );
            }

            else if (storyStage == CAMPUS_AFTER_CLUE)
            {
                int index =
                    min(
                        dialogueLine,
                        (int)karthiLines.size() - 1
                    );

                DrawDialogue(
                    "KARTHI",
                    karthiLines[index]
                );
            }
        }

        // ====================================================
        // CLUE
        // ====================================================

        if (clueOpen)
        {
            DrawClue();
        }

        // ====================================================
        // OBJECTIVE FLASH
        // ====================================================

        if (objectiveFlash)
        {
            float alpha =
                ClampFloat(
                    objectiveFlashTimer / 2.0f,
                    0,
                    1
                );

            DrawText(
                "NEW OBJECTIVE",
                SCREEN_W / 2 - 100,
                105,
                18,
                Fade(WHITE, alpha)
            );
        }

        // ====================================================
        // CONTROLS
        // ====================================================

        if (!dialogueOpen &&
            !clueOpen)
        {
            DrawText(
                "WASD Move",
                20,
                SCREEN_H - 28,
                15,
                Fade(WHITE, 0.55f)
            );

            DrawText(
                "SHIFT Run",
                110,
                SCREEN_H - 28,
                15,
                Fade(WHITE, 0.55f)
            );
        }

        EndDrawing();
    }

    CloseWindow();

    return 0;
}