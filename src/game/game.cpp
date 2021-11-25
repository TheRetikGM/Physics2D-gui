#include <iostream>
#include <string>
#include "game/game.h"
#include "resource_manager.h"
#include "config.h"
#include "sprite_renderer.h"
#include "tilemap_renderer.h"
#include "tilemap.h"
#include "camera/tileCamera2D.h"
#include "basic_renderer.h"
#include "game/Player.h"
#include "TextRenderer.h"
#include "Helper.hpp"
#include <GLFW/glfw3.h>
#include <thread>
#define printf_v(name, vec, prec) std::printf(name ": [%" prec "f, %" prec "f]\n", (vec).x, (vec).y)

using namespace std::placeholders;
using namespace Physics2D;

// Initialize static Game member variables.
glm::vec2 Game::TileSize = glm::vec2(32.0f, 32.0f);
std::vector<ITileSpace*> Game::tileSpaceObjects;

BasicRenderer*	 basic_renderer = nullptr;
TextRenderer* text_renderer = nullptr;

// Temp
Helper::Stopwatch w1;
Helper::Stopwatch w2;
Helper::Stopwatch w3;
PhysicsWorld* world;
glm::vec2 MousePosition;
bool renderDebugLines = false;
bool debugColors = false;

// Callbacks.
void onLayerDraw(const Tmx::Map *map, const Tmx::Layer *layer, int n_layer);
void onCameraScale(glm::vec2 scale);

Game::Game(unsigned int width, unsigned int height) : State(GameState::active), Width(width), Height(height), Keys(), KeysProcessed(), BackgroundColor(0.0f), CurrentLevel(-1)
{}
Game::~Game()
{	
	if (basic_renderer)
		delete basic_renderer;
	if (text_renderer)
		delete text_renderer;
	ResourceManager::Clear();
	for (auto& level : this->Levels)
		GameLevel::Delete(level);

	if (world)
		delete world;
}
void Game::SetTileSize(glm::vec2 new_size)
{
	Game::TileSize = new_size;
	std::for_each(tileSpaceObjects.begin(), tileSpaceObjects.end(), [&](ITileSpace* obj) { 
		obj->onTileSizeChanged(new_size); 
	});	
}
void Game::OnResize()
{
	// If resized, update projection matrix to match new width and height of window.
	glm::mat4 projection = glm::ortho(0.0f, (float)this->Width, (float)this->Height, 0.0f, -1.0f, 1.0f);

	ResourceManager::GetShader("basic_render").Use().SetMat4("projection", projection);
}
void Game::ProcessMouse(float xpos, float ypos)
{
	MousePosition = glm::vec2((float)xpos, (float)ypos);
}
void Game::ProcessScroll(float yoffset)
{
	TileCamera2D::SetScale(TileCamera2D::GetScale() + 0.1f * yoffset);
	Game::SetTileSize(Game::TileSize);
}

void RegenerateMap(Game* game)
{
	static std::vector<std::string> map = {
		"############################",
		"#--------------------------#",
		"#--------------------------#",
		"#--------------------------#",
		"#--------------------------#",
		"#--------------------------#",
		"#--------------------------#",
		"#--------------------------#",
		"#--------------------------#",
		"#--------------------------#",
		"#--------------------------#",
		"############################",
	};

	int h = map.size();
	int w = map[0].size();
	float block_width = game->Width / float(w);
	float block_height = game->Height / float(h);

	world->RemoveAllBodies();

	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			if (map[y][x] != '#')
				continue;

			float x_pix = x * block_width;
			float y_pix = y * block_height;

			world->AddRectangleBody(glm::vec2(x_pix, y_pix), glm::vec2(block_width, block_height), 0.5f, true, 0.5f, true);
		}
	}
}

