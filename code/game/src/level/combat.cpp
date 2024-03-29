#include "level/combat.hpp"
#include "level/level.hpp"
#include "options.hpp"
#define WORLDTIME_TURN          10

using namespace std;

ObjectCharacter* Combat::GetCurrentCharacter(void) const
{
  if (iterator != characters.end())
    return (*iterator);
  return (0);
}

bool Combat::CanStop(void) const
{
  auto it  = characters.begin();
  auto end = characters.end();

  for (; it != end ; ++it)
  {
    if ((*it)->IsAlive() && !((*it)->IsAlly(level.GetPlayer())))
    {
      if (!((*it)->GetNearbyEnemies().empty()))
      {
        cout << "Can't stop combat: " << (*it)->GetName() << " has enemies in sight." << endl;
        return (false);
      }
    }
  }
  return (true);
}

void Combat::InitializeLevelForFight(void)
{
  level.SetState(Level::Fight);
  level.GetLevelUi().GetMainBar().SetEnabledAP(true);
  level.GetLevelUi().GetMainBar().AppendToConsole("Combat started.");
}

void Combat::FinalizeFightForLevel(void)
{
  if (level.GetMouse().GetState() == MouseEvents::MouseTarget)
    level.GetMouse().SetState(MouseEvents::MouseAction);
  level.SetState(Level::Normal);
  level.GetLevelUi().GetMainBar().SetEnabledAP(false);
  level.GetLevelUi().GetMainBar().AppendToConsole("Combat ended.");
}

void Combat::Start(ObjectCharacter* character)
{
  if (iterator == characters.end())
  {
    iterator = find(characters.begin(), characters.end(), character);
    if (iterator != characters.end())
    {
      InitializeLevelForFight();
      InitializeCharacterTurn(character);
    }
  }
}

void Combat::Stop(void)
{
  if (iterator != characters.end())
  {
    iterator = characters.end();
    FinalizeFightForLevel();
    for_each(characters.begin(), characters.end(), [this](ObjectCharacter* character)
    {
      FinishFightForCharacter(character);
    });
  }
}

void Combat::NextTurn(void)
{
  if (CanStop())
    Stop();
  else
  {
    if (iterator != characters.end())
      FinalizeCharacterTurn(*iterator);
    iterator++;
    if (iterator == characters.end())
      FinalizeRound();
    if (iterator != characters.end())
      InitializeCharacterTurn(*iterator);
  }
}

void Combat::FinalizeRound(void)
{
  if (characters.begin() == iterator)
    Stop();
  else
  {
    iterator = characters.begin();
    level.GetTimeManager().AddElapsedTime(WORLDTIME_TURN);
  }
}

void Combat::FinalizeCharacterTurn(ObjectCharacter* character)
{
  character->ConvertRemainingActionPointsToArmorClass();
  character->PlayAnimation("idle");
  character->MovedFor1ActionPoint.DisconnectAll();
  character->GetPath().Hide();
}

void Combat::InitializeCharacterTurn(ObjectCharacter* character)
{
  cout << "Starting turn for character " << character->GetName() << endl;
  character->GetFieldOfView().RunCheck();
  character->RefreshActionPoints();
  RefreshScriptedTasks(character);
  if (character->GetActionPoints() == 0)
  {
    NextTurn();
    return ;
  }
  character->MovedFor1ActionPoint.Connect([this, character]()
  {
    character->UseActionPoints(1);
  });
  if (character->IsMoving())
  {
    if (character->IsPlayer() && OptionsManager::Get()["reset-movement-after-turn"].Value() != "1")
      character->StartRunAnimation();
    else
      character->TruncatePath(0);
  }
}

void Combat::RefreshScriptedTasks(ObjectCharacter* character)
{
  const TaskSet& task_set = character->GetTaskSet();

  cout << "Refreshing tasks for character " << character->GetName() << endl;
  for_each(task_set.begin(), task_set.end(), [](pair<string, ScriptedTask*> task)
  {
    cout << "Refreshing task " << task.first << endl;
    task.second->NextTurn();
  });
}

void Combat::FinishFightForCharacter(ObjectCharacter* character)
{
  character->RefreshActionPoints();
}

void Combat::Serialize(Utils::Packet& packet) const
{
  char has_combat_data = (iterator != characters.end() ? 1 : 0);

  packet << has_combat_data;
  if (has_combat_data)
  {
    unsigned int iterator_pos  = 0;
    auto         iterator_comp = characters.begin();
    
    for (; iterator_comp != iterator ; ++iterator_comp)
      iterator_pos++;
    packet << iterator_pos;
  }
}

void Combat::Unserialize(Utils::Packet& packet)
{
  char has_combat_data;
  
  packet >> has_combat_data;
  if (has_combat_data)
  {
    unsigned int iterator_pos;
    
    packet >> iterator_pos;
    iterator = characters.begin();
    std::advance(iterator, iterator_pos);
    InitializeLevelForFight();
  }
}
