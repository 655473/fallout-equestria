#include "globals.hpp"
#include "level/level.hpp"
#include "astar.hpp"

#include "level/objects/door.hpp"
#include "level/objects/shelf.hpp"
#include <level/objects/locker.hpp>
#include <i18n.hpp>

#include "ui/alert_ui.hpp"
#include "ui/ui_loot.hpp"
#include "ui/ui_equip_mode.hpp"
#include "options.hpp"
#include <mousecursor.hpp>

#include "loading_exception.hpp"

#define AP_COST_USE             2
#define WORLDTIME_TURN          10
#define WORLDTIME_DAYLIGHT_STEP 3

using namespace std;

Level* Level::CurrentLevel = 0;
Level::Level(const std::string& name, WindowFramework* window, GameUi& gameUi, Utils::Packet& packet, TimeManager& tm) : window(window),
  camera(*this, window, window->get_camera_group()),
  mouse (*this, window),
  time_manager(tm),
  main_script(name),
  level_ui(window, gameUi),
  mouse_hint(*this, level_ui, mouse),
  interactions(*this),
  chatter_manager(window),
  floors(*this),
  zones(*this)
{
  CurrentLevel  = this;
  level_state   = Normal;
  is_persistent = true;
  level_name    = name;
  sunlight      = 0;

  floors.EnableShowLowerFloors(true);

  hovered_path.SetRenderNode(window->get_render());

  obs.Connect(level_ui.InterfaceOpened, *this, &Level::SetInterrupted);

  timer.Restart();

  // WORLD LOADING
  world = new World(window);  
  try
  {
    world->UnSerialize(packet);
    _light_iterator = world->lights.begin();
  }
  catch (unsigned int&)
  {
    throw LoadingException("Couldn't load the level's world");
  }

  cout << "Level Loading Step #6" << endl;
  if (world->sunlight_enabled)
    InitializeSun();

  LPoint3 upperLeft, upperRight, bottomLeft;
  cout << "Level Loading Step #7" << endl;
  world->GetWaypointLimits(0, upperRight, upperLeft, bottomLeft);
  camera.SetLimits(bottomLeft.get_x() - 50, bottomLeft.get_y() - 50, upperRight.get_x() - 50, upperRight.get_y() - 50);  

  cout << "Level Loading Step #8" << endl;
  ForEach(world->entryZones, [this](EntryZone& zone) { zones.RegisterZone(zone); });
  ForEach(world->exitZones,  [this](ExitZone& zone)  { zones.RegisterZone(zone); });
  
  ForEach(world->dynamicObjects, [this](DynamicObject& dobj) { InsertDynamicObject(dobj); });
  combat_character_it = characters.end();

  world->SetWaypointsVisible(false);

  task_metabolism = time_manager.AddRepetitiveTask(TASK_LVL_CITY, DateTime::Hours(1));
  task_metabolism->Interval.Connect(*this, &Level::RunMetabolism);  

  /*
   * DIVIDE AND CONQUER WAYPOINTS
   */
  std::vector<Waypoint*>                      entries;
  
  for_each(world->waypoints.begin(), world->waypoints.end(), [&entries](Waypoint& wp) { entries.push_back(&wp); });
  world->waypoint_graph.SetHeuristic([](LPoint3f position1, LPoint3f position2) -> float
  {
    float xd = position2.get_x() - position1.get_x();
    float yd = position2.get_y() - position1.get_y();
    float zd = position2.get_z() - position1.get_z();

    return (SQRT(xd * xd + yd * yd + zd * zd));
  });
  world->waypoint_graph.Initialize(entries, [](const std::vector<Waypoint*>& entries) -> std::vector<LPoint3f>
  {
    std::vector<LPoint3f> positions;
    
    {
      LPoint3f block_size;
      LPoint3f max_pos(0, 0, 0);
      LPoint3f min_pos(0, 0, 0);
      auto     it  = entries.begin();
      auto     end = entries.end();

      for (; it != end ; ++it)
      {
        LPoint3f pos = (*it)->GetPosition();

        if (pos.get_x() < min_pos.get_x()) { min_pos.set_x(pos.get_x()); }
        if (pos.get_y() < min_pos.get_y()) { min_pos.set_y(pos.get_y()); }
        if (pos.get_z() < min_pos.get_z()) { min_pos.set_z(pos.get_z()); }
        if (pos.get_x() > max_pos.get_x()) { max_pos.set_x(pos.get_x()); }
        if (pos.get_y() > max_pos.get_y()) { max_pos.set_y(pos.get_y()); }
        if (pos.get_z() > max_pos.get_z()) { max_pos.set_z(pos.get_z()); }
      }
      
      function<float (float, float)> distance = [](float min_pos, float max_pos) -> float
      {
        if (min_pos < 0 && max_pos > 0)
          return (ABS(min_pos - max_pos));
        return (ABS(max_pos) - ABS(min_pos));
      };
      
      block_size.set_x(distance(min_pos.get_x(), max_pos.get_x()));
      block_size.set_y(distance(min_pos.get_y(), max_pos.get_y()));
      block_size.set_z(distance(min_pos.get_z(), max_pos.get_z()));

      unsigned short block_count = ceil(entries.size() / 200.f);
      for (unsigned short i = 0 ; i < block_count ; ++i)
      {
        LPoint3f block_position;
        
        block_position.set_x(min_pos.get_x() + block_size.get_x() / block_count * i);
        block_position.set_y(min_pos.get_y() + block_size.get_y() / block_count * i);
        block_position.set_z(min_pos.get_z() + block_size.get_z() / block_count * i);
        positions.push_back(block_position);
      }
    }
    return (positions);
  });
  /*
   * END DIVIDE AND CONQUER
   */

  //window->get_render().set_shader_auto();
}

