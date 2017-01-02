#pragma once
#include "stdafx.h"

const float BULLET_SPEED = 700;
const float BOSS_BULLET_SPEED = 300;
const sf::Vector2f BULLET_SIZE = { 14, 11 };

const float MAX_WEAPON_COLDOWN = 1;
const float MELEE_COLDOWN = 0.4f;
const float SHOOTGUN_COLDOWN = 0.5f;
const float AK_COLDOWN = 0.1f;

enum struct BulletType
{
	PLAYER_AK,
	PLAYER_SHOOTGUN,
	CLOWN_BULLET,
	BOSS_BULLET,
};

struct Bullet
{
	Bullet(sf::Vector2f const& startPos, int demage, int orientation, float maxRange, BulletType const& type);

	sf::RectangleShape bodyShape;
	sf::Texture bodyTexture;
	sf::FloatRect collisionRect;
	float currentRange = 0;
	float maxRange;
	float speed = BULLET_SPEED;
	float angle = 0;
	BulletType type;
	int movmentOrientation;
	int demage;
	bool isLive = true;

	void Update(float elapsedTime);
};