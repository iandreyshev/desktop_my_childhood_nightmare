#include "enemies.h"

using namespace std;

Enemy::Enemy(sf::Vector2f const& position, EnemyType const& type)
{
	switch (type)
	{
	case EnemyType::SHADOW:
		this->CreateShadow();
		break;
	case EnemyType::CLOWN:
		this->CreateClown();
		break;
	case EnemyType::GHOST:
		this->CreateGhost();
		break;
	case EnemyType::SPIDER:
		this->CreateSpider();
		break;
	case EnemyType::BOSS:
		this->CreateBoss();
		break;
	default:
		break;
	}

	currentRunStatus = MovementStatus(rand() % 2);

	bodyShape.setTexture(&bodyTexture);

	const sf::Vector2f BODY_SIZE = bodyShape.getSize();
	const sf::Vector2i BODY_SIZE_INT = static_cast<sf::Vector2i>(BODY_SIZE);

	if (enemyType != EnemyType::SPIDER)
	{
		bodyShape.setTextureRect(sf::IntRect({ 0, 0 }, BODY_SIZE_INT));
	}

	bodyShape.setSize(BODY_SIZE);
	bodyShape.setOrigin(BODY_SIZE.x / 2.0f, BODY_SIZE.y);

	const sf::Vector2f BONE_SIZE(BODY_SIZE.x / 2.0f, BODY_SIZE.y - 10);
	collisionRect = sf::FloatRect(position, BONE_SIZE);
}

void Enemy::CreateShadow()
{
	enemyType = EnemyType::SHADOW;

	bodyTexture.loadFromFile("resources/enemyShadow.png");
	bodyShape.setSize(SHADOW_SIZE);

	health = SHADOW_START_HEALTH;
	touchDemage = SHADOW_TOUCH_DEMAGE;

	float randomSpeed = SHADOW_MOVE_SPEED_RANDOM * (rand() % 101) / 100;
	moveSpeed = SHADOW_MOVE_SPEED + randomSpeed;

	UpdateActivityStatus = [&](Character const& player) {
		UpdateShadowActivityStatus(player);
	};

	Pursuit = [&](Character const& player, std::vector<Bullet*>& bullets, std::vector<Object> const& blocks) {
		(void)bullets;
		(void)player;
		(void)blocks;
	};

	Idle = [&](float elapsedTime, std::vector<Object> const& blocks) {
		ShadowWalk(elapsedTime, blocks);
	};
}

void Enemy::CreateClown()
{
	enemyType = EnemyType::CLOWN;

	bodyTexture.loadFromFile("resources/enemyClown.png");
	bodyShape.setSize(CLOWN_SIZE);

	health = CLOWN_START_HEALTH;
	shootDemage = CLOWN_SHOOT_DEMAGE;
	touchDemage = CLOWN_TOUCH_DEMAGE;
	shootRange = CLOWN_SHOOT_RANGE;

	UpdateActivityStatus = [&](Character const& player) {
		UpdateClownActivityStatus(player);
	};

	Pursuit = [&](Character const& player, std::vector<Bullet*>& bullets, std::vector<Object> const& blocks) {
		(void)blocks;
		(void)player;
		ClownShoot(bullets);
	};

	Idle = [&](float elapsedTime, std::vector<Object> const& blocks) {
		(void)blocks;
		(void)elapsedTime;
	};
}

void Enemy::CreateGhost()
{
	enemyType = EnemyType::GHOST;

	bodyTexture.loadFromFile("resources/enemyGhost.png");
	bodyShape.setSize(GHOST_SIZE);

	health = GHOST_START_HEALTH;
	touchDemage = GHOST_TOUCH_DEMAGE;
	moveSpeed = GHOST_MOVE_SPEED;

	UpdateActivityStatus = [&](Character const& player) {
		UpdateGhostActivityStatus(player);
	};

	Pursuit = [&](Character const& player, std::vector<Bullet*>& bullets, std::vector<Object> const& blocks) {
		(void)bullets;
		(void)blocks;
		GhostPursuite(player);
	};

	Idle = [&](float elapsedTime, std::vector<Object> const& blocks) {
		(void)blocks;
		GhostIdle(elapsedTime);
	};
}

