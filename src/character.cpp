#include "character.hpp"
#include "scene_camera.hpp"
#include <panda3d/nodePathCollection.h>

#include "level.hpp"

#define DEFAULT_WEAPON_1 "hooves"
#define DEFAULT_WEAPON_2 "buck"

using namespace std;

void InstanceDynamicObject::ThatDoesNothing() { _level->ConsoleWrite("That does nothing"); }

ObjectCharacter::ObjectCharacter(Level* level, DynamicObject* object) : InstanceDynamicObject(level, object)
{
  Data   items = _level->GetItems();  
  string defEquiped[2];

  _type         = Character;
  _actionPoints = 0;
  // Line of sight tools
  _losNode      = new CollisionNode("losRay");
  _losNode->set_from_collide_mask(CollideMask(ColMask::Object | ColMask::DynObject));
  _losPath      = object->nodePath.attach_new_node(_losNode);
  _losRay       = new CollisionRay();
  _losRay->set_origin(0, 0, 0);
  _losRay->set_direction(-10, 0, 0);
  _losPath.set_pos(0, 0, 5);
  //_losPath.show();
  _losNode->add_solid(_losRay);
  _losHandlerQueue = new CollisionHandlerQueue();
  _losTraverser.add_collider(_losPath, _losHandlerQueue);  
  
  // Statistics
  _statistics = DataTree::Factory::JSON("data/charsheets/" + object->charsheet + ".json");
  if (_statistics)
  {
    _inventory.SetCapacity(GetStatistics()["Statistics"]["Carry Weight"]);
  }
  
  _equiped[0] = 0;
  _equiped[1] = 0;
  defEquiped[0] = DEFAULT_WEAPON_1;
  defEquiped[1] = DEFAULT_WEAPON_2;

  // Script
  _scriptContext = 0;
  _scriptModule  = 0;
  _scriptMain    = 0;
  _scriptFight   = 0;
  if (object->script != "")
  {
    string prefixName = "IA_";
    string prefixPath = "scripts/ai/";

    _scriptContext = Script::Engine::Get()->CreateContext();
    if (_scriptContext)
    {
      _scriptModule  = Script::Engine::LoadModule(prefixName + GetName(), prefixPath + object->script + ".as");
      if (_scriptModule)
      {
	// Get the running functions
        _scriptMain    = _scriptModule->GetFunctionByDecl("void main(Character@, float)");
	_scriptFight   = _scriptModule->GetFunctionByDecl("void combat(Character@)");

	// Get Default Weapons from script
	asIScriptFunction* funcGetDefWeapon[2];

	funcGetDefWeapon[0] = _scriptModule->GetFunctionByDecl("string default_weapon_1()");
	funcGetDefWeapon[1] = _scriptModule->GetFunctionByDecl("string default_weapon_2()");
	for (int i = 0 ; i < 2 ; ++i)
	{
	  if (funcGetDefWeapon[i])
	  {
	    _scriptContext->Prepare(funcGetDefWeapon[i]);
	    _scriptContext->Execute();
	    defEquiped[i] = *(reinterpret_cast<string*>(_scriptContext->GetReturnAddress()));
	  }
	}
      }
    }
  }

  for (int i = 0 ; i < 2 ; ++i)
  {
    if (items[defEquiped[i]].Nil())
      continue ;
    _defEquiped[i] = new InventoryObject(items[defEquiped[i]]);
    _equiped[i] = _defEquiped[i];
    _equiped[i]->SetEquiped(true);
    _inventory.AddObject(_equiped[i]);
  }
}

InventoryObject* ObjectCharacter::GetEquipedItem(unsigned short it)
{
  return (_equiped[it]);
}

void ObjectCharacter::SetEquipedItem(unsigned short it, InventoryObject* item)
{
  _equiped[it]->SetEquiped(false);
  _equiped[it] = item;
  item->SetEquiped(true);
  EquipedItemChanged.Emit(it, item);
  _inventory.ContentChanged.Emit();
}

void ObjectCharacter::UnequipItem(unsigned short it)
{
  SetEquipedItem(it, _defEquiped[it]);
}

void ObjectCharacter::RestartActionPoints(void)
{
  if (_statistics)
  {
    Data stats(_statistics);

    _actionPoints = stats["Statistics"]["Action Points"];
  }
  else
    _actionPoints = 10;
  ActionPointChanged.Emit(_actionPoints);
}

void ObjectCharacter::Run(float elapsedTime)
{
  Level::State state = _level->GetState();
  
  if (state == Level::Normal && _scriptMain)
  {
    _scriptContext->Prepare(_scriptMain);
    _scriptContext->SetArgObject(0, this);
    _scriptContext->SetArgFloat(1, elapsedTime);
    _scriptContext->Execute();
  }
  else if (state == Level::Fight)
  {
    //std::cout << "ActionPoints: " << _actionPoints << std::endl;
    if (_actionPoints == 0)
      _level->NextTurn();
    else if (!(IsMoving()) && _scriptFight) // replace with something more appropriate
    {
      _scriptContext->Prepare(_scriptFight);
      _scriptContext->SetArgObject(0, this);
      _scriptContext->Execute();
    }
  }
  if (_path.size() > 0)
    RunMovement(elapsedTime);
}

