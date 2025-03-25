#pragma once


#include "WebSocketClient.h"
#include <winsock2.h>
#include <vector>
#include <string>

#include "GameObject.h"


class ArmControl
{
public:
    

    ArmControl(std::string host);

    void updateAnglesOnMsg(std::shared_ptr<GameObject::Map> gameObjects);
    void sendMousePosition(glm::vec2 mousePos, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, bool spacePressed);

    void setIDObjects(GameObject::id_t link1, GameObject::id_t link2, GameObject::id_t link3, GameObject::id_t gripper) {
        links = { link1, link2, link3, gripper };
    }
    
private:
    std::unique_ptr<WebSocketClient> client;  

    glm::vec3 getMouseProjectionOnPlane(glm::vec2 mousePos, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
    void setNewPos(std::shared_ptr<GameObject::Map> gameObjects, float angle1, float angle2, float angle3, float angle4);
    glm::mat4 mat4(float x, float y, float z, float tx, float ty, float tz);
    std::vector<GameObject::id_t> links{};
    const std::vector<float> sizeLinks = { .55f, 1.8f, 1.7f };
    const glm::vec3 basePos = { 7.f, -0.3f, 7.f };
};