void Game::Init()
{
	Game::SetTileSize(glm::vec2(32.0f, 32.0f));
	this->BackgroundColor = glm::vec3(0.0f);

	// Load shaders
	ResourceManager::LoadShader(SHADERS_DIR "BasicRender.vert", SHADERS_DIR "BasicRender.frag", nullptr, "basic_render");

	// Projection used for 2D projection.
	glm::mat4 projection = glm::ortho(0.0f, (float)this->Width, (float)this->Height, 0.0f, -1.0f, 1.0f);

	// Initialize basic renderer.
	ResourceManager::GetShader("basic_render").Use().SetMat4("projection", projection);
	basic_renderer = new BasicRenderer(ResourceManager::GetShader("basic_render"));
	basic_renderer->SetLineWidth(2.0f);

	// Initialize text renderer.
	text_renderer = new TextRenderer(Width, Height);
	text_renderer->Load(ASSETS_DIR "fonts/arial.ttf", 24);

	// Init Physics world.
	world = new PhysicsWorld(glm::vec2(0.0f, 0.0f), glm::vec2(float(Width), float(Height)), 50);
	world->CollisionTree->SortOnTests = true;
	world->Gravity = glm::vec2(0.0f, 0.0f);
	// world->Gravity = glm::vec2(0.0f, 0.0f);
	RegenerateMap(this);
}

void SpawnCircle(Game* g)
{
	bool s = g->Keys[GLFW_KEY_LEFT_SHIFT];
	glm::vec2 rand_size = glm::vec2(Helper::RandomFloat(20.0f, 40.0f), Helper::RandomFloat(20.0f, 40.0f));
	world->AddRectangleBody(MousePosition, rand_size, Helper::RandomFloat(2.0f, 10.0f), s, Helper::RandomFloat_0_1(), true);
}
void SpawnRectangle(Game* g)
{
	bool s = g->Keys[GLFW_KEY_LEFT_SHIFT];
	float random_radius = Helper::RandomFloat(10.0f, 20.0f);
	world->AddCircleBody(MousePosition, random_radius, Helper::RandomFloat(2.0f, 10.0f), s, Helper::RandomFloat_0_1(), true);
}

void Game::ProcessInput(float dt)
{
	if (Keys[GLFW_KEY_A] && !KeysProcessed[GLFW_KEY_A])
	{
		SpawnCircle(this);

		if (!Keys[GLFW_KEY_LEFT_SHIFT])
			KeysProcessed[GLFW_KEY_A] = true;
	}
	if (Keys[GLFW_KEY_D] && !KeysProcessed[GLFW_KEY_D])
	{
		SpawnRectangle(this);

		if (!Keys[GLFW_KEY_LEFT_SHIFT])
			KeysProcessed[GLFW_KEY_D] = true;
	}
	if (Keys[GLFW_KEY_R] && !KeysProcessed[GLFW_KEY_R])
	{
		RegenerateMap(this);
		KeysProcessed[GLFW_KEY_R] = true;
	}
	if (Keys[GLFW_KEY_F1] && !KeysProcessed[GLFW_KEY_F1])
	{
		renderDebugLines = !renderDebugLines;
		KeysProcessed[GLFW_KEY_F1] = true;
	}
	if (Keys[GLFW_KEY_F2] && !KeysProcessed[GLFW_KEY_F2])
	{
		debugColors = !debugColors;
		KeysProcessed[GLFW_KEY_F2] = true;
	}
	if (Keys[GLFW_KEY_F3] && !KeysProcessed[GLFW_KEY_F3])
	{
		world->CollisionTree->SortOnTests = !world->CollisionTree->SortOnTests;
		KeysProcessed[GLFW_KEY_F3] = true;
	}
	if (Keys[GLFW_KEY_LEFT_CONTROL] && !KeysProcessed[GLFW_KEY_LEFT_CONTROL])
	{
		printf("----- Stats ------\n");
		printf("Update step: %f ms\n", w1.ElapsedMilliseconds());
		printf("	 Bodies: %i\n", world->BodyCount());
		printf("------------------\n");
		KeysProcessed[GLFW_KEY_LEFT_CONTROL] = true;
	}
}
void Game::Update(float dt)
{
	w1.Restart();
	world->Update(dt, 15);
	w1.Stop();
}

