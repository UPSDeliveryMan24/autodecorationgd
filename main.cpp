#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

#include <cmath>
#include <cstdint>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace geode::prelude;

namespace {
    constexpr float CELL = 30.f;
    // Center Y of the lowest row of blocks that sits on the editor floor.
    constexpr float FLOOR_ROW_Y = 105.f;

    using Cell = std::pair<int, int>;

    Cell cellOf(CCPoint const& pos) {
        return {
            static_cast<int>(std::floor(pos.x / CELL)),
            static_cast<int>(std::floor(pos.y / CELL))
        };
    }

    CCPoint centerOf(Cell const& c) {
        return { c.first * CELL + CELL / 2.f, c.second * CELL + CELL / 2.f };
    }

    std::vector<int> parseIDs(std::string const& text) {
        std::vector<int> ids;
        std::stringstream ss(text);
        std::string part;
        while (std::getline(ss, part, ',')) {
            try {
                int id = std::stoi(part);
                if (id > 0) ids.push_back(id);
            } catch (...) {
                // ignore anything that isn't a number
            }
        }
        return ids;
    }

    bool inGroup(GameObject* obj, int group) {
        if (!obj->m_groups) return false;
        for (int i = 0; i < obj->m_groupCount; i++) {
            if ((*obj->m_groups)[i] == group) return true;
        }
        return false;
    }
}

class $modify(AutoDecoUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        auto sprite = ButtonSprite::create("Deco", "goldFont.fnt", "GJ_button_04.png", .7f);
        sprite->setScale(.75f);
        auto button = CCMenuItemSpriteExtra::create(
            sprite, this, menu_selector(AutoDecoUI::onAutoDeco)
        );
        button->setID("auto-deco-button"_spr);

        if (auto menu = this->getChildByID("undo-menu")) {
            menu->addChild(button);
            menu->updateLayout();
        } else {
            auto menu = CCMenu::create();
            auto win = CCDirector::get()->getWinSize();
            menu->setPosition({ 40.f, win.height - 80.f });
            menu->addChild(button);
            this->addChild(menu, 100);
        }
        return true;
    }

    void onAutoDeco(CCObject*) {
        createQuickPopup(
            "Auto Decorator",
            "<cy>Decorate</c> adds decorations around every block.\n"
            "<cr>Remove</c> deletes the decorations this mod added.",
            "Remove", "Decorate",
            [this](FLAlertLayer*, bool decorate) {
                if (decorate) this->decorateLevel();
                else this->removeDeco();
            }
        );
    }

    void decorateLevel() {
        auto editor = m_editorLayer;
        if (!editor || !editor->m_objects) return;

        auto density = Mod::get()->getSettingValue<int64_t>("density");
        auto group = static_cast<int>(Mod::get()->getSettingValue<int64_t>("deco-group"));
        bool hanging = Mod::get()->getSettingValue<bool>("hanging");
        auto topIDs = parseIDs(Mod::get()->getSettingValue<std::string>("top-ids"));
        auto hangIDs = parseIDs(Mod::get()->getSettingValue<std::string>("hanging-ids"));

        // Map out which grid cells hold solid blocks and which hold anything at all.
        std::set<Cell> solid;
        std::set<Cell> occupied;
        std::vector<Cell> blocks;
        for (auto obj : CCArrayExt<GameObject*>(editor->m_objects)) {
            auto cell = cellOf(obj->getPosition());
            occupied.insert(cell);
            if (obj->m_objectType == GameObjectType::Solid) {
                if (solid.insert(cell).second) blocks.push_back(cell);
            }
        }

        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> percent(1, 100);

        auto pick = [&](std::vector<int> const& ids) {
            std::uniform_int_distribution<size_t> d(0, ids.size() - 1);
            return ids[d(rng)];
        };

        int placed = 0;
        for (auto const& block : blocks) {
            // On top of blocks with open space above
            Cell above = { block.first, block.second + 1 };
            if (!topIDs.empty() && !occupied.contains(above) && percent(rng) <= density) {
                if (auto deco = editor->createObject(pick(topIDs), centerOf(above), true)) {
                    deco->addToGroup(group);
                    occupied.insert(above);
                    placed++;
                }
            }

            // Hanging under floating blocks
            Cell below = { block.first, block.second - 1 };
            if (hanging && !hangIDs.empty() && centerOf(below).y >= FLOOR_ROW_Y
                && !occupied.contains(below) && percent(rng) <= density) {
                if (auto deco = editor->createObject(pick(hangIDs), centerOf(below), true)) {
                    deco->setFlipY(true);
                    deco->addToGroup(group);
                    occupied.insert(below);
                    placed++;
                }
            }
        }

        Notification::create(
            placed > 0
                ? fmt::format("Added {} decorations", placed)
                : std::string("No open spots to decorate"),
            placed > 0 ? NotificationIcon::Success : NotificationIcon::Info
        )->show();
    }

    void removeDeco() {
        auto editor = m_editorLayer;
        if (!editor || !editor->m_objects) return;

        auto group = static_cast<int>(Mod::get()->getSettingValue<int64_t>("deco-group"));

        std::vector<GameObject*> toRemove;
        for (auto obj : CCArrayExt<GameObject*>(editor->m_objects)) {
            if (inGroup(obj, group)) toRemove.push_back(obj);
        }
        for (auto obj : toRemove) {
            editor->removeObject(obj, true);
        }

        Notification::create(
            fmt::format("Removed {} decorations", toRemove.size()),
            NotificationIcon::Success
        )->show();
    }
};