void Level::RefreshCharactersVisibility(void)
{
  ObjectCharacter* player              = GetPlayer();
  auto             detected_characters = player->GetFieldOfView().GetDetectedCharacters();

  for_each(characters.begin(), characters.end(),
           [this, detected_characters, player](ObjectCharacter* character)
  {
    if (character != player)
    {
      auto it = find(detected_characters.begin(), detected_characters.end(), character);

      character->SetVisible(it != detected_characters.end());
    }
  });
}

void Level::InsertCharacter(ObjectCharacter* character)
{
  character->GetFieldOfView().SetIntervalDurationInSeconds(3);
  character->GetFieldOfView().Launch();
  characters.push_back(character);
}

void Level::InsertInstanceDynamicObject(InstanceDynamicObject* object)
{
  objects.push_back(object);
}

void Level::InsertDynamicObject(DynamicObject& object)
{
  InstanceDynamicObject* instance = 0;

  switch (object.type)
  {
    case DynamicObject::Character:
      InsertCharacter(new ObjectCharacter(this, &object));
      break ;
    case DynamicObject::Door:
      instance = new ObjectDoor(this, &object);
      break ;
    case DynamicObject::Shelf:
      instance = new ObjectShelf(this, &object);
      break ;
    case DynamicObject::Item:
    {
      DataTree*        item_data = DataTree::Factory::StringJSON(object.inventory.front().first);
      InventoryObject* item;

      item_data->key = object.key;
      item           = new InventoryObject(item_data);
      instance       = new ObjectItem(this, &object, item);
      delete item_data;
      break ;
    }
    case DynamicObject::Locker:
      instance = new ObjectLocker(this, &object);
      break ;
    default:
    {
      stringstream stream;

      stream << "Inserted unimplemented dynamic object type (" << object.type << ')';
      AlertUi::NewAlert.Emit(stream.str());
    }
  }
  cout << "Added an instance => " << instance << endl;
  if (instance != 0)
    objects.push_back(instance);
}

void Level::InitializeSun(void)
{
  sunlight = new Sunlight(*world, time_manager);
  sunlight->SetAsRepetitiveTask(true);
  sunlight->SetIntervalDuration(DateTime::Minutes(1));
  sunlight->Launch();
}

