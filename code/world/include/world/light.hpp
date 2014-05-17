#ifndef  WORLD_LIGHT_HPP
# define WORLD_LIGHT_HPP

# include "globals.hpp"
# include <panda3d/pointLight.h>
# include <panda3d/directionalLight.h>
# include <panda3d/ambientLight.h>
# include <panda3d/spotlight.h>
# include "serializer.hpp"
# include "collider.hpp"

struct World;
struct MapObject;
struct DynamicObject;

struct WorldLight : public Utils::Serializable
{
  enum Type
  {
    Point,
    Directional,
    Ambient,
    Spot
  };

  enum ParentType
  {
    Type_None,
    Type_MapObject,
    Type_DynamicObject
  };

  WorldLight(Type type, ParentType ptype, NodePath parent, const std::string& name) : name(name), type(type), parent_type(ptype), parent(parent), parent_i(0)
  {
    Initialize();
  }

  WorldLight(NodePath parent) : parent(parent), parent_i(0) {}

  WorldLight() : parent_i(0) {}
  
  void   Initialize(void);
  void   InitializeShadowCaster(void);
  void   SetEnabled(bool);
  void   Destroy(void);
  bool   CastsShadows(void) const { return (type == Point || type == Spot); }

  LColor GetColor(void) const
  {
    return (light->get_color());
  }

  void   SetColor(float r, float g, float b, float a)
  {
    LColor color(r, g, b, a);

    light->set_color(color);
  }

  LVecBase3f GetAttenuation(void) const
  {
    switch (type)
    {
      case Point:
      {
        PT(PointLight) point_light = reinterpret_cast<PointLight*>(light.p());

        return (point_light->get_attenuation());
      }
      case Spot:
      {
        PT(Spotlight) spot_light = reinterpret_cast<Spotlight*>(light.p());

        return (spot_light->get_attenuation());
      }
      default:
          break;
    }
    return (LVecBase3f(0, 0, 0));
  }

  void   SetAttenuation(float a, float b, float c)
  {
    switch (type)
    {
      case Point:
      {
        PT(PointLight) point_light = reinterpret_cast<PointLight*>(light.p());

        point_light->set_attenuation(LVecBase3(a, b, c));
        break ;
      }
      case Spot:
      {
        PT(Spotlight) spot_light = reinterpret_cast<Spotlight*>(light.p());

        spot_light->set_attenuation(LVecBase3(a, b, c));
        break ;
      }
      default:
        break ;
    }
  }

  void   SetPosition(LPoint3 position)
  {
      nodePath.set_pos(position);
  }

  void   SetFrustumVisible(bool);

  bool operator==(const std::string& comp) { return (name == comp); }

  void Unserialize(Utils::Packet& packet);
  void Serialize(Utils::Packet& packet) const;

  void ReparentTo(World* world);
  void ReparentTo(DynamicObject* object);
  void ReparentTo(MapObject* object);

  MapObject*  Parent(void) const
  {
      return (parent_i);
  }

  std::string   name;
  NodePath      nodePath;
  Type          type;
  ParentType    parent_type;
  PT(Light)     light;
  Lens*         lens;
  float         zoneSize;
  unsigned char priority;
  bool          enabled;

  struct ShadowSettings : public Utils::Serializable
  {
    ShadowSettings()
    {
      buffer_size[0] = 8192;
      buffer_size[1] = 8192;
      distance_near  = 10.f;
      distance_far   = 10.f;
      film_size      = 1024;
    }

    void Serialize(Utils::Packet&) const;
    void Unserialize(Utils::Packet&);

    unsigned int buffer_size[2];
    float        distance_near, distance_far;
    unsigned int film_size;
  };

  ShadowSettings shadow_settings;
  Collider       collider;

  std::list<NodePath>      enlightened;
  std::vector<std::string> enlightened_index;
#ifdef GAME_EDITOR
  NodePath    symbol;
#endif
private:
  NodePath    parent;
  MapObject*  parent_i;
};


#endif
