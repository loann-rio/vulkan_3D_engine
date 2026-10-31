#include "ObjectManager.h"

ObjectManager::ObjectManager(GameObjectFactory& factory)
{}

GameObject& ObjectManager::add(std::unique_ptr<GameObject> object)
{
	// TODO: insert return statement here
}

GameObject* ObjectManager::find(ObjectId id) noexcept
{
	auto it = objects_.find(id);
	return (it != objects_.end()) ? it->second.get() : nullptr;
}

GameObject* ObjectManager::findByName(const std::string& name) noexcept
{
	auto it = idsByName_.find(name);
	return find((it != idsByName_.end()) ? it->second : 0);
}

bool ObjectManager::remove(ObjectId id) noexcept
{
	return false;
}

bool ObjectManager::removeByName(const std::string& name) noexcept
{
	return false;
}

void ObjectManager::update(float deltaTime)
{}

void ObjectManager::indexObject(GameObject& object)
{}

void ObjectManager::unindexObject(const GameObject& object)
{}
