#ifndef  GAME_UI_HPP
# define GAME_UI_HPP

# include <panda3d/pgVirtualFrame.h>
# include <panda3d/rocketRegion.h>
# include <Rocket/Core.h>
# include <panda3d/orthographicLens.h>
# include <Rocket/Core/XMLParser.h>
# include <panda3d/pandaFramework.h>

# include "observatory.hpp"
# include "scriptengine.hpp"

struct MyRocket
{
  static Rocket::Core::Element* GetChildren(Rocket::Core::Element* element, const std::string& name)
  {
    Rocket::Core::Element* elem;

    for (unsigned int i = 0 ; elem = element->GetChild(i) ; ++i)
    {
      if (elem->GetId().CString() == name)
        return (elem);
    }
    return (0);
  }
};

struct RocketListener : public Rocket::Core::EventListener
{
  void ProcessEvent(Rocket::Core::Event& event) { EventReceived.Emit(event); }

  Observatory::Signal<void (Rocket::Core::Event&)> EventReceived;
};

class GameMenu
{
public:
  GameMenu(WindowFramework* window);
  void MenuEventContinue(Rocket::Core::Event& event) { _rocket->set_active(false); }
  void MenuEventExit(Rocket::Core::Event& event);
  void Show(void) { _rocket->set_active(true);  }
  void Hide(void) { _rocket->set_active(false); }

private:
  WindowFramework*       _window;
  PT(RocketRegion)       _rocket;
  PT(RocketInputHandler) _ih;

  RocketListener         _continueClicked;
  RocketListener         _exitClicked;
  RocketListener         _optionsClicked;  
};

class GameMainBar
{
public:
  GameMainBar(WindowFramework* window);

  RocketListener         MenuButtonClicked;
  RocketListener         InventoryButtonClicked;
  
private:
  WindowFramework*       _window;
  PT(RocketRegion)       _rocket;
  PT(RocketInputHandler) _ih;
};

class GameInventory
{
public:
  GameInventory(WindowFramework* window);

  void Show(void) { _rocket->set_active(true);  }
  void Hide(void) { _rocket->set_active(false); }

private:
  WindowFramework*       _window;
  PT(RocketRegion)       _rocket;
  PT(RocketInputHandler) _ih;
};

class GameConsole
{
public:
  GameConsole(WindowFramework* window);
  ~GameConsole(void);

  void Show(void) { _rocket->set_active(true);  }
  void Hide(void) { _rocket->set_active(false); }

  RocketListener         ConsoleKeyUp;

private:
  void KeyUp(Rocket::Core::Event&);
  
  WindowFramework*       _window;
  PT(RocketRegion)       _rocket;
  PT(RocketInputHandler) _ih;
  std::string            _currentLine;
  asIScriptContext*      _scriptContext;
};

class GameUi
{
public:
  GameUi(WindowFramework* window);

  GameMenu& GetMenu(void) { return (_menu); }

  void      OpenMenu(Rocket::Core::Event&);
  void      OpenInventory(Rocket::Core::Event&);

private:
  WindowFramework* _window;
  GameConsole      _console;
  GameMenu         _menu;
  GameMainBar      _mainBar;
  GameInventory    _inventory;
};

#endif