int                 ObjectCharacter::GetBestWaypoint(InstanceDynamicObject* object, bool farthest)
{
  Waypoint* self  = GetOccupiedWaypoint();
  Waypoint* other = object->GetOccupiedWaypoint();
  Waypoint* wp    = self;
  float     currentDistance;
  
  if (other)
  {
    currentDistance = wp->GetDistanceEstimate(*other);
    UnprocessCollisions();
    {
      std::list<Waypoint*> list = self->GetSuccessors(other);
      
      for_each(list.begin(), list.end(), [&wp, &currentDistance, other, farthest](Waypoint* waypoint)
      {
	float compDistance = waypoint->GetDistanceEstimate(*other);
	bool  comp         = (farthest ? currentDistance < compDistance : currentDistance > compDistance);

	if (currentDistance < compDistance)
	  wp = waypoint;
      });
    }
    ProcessCollisions();
  }
  return (wp->id);
}

int                 ObjectCharacter::GetFarthestWaypoint(InstanceDynamicObject* object)
{
  return (GetBestWaypoint(object, true));
}

int                 ObjectCharacter::GetNearestWaypoint(InstanceDynamicObject* object)
{
  return (GetBestWaypoint(object, false));
}

float               ObjectCharacter::GetDistance(InstanceDynamicObject* object)
{
    LPoint3 pos_1  = _object->nodePath.get_pos();
    LPoint3 pos_2  = object->GetDynamicObject()->nodePath.get_pos();
    float   dist_x = pos_1.get_x() - pos_2.get_x();
    float   dist_y = pos_1.get_y() - pos_2.get_y();

    return (SQRT(dist_x * dist_x + dist_y * dist_y));
}

unsigned short      ObjectCharacter::GetPathDistance(InstanceDynamicObject* object)
{
  unsigned short ret;
  
  object->UnprocessCollisions();
  ret = GetPathDistance(object->GetOccupiedWaypoint());
  object->ProcessCollisions();
  return (ret);
}

unsigned short      ObjectCharacter::GetPathDistance(Waypoint* waypoint)
{
  std::list<Waypoint> path;
  
  UnprocessCollisions();
  _level->FindPath(path, *_waypointOccupied, *waypoint);
  if (path.size() > 0)
    path.erase(path.begin());
  ProcessCollisions();
  return (path.size());
}

void                ObjectCharacter::GoTo(unsigned int id)
{
  Waypoint*         wp = _level->GetWorld()->GetWaypointFromId(id);
  
  if (wp) GoTo(wp);
}

void                ObjectCharacter::GoTo(Waypoint* waypoint)
{
  ReachedDestination.DisconnectAll();
  _goToData.objective = 0;

  UnprocessCollisions();
  _path.clear();
  if (_waypointOccupied && waypoint)
  {
    if (!(_level->FindPath(_path, *_waypointOccupied, *waypoint)))
    {
      if (_level->GetPlayer() == this)
        _level->ConsoleWrite("No path.");
    }
  }
  else
    cout << "Character doesn't have a waypointOccupied" << endl;
  ProcessCollisions();
}

void                ObjectCharacter::GoTo(InstanceDynamicObject* object, int max_distance)
{
  if (object == 0)
  {
    Script::Engine::ScriptError.Emit("Character::GoTo: NullPointer Error");
    return ;
  }
  ReachedDestination.DisconnectAll();
  _goToData              = object->GetGoToData(this);
  _goToData.max_distance = max_distance;

  UnprocessCollisions();
  object->UnprocessCollisions();

  _path.clear();
  if (_waypointOccupied && _goToData.nearest)
  {
    if (!(_level->FindPath(_path, *_waypointOccupied, *_goToData.nearest)))
    {
      if (_level->GetPlayer() == this)
        _level->ConsoleWrite("Can't reach.");
    }
    while (_goToData.min_distance && _path.size() > 1)
    {
      _path.erase(--(_path.end()));
      _goToData.min_distance--;
    }
  }

  ProcessCollisions();
  object->ProcessCollisions();
}

void                ObjectCharacter::GoToRandomWaypoint(void)
{
  if (_waypointOccupied)
  {
    _goToData.objective = 0;
    _path.clear();
    ReachedDestination.DisconnectAll();
    UnprocessCollisions();
    {
      std::list<Waypoint*>           list = _waypointOccupied->GetSuccessors(0);
      int                            rit  = rand() % list.size();
      std::list<Waypoint*>::iterator it   = list.begin();

      for (it = list.begin() ; rit ; --rit, ++it);
      _path.push_back(**it);
    }
    ProcessCollisions();
  }
}