void Enemy::CreateSpider()
{
	enemyType = EnemyType::SPIDER;

	bodyTexture.loadFromFile("resources/enemySpider.png");
	bodyShape.setSize(SPIDER_SIZE);

	health = SPIDER_START_HEALTH;
	touchDemage = SPIDER_TOUCH_DEMAGE;
	moveSpeed = SPIDER_MOVE_SPEED;
	jumpSpeed = SIDER_JUMP_SPEED;

	UpdateActivityStatus = [&](Character const& player) {
		UpdateSpiderActivityStatus(player);
	};

	Pursuit = [&](Character const& player, std::vector<Bullet*>& bullets, std::vector<Object> const& blocks) {
		(void)bullets;
		SpiderPursuite(player, blocks);
	};

	Idle = [&](float elapsedTime, std::vector<Object> const& blocks) {
		(void)blocks;
		(void)elapsedTime;
	};

	const sf::Vector2i BODY_SIZE = {
		static_cast<int>(bodyShape.getSize().x),
		static_cast<int>(bodyShape.getSize().y)
	};
	bodyShape.setTextureRect(sf::IntRect(0, BODY_SIZE.y, BODY_SIZE.x, BODY_SIZE.y));
}

void Enemy::CreateBoss()
{
	enemyType = EnemyType::BOSS;

	bodyTexture.loadFromFile("resources/enemyBoss.png");
	bodyShape.setSize(BOSS_SIZE);

	health = BOSS_START_HEALTH;
	shootDemage = BOSS_SHOOT_DEMAGE;
	touchDemage = BOSS_TOUCH_DEMAGE;
	shootRange = BOSS_TARGET_RANGE;

	UpdateActivityStatus = [&](Character const& player) {
		UpdateBossActivityStatus(player);
	};

	Pursuit = [&](Character const& player, std::vector<Bullet*>& bullets, std::vector<Object> const& blocks) {
		BossPursuite(player, bullets);
		(void)bullets;
		(void)player;
		(void)blocks;
	};

	Idle = [&](float elapsedTime, std::vector<Object> const& blocks) {
		(void)blocks;
		(void)elapsedTime;
	};
}

void Enemy::UpdateAI(float elapsedTime, Character const& player, std::vector<Object> const& blocks, std::vector<Bullet*>& bullets)
{
	UpdateHealthStatus();
	UpdateActivityStatus(player);

	if (activityStatus == EnemyActivity::IDLE)
	{
		Idle(elapsedTime, blocks);
	}
	else
	{
		Pursuit(player, bullets, blocks);
	}

	UpdateOrientation();

	if (enemyType == EnemyType::GHOST)
	{
		UpdateGhostPos(elapsedTime);
	}
	else if (enemyType == EnemyType::SPIDER && activityStatus == EnemyActivity::IDLE)
	{
		UpdateSpiderPos(elapsedTime);
	}
	else
	{
		UpdatePos(elapsedTime, blocks);
	}

	if (!(enemyType == EnemyType::SPIDER && activityStatus == EnemyActivity::IDLE))
	{
		UpdateTexture();
	}

	UpdateHands();
}

void Enemy::UpdateShadowActivityStatus(Character const& player)
{
	moveSpeed = SHADOW_MOVE_SPEED;
	runStatus = currentRunStatus;
	bodyShape.setFillColor(sf::Color(255, 255, 255, 255));

	this->activityStatus = EnemyActivity::IDLE;

	const sf::Vector2f RIGHT_AREA_POS = GetCharacterPos() + SHADOW_TARGET_AREA_MARGIN;
	const sf::Vector2f LEFT_MARGIN = sf::Vector2f(-SHADOW_TARGET_AREA_SIZE.x, 0);
	const sf::Vector2f LEFT_AREA_POS = GetCharacterPos() + SHADOW_TARGET_AREA_MARGIN + LEFT_MARGIN;

	if (orientationStatus == OrientationStatus::RIGHT)
	{
		targetArea = sf::FloatRect(RIGHT_AREA_POS, SHADOW_TARGET_AREA_SIZE);
	}
	else
	{
		targetArea = sf::FloatRect(LEFT_AREA_POS, SHADOW_TARGET_AREA_SIZE);
	}

	if (targetArea.intersects(player.collisionRect))
	{
		moveSpeed = SHADOW_PURSUITE_MOVE_SPEED;
		bodyShape.setFillColor(sf::Color::Red);
	}
}

