#include "game.h"

using namespace std;
using namespace sf;

enum
{
	// Start menu buttons
	START_MENU_START = 0,
	START_MENU_OPTIONS,
	START_MENU_CLOSE,
	// Options menu buttons
	OPTIONS_MENU_EASY = 0,
	OPTIONS_MENU_NORMAL,
	OPTIONS_MENU_HARD,
	OPTIONS_MENU_BACK,
	// Pause menu buttons
	PAUSE_MENU_RESUME = 0,
	PAUSE_MENU_RESTART,
	PAUSE_MENU_EXIT,
};

template <class TContainer, class TPredicate>
void erase_if(TContainer &container, TPredicate && predicate)
{
	auto newEnd = std::remove_if(std::begin(container), std::end(container), [&](auto &pBullet) {
		return predicate(pBullet);
	});
	container.erase(newEnd, container.end());
}

Game::Game(float width, float height)
	:resolution(width, height)
	,interface(width, height)
	,menu(width, height)
{
	level_0.LoadFromFile("resources/previewTileset.tmx");
	level_1.LoadFromFile("resources/secondTileset.tmx");
	level_2.LoadFromFile("resources/firstTileset.tmx");
	
	backgroundTexture_level_0.loadFromFile("resources/background_level_preview.png");
	backgroundTexture_level_1.loadFromFile("resources/background_level_2.png");

	player.InitPlayer();
	InitGameSound();

	camera.reset(sf::FloatRect(0, 0, resolution.x, resolution.y));

	difficult = Difficult::EASY;
	menu.Select(CurrentMenu::DIFFICULT, difficult);
	menu.SetMenu(CurrentMenu::START, camera.getCenter());

	const sf::Vector2f BG_LVL_0_SISE = GetTextureSize(backgroundTexture_level_0);
	const sf::Vector2f BG_LVL_1_SISE = GetTextureSize(backgroundTexture_level_1);

	background_level_1.setTexture(&backgroundTexture_level_1);
	background_level_1.setSize(BG_LVL_1_SISE);
	background_level_1.setOrigin(BG_LVL_1_SISE.x / 2.0f, BG_LVL_1_SISE.y / 2.0f);

	background_level_0.setTexture(&backgroundTexture_level_0);
	background_level_0.setSize(BG_LVL_0_SISE);
	background_level_0.setOrigin(BG_LVL_0_SISE.x / 2.0f, BG_LVL_0_SISE.y / 2.0f);

	changeLevelMap = {
		{ &level_0, &level_1 },
		{ &level_1, &level_2 },
		{ &level_2, &level_0 }
	};

	boxesCountMap = {
		{ &level_0, LEVEL_0_BOXES_COUNT },
		{ &level_1, LEVEL_1_BOXES_COUNT },
		{ &level_2, LEVEL_2_BOXES_COUNT }
	};

	sceneMusicMap = {
		{ &menuScene, &menuSound },
		{ &gameplayScene, nullptr },
		{ &pauseScene, &menuSound },
		{ &gameOverScene, &gameOverSound },
		{ &previewScene, &menuSound },
		{ &winScene, &winSound }
	};

	levelMusicMap = {
		{ &level_0, &level_0_ost },
		{ &level_1, &level_1_ost },
		{ &level_2, &level_2_ost }
	};

	buttonColdown = 0;
}

void Game::StartGame()
{
	currentLevel = &level_0;

	ClearScene();
	GetMapData();

	player.InitPlayer();
	DifficultAdjustment();
	SpawnEntities();

	currentScene = &gameplayScene;
}

void Game::Restart()
{
	ClearScene();

	GetMapData();

	player.ReturnCopy();
	SpawnEntities();

	currentScene = &gameplayScene;
}

void Game::NextLevel(TmxLevel& level)
{
	ClearScene();
	player.Clear();
	player.CreateCopy();

	currentLevel = &level;
	GetMapData();

	SpawnEntities();
}

void Game::DifficultAdjustment()
{
	switch (difficult)
	{
	case Difficult::EASY:
	{
		bonusProbability = EASY_BONUS_PROBABILITY;
		demageIncrease = EASY_DEMAGE_INCREASE;
		break;
	}

	case Difficult::NORMAL:
	{
		bonusProbability = NORMAL_BONUS_PROBABILITY;
		demageIncrease = NORMAL_DEMAGE_INCREASE;
		break;
	}

	case Difficult::HARD:
	{
		bonusProbability = HARD_BONUS_PROBABILITY;
		demageIncrease = HARD_DEMAGE_INCREASE;
		break;
	}
	default:
		break;
	}
}

