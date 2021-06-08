#include <3ds.h>
#include <stdio.h>

#include <box2d/box2d.h>
#include <stdlib.h>

#include <citro2d.h>

#include <memory>
#include <math.h>

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

int main(int argc, char **argv)
{
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    cfguInit();

    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // Define the gravity vector.
    b2Vec2 gravity(0.0f, 100.0f);

    // Construct a world object, which will hold and simulate the rigid bodies.
    // std::unique_ptr allows for this to destroy automatically when the program exits
    std::unique_ptr<b2World> world = std::make_unique<b2World>(gravity);

    // Define the ground body.
    b2BodyDef groundBodyDef;
    groundBodyDef.position.Set(0.0f, SCREEN_HEIGHT);

    // Call the body factory which allocates memory for the ground body
    // from a pool and creates the ground box shape (also from a pool).
    // The body is also added to the world.
    b2Body *groundBody = world->CreateBody(&groundBodyDef);

    // Define the ground box shape.
    b2PolygonShape groundBox;

    // The extents are the half-widths of the box.
    groundBox.SetAsBox(320, 32);

    // Add the ground fixture to the ground body.
    groundBody->CreateFixture(&groundBox, 0.0f);

    // Define the dynamic body. We set its position and call the body factory.
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position.Set(152.0f, 0.0f);
    b2Body* body = world->CreateBody(&bodyDef);

    // Define another box shape for our dynamic body.
    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(16.0f, 16.0f);

    // Define the dynamic body fixture.
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &dynamicBox;

    // Set the box density to be non-zero, so it will be dynamic.
    fixtureDef.density = 1.0f;

    // Override the default friction.
    fixtureDef.friction = 0.8f;

    // Add the shape to the body.
    body->CreateFixture(&fixtureDef);

    // Prepare for simulation. Typically we use a time step of 1/60 of a
    // second (60Hz) and 10 iterations. This provides a high quality simulation
    // in most game scenarios.
    float timeStep = 1.0f / 60.0f;

    int32_t velocityIterations = 6;
    int32_t positionIterations = 2;

    b2Vec2 position = body->GetPosition();
    float angle = body->GetAngle();

    // When the world destructor is called, all bodies and joints are freed. This can
    // create orphaned pointers, so be careful about your world management.

    touchPosition touch;

    C2D_TextBuf dynamicBuffer;
    dynamicBuffer = C2D_TextBufNew(4096);

    // Main loop
    while (aptMainLoop())
    {
        // Instruct the world to perform a single step of simulation.
        // It is generally best to keep the time step and iterations fixed.
        world->Step(timeStep, velocityIterations, positionIterations);

        hidScanInput();

        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START)
            break; // break in order to return to hbmenu

        /*
        ** To move the body we need to read the touch coords.
        **
        ** Once that's done, position the bodyDefinition struct to
        ** the new touch coords, offset by half width and height of the box.
        **
        ** Since bodies need to update to this world coordinate, we destroy
        ** the body from the world and create a new one based on the new
        ** bodyDefinition and then attach the old fixture.
        */
        if (kHeld & KEY_TOUCH)
        {
            hidTouchRead(&touch);

            world->DestroyBody(body);

            bodyDef.position.Set(touch.px - 8, touch.py - 8);

            body = world->CreateBody(&bodyDef);
            body->CreateFixture(&fixtureDef);
        }

        // Now print the position and angle of the body.
        position = body->GetPosition();
        angle = body->GetAngle();

        C2D_TextBufClear(dynamicBuffer);
        C2D_Text dynamicText;

        char buffer[160];
        snprintf(buffer, sizeof(buffer), "Body Pos: %4.2f, %4.2f / Angle: %4.2f\nTouch to position Body!", position.x, position.y, angle);

        C2D_TextParse(&dynamicText, dynamicBuffer, buffer);
	    C2D_TextOptimize(&dynamicText);

        /* Render the scene, clear to black */
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(bottom, C2D_Color32f(0, 0, 0, 1));
        C2D_SceneBegin(bottom);

        C2D_DrawText(&dynamicText, C2D_WithColor, 10.0f, 10.0f, 0.5f, 0.5f, 0.5f, C2D_Color32f(1, 1, 1, 1));

        b2Vec2 groundPos = groundBody->GetPosition();
        C2D_DrawRectSolid(groundPos.x, groundPos.y - 32, 0.5, SCREEN_WIDTH, 64, C2D_Color32f(0, 1, 0, 1));

        /* draw the dynamic box */
        C2D_DrawRectSolid(position.x, position.y, 0.5, 16, 16, C2D_Color32f(1, 0, 0, 1));

        C3D_FrameEnd(0);
    }

    cfguExit();

    C2D_TextBufDelete(dynamicBuffer);

    // Deinit libs
    C2D_Fini();

    C3D_Fini();

    gfxExit();

    return 0;
}
