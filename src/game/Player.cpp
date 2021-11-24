#include "game/Player.h"
#include "camera/tileCamera2D.h"
#include "game/game.h"

#define ifFirstPressed(key, pred) if (keys[key] && !keys_processed[key]) { pred; keys_processed[key] = true; }

Player::Player(glm::vec2 position, glm::vec2 size, Sprite* sprite, glm::vec3 color)
	: GameObject(position, glm::vec2(0.0f, 0.0f), size, color)
	, PlayerSprite(sprite)
	, MovementSpeed(2.0f)
	, InCollision(false)
	, RBody(nullptr)
	, Animator(nullptr)
	, Jumping(true)
	, Controls()
{	
	Game::AddTileSpaceObject(this);
	onTileSizeChanged(Game::TileSize);
}
Player::~Player()
{
	Game::RemoveTileSpaceObject(this);
}

void Player::SetRigidBody(std::shared_ptr<Physics2D::RigidBody> body)
{
	if (this->RBody)
		body->OnCollisionEnter = [](Physics2D::RigidBody* b, const Physics2D::CollisionInfo& info) {};
	this->RBody = body;
	body->OnCollisionEnter = std::bind(&Player::onCollision, this, std::placeholders::_1, std::placeholders::_2);
}

void Player::Draw(SpriteRenderer* renderer)
{	
	PlayerSprite->Size = this->Size * Game::TileSize * TileCamera2D::GetScale();
	PlayerSprite->Position = GetScreenPosition();
	PlayerSprite->Draw(renderer);
}
void Player::DrawAt(SpriteRenderer* renderer, glm::vec2 pos)
{		
	renderer->DrawSprite(PlayerSprite->Texture, pos, PlayerSprite->Size, PlayerSprite->Rotation, PlayerSprite->Color);
}

bool in_range(float& num, float low, float high)
{
	return low <= num && num <= high;
}

template<typename T> T sign(T val) {
	return T((T(0) < val) - (val < T(0)));
}

void Player::Update(float dt)
{
	if (RBody->LinearVelocity.y > 9.81f * 3.0f * dt)
		Jumping = true;

	if (Animator)
	{
		if (RBody->LinearVelocity.x == 0.0f)
			Animator->SetParameter("state", "idle");
		else
			Animator->SetParameter("state", "run");

		if (Jumping) {
			Animator->SetParameter("vertical", (RBody->LinearVelocity.y < 0) ? 1 : -1);
		}
		else {
			Animator->SetParameter("vertical", 0);
		}
	}

	// Apply gravity.
	glm::vec2 gravity = glm::vec2(0.0f, 9.81f);
	RBody->Move(glm::vec2(0.0f, 1.0f) * gravity * dt);


	Velocity = RBody->LinearVelocity;
	Acceleration = RBody->Acceleration;

	// Update the sprite.
	PlayerSprite = Animator->GetSprite();
}
void Player::UpdatePositions()
{
	if (RBody) {
		this->Position = RBody->GetPosition();
	}
}
glm::vec2 Player::GetScreenPosition() 
{
	return TileCamera2D::GetScreenPosition(RBody->GetAABB().GetMin());
}

void Player::ProcessKeyboard(bool* keys, bool* keys_processed, float dt)
{
	int horizontal = 0;
	int vertical = 0;
	float movement_speed = 2.0f;
	float max_velocity = 10.0f;
	glm::vec2 rightVec = glm::vec2(1.0f, 0.0f);
	glm::vec2 leftVec = glm::vec2(-1.0f, 0.0f);
	glm::vec2 upVec = glm::vec2(0.0f, -1.0f);
	glm::vec2 downVec = glm::vec2(0.0f, 1.0f);


	if (keys[Controls.RunLeft])
	{
		RBody->Move(leftVec * movement_speed * dt);
	}
	if (keys[Controls.RunRight])
	{
		RBody->Move(rightVec * movement_speed * dt);
	}
}

void Player::onCollision(Physics2D::RigidBody* body, const Physics2D::CollisionInfo& info)
{
	// Move the body out of the collision.
	RBody->MoveOutOfCollision(info);
	if (body->Name == "ground")
	{
	}
}

void Player::SetPosition(glm::vec2 position)
{
	this->Position = position;
}

void Player::onTileSizeChanged(glm::vec2 newTileSize)
{
	// screenPosition = TileCamera2D::GetScreenPosition(Position) - (Size * Game::TileSize * TileCamera2D::GetScale()) / 2.0f;
}