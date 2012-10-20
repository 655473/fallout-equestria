#include <level/objects/locker.hpp>
#include <level/level.hpp>

using namespace std;

ObjectLocker::ObjectLocker(Level* level, DynamicObject* object) : ObjectShelf(level, object)
{
  _type   = ObjectTypes::Locker;
  _closed = true;
  _locked = object->locked;
  
  string anims[] = { "open", "close" };
  for (int i = 0 ; i < GET_ARRAY_SIZE(anims) ; ++i)
    LoadAnimation(anims[i]);
}

void ObjectLocker::CallbackActionUse(InstanceDynamicObject* object)
{
  if (_closed == false)
    ObjectShelf::CallbackActionUse(object);
  else if (IsLocked())
    _level->ConsoleWrite(i18n::T("It's locked"));
  else
  {
    AnimationEnded.DisconnectAll();
    AnimationEnded.Connect(*this, &ObjectLocker::PendingActionOpen);
    pendingActionOn = object;
    PlayAnimation("open");
  }
}

void ObjectLocker::PendingActionOpen(InstanceDynamicObject*)
{
  AnimationEnded.DisconnectAll();
  cout << "Count event listeners " << AnimationEnded.ObserverCount() << endl;
  ObjectShelf::CallbackActionUse(pendingActionOn);
}
