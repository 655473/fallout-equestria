#ifndef  WORLD_MAP_OBJECT_HPP
# define WORLD_MAP_OBJECT_HPP

# include "globals.hpp"
# include <panda3d/pandaFramework.h>
# include <vector>
# include "serializer.hpp"
# include "colmask.hpp"
# include "collider.hpp"

struct Waypoint;
struct World;
struct WorldLight;

struct MapObject : public Utils::Serializable
{
  typedef std::vector<Waypoint*>  Waypoints;
  typedef std::vector<MapObject*> Children;

  MapObject() : parent_object(0), world(0), use_texture(true), use_color(false), use_opacity(false), base_color(1, 1, 1, 1) {}
  virtual ~MapObject();

  NodePath      nodePath, render;
  PT(Texture)   texture;
  unsigned char floor;
  bool          inherits_floor;
  Waypoints     waypoints;
  NodePath      waypoints_root;

  std::string   name;
  std::string   strModel;
  std::string   strTexture;
  std::string   parent;
  Collider      collider;
  Children      children;
  MapObject*    parent_object;
  World*        world;
  bool          use_texture, use_color, use_opacity;
  LColorf       base_color;

  void          SetName(const std::string&);
  void          SetModel(const std::string&);
  void          SetTexture(const std::string&);
  void          SetFloor(unsigned char floor);
  void          ReparentTo(MapObject* object);
  virtual void  ReparentToFloor(World* world, unsigned char floor);

  void          UnserializeWaypoints(World*, Utils::Packet& packet);
  virtual void  Unserialize(Utils::Packet& packet);
  virtual void  Serialize(Utils::Packet& packet) const;
  void          InitializeTree(World* world);
  void          InitializeCollideMask();
  virtual int   GetObjectCollideMask() const { return (ColMask::Object); }
  void          SetLight(WorldLight* light, bool is_active);
  
  bool          IsCuttable(void) const;
};


#endif