void Enemy::UpdateClownActivityStatus(Character const& player)
{
	activityStatus = EnemyActivity::IDLE;

	const float TARGET_AREA_POS_X = GetCharacterPos().x - CLOWN_TARGET_RANGE;
	const float TARGET_AREA_POS_Y = GetCharacterPos().y - 1.5f * CLOWN_TARGET_AREA_SIZE.y;
	sf::Vector2f AREA_POSITION(TARGET_AREA_POS_X, TARGET_AREA_POS_Y);

	targetArea = sf::FloatRect(AREA_POSITION, CLOWN_TARGET_AREA_SIZE);

	if (player.collisionRect.intersects(targetArea))
	{
		activityStatus = EnemyActivity::PURSUIT;
		if (player.GetCharacterPos().x < GetCharacterPos().x)
		{
			orientationStatus = OrientationStatus::LEFT;
		}
		else
		{
			orientationStatus = OrientationStatus::RIGHT;
		}
	}
}

void Enemy::UpdateGhostActivityStatus(Character const& player)
{
	activityStatus = EnemyActivity::IDLE;

	sf::Vector2f playerPos = player.GetCharacterPos();

	float rangeX = playerPos.x - collisionRect.left;
	float rangeY = playerPos.y - collisionRect.top;

	float targetRadius = sqrt(pow(rangeX, 2) + pow(rangeY, 2));

	if (targetRadius <= GHOST_TARGET_RANGE)
	{
		activityStatus = EnemyActivity::PURSUIT;
	}
}

void Enemy::UpdateBossActivityStatus(Character const& player)
{
	activityStatus = EnemyActivity::PURSUIT;
	(void)player;
}

void Enemy::UpdateSpiderActivityStatus(Character const& player)
{
	auto rangeX = abs(player.GetCharacterPos().x - GetCharacterPos().x);
	auto rangeY = abs(player.GetCharacterPos().y - GetCharacterPos().y);

	auto range = sqrt(pow(rangeX, 2) + pow(rangeY, 2));

	if (range <= SPIDER_TARGET_RANGE)
	{
		activityStatus = EnemyActivity::PURSUIT;
	}
}

void Enemy::UpdateSpiderPos(float elapsedTime)
{
	(void)elapsedTime;
	bodyShape.setPosition(GetCharacterPos());
}

void Enemy::SpiderPursuite(Character const& player, std::vector<Object> const& blocks)
{
	auto targetPosX = player.GetCharacterPos().x;

	if (targetPosX < GetCharacterPos().x)
	{
		runStatus = MovementStatus::RUN_LEFT;

		if (!IsCollidesWithLevel(handLeftBottom, blocks) ||
			IsCollidesWithLevel(handLeftMiddle, blocks))
		{
			Jump();
		}
	}
	else if (targetPosX > GetCharacterPos().x)
	{
		runStatus = MovementStatus::RUN_RIGHT;

		if (!IsCollidesWithLevel(handRightBottom, blocks) ||
			IsCollidesWithLevel(handRightMiddle, blocks))
		{
			Jump();
		}
	}
}

void Enemy::ShadowWalk(float elapsedTime, std::vector<Object> const& blocks)
{
	if (jumpStatus != JumpingStatus::FLY)
	{
		sf::FloatRect handLeftMiddle_copy = handLeftMiddle;
		sf::FloatRect handLeftBottom_copy = handLeftBottom;

		sf::FloatRect handRightMiddle_copy = handRightMiddle;
		sf::FloatRect handRightBottom_copy = handRightBottom;

		auto mevoment = elapsedTime * moveSpeed;

		if (runStatus == MovementStatus::RUN_LEFT)
		{
			handLeftMiddle_copy.left -= mevoment;
			handLeftBottom_copy.left -= mevoment;

			if (IsCollidesWithLevel(handLeftMiddle_copy, blocks) ||
				!IsCollidesWithLevel(handLeftBottom_copy, blocks))
			{
				currentRunStatus = MovementStatus::RUN_RIGHT;
			}
		}
		else if (runStatus == MovementStatus::RUN_RIGHT)
		{
			handRightMiddle_copy.left += mevoment;
			handRightBottom_copy.left += mevoment;

			if (IsCollidesWithLevel(handRightMiddle_copy, blocks) ||
				!IsCollidesWithLevel(handRightBottom_copy, blocks))
			{
				currentRunStatus = MovementStatus::RUN_LEFT;
			}
		}
	}
}