void                ObjectCharacter::TruncatePath(unsigned short max_size)
{
  if ((*_path.begin()).id == GetOccupiedWaypointAsInt())
    max_size++;
  if (_path.size() > max_size)
    _path.resize(max_size);
}

void                ObjectCharacter::RunMovementNext(float elapsedTime)
{
  Waypoint* wp = _level->GetWorld()->GetWaypointFromId(_path.begin()->id);

  if (wp != _waypointOccupied)
    SetActionPoints(_actionPoints - 1);
  _waypointOccupied = wp;

  // Has reached object objective, if there is one ?
  if (_goToData.objective)
  {
    if (_path.size() <= _goToData.max_distance && HasLineOfSight(_goToData.objective))
    {
      _path.clear();
      ReachedDestination.Emit(this);
      ReachedDestination.DisconnectAll();
      return ;
    }
  }

  _path.erase(_path.begin());  

  if (_path.size() > 0)
  {
    bool pathAvailable = true;
    // Check if the next waypoint is still accessible
    UnprocessCollisions();
    Waypoint::Arc* arc = _waypointOccupied->GetArcTo(_path.begin()->id);

    if (arc)
    {
      if (arc->observer)
      {
	if (arc->observer->CanGoThrough(0))
	  arc->observer->GoingThrough();
	else
	  pathAvailable = false;
      }
    }
    else
      pathAvailable = false;
    ProcessCollisions();
    // End check if the next waypoint is still accessible

    if (pathAvailable)
      RunMovement(elapsedTime);
    else
    {
      if (_goToData.objective)
	GoTo(_goToData.objective);
      else
      {
	Waypoint* dest = _level->GetWorld()->GetWaypointFromId((*(--(_path.end()))).id);
	GoTo(dest);
      }
      if (_path.size() == 0)
      {
	_level->ConsoleWrite("Path is obstructed");
	ReachedDestination.DisconnectAll();
      }
    }
  }
  else
  {
    ReachedDestination.Emit(this);
    ReachedDestination.DisconnectAll();
  }
}

void                ObjectCharacter::RunMovement(float elapsedTime)
{
  //std::cout << "Running Movement" << std::endl;
  
  Waypoint&         next = *(_path.begin());
  // TODO: Speed walking / running / combat
  float             max_speed = 30.f * elapsedTime;
  LPoint3           distance;
  float             max_distance;    
  LPoint3           speed, axis_speed, dest;
  LPoint3           pos = _object->nodePath.get_pos();
  int               dirX, dirY, dirZ;

  distance  = pos - next.nodePath.get_pos();

  if (distance.get_x() == 0 && distance.get_y() == 0 && distance.get_z() == 0)
    RunMovementNext(elapsedTime);
  else
  {
    dirX = dirY = dirZ = 1;
    if (distance.get_x() < 0)
    {
      distance.set_x(-(distance.get_x()));
      dirX = -1;
    }
    if (distance.get_y() < 0)
    {
      distance.set_y(-(distance.get_y()));
      dirY = -1;
    }
    if (distance.get_z() < 0)
    {
      distance.set_z(-(distance.get_z()));
      dirZ = -1;
    }

    max_distance = (distance.get_x() > distance.get_y() ? distance.get_x() : distance.get_y());
    max_distance = (distance.get_z() > max_distance     ? distance.get_z() : max_distance);

    axis_speed.set_x(distance.get_x() / max_distance);
    axis_speed.set_y(distance.get_y() / max_distance);
    axis_speed.set_z(distance.get_z() / max_distance);
    if (max_speed > max_distance)
      max_speed = max_distance;
    speed.set_x(max_speed * axis_speed.get_x() * dirX);
    speed.set_y(max_speed * axis_speed.get_y() * dirY);
    speed.set_z(max_speed * axis_speed.get_z() * dirZ);

    dest = pos - speed;

    _object->nodePath.look_at(dest);
    _object->nodePath.set_pos(dest);
  }
}

bool                ObjectCharacter::HasLineOfSight(InstanceDynamicObject* object)
{
  if (object == this)
    return (true);
  NodePath root  = _object->nodePath;
  NodePath other = object->GetNodePath();
  bool ret = true;

  LVecBase3 rot = root.get_hpr();
  LVector3  dir = root.get_relative_vector(other, other.get_pos() - root.get_pos());

  _losPath.set_hpr(-rot.get_x(), -rot.get_y(), -rot.get_z());
  _losRay->set_direction(dir.get_x(), dir.get_y(), dir.get_z());
  _losTraverser.traverse(_level->GetWorld()->window->get_render());

  _losHandlerQueue->sort_entries();

  for (unsigned int i = 0 ; i < _losHandlerQueue->get_num_entries() ; ++i)
  {
    CollisionEntry* entry = _losHandlerQueue->get_entry(i);
    NodePath        node  = entry->get_into_node_path();

    if (root.is_ancestor_of(node))
      continue ;
    if (!(other.is_ancestor_of(node)))
      ret = false;
    break ;
  }
  return (ret);
}