void Game::GetMapData()
{
	mapSize = currentLevel->GetTilemapSize();
	interface.CreateBoxes(boxesCountMap, currentLevel);
	blocks = currentLevel->GetAllObjects("solid");
	lava = currentLevel->GetAllObjects("lava");
}

void Game::CheckCompletedLevel()
{
	const sf::FloatRect PLAYER_BODY = player.collisionRect;
	const sf::FloatRect LEVEL_DOOR_RECT = currentLevel->GetFirstObject("next_level").rect;

	const int NECESSARY_BOXES_COUNT = boxesCountMap.find(currentLevel)->second;
	const int PLAYER_BOXES_COUNT = player.boxes;

	TmxLevel* NEXT_LEVEL = changeLevelMap.find(currentLevel)->second;
	TmxLevel* START_LEVEL = &level_0;

	if (PLAYER_BODY.intersects(LEVEL_DOOR_RECT) && PLAYER_BOXES_COUNT >= NECESSARY_BOXES_COUNT)
	{
		if (NEXT_LEVEL == START_LEVEL)
		{
			currentScene = &winScene;
		}

		NextLevel(*NEXT_LEVEL);
	}
}

void Game::ClearScene()
{
	enemies.clear();
	bonuses.clear();
	enemyBullets.clear();
	delete interface.remark;
	interface.demageAnnouncementText.clear();
	interface.demageAnnouncementDuration.clear();

	interface.remark = nullptr;
}

void Game::SpawnEntities()
{
	const std::vector<TmxObject> SHADOW_SPAWNS = currentLevel->GetAllObjects("enemy_shadow_spawn");
	const std::vector<TmxObject> CLOWN_SPAWNS = currentLevel->GetAllObjects("enemy_clown_spawn");
	const std::vector<TmxObject> GHOST_SPAWNS = currentLevel->GetAllObjects("enemy_bird_spawn");
	const std::vector<TmxObject> SPIDER_SPAWNS = currentLevel->GetAllObjects("enemy_spider_spawn");
	const std::vector<TmxObject> BOSS_SPAWNS = currentLevel->GetAllObjects("enemy_boss_spawn");

	const std::vector<TmxObject> BOX_SPAWNS = currentLevel->GetAllObjects("item_box_spawn");
	const std::vector<TmxObject> HEALTH_SPAWNS = currentLevel->GetAllObjects("bonus_heath");
	const std::vector<TmxObject> AK_AMMO_SPAWNS = currentLevel->GetAllObjects("bonus_ak_ammo");
	const std::vector<TmxObject> GIFT_SPAWNS = currentLevel->GetAllObjects("bonus_random");
	const std::vector<TmxObject> SHOOTGUN_AMMO_SPAWNS = currentLevel->GetAllObjects("bonus_shootgun_ammo");

	SpawnEnemies(SHADOW_SPAWNS, EnemyType::SHADOW);
	SpawnEnemies(CLOWN_SPAWNS, EnemyType::CLOWN);
	SpawnEnemies(GHOST_SPAWNS, EnemyType::GHOST);
	SpawnEnemies(SPIDER_SPAWNS, EnemyType::SPIDER);
	SpawnEnemies(BOSS_SPAWNS, EnemyType::BOSS);

	SpawnItems(BOX_SPAWNS, BonusType::ITEM_BOX);
	SpawnItems(HEALTH_SPAWNS, BonusType::HEALTH);
	SpawnItems(AK_AMMO_SPAWNS, BonusType::AK_AMMO);
	SpawnItems(GIFT_SPAWNS, BonusType::GIFT);
	SpawnItems(SHOOTGUN_AMMO_SPAWNS, BonusType::SHOOTGUN_AMMO);

	const sf::Vector2f PLAYER_POS = currentLevel->GetFirstObject("player_spawn").sprite.getPosition();
	player.Spawn(PLAYER_POS);
}

void Game::SpawnItems(std::vector<TmxObject> const& spawns, BonusType const& type)
{
	for (auto const& spawn : spawns)
	{
		sf::Vector2f pos = spawn.sprite.getPosition();
		bonuses.push_back(new Bonus(pos, type));
	}
}