void Level::InitializePlayer(void)
{
  interactions.SetPlayer(GetPlayer());

  if (!(GetPlayer()->GetStatistics().Nil()))
  {
    Data stats(GetPlayer()->GetStatistics());
    
    if (!(stats["Statistics"]["Action Points"].Nil()))
      level_ui.GetMainBar().SetMaxAP(stats["Statistics"]["Action Points"]);
  }
  {
    Interactions::InteractionList& interactions_on_player = GetPlayer()->GetInteractions();

    interactions_on_player.clear();
    interactions_on_player.push_back(Interactions::Interaction("use_object", GetPlayer(), &(interactions.UseObjectOn)));
    interactions_on_player.push_back(Interactions::Interaction("use_skill",  GetPlayer(), &(interactions.UseSkillOn)));
    interactions_on_player.push_back(Interactions::Interaction("use_magic",  GetPlayer(), &(interactions.UseSpellOn)));
  }
  
  level_ui.GetMainBar().SetStatistics(GetPlayer()->GetStatController());
  level_ui.GetMainBar().OpenSkilldex.Connect([this]() { interactions.UseSkillOn.Emit(GetPlayer()); });
  level_ui.GetMainBar().OpenSpelldex.Connect([this]() { interactions.UseSpellOn.Emit(0);           });

  obs_player.Connect(GetPlayer()->EquipedItemActionChanged, level_ui.GetMainBar(),   &GameMainBar::SetEquipedItemAction);
  obs_player.Connect(GetPlayer()->EquipedItemChanged,       level_ui.GetMainBar(),   &GameMainBar::SetEquipedItem);
  obs_player.Connect(GetPlayer()->EquipedItemChanged,       level_ui.GetInventory(), &GameInventory::SetEquipedItem);
  level_ui.GetMainBar().EquipedItemNextAction.Connect(*GetPlayer(), &ObjectCharacter::ItemNextUseType);
  level_ui.GetMainBar().UseEquipedItem.Connect       (interactions, &Interactions::Player::ActionTargetUse);
  level_ui.GetMainBar().CombatEnd.Connect            ([this](void)
  {
    StopFight();
    if (level_state == Fight)
      ConsoleWrite("You can't leave combat mode if enemies are nearby.");
  });
  level_ui.GetMainBar().CombatPassTurn.Connect       (*this, &Level::NextTurn);

  obs.Connect(level_ui.GetInventory().EquipItem,   [this](const std::string& target, unsigned int slot, InventoryObject* object)
  {
    if (target == "equiped")
      interactions.ActionEquipForQuickUse(slot, object);
  });
  obs.Connect(level_ui.GetInventory().UnequipItem, [this](const std::string& target, unsigned int slot)
  {
    if (target == "equiped")
      GetPlayer()->UnequipItem(slot);
  });

  obs.Connect(level_ui.GetInventory().DropObject, interactions, &Interactions::Player::ActionDropObject);
  obs.Connect(level_ui.GetInventory().UseObject,  interactions, &Interactions::Player::ActionUseObject);

  for (unsigned short it = 0 ; it < 2 ; ++it) // For every equiped item slot
  {
    level_ui.GetMainBar().SetEquipedItem(it, GetPlayer()->GetEquipedItem(it));
    level_ui.GetInventory().SetEquipedItem(it, GetPlayer()->GetEquipedItem(it));
  }

  player_halo.SetTarget(GetPlayer());
  GetPlayer()->GetFieldOfView().Launch();
  target_outliner.UsePerspectiveOfCharacter(GetPlayer());

  //
  // Initializing Main Script
  //
  if (main_script.IsDefined("Initialize"))
    main_script.Call("Initialize");
}

void Level::SetAsPlayerParty(Party&)
{
  InitializePlayer();
  camera.SetConfigurationFromLevelState();
}

void Level::InsertParty(Party& party, const std::string& zone_name)
{
  auto party_members = party.GetPartyMembers();
  auto it            = party_members.rbegin();
  auto end           = party_members.rend();
  
  for (; it != end ; ++it)
  {
    Party::Member*   member         = *it;
    DynamicObject&   dynamic_object = member->GetDynamicObject();
    DynamicObject*   world_object   = world->InsertDynamicObject(dynamic_object);
    ObjectCharacter* character      = new ObjectCharacter(this, world_object);

    world->InsertDynamicObject(dynamic_object);
    member->LinkCharacter(character);
    characters.insert(characters.begin(), character);
  }
  GetZoneManager().InsertPartyInZone(party, zone_name);
  if (party.GetName() == "player")
    SetAsPlayerParty(party);
  parties.push_back(&party);
}

void Level::MatchPartyToExistingCharacters(Party& party)
{
  RunForPartyMembers(party, [](Party::Member* member, ObjectCharacter* character)
  {
    member->LinkCharacter(character);
  });
  if (party.GetName() == "player")
    SetAsPlayerParty(party);
  parties.push_back(&party);
}