void RenderQuadTreeDebugLines(CollisionQuadTree::Node* pTree)
{
	if (!pTree)
		return;
	static int depth = 0;
	
	depth++;
	for (int i = 0; i < 4; i++)
		RenderQuadTreeDebugLines(pTree->pChild[i]);

	glm::vec2 hLine[2], vLine[2];
	hLine[0] = world->ToUnits(pTree->center - glm::vec2(pTree->halfSize.x, 0.0f));
	hLine[1] = world->ToUnits(pTree->center + glm::vec2(pTree->halfSize.x, 0.0f));
	vLine[0] = world->ToUnits(pTree->center - glm::vec2(0.0f, pTree->halfSize.y));
	vLine[1] = world->ToUnits(pTree->center + glm::vec2(0.0f, pTree->halfSize.y));

	int color = 3;
	if (debugColors)
		color = depth - 1;
	float r = (~color & (1 << 2)) ? 1.0f : 0.0f;
	float g = (~color & (1 << 1)) ? 1.0f : 0.0f;
	float b = (~color & (1 << 0)) ? 1.0f : 0.0f;

	basic_renderer->RenderLine(hLine[0], hLine[1], glm::vec3(r, g, b));
	basic_renderer->RenderLine(vLine[0], vLine[1], glm::vec3(r, g, b));
	depth--;
}
void RenderBodies(CollisionQuadTree::Node* pTree)
{
	if (!pTree)
		return;

	static int depth = 0;

	depth++;
	for (int i = 0; i < 4; i++)
		RenderBodies(pTree->pChild[i]);

	for (RigidBody* body = pTree->pObjList; body; body = body->GetNextObject())
	{
		ColliderType type = body->GetCollider()->GetType();
		const AABB& aabb = body->GetAABB();
		glm::vec2 center = body->GetCenter(true);
		glm::vec2 offset = -text_renderer->GetStringSize(std::to_string(depth)) / 2.0f;

		if (type == ColliderType::rectangle)
			basic_renderer->RenderShape(br_Shape::rectangle, aabb.GetMin(true), aabb.GetSize(true), 0.0f, glm::vec3(1.0f));
		else if (type == ColliderType::circle)
			basic_renderer->RenderShape(br_Shape::circle_empty, aabb.GetMin(true), aabb.GetSize(true), 0.0f, glm::vec3(1.0f));
		text_renderer->RenderText(std::to_string(depth), center.x + offset.x, center.y + offset.y, 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	}

	depth--;
}

void Game::Render()
{
	// RenderBodies(world->CollisionTree->root);

	for (auto body : *world->CollisionTree)
	{
		ColliderType type = body->GetCollider()->GetType();
		const AABB& aabb = body->GetAABB();
		glm::vec2 center = body->GetCenter(true);

		if (type == ColliderType::rectangle)
			basic_renderer->RenderShape(br_Shape::rectangle, aabb.GetMin(true), aabb.GetSize(true), 0.0f, glm::vec3(1.0f));
		else if (type == ColliderType::circle)
			basic_renderer->RenderShape(br_Shape::circle_empty, aabb.GetMin(true), aabb.GetSize(true), 0.0f, glm::vec3(1.0f));
	}

	if (renderDebugLines)
		RenderQuadTreeDebugLines(world->CollisionTree->root);

	std::string controls = 
		"(" + std::string(renderDebugLines ? "on" : "off") + ") F1 - render quadtree\n"
		"(" + std::string(debugColors ? "on" : "off") + ") F2 - colors\n"
		"(" + std::string(world->CollisionTree->SortOnTests ? "on" : "off") +  ") F3 - sort collision tests\n"
		"Left ctrl - print world info";

	text_renderer->RenderText(controls, 5.0f, 5.0f, 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
}

// Callbacks
/// Called AFTER the layer is drawn.
void Game::OnLayerRendered(const Tmx::Map *map, const Tmx::Layer *layer, int n_layer)
{
}
void onCameraScale(glm::vec2 scale)
{
}