void Game::SpawnEnemies(std::vector<TmxObject> const& spawns, EnemyType const& type)
{
	for (auto const& spawn : spawns)
	{
		sf::Vector2f pos = spawn.sprite.getPosition();
		enemies.push_back(new Enemy(pos, type, demageIncrease));
	}
}

void Game::SetElapsedTime()
{
	elapsedTime = clock.getElapsedTime().asSeconds();
	clock.restart();
}

sf::FloatRect Game::GetCameraArea()
{
	auto camCenter = camera.getCenter();
	sf::FloatRect cameraArea(
		camCenter.x - resolution.x / 2.0f,
		camCenter.y - resolution.y / 2.0f, resolution.x, resolution.y
	);
	
	return cameraArea;
}

bool Game::IsCollidesWithLevel(sf::FloatRect const& rect)
{
	return std::any_of(blocks.begin(), blocks.end(), [&](const TmxObject&block) {
		return (rect.intersects(block.rect) && block.name == "solid");
	});
}

void Game::ControlPlayer()
{
	if (Keyboard::isKeyPressed(Keyboard::Escape) && menu.buttonsColdown >= BUTTONS_COLDOWN)
	{
		menu.SetMenu(CurrentMenu::PAUSE, camera.getCenter());
		currentScene = &menuScene;
	}
	else if (player.existStatus != ExistenceStatus::DEAD)
	{
		if (Keyboard::isKeyPressed(Keyboard::Space))
		{
			player.Jump();
		}
		if (Keyboard::isKeyPressed(Keyboard::A) ||
			Keyboard::isKeyPressed(Keyboard::Left))
		{
			player.runStatus = RUN_LEFT;
		}
		if (Keyboard::isKeyPressed(Keyboard::D) ||
			Keyboard::isKeyPressed(Keyboard::Right))
		{
			player.runStatus = RUN_RIGHT;
		}
		if (Keyboard::isKeyPressed(Keyboard::O) && buttonColdown >= BUTTONS_COLDOWN)
		{
			buttonColdown = 0;
			player.SwitchWeapon();
		}
		if (Keyboard::isKeyPressed(Keyboard::P))
		{
			player.Attack();
		}
	}
}

void Game::ControlMenu(sf::RenderWindow& window)
{
	if (Keyboard::isKeyPressed(Keyboard::Escape) &&
		menu.buttonsColdown >= BUTTONS_COLDOWN &&
		menu.currentMenu == CurrentMenu::PAUSE)
	{
		menu.buttonsColdown = 0;
		currentScene = &gameplayScene;
	}
	else if ((Keyboard::isKeyPressed(Keyboard::F) || Keyboard::isKeyPressed(Keyboard::Return)) &&
			menu.buttonsColdown >= BUTTONS_COLDOWN)
	{
		menu.buttonsColdown = 0;
		ControlMenuLogic(window);
	}
	else if (Keyboard::isKeyPressed(Keyboard::Up) &&
			menu.buttonsColdown >= BUTTONS_COLDOWN)
	{
		menu.SwitchButtonUp();
		menuButtonSwitchSound.play();
	}
	else if (Keyboard::isKeyPressed(Keyboard::Down) &&
		menu.buttonsColdown >= BUTTONS_COLDOWN)
	{
		menu.SwitchButtonDown();
		menuButtonSwitchSound.play();
	}
}