void Level::RemovePartyFromLevel(Party& party)
{
  if (party.GetName() == "player")
    obs_player.DisconnectAll();
  RunForPartyMembers(party, [this](Party::Member* member, ObjectCharacter* character)
  {
    auto character_it = find(characters.begin(), characters.end(), character);

    member->SaveCharacter(character);
    character->UnprocessCollisions();
    delete character;
    characters.erase(character_it);
  });
  parties.remove(&party);
}

void Level::RunForPartyMembers(Party& party, function<void (Party::Member*,ObjectCharacter*)> callback)
{
  auto party_members = party.GetPartyMembers();
  auto it            = party_members.begin();
  auto end           = party_members.end();
  
  for (; it != end ; ++it)
  {
    Party::Member* member         = *it;
    DynamicObject& dynamic_object = member->GetDynamicObject();
    CharacterList  matches        = FindCharacters([&dynamic_object](ObjectCharacter* character) -> bool
    {
      return (character->GetDynamicObject()->name == dynamic_object.name);
    });
    
    if (matches.size() == 0)
      AlertUi::NewAlert.Emit("Couldn't match party member '" + dynamic_object.name + "' to anyone in the level.");
    else
    {
      if (matches.size() > 1)
        AlertUi::NewAlert.Emit("There was more than one match when matching party character '" + dynamic_object.name + "' to the level's characters.");
      callback(member, *matches.begin());
    }
  }
}

void Level::SetPlayerInventory(Inventory* inventory)
{
  ObjectCharacter* player = GetPlayer();

  if (!inventory)
  {
    cout << "Using map's inventory" << endl;
    inventory = &(player->GetInventory());
  }
  else
  {  player->SetInventory(inventory);       }
  level_ui.GetInventory().SetInventory(*inventory);
  player->EquipedItemChanged.Emit(0, player->GetEquipedItem(0));
  player->EquipedItemChanged.Emit(1, player->GetEquipedItem(1));
}

Level::~Level()
{
  cout << "- Destroying Level" << endl;
  try
  {
    if (main_script.IsDefined("Finalize"))
      main_script.Call("Finalize");
  }
  catch (const AngelScript::Exception& exception)
  {
	AlertUi::NewAlert.Emit(std::string("Script crashed during Level destruction: ") + exception.what());
  }
  
  for_each(parties.begin(), parties.end(), [](Party* party)
  {
    if (party->GetName() != "player")
      delete party;
  });
  
  
  MouseCursor::Get()->SetHint("");
  window->get_render().clear_light();

  time_manager.ClearTasks(TASK_LVL_CITY);
  obs.DisconnectAll();
  obs_player.DisconnectAll();
  projectiles.CleanUp();
  ForEach(objects,     [](InstanceDynamicObject* obj) { delete obj;        });
  ForEach(parties,     [](Party* party)               { delete party;      });
  zones.UnregisterAllZones();
  CurrentLevel = 0;
  if (sunlight) delete sunlight;
  if (world)    delete world;
  cout << "-> Done." << endl;
}

InstanceDynamicObject* Level::GetObject(const string& name)
{
  InstanceObjects::iterator it  = objects.begin();
  InstanceObjects::iterator end = objects.end();

  for (; it != end ; ++it)
  {
    if ((*(*it)) == name)
      return (*it);
  }
  return (0);
}

Level::CharacterList Level::FindCharacters(function<bool (ObjectCharacter*)> selector) const
{
  CharacterList              list;
  Characters::const_iterator it  = characters.begin();
  Characters::const_iterator end = characters.end();

  for (; it != end ; ++it)
  {
    ObjectCharacter* character = *it;
    
    if (selector(character))
      list.push_back(character);
  }
  return (list);
}

ObjectCharacter* Level::GetCharacter(const string& name)
{
  Characters::const_iterator it  = characters.begin();
  Characters::const_iterator end = characters.end();

  for (; it != end ; ++it)
  {
    if ((*(*it)) == name)
      return (*it);
  }
  return (0);
}

ObjectCharacter* Level::GetCharacter(const DynamicObject* object)
{
  Characters::iterator it  = characters.begin();
  Characters::iterator end = characters.end();

  for (; it != end ; ++it)
  {
    if ((*(*it)).GetDynamicObject() == object)
      return (*it);
  }
  return (0);
}

