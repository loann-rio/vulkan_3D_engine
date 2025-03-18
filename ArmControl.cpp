#include "ArmControl.h"

#include <sstream>

ArmControl::ArmControl(std::string host)
{
    client = std::make_unique<WebSocketClient>(host); 

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { 
        std::cerr << "WSAStartup failed!" << std::endl;
        return;
    }

    client->run(); 
}
 
void ArmControl::updateAnglesOnMsg(std::shared_ptr<GameObject::Map> gameObjects)
{
	std::string msg = client->getMsgFromQueue(); 

	if (msg.size()) {

        std::istringstream ss(msg);

        float angle1, angle2, angle3;
        char comma; 

        if (ss >> angle1 >> comma >> angle2 >> comma >> angle3 && comma == ',')
        {
            setNewPos(gameObjects, angle1, angle2, angle3); 
        }
        else {
            std::cout << "Invalid message format!" << std::endl;
        }
	}
}

void ArmControl::sendMousePosition(glm::vec2 mousePos, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{

    if (abs(mousePos.x) > 1 || abs(mousePos.y) > 1) return;
    
    glm::vec3 mouseWorldPos = getMouseProjectionOnPlane(mousePos, viewMatrix, projectionMatrix); 

    glm::vec3 posFromBase = mouseWorldPos - basePos; // offset the base height

    std::string pos = std::to_string(posFromBase.x) + "," + std::to_string(std::max((-sizeLinks[0]), -posFromBase.y)) + "," + std::to_string(posFromBase.z);
    client->sendMessage(pos); 
}

glm::vec3 ArmControl::getMouseProjectionOnPlane(glm::vec2 mousePos, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    glm::vec4 nearPoint = glm::vec4(mousePos.x, -mousePos.y, -1.0f, 1.0f);
    glm::vec4 farPoint = glm::vec4(mousePos.x, -mousePos.y, 1.0f, 1.0f);

    // Unproject points from clip space to world space
    glm::mat4 inverseVP = glm::inverse(projectionMatrix * viewMatrix);
    glm::vec4 nearWorld = inverseVP * nearPoint;
    glm::vec4 farWorld = inverseVP * farPoint;

    nearWorld /= nearWorld.w;
    farWorld /= farWorld.w;

    glm::vec3 rayOrigin = glm::vec3(nearWorld);
    glm::vec3 rayDirection = glm::normalize(glm::vec3(farWorld) - rayOrigin);

    glm::vec3 planeNormal = glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);  // Camera forward direction

    // Compute intersection using: t = (planeOrigin - rayOrigin) ⋅ planeNormal / (rayDirection ⋅ planeNormal)
    float denom = glm::dot(rayDirection, planeNormal);

    if (glm::abs(denom) > 1e-6) {  // Avoid division by zero
        float t = glm::dot(basePos - rayOrigin, planeNormal) / denom; 
        return rayOrigin + t * rayDirection;  // Intersection point
    }
      
    return glm::vec3(0.0f); 
}

void ArmControl::setNewPos(std::shared_ptr<GameObject::Map> gameObjects, float angle1, float angle2, float angle3)
{
    gameObjects->at(links[0]).transform.rotation = { glm::pi<float>()   , angle1, 0 };
    gameObjects->at(links[1]).transform.rotation = { -angle2            , angle1, glm::pi<float>() / 2 };
    gameObjects->at(links[2]).transform.rotation = { -angle2 + angle3   , angle1, glm::pi<float>() / 2 };

    const float cT = glm::cos(angle1);
    const float sT = glm::sin(angle1);

    const float cA = glm::cos(-angle2);
    const float sA = glm::sin(-angle2);

    glm::vec3 posLink2 = -sizeLinks[1] * glm::vec3{ cA * sT, -sA, cT * cA };

    gameObjects->at(links[2]).transform.translation = gameObjects->at(links[1]).transform.translation + posLink2;
}
