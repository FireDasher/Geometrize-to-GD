#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

class $modify(MyEditorUI, EditorUI) {
	void createMoveMenu() {
		auto* theButton = this->getSpriteButton("logo.png"_spr, menu_selector(MyEditorUI::onClick), nullptr, 0.9f);

		EditorUI::createMoveMenu();

		int rows = GameManager::sharedState()->getIntGameVariable("0049");
		int cols = GameManager::sharedState()->getIntGameVariable("0050");

		m_editButtonBar->m_buttonArray->addObject(theButton);

		m_editButtonBar->reloadItems(rows, cols);

		theButton->setID("geometrize2objs-button"_spr);
	}

	void onClick(CCObject*) {
		if (this->getSelectedObjects()->count() != 1) {
			FLAlertLayer::create(
				"Info",
				"You must select <cg>exactly one</c> object to be the bottom-left corner of the image!",
				"OK"
			)->show();
			return;
		}

		static EventListener<Task<Result<std::filesystem::path>>> listener;

		file::FilePickOptions::Filter filter = {};

		file::FilePickOptions options = {
			std::nullopt,
			{filter}
		};

		listener.bind([this](Task<Result<std::filesystem::path>>::Event* event){
			if (event->isCancelled()) {
    	        FLAlertLayer::create(
            	    "Error",
            	    "<cr>Failed</c> to open file (Task Cancelled).",
            	    "Ok"
            	)->show();
    	        return;
    	    }
			
    	    auto res = event->getValue();
    	    if (res->isErr()) {
				FLAlertLayer::create(
            	    "Error",
            	    fmt::format("<cr>Failed</c> to open file. Error: {}", res->err()),
            	    "Ok"
            	)->show();
    	        return;
			}

    	    auto path = res->unwrap();

			if (path.extension() != ".json") {
				FLAlertLayer::create(
            	    "Error",
            	    "File must be of type JSON.",
            	    "Ok"
            	)->show();
    	        return;
			}

			auto json = file::readJson(path).unwrap();

			GameObject* selectedObj = CCArrayExt<GameObject*>(this->getSelectedObjects())[0];
			MyEditorUI::handleJson(json, selectedObj);
    	});
		listener.setFilter(file::pick(file::PickMode::OpenFile, options));
	}

	void handleJson(matjson::Value json, GameObject* posObj) {
		std::stringstream objs;
		auto shapes = json["shapes"].asArray().unwrap();
		
		for (int i = 0; i < shapes.size(); i++) {
			int type = shapes.at(i)["type"].asInt().unwrap();
			auto data = shapes.at(i)["data"].asArray().unwrap();
			float h, s, v;
			auto color = shapes.at(i)["color"].asArray().unwrap();
			float r = (float)color.at(0).asDouble().unwrap();
			float g = (float)color.at(1).asDouble().unwrap();
			float b = (float)color.at(2).asDouble().unwrap();
			rgb2hsv(r / 255.f, g / 255.f, b / 255.f, h, s, v);

			// type 32 = circle | type 8 = elipse | type 16 = rotated elipse | type 1 = rectangle
			if (type == 32 || type == 8 || type == 16) {
				// obj type | Full circle: 3621 | Smaller circle: 1764 | Square: 211
				objs << "1," << "3621";
				// x pos
				objs << ",2," << (float)data.at(0).asDouble().unwrap() + posObj->getPositionX();
				// y pos
				objs << ",3," << (float)data.at(1).asDouble().unwrap() + posObj->getPositionY();
				if (type == 8 || type == 16) {
					// x scale
					objs << ",128," << (float)data.at(2).asDouble().unwrap() / 16;
					// y scale
					objs << ",129," << (float)data.at(3).asDouble().unwrap() / 16;
				} else if (type == 32) {
					// scale
					objs << ",32," << (float)data.at(2).asDouble().unwrap() / 16;
				}
				if (type == 16) {
					// rotation
					objs << ",6," << -(float)data.at(4).asDouble().unwrap();
				}
				// enable hsv
				objs << ",41," << "1";
				// color
				objs << ",43," << h << "a" << s << "a" << v << "a" << 1 << "a" << 1;
				// end obj
				objs << ";";
			}
		}

		auto editor = LevelEditorLayer::get();
		auto editorUI = editor->m_editorUI;
		// delete center object
		editorUI->onDeleteSelected(nullptr);
		// add objects
		auto objsArray = editor->createObjectsFromString(objs.str(), true, true);
		editorUI->flipObjectsY(objsArray);
		editorUI->selectObjects(objsArray, true);
	}

	void rgb2hsv(float r, float g, float b, float& h, float& s, float& v) {
		v = std::max({ r, g, b });
		float dt = v - std::min({ r, g, b });
		if (v == 0) {
			s = 0;
			h = 0;
		} else {
			s = dt / v;
			if (dt == 0) {
				h = 0;
			} else {
				if (v == r) {
					h = (g - b) / dt;
				} else if (v == g) {
					h = 2 + (b - r) / dt;
				} else {
					h = 4 + (r - g) / dt;
				}
				h *= 60;
				if (h < 0) {
					h += 360;
				}
			}
		}
	}
};