ObjectCharacter* Level::GetPlayer(void)
{
  if (characters.size() == 0)
    return (0);
  return (characters.front());
}

void Level::SetState(State state)
{
  level_state = state;
  if (state == Normal)
    combat_character_it = characters.end();
  if (state != Fight)
  {
    hovered_path.Hide();
    target_outliner.DisableOutline();
  }
  camera.SetEnabledScroll(state != Interrupted);
  camera.SetConfigurationFromLevelState();
}

void Level::SetInterrupted(bool set)
{
  if (set == false && level_ui.HasOpenedWindows())
    set = true;
  
  if (set)
    SetState(Interrupted);
  else
  {
    if (combat_character_it == characters.end())
      SetState(Normal);
    else
      SetState(Fight);
  }
}

void Level::StartFight(ObjectCharacter* starter)
{
  combat_character_it = std::find(characters.begin(), characters.end(), starter);
  if (combat_character_it == characters.end())
  { 
    cout << "[FATAL ERROR][Level::StartFight] Unable to find starting character" << endl;
    if (characters.size() < 1)
    {
      cout << "[FATAL ERROR][Level::StartFight] Can't find a single character" << endl;
      return ;
    }
    combat_character_it = characters.begin();
  }
  level_ui.GetMainBar().SetEnabledAP(true);
  {
    ObjectCharacter* current_fighter = *combat_character_it;

    current_fighter->SetActionPoints(current_fighter->GetMaxActionPoints());
  }
  if (level_state != Fight)
    ConsoleWrite("You are now in combat mode.");
  SetState(Fight);
}

void Level::StopFight(void)
{
  if (level_state == Fight)
  {
    Characters::iterator it  = characters.begin();
    Characters::iterator end = characters.end();
    
    for (; it != end ; ++it)
    {
      if (!((*it)->IsAlly(GetPlayer())))
      {
        list<ObjectCharacter*> listEnemies = (*it)->GetNearbyEnemies();

        if (!(listEnemies.empty()) && (*it)->IsAlive())
          return ;
      }
    }
    if (mouse.GetState() == MouseEvents::MouseTarget)
      mouse.SetState(MouseEvents::MouseAction);
    ConsoleWrite("Combat ended.");
    SetState(Normal);
    level_ui.GetMainBar().SetEnabledAP(false);
  }
}

void Level::NextTurn(void)
{
  if (level_state != Fight || current_character != combat_character_it)
  {
    cout << "cannot go to next turn" << endl;
    return ;
  }
  if (*combat_character_it == GetPlayer())
    StopFight();
  if (combat_character_it != characters.end())
  {
    cout << "Playing animation idle" << endl;
    (*combat_character_it)->PlayAnimation("idle");
  }
  if ((++combat_character_it) == characters.end())
  {
    combat_character_it = characters.begin();
    (*combat_character_it)->GetFieldOfView().RunCheck();
    time_manager.AddElapsedTime(WORLDTIME_TURN);
  }
  if (combat_character_it != characters.end())
  {
    ObjectCharacter* current_fighter = *combat_character_it;
    
    current_fighter->SetActionPoints(current_fighter->GetMaxActionPoints());
  }
  else
    cout << "[FATAL ERROR][Level::NextTurn] Character Iterator points to nothing (n_characters = " << characters.size() << ")" << endl;
  camera.SetConfigurationFromLevelState();
}

void Level::RunMetabolism(void)
{
  for_each(characters.begin(), characters.end(), [this](ObjectCharacter* character)
  {
    if (character != GetPlayer() && character->GetHitPoints() > 0)
    {
      StatController* controller = character->GetStatController();

      controller->RunMetabolism();
    }
  });
}

