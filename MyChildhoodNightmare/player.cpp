#include "player.h"

using namespace sf;
using namespace std;

bool Player::InitPlayer()
{
	if (!bodyTexture.loadFromFile("resources/player.png") ||
		!InitCharacterSound())
	{
		return false;
	}

	collisionRect.width = PLAYER_SIZE.x / 2.0f;
	collisionRect.height = PLAYER_SIZE.y - 10;

	bodyShape.setTexture(&bodyTexture);
	bodyShape.setSize(PLAYER_SIZE);
	bodyShape.setOrigin(PLAYER_SIZE.x / 2.0f, PLAYER_SIZE.y);
	bodyShape.setRotation(0);

	moveSpeed = PLAYER_MOVE_SPEED;
	jumpSpeed = 0;
	maxHealth = PLAYER_START_HEALTH;
	health = PLAYER_START_HEALTH;

	injuredColdown = INJURED_COLDOWN;

	boxes = 0;
	existStatus = ExistenceStatus::LIVE;
	shootDemage = PLAYER_PISTOL_DEMAGE;
	ammo = { -1, PLAYER_START_SHOOTGUN_AMMO, PLAYER_START_AK_AMMO };
	shootRange = PLAYER_START_SHOOT_RANGE;
	deathSound = &playerDeath;

	characterBullets.clear();
	CreateCopy();

	return true;
}

void Player::UpdateStatuses()
{
	if (injuredColdown < INJURED_COLDOWN)
	{
		bodyShape.setFillColor(Color(255, 255, 255, 140));
	}
	else
	{
		bodyShape.setFillColor(Color(255, 255, 255, 255));
	}
}

void Player::SwitchWeapon()
{
	switchWeapon.play();
	int currWeapon = static_cast<int>(currentWeapon);

	if (currentWeapon == Weapon::AK)
	{
		currentWeapon = Weapon::MELEE;
	}
	else
	{
		currentWeapon = Weapon(currWeapon + 1);
	}
}

void Player::Attack()
{
	int orientationId = static_cast<int>(orientationStatus);
	int rounds = ammo[(int)currentWeapon];

	switch (currentWeapon)
	{
	case Weapon::MELEE:
		if (shootColdown > MELEE_COLDOWN)
		{
			weaponPistol.play();

			characterBullets.push_back(new Bullet(GetCharacterPos(), shootDemage, orientationId, shootRange, BulletType::PLAYER_AK));
			shootColdown = 0;
		}
		break;
	case Weapon::SHOOTGUN:
		if (shootColdown > SHOOTGUN_COLDOWN && rounds > 0)
		{
			weaponShootgun.play();

			Vector2f topBullPos = GetCharacterPos() + Vector2f(0, -25);
			Vector2f bottomBullPos = GetCharacterPos() + Vector2f(0, 25);

			characterBullets.push_back(new Bullet(topBullPos, shootgunDemage, orientationId, shootRange, BulletType::PLAYER_SHOOTGUN));
			characterBullets.push_back(new Bullet(GetCharacterPos(), shootgunDemage, orientationId, shootRange, BulletType::PLAYER_SHOOTGUN));
			characterBullets.push_back(new Bullet(bottomBullPos, shootgunDemage, orientationId, shootRange, BulletType::PLAYER_SHOOTGUN));
			ammo[(int)currentWeapon] = rounds - 1;
			shootColdown = 0;
		}
		break;
	case Weapon::AK:
		if (shootColdown > AK_COLDOWN && rounds > 0)
		{
			weaponAK.play();

			characterBullets.push_back(new Bullet(GetCharacterPos(), akDemage, orientationId, shootRange, BulletType::PLAYER_AK));
			ammo[(int)currentWeapon] = rounds - 1;
			shootColdown = 0;
		}
		break;
	default:
		break;
	}
}

void Player::RotateDeadBody(float elapsedTime)
{
	auto bodyRotation = bodyShape.getRotation();
	bodyShape.setRotation(bodyRotation + DEAD_ROTATION * elapsedTime / GAME_OVER_COLDOWN);
}

void Player::CreateCopy()
{
	copy_ak_demage = akDemage;
	copy_shootgun_demage = shootDemage;
	copy_ak_ammo = ammo[AK_AMMO];
	copy_shootgun_ammo = ammo[SHOOTGUN_AMMO];
	copy_maxHealth = maxHealth;
	copy_health = health;
}

void Player::ReturnCopy()
{
	akDemage = copy_ak_demage;
	shootDemage = copy_shootgun_demage;
	ammo[AK_AMMO] = copy_ak_ammo;
	ammo[SHOOTGUN_AMMO] = copy_shootgun_ammo;
	maxHealth = copy_maxHealth;
	health = copy_maxHealth; // all levels start with max health
}

void Player::Clear()
{
	boxes = 0;
}