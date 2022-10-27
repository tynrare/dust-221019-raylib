
#include "raylib.h"
#include "rlgl.h"

#include <time.h>       // Required for: localtime(), asctime()
#include <math.h>

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

const int DATASET_SIZE = 10;

Color PosToColor(int x, int y) {
	return (Color){
		(unsigned char)x, 
		(unsigned char)((int)x >> 8),
		(unsigned char)y,
		(unsigned char)((int)y >> 8)
	};
}

Vector2 ColorToPos(Color c) {
	return (Vector2) {
		c.r | (c.g << 8),
		c.b | (c.a << 8)
	};
}

Vector2 IndexToPos( int index, int xdimension )
{
	return (Vector2){
		index % xdimension,
		floor((float)index / (float)xdimension)
	};
}

typedef enum {
	CIRCLE = 0,
	BOX    = 1
} ShapeType;

/**
 * @returns {int} new empty index
 */
int WriteEntity(int shift, ShapeType type, Vector2 position, Vector2 size, int rotation)
{
	DrawPixelV(IndexToPos(shift, DATASET_SIZE), (Color){ 4, 0, 0, 0 });

	// entity type
	// a == 0 : circle
	// a == 1 : box
	DrawPixelV(IndexToPos(shift + 1, DATASET_SIZE), (Color){ (int)type, 0, 0, 0 });
	// entity position
	DrawPixelV(IndexToPos(shift + 2, DATASET_SIZE), 
			PosToColor((int)position.x, (int)position.y));
	// entity size
	DrawPixelV(IndexToPos(shift + 3, DATASET_SIZE), PosToColor(size.x, size.y));
	// entity rotation
	DrawPixelV(IndexToPos(shift + 4, DATASET_SIZE), PosToColor(rotation, 0));

	return shift + 5;
}


//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 512;
    const int screenHeight = 512;

    InitWindow(screenWidth, screenHeight, "raylib [shaders] example - hot reloading");

    RenderTexture2D texdataset = LoadRenderTexture(DATASET_SIZE, DATASET_SIZE);

    // blahblah
    // ===
    const char *fragShaderFileName = "res/reload.fs";
    long fragShaderFileModTime = GetFileModTime(TextFormat(fragShaderFileName, GLSL_VERSION));

    // Load raymarching shader
    // NOTE: Defining 0 (NULL) for vertex shader forces usage of internal default vertex shader
    Shader shader = LoadShader(0, TextFormat(fragShaderFileName, GLSL_VERSION));

    // Get shader locations for required uniforms
    int resolutionLoc = GetShaderLocation(shader, "resolution");
    int mouseLoc = GetShaderLocation(shader, "mouse");
    int timeLoc = GetShaderLocation(shader, "time");
    int datasetLoc = GetShaderLocation(shader, "dataset");

    float resolution[2] = { (float)screenWidth, (float)screenHeight };
    SetShaderValue(shader, resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

    float totalTime = 0.0f;
        
        
    bool shaderAutoReloading = false;

    SetTargetFPS(60);                       // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())            // Detect window close button or ESC key
    {
        Vector2 mouse = GetMousePosition();
        
        BeginTextureMode(texdataset);
	BeginBlendMode(BLEND_ADD_COLORS);
            ClearBackground(BLANK);

	    int shift = 0;
	    shift = WriteEntity(shift, CIRCLE, mouse, (Vector2){20, 0}, 0);
	    shift = WriteEntity(shift, BOX, (Vector2){ 100, 100 }, (Vector2){ 20, 20 }, 45);

	    EndBlendMode();
        EndTextureMode();
        
        // blahblah
        // ===
        // Update
        //----------------------------------------------------------------------------------
        totalTime += GetFrameTime();
        float mousePos[2] = { mouse.x, mouse.y };

        // Set shader required uniform values
        SetShaderValue(shader, timeLoc, &totalTime, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, mouseLoc, mousePos, SHADER_UNIFORM_VEC2);

        // Hot shader reloading
        if (shaderAutoReloading || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)))
        {
            long currentFragShaderModTime = GetFileModTime(TextFormat(fragShaderFileName, GLSL_VERSION));

            // Check if shader file has been modified
            if (currentFragShaderModTime != fragShaderFileModTime)
            {
                // Try reloading updated shader
                Shader updatedShader = LoadShader(0, TextFormat(fragShaderFileName, GLSL_VERSION));

                if (updatedShader.id != rlGetShaderIdDefault())      // It was correctly loaded
                {
                    UnloadShader(shader);
                    shader = updatedShader;

                    // Get shader locations for required uniforms
                    resolutionLoc = GetShaderLocation(shader, "resolution");
                    mouseLoc = GetShaderLocation(shader, "mouse");
                    timeLoc = GetShaderLocation(shader, "time");
		    datasetLoc = GetShaderLocation(shader, "dataset");

                    // Reset required uniforms
                    SetShaderValue(shader, resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
                }

                fragShaderFileModTime = currentFragShaderModTime;
            }
        }

        if (IsKeyPressed(KEY_A)) shaderAutoReloading = !shaderAutoReloading;
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            // We only draw a white full-screen rectangle, frame is generated in shader
            BeginShaderMode(shader);
		    SetShaderValueTexture(shader, datasetLoc, texdataset.texture);
                DrawRectangle(0, 0, screenWidth, screenHeight, WHITE);
            EndShaderMode();

            DrawText(TextFormat("PRESS [A] to TOGGLE SHADER AUTOLOADING: %s",
                     shaderAutoReloading? "AUTO" : "MANUAL"), 10, 10, 10, shaderAutoReloading? RED : BLACK);

            DrawText(TextFormat("Shader last modification: %s", asctime(localtime(&fragShaderFileModTime))), 10, 430, 10, BLACK);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadShader(shader);           // Unload shader

    CloseWindow();                  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