AsyncTask::DoneStatus Level::do_task(void)
{
  float elapsedTime = timer.GetElapsedTime();

  if (level_ui.GetContext()->GetHoverElement() == level_ui.GetContext()->GetRootElement())
    mouse.SetCursorFromState();
  else
    mouse.SetMouseState('i');

  player_halo.Run();

  bool use_fog_of_war = false;
  if (use_fog_of_war == false)
    RefreshCharactersVisibility();

  camera.SlideToHeight(GetPlayer()->GetDynamicObject()->nodePath.get_z());
  camera.Run(elapsedTime);  

  mouse_hint.Run();

  std::function<void (InstanceDynamicObject*)> run_object = [elapsedTime](InstanceDynamicObject* obj)
  {
    obj->Run(elapsedTime);
    obj->UnprocessCollisions();
    obj->ProcessCollisions();
  };
  switch (level_state)
  {
    case Fight:
      ForEach(objects, run_object);
      // If projectiles are moving, run them. Otherwise, run the current character
      if (projectiles.size() > 0)
        projectiles.Run(elapsedTime);
      else
      {
        current_character = combat_character_it; // Keep a character from askin NextTurn several times
        if (combat_character_it != characters.end())
          run_object(*combat_character_it);
        if (mouse.Hovering().hasWaypoint && mouse.GetState() == MouseEvents::MouseAction)
          hovered_path.DisplayHoveredPath(GetPlayer(), mouse);
      }
      break ;
    case Normal:
      projectiles.Run(elapsedTime);
      time_manager.AddElapsedSeconds(elapsedTime);
      ForEach(objects,    run_object);
      ForEach(characters, run_object);
      break ;
    case Interrupted:
      break ;
  }
  //ForEach(_characters, [elapsedTime](ObjectCharacter* character) { character->RunEffects(elapsedTime); });
  zones.Refresh();
  
  if (main_script.IsDefined("Run"))
  {
    AngelScript::Type<float> param_time(elapsedTime);

    main_script.Call("Run", 1, &param_time);
  }

  floors.SetCurrentFloorFromObject(GetPlayer());
  floors.RunFadingEffect(elapsedTime);
  chatter_manager.Run(elapsedTime, camera.GetNodePath());
  particle_manager.do_particles(ClockObject::get_global_clock()->get_dt());
  mouse.Run();
  timer.Restart();
  return (exit.ReadyForNextZone() ? AsyncTask::DS_done : AsyncTask::DS_cont);
}

/*
 * Nodes Management
 */
InstanceDynamicObject* Level::FindObjectFromNode(NodePath node)
{
  {
    InstanceObjects::iterator cur = objects.begin();
    
    while (cur != objects.end())
    {
      if ((**cur) == node)
	return (*cur);
      ++cur;
    }
  }
  {
    Characters::iterator      cur = characters.begin();
    
    while (cur != characters.end())
    {
      if ((**cur) == node)
	return (*cur);
      ++cur;
    }
  }
  return (0);
}

void                   Level::RemoveObject(InstanceDynamicObject* object)
{
  InstanceObjects::iterator it = std::find(objects.begin(), objects.end(), object);
  
  if (it != objects.end())
  {
    world->DeleteDynamicObject((*it)->GetDynamicObject());
    delete (*it);
    objects.erase(it);
  }
}

void                   Level::UnprocessAllCollisions(void)
{
  ForEach(objects,    [](InstanceDynamicObject* object) { object->UnprocessCollisions(); });
  ForEach(characters, [](ObjectCharacter*       object) { object->UnprocessCollisions(); });
}

void                   Level::ProcessAllCollisions(void)
{
  ForEach(objects,    [](InstanceDynamicObject* object) { object->ProcessCollisions(); });
  ForEach(characters, [](ObjectCharacter*       object) { object->ProcessCollisions(); });
}

void Level::ConsoleWrite(const string& str)
{
  level_ui.GetMainBar().AppendToConsole(str);
}

bool Level::IsWaypointOccupied(unsigned int id) const
{
  InstanceObjects::const_iterator it_object;
  Characters::const_iterator      it_character;
  
  for (it_object = objects.begin() ; it_object != objects.end() ; ++it_object)
  {
    if ((*it_object)->HasOccupiedWaypoint() && id == ((*it_object)->GetOccupiedWaypointAsInt()))
      return (true);
  }
  for (it_character = characters.begin() ; it_character != characters.end() ; ++it_character)
  {
    if ((*it_character)->HasOccupiedWaypoint() && id == ((*it_character)->GetOccupiedWaypointAsInt()))
      return (true);
  }
  return (false);
}

ISampleInstance* Level::PlaySound(const string& name)
{
  if (sound_manager.Require(name))
  {
    ISampleInstance* instance = sound_manager.CreateInstance(name);

    instance->Start();
    return (instance);
  }
  return (0);
}