void Enemy::ClownShoot(std::vector<Bullet*>& bullets)
{
	int orientationId = static_cast<int>(orientationStatus);

	if (shootColdown >= CLOWN_SHOOT_COLDOWN)
	{
		auto bullet = new Bullet(GetCharacterPos(), shootDemage, orientationId, shootRange, BulletType::CLOWN_BULLET);
		bullets.push_back(bullet);
		shootColdown = 0;
	}
}

void Enemy::UpdateGhostPos(float elapsedTime)
{
	if (ghostMove.x < 0)
	{
		runStatus = MovementStatus::RUN_LEFT;
	}
	else if (ghostMove.x > 0)
	{
		runStatus = MovementStatus::RUN_RIGHT;
	}

	collisionRect.left += elapsedTime * moveSpeed * ghostMove.x;
	collisionRect.top += elapsedTime * moveSpeed * ghostMove.y;

	bodyShape.setPosition(GetCharacterPos());
}

void Enemy::GhostIdle(float elapsedTime)
{
	(void)elapsedTime;
}

void Enemy::GhostPursuite(Character const& player)
{
	sf::Vector2f playerPos = player.GetCharacterPos();

	float halfPlayerBody = player.bodyShape.getSize().y / 2.0f;
	
	ghostMove = { 0 , 0 };

	if (playerPos.x < GetCharacterPos().x)
	{
		ghostMove.x = -1;
	}
	else if (playerPos.x > GetCharacterPos().x)
	{
		ghostMove.x = 1;
	}

	if (playerPos.y - halfPlayerBody < GetCharacterPos().y)
	{
		ghostMove.y = -1;
	}
	else if (playerPos.y - halfPlayerBody > GetCharacterPos().y)
	{
		ghostMove.y = 1;
	}
}

void Enemy::BossPursuite(Character const& player, std::vector<Bullet*>& bullets)
{
	(void)player;
	sf::Vector2f AREA_SIZE(BOSS_TARGET_RANGE, BOSS_SIZE.y);
	sf::Vector2f AREA_POS(GetCharacterPos() - AREA_SIZE);

	targetArea = sf::FloatRect(AREA_POS, AREA_SIZE);

	if (player.collisionRect.intersects(targetArea))
	{
		BossAttack(bullets, player.GetCharacterPos());
	}
}

void Enemy::BossIdle()
{

}

void Enemy::BossAttack(std::vector<Bullet*>& bullets, sf::Vector2f const& targetPos)
{
	if (shootColdown >= BOSS_SHOOT_COLDOWN)
	{
		sf::Vector2f bulletStartPosMirror = { GetCharacterPos().x , targetPos.y };
		bullets.push_back(new Bullet(bulletStartPosMirror, shootDemage, 1, shootRange, BulletType::BOSS_BULLET));
		shootColdown = 0;
	}
}

void Enemy::UpdateHands()
{
	const float LEFT_HAND_POS_X = GetCharacterPos().x - bodyShape.getSize().x / 2.0f;
	const float RIGHT_HAND_POS_X = GetCharacterPos().x + bodyShape.getSize().x / 2.0f;
	const float BOTTOM_HAND_POS_Y = GetCharacterPos().y + BOTTOM_HAND_MARGIN;
	const float MIDDLE_HAND_POS_Y = GetCharacterPos().y - bodyShape.getSize().y / 4.0f;

	handLeftMiddle = sf::FloatRect({ LEFT_HAND_POS_X, MIDDLE_HAND_POS_Y }, HAND_SIZE);
	handLeftBottom = sf::FloatRect({ LEFT_HAND_POS_X, BOTTOM_HAND_POS_Y }, HAND_SIZE );

	handRightMiddle = sf::FloatRect({ RIGHT_HAND_POS_X, MIDDLE_HAND_POS_Y }, HAND_SIZE);
	handRightBottom = sf::FloatRect({ RIGHT_HAND_POS_X, BOTTOM_HAND_POS_Y }, HAND_SIZE);
}