#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

using namespace geode::prelude;

// Base cube speed at 1x, in units per second.
static constexpr float BASE_SPEED = 311.58f;

class $modify(AutoPlayLayer, PlayLayer) {
	struct Fields {
		std::vector<CCRect> hazards; // sorted by left edge
		size_t idx = 0;              // first hazard still ahead of the player
		bool holding = false;
	};

	void setupHasCompleted() {
		PlayLayer::setupHasCompleted();

		auto& hz = m_fields->hazards;
		hz.clear();

		for (auto obj : CCArrayExt<GameObject*>(m_objects)) {
			if (obj && obj->m_objectType == GameObjectType::Hazard) {
				hz.push_back(obj->getObjectRect());
			}
		}
		std::sort(hz.begin(), hz.end(), [](CCRect const& a, CCRect const& b) {
			return a.getMinX() < b.getMinX();
		});
		m_fields->idx = 0;
		m_fields->holding = false;
	}

	void resetLevel() {
		PlayLayer::resetLevel();
		m_fields->idx = 0;
		m_fields->holding = false;
	}

	void postUpdate(float dt) {
		PlayLayer::postUpdate(dt);

		auto mod = Mod::get();
		if (!mod->getSettingValue<bool>("enabled")) return;
		if (mod->getSettingValue<bool>("practice-only") && !m_isPracticeMode) return;
		if (!m_player1 || m_player1->m_isDead) return;

		auto p = m_player1;
		auto& f = *(m_fields.operator->());

		// Only cube is handled. Other modes are left to the player.
		bool cube = !(p->m_isShip || p->m_isBird || p->m_isBall || p->m_isDart ||
		              p->m_isRobot || p->m_isSpider || p->m_isSwing);
		if (!cube) {
			if (f.holding) {
				this->handleButton(false, 1, true);
				f.holding = false;
			}
			return;
		}

		float px = p->getPositionX();
		float py = p->getPositionY();
		float speed = BASE_SPEED * p->m_playerSpeed;
		float lead = static_cast<float>(mod->getSettingValue<double>("lead-time")) * speed;

		// Keep the pointer correct after practice checkpoints / restarts.
		while (f.