void Game::ControlMenuLogic(sf::RenderWindow& window)
{
	switch (menu.currentMenu)
	{
	case CurrentMenu::START:
		switch (menu.currentButton)
		{
		case START_MENU_START:
			StartGame();
			break;
		case START_MENU_OPTIONS:
			menu.SetMenu(CurrentMenu::DIFFICULT, camera.getCenter());
			break;
		case START_MENU_CLOSE:
			window.close();
			break;
		default:
			break;
		}
		menu.currentButton = 0;
		break;
	case CurrentMenu::DIFFICULT:
		switch (menu.currentButton)
		{
		case OPTIONS_MENU_EASY:
			difficult = Difficult::EASY;
			break;
		case OPTIONS_MENU_NORMAL:
			difficult = Difficult::NORMAL;
			break;
		case OPTIONS_MENU_HARD:
			difficult = Difficult::HARD;
			break;
		case OPTIONS_MENU_BACK:
			menu.SetMenu(CurrentMenu::START, camera.getCenter());
			break;
		default:
			break;
		}
		menu.Select(CurrentMenu::DIFFICULT, difficult);
		break;

	case CurrentMenu::PAUSE:
		switch (menu.currentButton)
		{
		case PAUSE_MENU_RESUME:
			currentScene = &gameplayScene;
			break;
		case PAUSE_MENU_RESTART:
			Restart();
			break;
		case PAUSE_MENU_EXIT:
			menu.SetMenu(CurrentMenu::START, camera.getCenter());
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

void Game::ControlGameOver(sf::RenderWindow& window)
{
	(void)window;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::R))
	{
		Restart();
	}
	if (sf::Keyboard::isKeyPressed(Keyboard::Escape))
	{
		menu.SetMenu(CurrentMenu::START, camera.getCenter());
		currentScene = &menuScene;
	}
}

void Game::UpdatePlayer()
{
	player.UpdateOrientation();
	player.UpdatePos(elapsedTime, blocks);
	player.UpdateHealthStatus();
	player.UpdateStatuses();
	player.UpdateTexture(elapsedTime);

	for (auto blood : player.wounds)
	{
		blood->Update(player.GetCharacterPos(), elapsedTime);
	}
	erase_if(player.wounds, [&](Blood* blood) {
		bool durationEnd = blood->duration >= BLOOD_DURATION;
		if (durationEnd)
			delete(blood);

		return durationEnd;
	});

	if (player.existStatus == ExistenceStatus::DEAD)
	{
		interface.CreateRemark(RemarkType::DEATH);
		player.RotateDeadBody(elapsedTime);

		if (gameOverColdown >= GAME_OVER_DURATION)
		{
			gameOverColdown = 0;
			currentScene = &gameOverScene;
		}
	}

	CheckCompletedLevel();
}

void Game::UpdateBullets()
{
	auto updateBullets = [&](vector<Bullet*> &bullets) {
		for (Bullet* pBullet : bullets)
		{
			pBullet->Update(elapsedTime);
		}
		erase_if(bullets, [&](Bullet* pBullet) {
			bool dead = (IsCollidesWithLevel(pBullet->collisionRect) || !pBullet->isLive);
			if (dead) 
				delete(pBullet);

			return dead;
		});
	};

	updateBullets(player.characterBullets);
	updateBullets(enemyBullets);
}

void Game::UpdateEnemies()
{
	for (auto it = enemies.begin(); it != enemies.end();)
	{
		Enemy* enemy = *it;
		enemy->UpdateAI(elapsedTime, player, blocks, enemyBullets);

		for (auto blood : enemy->wounds)
		{
			blood->Update(enemy->GetCharacterPos(), elapsedTime);
		}
		erase_if(enemy->wounds, [&](Blood* blood) {
			bool durationEnd = blood->duration >= BLOOD_DURATION;
			if (durationEnd)
				delete(blood);

			return durationEnd;
		});

		if (enemy->existStatus == ExistenceStatus::DEAD)
		{
			interface.CreateRemark(RemarkType::KILL);
			if (enemy->enemyType == EnemyType::BOSS)
			{
				bonuses.push_back(new Bonus(enemy->GetCharacterPos(), BonusType::ITEM_BOX));
			}
			else
			{
				DropBonusFromEnemy(enemy->GetCharacterPos(), bonuses, bonusProbability);
			}
			it = enemies.erase(it);
			delete(enemy);
		}
		else
		{
			++it;
		}
	}
}

void Game::UpdateBonuses()
{
	for (auto bonus : bonuses)
	{
		bonus->Update(elapsedTime, blocks);
	}
}

void Game::UpdateOST()
{
	sf::Music* OST;

	if (currentScene == &gameplayScene)
	{
		OST = levelMusicMap.find(currentLevel)->second;
	}
	else
	{
		OST = sceneMusicMap.find(currentScene)->second;
	}

	if (OST != currentMusic)
	{
		if (currentMusic != nullptr)
		{
			currentMusic->stop();
		}

		currentMusic = OST;
	}

	if (currentMusic->getStatus() != sf::Music::Playing)
	{
		currentMusic->play();
	}
}

void Game::CheckEntitiesCollides()
{
	EnemyPlayerCollides();
	PlayerBulletsEnemyCollides();
	EnemyBulletsPlayerCollides();
	BonusesPlayerCollides();
	PlayerLavaCollides();
}

void Game::EnemyBulletsPlayerCollides()
{
	for (auto bullet : enemyBullets)
	{
		if (player.collisionRect.intersects(bullet->collisionRect))
		{
			if (player.injuredColdown >= INJURED_DURATION)
			{
				PlayWithoutDouble(player.playerHurtGrunt);
				player.health -= bullet->demage;
				player.wounds.push_back(new Blood(player.GetCharacterPos(), bullet->bodyShape.getPosition()));
				bullet->isLive = false;
			}
		}
	}
}

void Game::PlayerLavaCollides()
{
	for (auto lavaBlock : this->lava)
	{
		if (lavaBlock.rect.intersects(player.collisionRect))
		{
			if (player.injuredColdown >= INJURED_DURATION)
			{
				PlayWithoutDouble(player.playerHurtGrunt);
				player.health -= LAVA_DEMAGE;
				player.injuredColdown = 0;
			}
		}
	}
}

void Game::BonusesPlayerCollides()
{
	for (auto it = bonuses.begin(); it != bonuses.end();)
	{
		Bonus* bonus = *it;
		if (bonus->collisionRect.intersects(player.collisionRect) && bonus->AddBonusEffect(player))
		{
			if (bonus->bonusType == BonusType::GIFT)
				interface.CreateRemark(RemarkType::GIFT);
			else
				interface.CreateRemark(RemarkType::BONUS);

			const sf::Vector2f BONUS_POSITION(bonus->bodyShape.getPosition());
			interface.CreateAnnouncement(BONUS_POSITION, bonus->announcementText);
			CollideWithBonusSound(static_cast<int>(bonus->bonusType));

			it = bonuses.erase(it);
			delete(bonus);
		}
		else
		{
			++it;
		}
	}
}

void Game::PlayerBulletsEnemyCollides()
{
	for (auto bullet : player.characterBullets)
	{
		for (auto enemy : enemies)
		{
			if (enemy->collisionRect.intersects(bullet->collisionRect))
			{
				const std::string dmgStr = "-" + to_string(bullet->demage);
				enemy->health -= bullet->demage;
				enemy->activityStatus = EnemyActivity::PURSUIT;
				enemy->wounds.push_back(new Blood(enemy->GetCharacterPos(), bullet->bodyShape.getPosition()));
				interface.CreateAnnouncement(bullet->bodyShape.getPosition(), dmgStr);
				bullet->isLive = false;
			}
		}
	}
}

void Game::EnemyPlayerCollides()
{
	for (auto enemy : enemies)
	{
		if (enemy->collisionRect.intersects(player.collisionRect))
		{
			if (player.injuredColdown >= INJURED_DURATION)
			{
				PlayWithoutDouble(player.playerHurtGrunt);
				CollideWithEnemySound((int)enemy->enemyType);
				player.health -= enemy->touchDemage;
				player.injuredColdown = 0;
			}
		}
	}
}

void Game::UpdateColdowns()
{
	auto AddTime = [&](float& timer, float limit, bool criterion = true) {
		if (timer <= limit && criterion) { timer += elapsedTime; }
	};

	AddTime(player.shootColdown, MAX_WEAPON_COLDOWN);
	AddTime(player.injuredColdown, INJURED_DURATION);
	AddTime(menu.buttonsColdown, BUTTONS_COLDOWN);
	AddTime(buttonColdown, BUTTONS_COLDOWN);
	AddTime(interface.randomRemarkColdown, REMARK_RANDOM_COLDOWN);
	AddTime(gameOverColdown, GAME_OVER_DURATION, player.existStatus == ExistenceStatus::DEAD);

	for (auto enemy : enemies)
	{
		AddTime(enemy->shootColdown, CLOWN_SHOOT_COLDOWN, enemy->enemyType == EnemyType::CLOWN);
		AddTime(enemy->shootColdown, BOSS_SHOOT_COLDOWN, enemy->enemyType == EnemyType::BOSS);
	}
}

void Game::UpdateCamera(sf::RenderWindow& window)
{
	sf::Vector2f halfWindow = { resolution.x / 2.0f , resolution.y / 2.0f };
	sf::Vector2f cameraCenter = {
		player.GetCharacterPos().x,
		player.GetCharacterPos().y - CAMERA_VERTICAL_MARGIN
	};
	camera.setCenter(cameraCenter);

	if (cameraCenter.x - halfWindow.x < 0)
	{
		camera.setCenter(halfWindow.x, cameraCenter.y);
		cameraCenter = camera.getCenter();
	}
	if (cameraCenter.x + halfWindow.x > mapSize.x)
	{
		camera.setCenter(mapSize.x - halfWindow.x, cameraCenter.y);
		cameraCenter = camera.getCenter();
	}
	if (cameraCenter.y - halfWindow.y < 0)
	{
		camera.setCenter(cameraCenter.x, halfWindow.y);
		cameraCenter = camera.getCenter();
	}
	if (cameraCenter.y + halfWindow.y > mapSize.y)
	{
		camera.setCenter(cameraCenter.x, mapSize.y - halfWindow.y);
		cameraCenter = camera.getCenter();
	}

	window.setView(camera);
}

void Game::UpdateInterface()
{
	const int AMMO = player.ammoMap.find(player.currentWeapon)->second;

	interface.UpdateBarsPos(camera.getCenter());
	interface.UpdatePlayerHP(player.health);
	interface.UpdatePlayerWeapon(player.currentWeapon, AMMO);
	interface.UpdatePlayerBoxes(player.boxes);
	interface.UpdateAnnouncement(elapsedTime);
	interface.UpdateRemark(elapsedTime, player.GetCharacterPos());

	if (interface.randomRemarkColdown >= REMARK_RANDOM_COLDOWN)
	{
		interface.CreateRemark(RemarkType::RANDOM);
		interface.randomRemarkColdown = 0;
	}

	for (auto enemy : enemies)
	{
		if (enemy->enemyType == EnemyType::BOSS)
		{
			interface.UpdateBossBar(BOSS_START_HEALTH, enemy->health);
		}
	}
}

void Game::UpdateBackground()
{
	auto mapWidth = currentLevel->GetTilemapWidth();
	auto mapHeight = currentLevel->GetTilemapHeight();
	auto backgroundWidth = currentBackground.getSize().x;
	auto backgroundHeight = currentBackground.getSize().y;

	if (currentLevel == &level_0)
	{
		currentBackground = background_level_0;
	}
	else
	{
		currentBackground = background_level_1;
	}

	float bgPosX_Percent = (camera.getCenter().x - resolution.x / 2.0f) / (mapWidth - resolution.x);
	float bgPosY_Percent = (camera.getCenter().y - resolution.y / 2.0f) / (mapHeight - resolution.y);

	float bgAllowedWidthX = mapWidth - backgroundWidth;
	float bgAllowedWidthY = mapHeight - backgroundHeight;

	float bgPosX_Pixel = backgroundWidth / 2.0f + bgPosX_Percent * bgAllowedWidthX;
	float bgPosY_Pixel = backgroundHeight / 2.0f + bgPosY_Percent * bgAllowedWidthY;

	currentBackground.setPosition(bgPosX_Pixel, bgPosY_Pixel);
}

void Game::DrawLevel(sf::RenderWindow& window)
{
	currentLevel->Draw(window, GetCameraArea());
}

void Game::DrawBullets(sf::RenderWindow& window) // player bullets generally in window
{
	for (auto const& playerBullet : player.characterBullets)
	{
		window.draw(playerBullet->bodyShape);
	}

	for (auto const& bullet : enemyBullets)
	{
		if (GetCameraArea().intersects(bullet->collisionRect))
		{
			window.draw(bullet->bodyShape);
		}
	}
}

void Game::DrawEnemies(sf::RenderWindow& window)
{
	for (auto const& enemy : enemies)
	{
		if (GetCameraArea().intersects(enemy->collisionRect))
		{
			enemy->DrawCharacter(window);
		}
	}
}

void Game::DrawBonuses(sf::RenderWindow& window)
{
	for (auto const& bonus : bonuses)
	{
		if (GetCameraArea().intersects(bonus->collisionRect))
		{
			bonus->DrawBonus(window);
		}
	}
}

void Game::DrawInterface(sf::RenderWindow& window)
{
	interface.Draw(window);

	for (auto const& enemy : enemies)
	{
		if (enemy->enemyType == EnemyType::BOSS)
		{
			interface.DrawBossBar(window);
		}
	}
}